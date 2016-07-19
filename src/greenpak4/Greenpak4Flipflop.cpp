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

Greenpak4Flipflop::Greenpak4Flipflop(
	Greenpak4Device* device,
	unsigned int ffnum,
	bool has_sr,
	unsigned int matrix,
	unsigned int ibase,
	unsigned int oword,
	unsigned int cbase)
	: Greenpak4BitstreamEntity(device, matrix, ibase, oword, cbase)
	, m_ffnum(ffnum)
	, m_hasSR(has_sr)
	, m_initValue(false)
	, m_srmode(false)
	, m_input(device->GetGround())
	, m_clock(device->GetGround())
	, m_nsr(device->GetPower())
{
}

Greenpak4Flipflop::~Greenpak4Flipflop()
{
	
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Accessors

vector<string> Greenpak4Flipflop::GetInputPorts() const
{
	vector<string> r;
	r.push_back("D");
	r.push_back("CLK");
	if(m_hasSR)
		r.push_back("nSR");
	return r;
}

void Greenpak4Flipflop::SetInput(string port, Greenpak4EntityOutput src)
{
	if(port == "CLK")
		m_clock = src;
	else if(port == "D")
		m_input = src;
	
	//multiple set/reset modes possible
	else if(port == "nSR")
		m_nsr = src;
	else if(port == "nSET")
	{
		m_srmode = true;
		m_nsr = src;
	}
	else if(port == "nRST")
	{
		m_srmode = false;
		m_nsr = src;
	}
	
	//ignore anything else silently (should not be possible since synthesis would error out)
}

vector<string> Greenpak4Flipflop::GetOutputPorts() const
{
	vector<string> r;
	r.push_back("Q");
	return r;
}

unsigned int Greenpak4Flipflop::GetOutputNetNumber(string port)
{
	if(port == "Q")
		return m_outputBaseWord;
	else
		return -1;
}

string Greenpak4Flipflop::GetDescription()
{
	char buf[128];
	snprintf(buf, sizeof(buf), "DFF_%u", m_ffnum);
	return string(buf);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Serialization

void Greenpak4Flipflop::CommitChanges()
{
	//Get our cell, or bail if we're unassigned
	auto ncell = dynamic_cast<Greenpak4NetlistCell*>(GetNetlistEntity());
	if(ncell == NULL)
		return;
	
	if(ncell->HasParameter("SRMODE"))
		m_srmode = (ncell->m_parameters["SRMODE"] == "1");

	if(ncell->HasParameter("INIT"))
		m_initValue = (ncell->m_parameters["INIT"] == "1");
}

bool Greenpak4Flipflop::Load(bool* /*bitstream*/)
{
	LogFatal("Unimplemented\n");
}

bool Greenpak4Flipflop::Save(bool* bitstream)
{
	//Sanity check: cannot have set/reset on a DFF, only a DFFSR
	bool has_sr = !m_nsr.IsPowerRail();
	if(has_sr && !m_hasSR)
		LogError("Tried to configure set/reset on a DFF cell with no S/R input\n");
	
	//Check if we're unused (input and clock pins are tied to ground)
	bool no_input = ( m_input.IsPowerRail() && !m_input.GetPowerRailValue() );
	bool no_clock = ( m_input.IsPowerRail() && !m_clock.GetPowerRailValue() );
	bool unused = (no_input && no_clock);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// INPUT BUS
	
	if(m_hasSR)
	{
		//Set/reset defaults to constant 1 if not hooked up
		//but if we have set/reset then use that.
		//If we're totally unused, hold us in reset
		Greenpak4EntityOutput sr = m_device->GetPower();
		if(unused)
			sr = m_device->GetGround();
		else if(has_sr)
			sr = m_nsr;
		if(!WriteMatrixSelector(bitstream, m_inputBaseWord + 0, sr))
			return false;
			
		//Hook up data and clock
		if(!WriteMatrixSelector(bitstream, m_inputBaseWord + 1, m_input))
			return false;
		if(!WriteMatrixSelector(bitstream, m_inputBaseWord + 2, m_clock))
			return false;
	}
	
	else
	{
		//Hook up data and clock
		if(!WriteMatrixSelector(bitstream, m_inputBaseWord + 0, m_input))
			return false;
		if(!WriteMatrixSelector(bitstream, m_inputBaseWord + 1, m_clock))
			return false;
	}
	
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Configuration
	
	if(m_hasSR)
	{
		//Mode select (hard wire to DFF for now)
		bitstream[m_configBase + 0] = false;
		
		//Output polarity (hard wire to active-high for now)
		bitstream[m_configBase + 1] = false;
		
		//Set/reset mode
		bitstream[m_configBase + 2] = m_srmode;
			
		//Initial state
		bitstream[m_configBase + 3] = m_initValue;
	}
	
	else
	{
		//Mode select (hard wire to DFF for now)
		bitstream[m_configBase + 0] = false;
		
		//Output polarity (hard wire to active-high for now)
		bitstream[m_configBase + 1] = false;
		
		//Initial state
		bitstream[m_configBase + 2] = m_initValue;
	}
	
	return true;
}

