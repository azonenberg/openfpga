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

Greenpak4Counter::Greenpak4Counter(
	Greenpak4Device* device,
	unsigned int depth,
	unsigned int countnum,
	unsigned int matrix,
	unsigned int ibase,
	unsigned int oword,
	unsigned int cbase)
	: Greenpak4BitstreamEntity(device, matrix, ibase, oword, cbase)	
	, m_depth(depth)
	, m_countnum(countnum)
{

}

Greenpak4Counter::~Greenpak4Counter()
{
	
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Bitfile metadata

unsigned int Greenpak4Counter::GetConfigLen()
{
	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Accessors

string Greenpak4Counter::GetDescription()
{
	char buf[128];
	snprintf(buf, sizeof(buf), "COUNT%d_%d", m_depth, m_countnum);
	return string(buf);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Serialization

bool Greenpak4Counter::Load(bool* /*bitstream*/)
{
	printf("Greenpak4Counter::Load() not yet implemented\n");
	return false;
}

bool Greenpak4Counter::Save(bool* bitstream)
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// INPUT BUS
	
	/*
	//Write the power-down input iff we have power-down enabled.
	if(real_pwrdn_en)
	{
		if(!WriteMatrixSelector(bitstream, m_inputBaseWord, m_powerDown))
			return false;
	}
	*/
	
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Configuration
	
	/*
	//Enable power-down if we have it hooked up.
	bitstream[m_configBase + 0] = real_pwrdn_en;
	*/

	return true;
}
