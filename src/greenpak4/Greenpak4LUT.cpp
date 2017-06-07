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
#include <Greenpak4.h>

using namespace std;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Construction / destruction

Greenpak4LUT::Greenpak4LUT(
	Greenpak4Device* device,
	unsigned int lutnum,
	unsigned int matrix,
	unsigned int ibase,
	unsigned int oword,
	unsigned int cbase,
	unsigned int order)
	: Greenpak4BitstreamEntity(device, matrix, ibase, oword, cbase)
	, m_lutnum(lutnum)
	, m_order(order)
{
	for(unsigned int i=0; i<4; i++)
		m_inputs[i] = device->GetGround();

	for(unsigned int i=0; i<16; i++)
		m_truthtable[i] = false;
}

Greenpak4LUT::~Greenpak4LUT()
{

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Serialization of the truth table

bool Greenpak4LUT::Load(bool* bitstream)
{
	//TODO: Do our inputs

	//Do the LUT
	unsigned int nmax = 1 << m_order;
	for(unsigned int i=0; i<nmax; i++)
		m_truthtable[i] = bitstream[m_configBase + i];

	return true;
}

bool Greenpak4LUT::Save(bool* bitstream)
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// INPUT BUS

	for(unsigned int i=0; i<m_order; i++)
	{
		if(!WriteMatrixSelector(bitstream, m_inputBaseWord + i, m_inputs[i]))
			return false;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// LUT CONTENTS

	unsigned int nmax = 1 << m_order;
	for(unsigned int i=0; i<nmax; i++)
		bitstream[m_configBase + i] = m_truthtable[i];

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Accessors

bool Greenpak4LUT::CommitChanges()
{
	//Get our cell, or bail if we're unassigned
	auto ncell = dynamic_cast<Greenpak4NetlistCell*>(GetNetlistEntity());
	if(ncell == NULL)
		return true;

	//If the cell is an inverter, we need special processing to up-map
	if(ncell->m_type == "GP_INV")
	{
		//Set up the truth table
		m_truthtable[0] = true;
		for(int i=1; i<16; i++)
			m_truthtable[i] = false;

		//Tie upper bits off since they're not in the netlist
		for(int i=1; i<4; i++)
			m_inputs[i] = m_device->GetGround();
	}

	//Not an inverter, treat it as a LUT
	else
	{
		for(auto x : ncell->m_parameters)
		{
			//LUT initialization value, as decimal
			if(x.first == "INIT")
			{
				//convert to bit array format for the bitstream library
				uint32_t truth_table = atoi(x.second.c_str());
				unsigned int nbits = 1 << m_order;
				for(unsigned int i=0; i<nbits; i++)
				{
					bool a3 = (i & 8) ? true : false;
					bool a2 = (i & 4) ? true : false;
					bool a1 = (i & 2) ? true : false;
					bool a0 = (i & 1) ? true : false;
					m_truthtable[a3*8 | a2*4 | a1*2 | a0] = (truth_table & (1 << i)) ? true : false;
				}
			}

			else
			{
				LogWarning("Cell\"%s\" has unrecognized parameter %s, ignoring\n",
					ncell->m_name.c_str(), x.first.c_str());
			}
		}
	}

	return true;
}

vector<string> Greenpak4LUT::GetInputPorts() const
{
	vector<string> r;
	switch(m_order)
	{
		case 4: r.push_back("IN3");
		case 3: r.push_back("IN2");
		case 2: r.push_back("IN1");
		case 1: r.push_back("IN0");
		default:
			r.push_back("IN");	//used for up-mapping GP_INV to GP_LUTx
			break;
	}
	return r;
}

void Greenpak4LUT::SetInput(string port, Greenpak4EntityOutput src)
{
	//used for up-mapping GP_INV to GP_LUTx
	if( (port == "IN0") || (port == "IN") )
		m_inputs[0] = src;

	else if(port == "IN1")
		m_inputs[1] = src;
	else if(port == "IN2")
		m_inputs[2] = src;
	else if(port == "IN3")
		m_inputs[3] = src;

	//ignore anything else silently (should not be possible since synthesis would error out)
}

Greenpak4EntityOutput Greenpak4LUT::GetInput(string port) const
{
	if( (port == "IN0") || (port == "IN") )
		return m_inputs[0];
	else if(port == "IN1")
		return m_inputs[1];
	else if(port == "IN2")
		return m_inputs[2];
	else if(port == "IN3")
		return m_inputs[3];
	else
		return Greenpak4EntityOutput(NULL);
}

void Greenpak4LUT::MakeXOR()
{
	for(int i=0; i<16; i++)
	{
		bool a = i & 1 ? true : false;
		bool b = i & 2 ? true : false;
		bool c = i & 4 ? true : false;
		bool d = i & 8 ? true : false;

		m_truthtable[i] = a ^ b ^ c ^ d;
	}
}

void Greenpak4LUT::MakeNOT()
{
	m_truthtable[0] = true;
	m_truthtable[1] = false;
	m_truthtable[2] = true;

	for(int i=3; i<16; i++)
		m_truthtable[i] = false;
}

vector<string> Greenpak4LUT::GetOutputPorts() const
{
	vector<string> r;
	r.push_back("OUT");
	return r;
}

unsigned int Greenpak4LUT::GetOutputNetNumber(string port)
{
	if(port == "OUT")
		return m_outputBaseWord;
	else
		return -1;
}

string Greenpak4LUT::GetDescription() const
{
	char buf[128];
	snprintf(buf, sizeof(buf), "LUT%u_%u", m_order, m_lutnum);
	return string(buf);
}
