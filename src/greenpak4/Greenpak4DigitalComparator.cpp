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

#include <log.h>
#include <Greenpak4.h>

using namespace std;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Construction / destruction

Greenpak4DigitalComparator::Greenpak4DigitalComparator(
	Greenpak4Device* device,
	unsigned int cmpnum,
	unsigned int matrix,
	unsigned int ibase,
	unsigned int oword,
	unsigned int cbase)
	: Greenpak4BitstreamEntity(device, matrix, ibase, oword, cbase)
	, m_powerDown(device->GetPower())
	, m_clock(device->GetGround())
	, m_cmpNum(cmpnum)
	, m_dcmpMode(false)
	, m_pwmDeadband(10)
	, m_compareGreaterEqual(false)
	, m_clockInvert(false)
	, m_pdSync(false)
{
	for(int i=0; i<8; i++)
	{
		m_inp[i] = device->GetGround();
		m_inn[i] = device->GetGround();
	}
}

Greenpak4DigitalComparator::~Greenpak4DigitalComparator()
{

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Accessors

string Greenpak4DigitalComparator::GetDescription()
{
	char buf[128];
	snprintf(buf, sizeof(buf), "DCMP_%u", m_cmpNum);
	return string(buf);
}

vector<string> Greenpak4DigitalComparator::GetInputPorts() const
{
	vector<string> r;
	r.push_back("PWRDN");
	return r;
}

void Greenpak4DigitalComparator::SetInput(string port, Greenpak4EntityOutput src)
{
	if(port.find("INP") != string::npos)
	{
		int i;
		if(1 != sscanf(port.c_str(), "INP[%d]", &i))
			LogWarning("GP_DCMP does not have an input named %s\n", port.c_str());
		else
			m_inp[i] = src;
	}

	else if(port.find("INN") != string::npos)
	{
		int i;
		if(1 != sscanf(port.c_str(), "INN[%d]", &i))
			LogWarning("GP_DCMP does not have an input named %s\n", port.c_str());
		else
			m_inn[i] = src;
	}

	else if(port == "PWRDN")
		m_powerDown = src;
	else if(port == "CLK")
		m_clock = src;

	else
		LogWarning("GP_DCMP does not have an input named %s\n", port.c_str());
}

vector<string> Greenpak4DigitalComparator::GetOutputPorts() const
{
	vector<string> r;
	r.push_back("OUTP");
	r.push_back("OUTN");
	r.push_back("GREATER");
	r.push_back("EQUAL");
	return r;
}

unsigned int Greenpak4DigitalComparator::GetOutputNetNumber(string port)
{
	if( (port == "OUTN") || (port == "EQUAL") )
		return m_outputBaseWord;
	else if( (port == "OUTP") || (port == "GREATER") )
		return m_outputBaseWord + 1;
	else
		return -1;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Serialization

bool Greenpak4DigitalComparator::CommitChanges()
{
	//Get our cell, or bail if we're unassigned
	auto ncell = dynamic_cast<Greenpak4NetlistCell*>(GetNetlistEntity());
	if(ncell == NULL)
		return true;

	//Set primitive type
	if(ncell->m_type == "GP_DCMP")
		m_dcmpMode = true;
	else if(ncell->m_type == "GP_PWM")
		m_dcmpMode = false;
	else
	{
		LogError("Greenpak4DigitalComparator must be mapped to a GP_DCMP or GP_PWM primitive\n");
		return false;
	}

	if(ncell->HasParameter("CLK_EDGE"))
		m_clockInvert = (ncell->m_parameters["CLK_EDGE"] == "FALLING") ? true : false;

	if(ncell->HasParameter("PWRDN_SYNC"))
		m_pdSync = atoi(ncell->m_parameters["PWRDN_SYNC"].c_str()) ? true : false;

	if(ncell->HasParameter("GREATER_OR_EQUAL"))
		m_compareGreaterEqual = atoi(ncell->m_parameters["GREATER_OR_EQUAL"].c_str()) ? true : false;

	return true;
}

bool Greenpak4DigitalComparator::Load(bool* /*bitstream*/)
{
	LogError("Unimplemented\n");
	return false;
}

bool Greenpak4DigitalComparator::Save(bool* bitstream)
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// INPUT BUS

	if(!WriteMatrixSelector(bitstream, m_inputBaseWord, m_powerDown))
		return false;

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// CONFIGURATION

	//PWM dead time
	if(m_pwmDeadband % 10)
	{
		LogError("PWM dead time needs to be a multiple of 10 ns\n");
		return false;
	}
	unsigned int deadtime = m_pwmDeadband / 10;
	deadtime --;
	if(deadtime > 7)
	{
		LogError("PWM dead time must be 80 ns or less\n");
		return false;
	}
	bitstream[m_configBase + 0] = (deadtime & 1) ? true : false;
	bitstream[m_configBase + 1] = (deadtime & 2) ? true : false;
	bitstream[m_configBase + 2] = (deadtime & 4) ? true : false;

	//Geq/eq mode
	bitstream[m_configBase + 3] = m_compareGreaterEqual;

	//Function select (DCMP vs PWM)
	bitstream[m_configBase + 4] = m_dcmpMode;

	//Verify that all 8 bits of each input came from the same entity
	for(int i=1; i<8; i++)
	{
		if(m_inp[i].GetRealEntity() != m_inp[i].GetRealEntity())
		{
			LogError("All bits of GP_DCMP INP must come from the same source node\n");
			return false;
		}

		if(m_inn[i].GetRealEntity() != m_inn[i].GetRealEntity())
		{
			LogError("All bits of GP_DCMP INN must come from the same source node\n");
			return false;
		}
	}

	//If no input, stop
	bool noClock = (m_clock.IsPowerRail() && !m_clock.GetPowerRailValue());
	bool noInP = (m_inp[0].IsPowerRail() && !m_inp[0].GetPowerRailValue());
	bool noInN = (m_inn[0].IsPowerRail() && !m_inn[0].GetPowerRailValue());
	if(noClock && noInP && noInN)
	{
		return true;
	}

	//If null inputs, but not unused, complain
	if(noClock || noInP || noInN)
	{
		LogError("Missing clock or input to DCMP_%d (%s clock, %s inP, %s inN)\n",
			m_cmpNum,
			noClock ? "no" : "",
			noInP ? "no" : "",
			noInN ? "no" : ""
			);
		return false;
	}

	//configBase + 5 is input clock source (clkbuf5 = 0, clkbuf2 = 1, anything else = illegal)
	auto entity = m_clock.GetRealEntity();
	auto ck = dynamic_cast<Greenpak4ClockBuffer*>(entity);
	if(!ck)
	{
		LogError("Input clock source for GP_DCMP must be a clock buffer\n");
		return false;
	}
	if(ck->GetBufferNumber() == 5)
		bitstream[m_configBase + 5] = false;
	else if(ck->GetBufferNumber() == 2)
		bitstream[m_configBase + 5] = true;
	else
	{
		LogError("Input clock source for GP_DCMP must be CLKBUF_2 or CLKBUF_5\n");
		return false;
	}

	//Clock inversion
	bitstream[m_configBase + 6] = m_clockInvert;

	//insert powerdown sync bit here for DCMP0, others don't have it
	unsigned int cbase = m_configBase + 7;
	if(m_cmpNum == 0)
	{
		bitstream[m_configBase + 7] = m_pdSync;
		cbase ++;
	}

	//Enable bit
	//Set this true if our power-down input is anything but Vdd
	bitstream[cbase + 0] = (m_powerDown != m_device->GetPower());

	//Invalid input
	if(m_inpsels.find(m_inp[0]) == m_inpsels.end())
	{
		LogError("Invalid DCMP input (tried to feed %s to %s INP)\n",
			m_inp[0].GetDescription().c_str(), GetDescription().c_str());
		return false;
	}

	//Valid input, hook it up
	else
	{
		unsigned int sel = m_inpsels[m_inp[0]];
		bitstream[cbase + 1] = (sel & 1) ? true : false;
		bitstream[cbase + 2] = (sel & 2) ? true : false;
	}

	//Invalid input
	if(m_innsels.find(m_inn[0]) == m_innsels.end())
	{
		LogError("Invalid DCMP input (tried to feed %s to %s INN)\n",
			m_inn[0].GetDescription().c_str(), GetDescription().c_str());
		return false;
	}

	//Valid input, hook it up
	else
	{
		unsigned int sel = m_innsels[m_inn[0]];
		bitstream[cbase + 3] = (sel & 1) ? true : false;
		bitstream[cbase + 4] = (sel & 2) ? true : false;
	}

	return true;
}
