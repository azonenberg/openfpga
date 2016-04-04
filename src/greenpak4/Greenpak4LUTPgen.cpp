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

Greenpak4LUTPgen::Greenpak4LUTPgen(
	Greenpak4Device* device,
	unsigned int lutnum,
	unsigned int matrix,
	unsigned int ibase,
	unsigned int oword,
	unsigned int cbase,
	unsigned int order)
	: Greenpak4LUT(device, lutnum, matrix, ibase, oword, cbase, order)
{

}

Greenpak4LUTPgen::~Greenpak4LUTPgen()
{
	
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Bitfile metadata

unsigned int Greenpak4LUTPgen::GetConfigLen()
{
	return Greenpak4LUT::GetConfigLen() + 5;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Serialization of the truth table

bool Greenpak4LUTPgen::Load(bool* bitstream)
{
	//TODO: load Pgen stuff
	return Greenpak4LUT::Load(bitstream);
}

bool Greenpak4LUTPgen::Save(bool* bitstream)
{
	//4-bit counter data in (TODO)
	bitstream[m_configBase + 16] = false;
	bitstream[m_configBase + 17] = false;
	bitstream[m_configBase + 18] = false;
	bitstream[m_configBase + 19] = false;
	
	//Mode (for now, always LUT4)
	bitstream[m_configBase + 20] = false;
	
	//Save LUT stuff
	return Greenpak4LUT::Save(bitstream);
}
