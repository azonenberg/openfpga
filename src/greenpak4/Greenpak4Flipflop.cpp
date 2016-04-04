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
{
	m_input = device->GetPowerRail(0);
	m_clock = device->GetPowerRail(0);
	
	m_nsr = device->GetPowerRail(1);
}

Greenpak4Flipflop::~Greenpak4Flipflop()
{
	
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Bitfile metadata

unsigned int Greenpak4Flipflop::GetConfigLen()
{
	//DFF/latch, set/reset mode if implemented, initial value, output polarity
	if(m_hasSR)
		return 4;
	else
		return 3;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Accessors

vector<string> Greenpak4Flipflop::GetInputPorts()
{
	vector<string> r;
	r.push_back("D");
	r.push_back("CLK");
	if(m_hasSR)
		r.push_back("nSR");
	return r;
}

vector<string> Greenpak4Flipflop::GetOutputPorts()
{
	vector<string> r;
	r.push_back("Q");
	return r;
}

string Greenpak4Flipflop::GetDescription()
{
	char buf[128];
	if(m_hasSR)
		snprintf(buf, sizeof(buf), "DFFSR_%d", m_ffnum);
	else
		snprintf(buf, sizeof(buf), "DFF_%d", m_ffnum);
	return string(buf);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Serialization

bool Greenpak4Flipflop::Load(bool* /*bitstream*/)
{
	printf("Greenpak4Flipflop::Load() not yet implemented\n");
	return false;
	//return true;
}

bool Greenpak4Flipflop::Save(bool* bitstream)
{
	//Sanity check: cannot have both set and reset
	Greenpak4PowerRail* nsr = dynamic_cast<Greenpak4PowerRail*>(m_nsr);
	bool has_sr = (nsr == NULL);
	if(has_sr && !m_hasSR)
		fprintf(stderr, "ERROR: Tried to configure set/reset on a DFF cell with no S/R input\n");
	
	//Check if we're unused (input and clock pins are tied to ground)
	Greenpak4PowerRail* ni = dynamic_cast<Greenpak4PowerRail*>(m_input);
	Greenpak4PowerRail* nc = dynamic_cast<Greenpak4PowerRail*>(m_clock);
	bool no_input = ( (ni != NULL) && (ni->GetDigitalValue() == false) );
	bool no_clock = ( (nc != NULL) && (nc->GetDigitalValue() == false) );
	bool unused = (no_input && no_clock);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// INPUT BUS
	
	if(m_hasSR)
	{
		//Set/reset defaults to constant 1 if not hooked up
		//but if we have set/reset then use that.
		//If we're totally unused, hold us in reset
		Greenpak4BitstreamEntity* sr = m_device->GetPowerRail(true);
		if(unused)
			sr = m_device->GetPowerRail(false);
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

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Accessors

void Greenpak4Flipflop::SetInputSignal(Greenpak4BitstreamEntity* sig)
{
	m_input = sig;
}

void Greenpak4Flipflop::SetClockSignal(Greenpak4BitstreamEntity* sig)
{
	m_clock = sig;
}

void Greenpak4Flipflop::SetNSRSignal(Greenpak4BitstreamEntity* sig)
{
	m_nsr = sig;
}
