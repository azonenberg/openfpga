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
	unsigned int matrix,
	unsigned int ibase,
	unsigned int oword,
	unsigned int cbase,
	unsigned int order)
	: Greenpak4BitstreamEntity(device, matrix, ibase, oword, cbase)
	, m_order(order)
{
	for(unsigned int i=0; i<16; i++)
		m_truthtable[i] = 0;
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
	
	//Do the LUT
	unsigned int nmax = GetConfigLen();
	for(unsigned int i=0; i<nmax; i++)
		bitstream[m_configBase + i] = m_truthtable[i];
		
	return true;
}
