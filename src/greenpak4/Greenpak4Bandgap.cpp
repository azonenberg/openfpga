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

Greenpak4Bandgap::Greenpak4Bandgap(
	Greenpak4Device* device,
	unsigned int matrix,
	unsigned int ibase,
	unsigned int oword,
	unsigned int cbase)
	: Greenpak4BitstreamEntity(device, matrix, ibase, oword, cbase)
	//, m_powerDownEn(false)
	//, m_autoPowerDown(true)
{
	m_dual = new Greenpak4DualEntity(this);
}

Greenpak4Bandgap::~Greenpak4Bandgap()
{
	
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Bitfile metadata

unsigned int Greenpak4Bandgap::GetConfigLen()
{
	//FIXME
	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Accessors

string Greenpak4Bandgap::GetDescription()
{
	return "BANDGAP0";
}

vector<string> Greenpak4Bandgap::GetInputPorts()
{
	vector<string> r;
	//no inputs
	return r;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Serialization

bool Greenpak4Bandgap::Load(bool* /*bitstream*/)
{
	printf("Greenpak4Bandgap::Load() not yet implemented\n");
	return false;
}

bool Greenpak4Bandgap::Save(bool* bitstream)
{
	/*
	//Optimize PWRDN = 1'b0 and PWRDN_EN = 1 to PWRDN = dontcare and PWRDN_EN = 0
	bool real_pwrdn_en = m_powerDownEn;
	Greenpak4PowerRail* rail = dynamic_cast<Greenpak4PowerRail*>(m_powerDown);
	if( (rail != NULL) && (rail->GetDigitalValue() == 0) )
		real_pwrdn_en = false;
	
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// INPUT BUS
	
	//Write the power-down input iff we have power-down enabled.
	if(real_pwrdn_en)
	{
		if(!WriteMatrixSelector(bitstream, m_inputBaseWord, m_powerDown))
			return false;
	}
	
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Configuration
	
	//Enable power-down if we have it hooked up.
	bitstream[m_configBase + 0] = real_pwrdn_en;
	
	//Auto power-down
	bitstream[m_configBase + 1] = !m_autoPowerDown;
	
	//Output clock divider
	switch(m_outDiv)
	{
		case 1:
			bitstream[m_configBase + 3] = false;
			bitstream[m_configBase + 2] = false;
			break;
			
		case 2:
			bitstream[m_configBase + 3] = false;
			bitstream[m_configBase + 2] = true;
			break;
			
		case 4:
			bitstream[m_configBase + 3] = true;
			bitstream[m_configBase + 2] = false;
			break;
			
		case 16:
			bitstream[m_configBase + 3] = true;
			bitstream[m_configBase + 2] = true;
			break;
			
		default:
			fprintf(stderr, "INTERNAL ERROR: GP4_LFOSC output divider must be 1, 2, 4, or 16\n");
			exit(1);
			break;
	}
	*/
	return true;
}
