/***********************************************************************************************************************
 * Copyright (C) 2016-2017 Andrew Zonenberg and contributors                                                           *
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

Greenpak4RCOscillator::Greenpak4RCOscillator(
	Greenpak4Device* device,
	unsigned int matrix,
	unsigned int ibase,
	unsigned int oword,
	unsigned int cbase)
	: Greenpak4BitstreamEntity(device, matrix, ibase, oword, cbase)
	, m_powerDown(device->GetGround())
	, m_powerDownEn(false)
	, m_autoPowerDown(true)
	, m_preDiv(1)
	, m_postDiv(1)
	, m_fastClock(true)
{
	m_dual = new Greenpak4DualEntity(this);
}

Greenpak4RCOscillator::~Greenpak4RCOscillator()
{

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Accessors

bool Greenpak4RCOscillator::IsConstantPowerDown()
{
	return m_powerDown.IsPowerRail();
}

vector<string> Greenpak4RCOscillator::GetInputPorts() const
{
	vector<string> r;
	r.push_back("PWRDN");
	return r;
}

void Greenpak4RCOscillator::SetInput(string port, Greenpak4EntityOutput src)
{
	if(port == "PWRDN")
		m_powerDown = src;

	//ignore anything else silently (should not be possible since synthesis would error out)
}

Greenpak4EntityOutput Greenpak4RCOscillator::GetInput(string port) const
{
	if(port == "PWRDN")
		return m_powerDown;
	else
		return Greenpak4EntityOutput(NULL);
}

vector<string> Greenpak4RCOscillator::GetOutputPorts() const
{
	vector<string> r;
	r.push_back("CLKOUT_FABRIC");
	return r;
}

unsigned int Greenpak4RCOscillator::GetOutputNetNumber(string port)
{
	if(port == "CLKOUT_FABRIC")
		return m_outputBaseWord;
	else
		return -1;
}

string Greenpak4RCOscillator::GetDescription() const
{
	return "RCOSC0";
}

string Greenpak4RCOscillator::GetPrimitiveName() const
{
	return "GP_RCOSC";
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Serialization

bool Greenpak4RCOscillator::CommitChanges()
{
	//Get our cell, or bail if we're unassigned
	auto ncell = dynamic_cast<Greenpak4NetlistCell*>(GetNetlistEntity());
	if(ncell == NULL)
		return true;

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

		if(	(div == 1) || (div == 2) || (div == 4) || (div == 8) )
		{
			m_preDiv = div;
		}
		else
		{
			LogError("GP_RCOSC pre divider must be 1, 2, 4, 8\n");
			return false;
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
			LogError("GP_RCOSC post divider must be 1, 2, 3, 4, 8, 12, 24, or 64\n");
			return false;
		}
	}

	if(ncell->HasParameter("OSC_FREQ"))
	{
		string s = ncell->m_parameters["OSC_FREQ"];
		if(s == "2M")
			m_fastClock = true;
		else if(s == "25k")
			m_fastClock = false;
		else
		{
			LogError("GP_RCOSC OSC_FREQ must be \"2M\" or \"25k\"\n");
			return false;
		}
	}

	return true;
}

bool Greenpak4RCOscillator::Load(bool* bitstream)
{
	//Load PWRDN
	ReadMatrixSelector(bitstream, m_inputBaseWord, m_matrix, m_powerDown);

	//If output is disabled, force us to be powered down
	if(!bitstream[m_configBase + 0])
		m_powerDown = m_device->GetPower();

	//Ignore power-down enable

	//Read other status bits
	m_autoPowerDown = !bitstream[m_configBase + 7];
	m_fastClock = bitstream[m_configBase + 8];

	//TODO: bypass

	//Output pre-divider
	int prediv = bitstream[m_configBase + 1] | (bitstream[m_configBase + 2] << 1);
	int predivs[4] = {1, 2, 4, 8};
	m_preDiv = predivs[prediv];

	//Output post-divider
	int postdiv = bitstream[m_configBase + 3] | (bitstream[m_configBase + 4] << 1) | (bitstream[m_configBase + 5] << 2);
	int postdivs[8] = {1, 2, 4, 3, 8, 12, 24, 64};
	m_postDiv = postdivs[postdiv];

	return true;
}

bool Greenpak4RCOscillator::Save(bool* bitstream)
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
	bitstream[m_configBase + 0] = !unused;

	//Enable power-down if we have it hooked up.
	bitstream[m_configBase + 6] = real_pwrdn_en;

	//Auto power-down
	bitstream[m_configBase + 7] = !m_autoPowerDown;

	//Frequency selection
	bitstream[m_configBase + 8] = m_fastClock;

	//Bypass RC oscillator (feed external clock to our output) matrix_out1_73
	bitstream[m_configBase + 9] = false;

	//Output pre-divider
	switch(m_preDiv)
	{
		case 1:
			bitstream[m_configBase + 2]	= false;
			bitstream[m_configBase + 1]	= false;
			break;

		case 2:
			bitstream[m_configBase + 2]	= false;
			bitstream[m_configBase + 1]	= true;
			break;

		case 4:
			bitstream[m_configBase + 2]	= true;
			bitstream[m_configBase + 1]	= false;
			break;

		case 8:
			bitstream[m_configBase + 2]	= true;
			bitstream[m_configBase + 1]	= true;
			break;

		default:
			LogError("GP_RCOSC pre divider is bogus");
			return false;
	}

	//Output post-divider
	switch(m_postDiv)
	{
		case 1:
			bitstream[m_configBase + 5] = false;
			bitstream[m_configBase + 4] = false;
			bitstream[m_configBase + 3] = false;
			break;

		case 2:
			bitstream[m_configBase + 5] = false;
			bitstream[m_configBase + 4] = false;
			bitstream[m_configBase + 3] = true;
			break;

		case 4:
			bitstream[m_configBase + 5] = false;
			bitstream[m_configBase + 4] = true;
			bitstream[m_configBase + 3] = false;
			break;

		case 3:
			bitstream[m_configBase + 5] = false;
			bitstream[m_configBase + 4] = true;
			bitstream[m_configBase + 3] = true;
			break;

		case 8:
			bitstream[m_configBase + 5] = true;
			bitstream[m_configBase + 4] = false;
			bitstream[m_configBase + 3] = false;
			break;

		case 12:
			bitstream[m_configBase + 5] = true;
			bitstream[m_configBase + 4] = false;
			bitstream[m_configBase + 3] = true;
			break;

		case 24:
			bitstream[m_configBase + 5] = true;
			bitstream[m_configBase + 4] = true;
			bitstream[m_configBase + 3] = false;
			break;

		case 64:
			bitstream[m_configBase + 5] = true;
			bitstream[m_configBase + 4] = true;
			bitstream[m_configBase + 3] = true;
			break;

		default:
			LogError("GP_RCOSC post divider is bogus");
			return false;
	}

	return true;
}
