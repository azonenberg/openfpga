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

Greenpak4SystemReset::Greenpak4SystemReset(
	Greenpak4Device* device,
	unsigned int matrix,
	unsigned int ibase,
	unsigned int oword,
	unsigned int cbase)
	: Greenpak4BitstreamEntity(device, matrix, ibase, oword, cbase)
	, m_resetMode(RISING_EDGE)
	, m_reset(device->GetPowerRail(m_matrix, 0))
{

}

Greenpak4SystemReset::~Greenpak4SystemReset()
{
	
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Bitfile metadata

unsigned int Greenpak4SystemReset::GetConfigLen()
{
	return 3;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Accessors

string Greenpak4SystemReset::GetDescription()
{
	return "SYSRST0";
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Serialization

bool Greenpak4SystemReset::Load(bool* /*bitstream*/)
{
	printf("Greenpak4SystemReset::Load() not yet implemented\n");
	return false;
}

bool Greenpak4SystemReset::Save(bool* bitstream)
{
	//No DRC needed - cannot route anything but pin 2 to us
	//If somebody tries something stupid PAR will fail with an unroutable design
	
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// INPUT BUS
	
	//no logic needed, hard-wired to pin #2
	
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Configuration
	
	//Reset mode
	switch(m_resetMode)
	{
		case HIGH_LEVEL:
			bitstream[m_configBase + 0] = true;
			bitstream[m_configBase + 1] = false;
			break;
			
		case RISING_EDGE:
			bitstream[m_configBase + 0] = false;
			bitstream[m_configBase + 1] = false;
			break;
			
		case FALLING_EDGE:
			bitstream[m_configBase + 0] = false;
			bitstream[m_configBase + 1] = true;
			break;
	}
	
	//Reset enable if m_reset is not a power rail (ground)
	if(dynamic_cast<Greenpak4PowerRail*>(m_reset) == NULL)
		bitstream[m_configBase + 2] = true;
	else
		bitstream[m_configBase + 2] = false;
	
	return true;
}
