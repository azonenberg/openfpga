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

Greenpak4LFOscillator::Greenpak4LFOscillator(
	Greenpak4Device* device,
	unsigned int matrix,
	unsigned int ibase,
	unsigned int oword,
	unsigned int cbase,
	unsigned int cbase_clkdiv)
	: Greenpak4BitstreamEntity(device, matrix, ibase, oword, cbase)
	, m_powerDown(device->GetGround())	//default to auto powerdown only
	, m_powerDownEn(false)
	, m_autoPowerDown(true)
	, m_outDiv(1)
	, m_cbaseClkdiv(cbase_clkdiv)
{
	m_dual = new Greenpak4DualEntity(this);
}

Greenpak4LFOscillator::~Greenpak4LFOscillator()
{

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Accessors

bool Greenpak4LFOscillator::IsConstantPowerDown()
{
	return m_powerDown.IsPowerRail();
}

vector<string> Greenpak4LFOscillator::GetInputPorts() const
{
	vector<string> r;
	r.push_back("PWRDN");
	return r;
}

void Greenpak4LFOscillator::SetInput(string port, Greenpak4EntityOutput src)
{
	if(port == "PWRDN")
		m_powerDown = src;

	//ignore anything else silently (should not be possible since synthesis would error out)
}

Greenpak4EntityOutput Greenpak4LFOscillator::GetInput(string port) const
{
	if(port == "PWRDN")
		return m_powerDown;
	else
		return Greenpak4EntityOutput(NULL);
}

vector<string> Greenpak4LFOscillator::GetOutputPorts() const
{
	vector<string> r;
	r.push_back("CLKOUT");
	return r;
}

unsigned int Greenpak4LFOscillator::GetOutputNetNumber(string port)
{
	if(port == "CLKOUT")
		return m_outputBaseWord;
	else
		return -1;
}

string Greenpak4LFOscillator::GetDescription() const
{
	return "LFOSC0";
}

string Greenpak4LFOscillator::GetPrimitiveName() const
{
	return "GP_LFOSC";
}

map<string, string> Greenpak4LFOscillator::GetParameters() const
{
	map<string, string> params;

	if(m_powerDownEn)
		params["PWRDN_EN"] = "1";
	else
		params["PWRDN_EN"] = "0";

	if(m_autoPowerDown)
		params["AUTO_PWRDN"] = "1";
	else
		params["AUTO_PWRDN"] = "0";

	char tmp[128];
	snprintf(tmp, sizeof(tmp), "%d", m_outDiv);
	params["OUT_DIV"] = tmp;

	return params;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Serialization

bool Greenpak4LFOscillator::CommitChanges()
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

	if(ncell->HasParameter("OUT_DIV"))
	{
		int div = atoi(ncell->m_parameters["OUT_DIV"].c_str());

		if(	(div == 1) || (div == 2) || (div == 4) || (div == 16) )
			m_outDiv = div;

		else
		{
			LogError("GP4_LFOSC output divider must be 1, 2, 4, or 16\n");
			return false;
		}
	}

	return true;
}

bool Greenpak4LFOscillator::Load(bool* bitstream)
{
	//Load PWRDN
	ReadMatrixSelector(bitstream, m_inputBaseWord, m_matrix, m_powerDown);

	//If powerdown isn't enabled, tie powerdown off
	if(!bitstream[m_configBase + 0])
		m_powerDown = m_device->GetGround();

	//Read other config
	m_autoPowerDown = !bitstream[m_configBase + 1];

	//Read clock dividers
	int clkdiv = (bitstream[m_cbaseClkdiv + 1] << 1) | bitstream[m_cbaseClkdiv + 0];
	int clkdivs[4] = {1, 2, 4, 16};
	m_outDiv = clkdivs[clkdiv];

	return true;
}

bool Greenpak4LFOscillator::Save(bool* bitstream)
{
	//Optimize PWRDN = 1'b0 and PWRDN_EN = 1 to PWRDN = dontcare and PWRDN_EN = 0
	bool real_pwrdn_en = m_powerDownEn;
	if( m_powerDown.IsPowerRail() && !m_powerDown.GetPowerRailValue() )
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
			bitstream[m_cbaseClkdiv + 1] = false;
			bitstream[m_cbaseClkdiv + 0] = false;
			break;

		case 2:
			bitstream[m_cbaseClkdiv + 1] = false;
			bitstream[m_cbaseClkdiv + 0] = true;
			break;

		case 4:
			bitstream[m_cbaseClkdiv + 1] = true;
			bitstream[m_cbaseClkdiv + 0] = false;
			break;

		case 16:
			bitstream[m_cbaseClkdiv + 1] = true;
			bitstream[m_cbaseClkdiv + 0] = true;
			break;

		default:
			LogError("GP4_LFOSC output divider must be 1, 2, 4, or 16\n");
			return false;
	}

	return true;
}
