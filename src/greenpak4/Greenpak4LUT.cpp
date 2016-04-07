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
 
#include "Greenpak4.h"

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
	for(unsigned int i=0; i<16; i++)
		m_truthtable[i] = false;
	for(unsigned int i=0; i<4; i++)
		m_inputs[i] = device->GetPowerRail(0);
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

void Greenpak4LUT::CommitChanges()
{
	//Get our cell, or bail if we're unassigned
	auto ncell = dynamic_cast<Greenpak4NetlistCell*>(GetNetlistEntity());
	if(ncell == NULL)
		return;
	
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
				bool nbit = (truth_table & (1 << i)) ? true : false;
				//printf("        inputs %d %d %d %d: %d\n", a3, a2, a1, a0, nbit);
				SetBit(nbit, a0, a1, a2, a3);
			}
		}
		
		else
		{
			printf("WARNING: Cell\"%s\" has unrecognized parameter %s, ignoring\n",
				ncell->m_name.c_str(), x.first.c_str());
		}
	}
}

vector<string> Greenpak4LUT::GetInputPorts()
{
	vector<string> r;
	switch(m_order)
	{
		case 4: r.push_back("IN3");
		case 3: r.push_back("IN2");
		case 2: r.push_back("IN1");
		case 1: r.push_back("IN0");
		default:
			break;
	}
	return r;
}

vector<string> Greenpak4LUT::GetOutputPorts()
{
	vector<string> r;
	r.push_back("OUT");
	return r;
}

void Greenpak4LUT::SetInputSignal(unsigned int n, Greenpak4BitstreamEntity* sig)
{
	if(n >= m_order)
	{
		fprintf(stderr, "Tried to use input not physically present on this LUT\n");
		return;
	}
	
	m_inputs[n] = sig;
}

Greenpak4BitstreamEntity* Greenpak4LUT::GetInputSignal(unsigned int n)
{
	if(n >= m_order)
	{
		fprintf(stderr, "Tried to get input not physically present on this LUT\n");
		return NULL;
	}
	
	return m_inputs[n];
}

string Greenpak4LUT::GetDescription()
{
	char buf[128];
	snprintf(buf, sizeof(buf), "LUT%d_%d", m_order, m_lutnum);
	return string(buf);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Initialization helpers

void Greenpak4LUT::SetBit(bool val, bool a0, bool a1, bool a2, bool a3)
{
	m_truthtable[a3*8 | a2*4 | a1*2 | a0] = val;
}
