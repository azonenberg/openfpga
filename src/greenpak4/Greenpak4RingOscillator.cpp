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
	, m_powerDown(device->GetGround())	//default to auto powerdown only
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

bool Greenpak4RingOscillator::IsConstantPowerDown()
{
	return m_powerDown.IsPowerRail();
}

vector<string> Greenpak4RingOscillator::GetInputPorts() const
{
	vector<string> r;
	r.push_back("PWRDN");
	return r;
}

void Greenpak4RingOscillator::SetInput(string port, Greenpak4EntityOutput src)
{
	if(port == "PWRDN")
		m_powerDown = src;

	//ignore anything else silently (should not be possible since synthesis would error out)
}

vector<string> Greenpak4RingOscillator::GetOutputPorts() const
{
	vector<string> r;
	r.push_back("CLKOUT_FABRIC");
	return r;
}

unsigned int Greenpak4RingOscillator::GetOutputNetNumber(string port)
{
	if(port == "CLKOUT_FABRIC")
		return m_outputBaseWord;
	else
		return -1;
}

string Greenpak4RingOscillator::GetDescription()
{
	return "RINGOSC0";
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
		m_powerDownEn = (ncell->m_parameters["PWRDN_EN"] == "1");

	//If auto-powerdown is not specified, but the cell is instantiated, default to always running the osc
	if(ncell->HasParameter("AUTO_PWRDN"))
		m_autoPowerDown = (ncell->m_parameters["AUTO_PWRDN"] == "1");
	else
		m_autoPowerDown = false;

	if(ncell->HasParameter("HARDIP_DIV"))
	{
		int div = atoi(ncell->m_parameters["HARDIP_DIV"].c_str());

		if(	(div == 1) || (div == 4) || (div == 8) || (div == 16) )
			m_preDiv = div;
		else
		{
			LogError("GP_RINGOSC pre divider must be 1, 4, 8, 16\n");
			exit(1);
		}
	}

	if(ncell->HasParameter("FABRIC_DIV"))
	{
		int div = atoi(ncell->m_parameters["FABRIC_DIV"].c_str());

		if(	(div == 1) || (div == 2) || (div == 3) || (div == 4) || (div == 8) ||
			(div == 12) || (div == 24) || (div == 64))
		{
			m_postDiv = div;
		}
		else
		{
			LogError("GP_RINGOSC post divider must be 1, 2, 3, 4, 8, 12, 24, or 64\n");
			exit(1);
		}
	}
}

bool Greenpak4RingOscillator::Load(bool* /*bitstream*/)
{
	LogFatal("Unimplemented\n");
}

bool Greenpak4RingOscillator::Save(bool* bitstream)
{
	//Optimize PWRDN = 1'b0 and PWRDN_EN = 1 to PWRDN = dontcare and PWRDN_EN = 0.
	//Detect constant power-down of 1 as "unused port"
	bool real_pwrdn_en = m_powerDownEn;
	bool unused = false;
	if(m_powerDown.IsPowerRail())
	{
		if( !m_powerDown.GetPowerRailValue() )
			real_pwrdn_en = false;
		if(!m_powerDown.GetPowerRailValue() && m_powerDownEn )
			unused = true;
	}

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
			LogFatal("GP_RINGOSC pre divider is bogus");
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
			LogFatal("GP_RINGOSC post divider is bogus");
			break;
	}

	return true;
}
