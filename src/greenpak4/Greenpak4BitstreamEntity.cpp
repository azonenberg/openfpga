/***********************************************************************************************************************
 * Copyright (C) 2016 Andrew Zonenberg and contributors                                                                *
 *                                                                                                                     *
 * This program is free software; you can redistribute it and/or modify it under the terms of the GNU Lesser General   *
 * Public License as published by the Free Software Foundation; either version 2.1 of the License, or (at your option) *
 * any later version.                                                                                                  *
 *                                                                                                                     *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied  *
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for     *
 * more details.                                                                                                       *
 *                                                                                                                     *
 * You should have received a copy of the GNU Lesser General Public License along with this program; if not, you may   *
 * find one here:                                                                                                      *
 * https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt                                                              *
 * or you may search the http://www.gnu.org website for the version 2.1 license, or you may write to the Free Software *
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA                                      *
 **********************************************************************************************************************/

#include <log.h>
#include <xbpar.h>
#include <Greenpak4.h>

using namespace std;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Construction / destruction

Greenpak4BitstreamEntity::Greenpak4BitstreamEntity(
	Greenpak4Device* device,
	unsigned int matrix,
	unsigned int ibase,
	unsigned int obase,
	unsigned int cbase
	)
	: m_device(device)
	, m_matrix(matrix)
	, m_inputBaseWord(ibase)
	, m_outputBaseWord(obase)
	, m_configBase(cbase)
	, m_parnode(NULL)
	, m_dual(NULL)
	, m_dualMaster(true)
{

}

Greenpak4BitstreamEntity::~Greenpak4BitstreamEntity()
{
	//Delete our dual if we're the master
	if(m_dual && m_dualMaster)
	{
		delete m_dual;
		m_dual = NULL;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Connectivity tracing helpers

/**
	@brief Check if we have any loads on a particular port
 */
bool Greenpak4BitstreamEntity::HasLoadsOnPort(string port)
{
	//Get our cell, or bail if we're unassigned
	auto ncell = dynamic_cast<Greenpak4NetlistCell*>(GetNetlistEntity());
	if(ncell == NULL)
		return false;

	//If nothing on the port, stop
	if(ncell->m_connections.find(port) == ncell->m_connections.end())
		return false;

	//Check if any connections other than ourself
	auto vec = ncell->m_connections[port];
	for(auto node : vec)
	{
		for(auto point : node->m_nodeports)
		{
			if(point.m_cell != ncell)
				return true;
		}

		if(!node->m_ports.empty())
			return true;
	}

	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Net numbering helpers

Greenpak4EntityOutput Greenpak4BitstreamEntity::GetOutput(std::string port)
{
	return Greenpak4EntityOutput(this, port, m_matrix);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Commit helpers

Greenpak4NetlistEntity* Greenpak4BitstreamEntity::GetNetlistEntity()
{
	PARGraphNode* mate = m_parnode->GetMate();
	if(mate == NULL)
		return NULL;
	return static_cast<Greenpak4NetlistEntity*>(mate->GetData());
}

/**
	@brief Returns true if the given named port is general fabric routing
 */
bool Greenpak4BitstreamEntity::IsGeneralFabricInput(string port) const
{
	auto ports = GetInputPorts();
	for(auto p : ports)
	{
		if(p == port)
			return true;
	}
	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Debug log helpers

string Greenpak4BitstreamEntity::GetOutputName()
{
	auto mate = m_parnode->GetMate();
	if(mate == NULL)
		return "";
	auto entity = static_cast<Greenpak4NetlistEntity*>(mate->GetData());

	//If it's an IOB, return the IOB name
	if(dynamic_cast<Greenpak4NetlistPort*>(entity))
		return entity->m_name;

	//Nope, it's a cell
	auto cell = dynamic_cast<Greenpak4NetlistCell*>(entity);
	if(!cell)
		return "error";

	//Look up our first output port... HACK!
	//TODO: Fix this
	string portname = GetOutputPorts()[0];

	//Find the net we connect to
	if(cell->m_connections.find(portname) == cell->m_connections.end())
		return "error";
	return cell->m_connections[portname][0]->m_name;	//FIXME:VECTOR
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Load/save helpers

Greenpak4BitstreamEntity* Greenpak4BitstreamEntity::GetRealEntity()
{
	if(m_dual && !m_dualMaster)
		return m_dual;

	return this;
}

bool Greenpak4BitstreamEntity::WriteMatrixSelector(
	bool* bitstream,
	unsigned int wordpos,
	Greenpak4EntityOutput signal,
	bool cross_matrix)
{
	//Can't hook up non-routable signals
	if(signal.GetNetNumber() > 255)
	{
		LogError("Tried to write signal from invalid net %x\n", signal.GetNetNumber());
		return false;
	}

	//SANITY CHECK - must be attached to the same matrix
	//cross connections use opposite, though
	if(cross_matrix)
	{
		//Do not do check if the signal is a power rail (this is the case for unused cross connections)
		if(signal.IsPowerRail())
		{}

		//No other signal, dual or not, should do this
		else if(m_matrix == signal.GetMatrix())
		{
			LogError("Tried to write signal from same matrix through a cross connection\n");
			return false;
		}
	}
	else if(m_matrix != signal.GetMatrix())
	{
		//If we have a dual, use that
		if(signal.HasDual())
			signal = signal.GetDual();

		//otherwise something is fishy
		else
		{
			LogError("Tried to write signal from opposite matrix without using a cross connection\n");
			return false;
		}
	}

	//Good to go, write it
	unsigned int sel = signal.GetNetNumber();

	//Calculate right matrix for cross connections etc
	unsigned int matrix = m_matrix;
	if(cross_matrix)
		matrix = 1 - matrix;

	unsigned int nbits = m_device->GetMatrixBits();
	unsigned int startbit = m_device->GetMatrixBase(matrix) + wordpos * nbits;

	//Need to flip bit ordering since lowest array index is the MSB
	for(unsigned int i=0; i<nbits; i++)
	{
		if( (sel >> i) & 1 )
			bitstream[startbit + i] = true;
		else
			bitstream[startbit + i] = false;
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Timing analysis

bool Greenpak4BitstreamEntity::GetCombinatorialDelay(
	string srcport,
	string dstport,
	PTVCorner corner,
	CombinatorialDelay& delay) const
{
	//If this isn't a corner we know about, give up
	auto pit = m_pinToPinDelays.find(corner);
	if(pit == m_pinToPinDelays.end())
	{
		LogWarning("Greenpak4BitstreamEntity::GetCombinatorialDelay: no delay for process corner %s\n",
			corner.toString().c_str());
		return false;
	}

	//If we don't have data for this pin pair, give up
	PinPair pair(srcport, dstport);
	auto& dmap = pit->second;
	auto dit = dmap.find(pair);
	if(dit == dmap.end())
	{
		LogWarning("Greenpak4BitstreamEntity::GetCombinatorialDelay: no delay for path %s to %s\n",
			srcport.c_str(), dstport.c_str());
		return false;
	}

	//Got it
	delay = dit->second;
	return true;
}

void Greenpak4BitstreamEntity::AddCombinatorialDelay(
	string srcport,
	string dstport,
	PTVCorner corner,
	CombinatorialDelay delay)
{
	m_pinToPinDelays[corner][PinPair(srcport, dstport)] = delay;
}

void Greenpak4BitstreamEntity::PrintTimingData() const
{
	//Early-out if we have no timing data
	if(m_pinToPinDelays.empty())
		return;

	LogNotice("%s\n", GetDescription().c_str());

	//Combinatorial delays
	LogIndenter li;
	for(auto& it : m_pinToPinDelays)
	{
		LogNotice("%s\n", it.first.toString().c_str());
		LogIndenter li2;
		for(auto& jt : it.second)
		{
			auto& pair = jt.first;
			auto& time = jt.second;
			LogNotice("%10s to %10s: %.3f ns rising, %.3f ns falling\n",
				pair.first.c_str(),
				pair.second.c_str(),
				time.m_rising,
				time.m_falling);
		}

		PrintExtraTimingData(it.first);
	}

	//TODO: Setup/hold margins
}

void Greenpak4BitstreamEntity::PrintExtraTimingData(PTVCorner /*corner*/) const
{
	//Empty in base class but need to provide an empty default implementation
	//so children don't HAVE to override
}

/**
	@brief Writes our parent data
 */
void Greenpak4BitstreamEntity::SaveTimingData(FILE* fp, bool last)
{
	fprintf(fp, "    \"%s\":\n    [\n", GetDescription().c_str());

	if(!m_pinToPinDelays.empty())
	{
		//Loop over each process corner and export the data
		auto end = m_pinToPinDelays.end();
		end--;
		for(auto it : m_pinToPinDelays)
		{
			fprintf(fp, "        {\n");
			auto corner = it.first;
			fprintf(fp, "            \"process\" : \"%s\",\n", corner.GetSpeedAsString().c_str());
			fprintf(fp, "            \"temp\" : \"%d\",\n", corner.GetTemp());
			fprintf(fp, "            \"voltage_mv\" : \"%d\",\n", corner.GetVoltage());

			fprintf(fp, "            \"delays\" :\n            {\n");
			SaveTimingData(fp, corner);
			fprintf(fp, "            }\n");

			//key is last element, we're done
			if(it.first == end->first)
				fprintf(fp, "        }\n");
			else
				fprintf(fp, "        },\n");
		}
	}

	if(last)
		fprintf(fp, "    ]\n");
	else
		fprintf(fp, "    ],\n");
}

void Greenpak4BitstreamEntity::SaveTimingData(FILE* fp, PTVCorner corner)
{
}
