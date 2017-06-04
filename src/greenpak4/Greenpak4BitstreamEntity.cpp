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
	@brief Writes the metadata around our actual timing numbers
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

			fprintf(fp, "            \"delays\" :\n            [\n");
			SaveTimingData(fp, corner);
			fprintf(fp, "            ]\n");

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

/**
	@brief Write the timing numbers
 */
void Greenpak4BitstreamEntity::SaveTimingData(FILE* fp, PTVCorner corner)
{
	auto& map = m_pinToPinDelays[corner];
	if(map.empty())
		return;

	auto end = map.end();
	end--;
	for(auto it : map)
	{
		auto pins = it.first;
		auto delay = it.second;
		fprintf(fp, "                {\n");
		fprintf(fp, "                    \"type\" : \"propagation\",\n");
		fprintf(fp, "                    \"from\" : \"%s\",\n", pins.first.c_str());
		fprintf(fp, "                    \"to\" : \"%s\",\n", pins.second.c_str());
		fprintf(fp, "                    \"rising\" : \"%f\",\n", delay.m_rising);
		fprintf(fp, "                    \"falling\" : \"%f\"\n", delay.m_falling);

		if(it.first == end->first)
			fprintf(fp, "                }\n");
		else
			fprintf(fp, "                },\n");
	}
}

/**
	@brief Load our delay info
 */
bool Greenpak4BitstreamEntity::LoadTimingData(json_object* object)
{
	for(int i=0; i<json_object_array_length(object); i++)
	{
		auto child = json_object_array_get_idx(object, i);
		if(!LoadTimingDataForCorner(child))
			return false;
	}

	return true;
}

/**
	@brief Loads delay info for a single process-corner object in the JSON file
 */
bool Greenpak4BitstreamEntity::LoadTimingDataForCorner(json_object* object)
{
	//Look up the corner info
	json_object* process;
	if(!json_object_object_get_ex(object, "process", &process))
	{
		LogError("No process info for this corner\n");
		return false;
	}
	string sprocess = json_object_get_string(process);

	json_object* temp;
	if(!json_object_object_get_ex(object, "temp", &temp))
	{
		LogError("No temp info for this corner\n");
		return false;
	}
	int ntemp = json_object_get_int(temp);

	json_object* voltage;
	if(!json_object_object_get_ex(object, "voltage_mv", &voltage))
	{
		LogError("No voltage info for this corner\n");
		return false;
	}
	int nvoltage = json_object_get_int(voltage);

	//TODO: move this into PTVCorner class?
	PTVCorner::ProcessSpeed speed;
	if(sprocess == "fast")
		speed = PTVCorner::SPEED_FAST;
	else if(sprocess == "slow")
		speed = PTVCorner::SPEED_SLOW;
	else if(sprocess == "typical")
		speed = PTVCorner::SPEED_TYPICAL;

	//This is the process corner we're loading
	PTVCorner corner(speed, ntemp, nvoltage);

	//This is the actual timing data!
	json_object* delays;
	if(!json_object_object_get_ex(object, "delays", &delays))
	{
		LogError("No delay info for this corner\n");
		return false;
	}

	//Now that we know where to put it, we can load the actual timing data
	for(int i=0; i<json_object_array_length(delays); i++)
	{
		auto child = json_object_array_get_idx(delays, i);

		//We need to know the type of delay. "propagation" is handled by us
		//Anything else is a derived class
		json_object* type;
		if(!json_object_object_get_ex(child, "type", &type))
		{
			LogError("No type info for this delay value\n");
			return false;
		}
		string stype = json_object_get_string(type);

		//Load rising/falling delays and save them
		if(stype == "propagation")
		{
			if(!LoadPropagationDelay(corner, child))
				return false;
			continue;
		}

		//Nope, something special
		if(!LoadExtraTimingData(corner, stype, child))
			return false;
	}

	return true;
}

/**
	@brief Loads propagation delay for a single endpoint and process corner
 */
bool Greenpak4BitstreamEntity::LoadPropagationDelay(PTVCorner corner, json_object* object)
{
	//Pull out all of the json stuff
	json_object* from;
	if(!json_object_object_get_ex(object, "from", &from))
	{
		LogError("No source for this delay\n");
		return false;
	}
	string sfrom = json_object_get_string(from);

	json_object* to;
	if(!json_object_object_get_ex(object, "to", &to))
	{
		LogError("No dest for this delay\n");
		return false;
	}
	string sto = json_object_get_string(to);

	json_object* rising;
	if(!json_object_object_get_ex(object, "rising", &rising))
	{
		LogError("No rising info for this corner\n");
		return false;
	}
	float nrising = json_object_get_double(rising);

	json_object* falling;
	if(!json_object_object_get_ex(object, "falling", &falling))
	{
		LogError("No falling info for this corner\n");
		return false;
	}
	float nfalling = json_object_get_double(falling);

	//Finally, we can actually save the delay!
	m_pinToPinDelays[corner][PinPair(sfrom, sto)] = CombinatorialDelay(nrising, nfalling);

	return true;
}

/**
	@brief Loads timing data other than normal propagation info

	Base class implementation should never be called (base class should override) if there's any extra data,
	but we need a default implementation to avoid every derived class having an empty stub
 */
bool Greenpak4BitstreamEntity::LoadExtraTimingData(PTVCorner /*corner*/, string /*delaytype*/, json_object* /*object*/)
{
	LogWarning("Greenpak4BitstreamEntity: Don't know what to do with delay type %s\n", delaytype.c_str());
	return true;
}
