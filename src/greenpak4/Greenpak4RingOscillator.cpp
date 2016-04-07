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

Greenpak4RingOscillator::Greenpak4RingOscillator(
	Greenpak4Device* device,
	unsigned int matrix,
	unsigned int ibase,
	unsigned int oword,
	unsigned int cbase)
	: Greenpak4BitstreamEntity(device, matrix, ibase, oword, cbase)
	, m_powerDown(device->GetPowerRail(0))	//default to auto powerdown only
	, m_powerDownEn(false)
	, m_autoPowerDown(true)
	, m_preDiv(1)
	, m_postDiv(1)
{
	m_dual = new Greenpak4DualEntity(this);
}

Greenpak4RingOscillator::~Greenpak4RingOscillator()
{
	
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Accessors

vector<string> Greenpak4RingOscillator::GetInputPorts()
{
	vector<string> r;
	r.push_back("PWRDN");
	return r;
}

vector<string> Greenpak4RingOscillator::GetOutputPorts()
{
	vector<string> r;
	r.push_back("CLKOUT_FABRIC");
	return r;
}

string Greenpak4RingOscillator::GetDescription()
{
	return "RINGOSC0";
}

void Greenpak4RingOscillator::SetPowerDown(Greenpak4BitstreamEntity* pwrdn)
{
	m_powerDown = pwrdn;
}

void Greenpak4RingOscillator::SetPreDivider(int div)
{
	if(	(div == 1) || (div == 4) || (div == 8) || (div == 16) )
	{
		m_preDiv = div;
	}
	else
	{
		fprintf(stderr, "ERROR: GP_RINGOSC pre divider must be 1, 4, 8, 16\n");
		exit(1);
	}
}

void Greenpak4RingOscillator::SetPostDivider(int div)
{
	if(	(div == 1) || (div == 2) || (div == 3) || (div == 4) || (div == 8) ||
		(div == 12) || (div == 24) || (div == 64))
	{
		m_postDiv = div;
	}
	else
	{
		fprintf(stderr, "ERROR: GP_RINGOSC post divider must be 1, 2, 3, 4, 8, 12, 24, or 64\n");
		exit(1);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Serialization

void Greenpak4RingOscillator::CommitChanges()
{
	//Get our cell, or bail if we're unassigned
	auto ncell = dynamic_cast<Greenpak4NetlistCell*>(GetNetlistEntity());
	if(ncell == NULL)
		return;
	
	if(ncell->HasParameter("PWRDN_EN"))
		SetPowerDownEn(ncell->m_parameters["PWRDN_EN"] == "1");
		
	if(ncell->HasParameter("AUTO_PWRDN"))
		SetAutoPowerDown(ncell->m_parameters["AUTO_PWRDN"] == "1");
		
	if(ncell->HasParameter("PRE_DIV"))
		SetPreDivider(atoi(ncell->m_parameters["PRE_DIV"].c_str()));
		
	if(ncell->HasParameter("FABRIC_DIV"))
		SetPostDivider(atoi(ncell->m_parameters["FABRIC_DIV"].c_str()));
}

bool Greenpak4RingOscillator::Load(bool* /*bitstream*/)
{
	printf("Greenpak4RingOscillator::Load() not yet implemented\n");
	return false;
}

bool Greenpak4RingOscillator::Save(bool* bitstream)
{
	//Optimize PWRDN = 1'b0 and PWRDN_EN = 1 to PWRDN = dontcare and PWRDN_EN = 0
	bool real_pwrdn_en = m_powerDownEn;
	Greenpak4PowerRail* rail = dynamic_cast<Greenpak4PowerRail*>(m_powerDown);
	if( (rail != NULL) && (rail->GetDigitalValue() == 0) )
		real_pwrdn_en = false;
		
	bool unused = false;
	if( (rail != NULL) && (rail->GetDigitalValue() == 1) && m_powerDownEn )
		unused = true;
	
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
		
	//Enable output if we're hooked up
	bitstream[m_configBase + 7] = !unused;
	
	//Enable power-down if we have it hooked up.
	bitstream[m_configBase + 8] = real_pwrdn_en;
	
	//Auto power-down
	bitstream[m_configBase + 10] = !m_autoPowerDown;

	//Output pre-divider
	switch(m_preDiv)
	{
		case 1:
			bitstream[m_configBase + 6]	= false;
			bitstream[m_configBase + 5]	= false;
			break;
		
		case 4:
			bitstream[m_configBase + 6]	= false;
			bitstream[m_configBase + 5]	= true;
			break;
			
		case 8:
			bitstream[m_configBase + 6]	= true;
			bitstream[m_configBase + 5]	= false;
			break;
			
		case 16:
			bitstream[m_configBase + 6]	= true;
			bitstream[m_configBase + 5]	= true;
			break;
			
		default:
			fprintf(stderr, "INTERNAL ERROR: GP_RINGOSC pre divider is bogus");
			exit(1);
			break;
	}

	//Output post-divider
	switch(m_postDiv)
	{
		case 1:
			bitstream[m_configBase + 2] = false;
			bitstream[m_configBase + 1] = false;
			bitstream[m_configBase + 0] = false;
			break;
			
		case 2:
			bitstream[m_configBase + 2] = false;
			bitstream[m_configBase + 1] = false;
			bitstream[m_configBase + 0] = true;
			break;
			
		case 4:
			bitstream[m_configBase + 2] = false;
			bitstream[m_configBase + 1] = true;
			bitstream[m_configBase + 0] = false;
			break;
			
		case 3:
			bitstream[m_configBase + 2] = false;
			bitstream[m_configBase + 1] = true;
			bitstream[m_configBase + 0] = true;
			break;
			
		case 8:
			bitstream[m_configBase + 2] = true;
			bitstream[m_configBase + 1] = false;
			bitstream[m_configBase + 0] = false;
			break;
			
		case 12:
			bitstream[m_configBase + 2] = true;
			bitstream[m_configBase + 1] = false;
			bitstream[m_configBase + 0] = true;
			break;
			
		case 24:
			bitstream[m_configBase + 2] = true;
			bitstream[m_configBase + 1] = true;
			bitstream[m_configBase + 0] = false;
			break;
			
		case 64:
			bitstream[m_configBase + 2] = true;
			bitstream[m_configBase + 1] = true;
			bitstream[m_configBase + 0] = true;
			break;
	
		default:
			fprintf(stderr, "INTERNAL ERROR: GP_RINGOSC post divider is bogus");
			exit(1);
			break;
	}

	return true;
}
