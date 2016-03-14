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
		m_inputs[i] = device->GetPowerRail(matrix, 0);
}

Greenpak4LUT::~Greenpak4LUT()
{
	
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Bitfile metadata

unsigned int Greenpak4LUT::GetConfigLen()
{
	return 1 << m_order;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Serialization of the truth table

bool Greenpak4LUT::Load(bool* bitstream)
{
	//TODO: Do our inputs
	
	//Do the LUT
	unsigned int nmax = GetConfigLen();
	for(unsigned int i=0; i<nmax; i++)
		m_truthtable[i] = bitstream[m_configBase + i];
		
	return true;
}

bool Greenpak4LUT::Save(bool* bitstream)
{
	//TODO: Do our inputs
	
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// INPUT BUS
	
	for(unsigned int i=0; i<m_order; i++)
	{
		if(!WriteMatrixSelector(bitstream, m_inputBaseWord + i, m_inputs[i]))
			return false;
	}
	
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// LUT CONTENTS
	
	unsigned int nmax = GetConfigLen();
	for(unsigned int i=0; i<nmax; i++)
		bitstream[m_configBase + i] = m_truthtable[i];
		
	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Accessors

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

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Initialization helpers

void Greenpak4LUT::MakeOR()
{
	//Set input 0 (all inputs false) to false
	m_truthtable[0] = false;
	
	//everything else true
	for(unsigned int i=1; i<16; i++)
		m_truthtable[i] = true;
}

void Greenpak4LUT::MakeAND()
{
	//Default to false
	for(unsigned int i=0; i<16; i++)
		m_truthtable[i] = false;
		
	//True depending on our order
	switch(m_order)
	{
		case 2:
			m_truthtable[3] = true;
			break;
			
		case 3:
			m_truthtable[7] = true;
			break;
			
		case 4:
			m_truthtable[15] = true;
			break;
	}
}

void Greenpak4LUT::MakeNOR()
{
	//Set input 0 (all inputs false) to true
	m_truthtable[0] = true;
	
	//everything else false
	for(unsigned int i=1; i<16; i++)
		m_truthtable[i] = false;
}

void Greenpak4LUT::MakeNAND()
{
	//Default to true
	for(unsigned int i=0; i<16; i++)
		m_truthtable[i] = true;
		
	//False depending on our order
	switch(m_order)
	{
		case 2:
			m_truthtable[3] = false;
			break;
			
		case 3:
			m_truthtable[7] = false;
			break;
			
		case 4:
			m_truthtable[15] = false;
			break;
	}
}

void Greenpak4LUT::SetBit(bool val, bool a0, bool a1, bool a2, bool a3)
{
	m_truthtable[a3*8 | a2*4 | a1*2 | a0] = val;
}
