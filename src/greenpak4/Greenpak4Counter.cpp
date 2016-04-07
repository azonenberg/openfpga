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

Greenpak4Counter::Greenpak4Counter(
	Greenpak4Device* device,
	unsigned int depth,
	bool has_fsm,
	bool has_wspwrdn,
	bool has_edgedetect,
	bool has_pwm,
	unsigned int countnum,
	unsigned int matrix,
	unsigned int ibase,
	unsigned int oword,
	unsigned int cbase)
	: Greenpak4BitstreamEntity(device, matrix, ibase, oword, cbase)	
	, m_depth(depth)
	, m_countnum(countnum)
	, m_reset(device->GetPowerRail(false))	//default reset is ground
	, m_clock(device->GetPowerRail(false))
	, m_hasFSM(has_fsm)
	, m_countVal(0)
	, m_preDivide(1)
	, m_resetMode(BOTH_EDGE)						//default reset mode is both edges
	, m_hasWakeSleepPowerDown(has_wspwrdn)
	, m_hasEdgeDetect(has_edgedetect)
	, m_hasPWM(has_pwm)
{

}

Greenpak4Counter::~Greenpak4Counter()
{
	
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Bitfile metadata

unsigned int Greenpak4Counter::GetConfigLen()
{
	//Counter bits + 10 config bits + edge detect
	if(m_hasFSM)
		return m_depth + 10 + (m_hasEdgeDetect ? 1 : 0);
		
	//Counter bits + 7 config bits
	else if(m_hasPWM)
		return m_depth + 7;
	
	//Counter bits + 7 config bits + W/S
	else
		return m_depth + 7 + (m_hasWakeSleepPowerDown ? 1 : 0);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Accessors

string Greenpak4Counter::GetDescription()
{
	char buf[128];
	snprintf(buf, sizeof(buf), "COUNT%d_%d", m_depth, m_countnum);
	return string(buf);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Serialization

void Greenpak4Counter::CommitChanges()
{
	//Get our cell, or bail if we're unassigned
	auto ncell = dynamic_cast<Greenpak4NetlistCell*>(GetNetlistEntity());
	if(ncell == NULL)
		return;
		
	if(ncell->HasParameter("RESET_MODE"))
	{
		Greenpak4Counter::ResetMode mode = Greenpak4Counter::RISING_EDGE;
		string p = ncell->m_parameters["RESET_MODE"];
		if(p == "RISING")
			mode = Greenpak4Counter::RISING_EDGE;
		else if(p == "FALLING")
			mode = Greenpak4Counter::FALLING_EDGE;
		else if(p == "BOTH")
			mode = Greenpak4Counter::BOTH_EDGE;
		else if(p == "LEVEL")
			mode = Greenpak4Counter::HIGH_LEVEL;
		else
		{
			fprintf(
				stderr,
				"ERROR: Counter \"%s\" has illegal reset mode \"%s\" (must be RISING, FALLING, BOTH, or LEVEL)\n",
				ncell->m_name.c_str(),
				p.c_str());
			exit(-1);
		}
		SetResetMode(mode);
	}
	
	if(ncell->HasParameter("COUNT_TO"))
		SetCounterValue(atoi(ncell->m_parameters["COUNT_TO"].c_str()));
	
	if(ncell->HasParameter("CLKIN_DIVIDE"))
		SetPreDivide(atoi(ncell->m_parameters["CLKIN_DIVIDE"].c_str()));
}

vector<string> Greenpak4Counter::GetInputPorts()
{
	vector<string> r;
	r.push_back("RST");
	return r;
}

vector<string> Greenpak4Counter::GetOutputPorts()
{
	vector<string> r;
	r.push_back("OUT");
	return r;
}

bool Greenpak4Counter::Load(bool* /*bitstream*/)
{
	printf("Greenpak4Counter::Load() not yet implemented\n");
	return false;
}

bool Greenpak4Counter::Save(bool* bitstream)
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// INPUT BUS
	
	//COUNTER MODE
	if(true)
	{
		if(m_hasFSM)
		{
			//Reset input
			if(!WriteMatrixSelector(bitstream, m_inputBaseWord + 0, m_reset))
				return false;
				
			//KEEP (ignored)
			if(!WriteMatrixSelector(bitstream, m_inputBaseWord + 1, m_device->GetPowerRail(false)))
				return false;
				
			//UP (ignored)
			if(!WriteMatrixSelector(bitstream, m_inputBaseWord + 1, m_device->GetPowerRail(false)))
				return false;
		}
		
		else
		{
			//Reset input
			if(!WriteMatrixSelector(bitstream, m_inputBaseWord + 0, m_reset))
				return false;
		}
		
		//TODO: dedicated input clock matrix stuff
	}
	
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Configuration
	
	//Count value (the same in all modes, just varies with depth)
	if(m_depth > 8)
	{
		bitstream[m_configBase + 13] = (m_countVal & 0x2000) ? true : false;
		bitstream[m_configBase + 12] = (m_countVal & 0x1000) ? true : false;
		bitstream[m_configBase + 11] = (m_countVal & 0x0800) ? true : false;
		bitstream[m_configBase + 10] = (m_countVal & 0x0400) ? true : false;
		bitstream[m_configBase + 9]  = (m_countVal & 0x0200) ? true : false;
		bitstream[m_configBase + 8]  = (m_countVal & 0x0100) ? true : false;
	}
	bitstream[m_configBase + 7] = (m_countVal & 0x80) ? true : false;
	bitstream[m_configBase + 6] = (m_countVal & 0x40) ? true : false;
	bitstream[m_configBase + 5] = (m_countVal & 0x20) ? true : false;
	bitstream[m_configBase + 4] = (m_countVal & 0x10) ? true : false;
	bitstream[m_configBase + 3] = (m_countVal & 0x08) ? true : false;
	bitstream[m_configBase + 2] = (m_countVal & 0x04) ? true : false;
	bitstream[m_configBase + 1] = (m_countVal & 0x02) ? true : false;
	bitstream[m_configBase + 0] = (m_countVal & 0x01) ? true : false;
	
	//Base for remaining configuration data
	uint32_t nbase = m_configBase + m_depth;
	
	//Get the real clock node (even if in the wrong matrix) for RTTI
	Greenpak4BitstreamEntity* clk = m_clock->GetRealEntity();
	bool unused = false;
	
	//Check if we're unused
	if(dynamic_cast<Greenpak4PowerRail*>(clk) != NULL)
		unused = true;
	
	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Input clock
	
	//FSM/PWM have 4-bit clock selector
	if(m_hasFSM || m_hasPWM)
	{			
		//Low-frequency oscillator
		if(dynamic_cast<Greenpak4LFOscillator*>(clk) != NULL)
		{
			if(m_preDivide != 1)
			{
				fprintf(
					stderr,
					"ERROR: Counter %d does not support pre-divider values other than 1 when clocked by LF osc\n",
					m_countnum);
				return false;
			}
			
			//4'b1010
			bitstream[nbase + 3] = true;
			bitstream[nbase + 2] = false;
			bitstream[nbase + 1] = true;
			bitstream[nbase + 0] = false;
		}
		//TODO: RCOSC with dividers
		//TODO: Matrix outputs
		
		//Ring oscillator
		else if(dynamic_cast<Greenpak4RingOscillator*>(clk) != NULL)
		{
			if(m_preDivide != 1)
			{
				fprintf(
					stderr,
					"ERROR: Counter %d does not support pre-divider values other than 1 when clocked by ring osc\n",
					m_countnum);
				return false;
			}
			
			//4'b1000
			bitstream[nbase + 3] = true;
			bitstream[nbase + 2] = false;
			bitstream[nbase + 1] = false;
			bitstream[nbase + 0] = false;
		}
		
		//TODO: SPI clock
		//TODO: FSM clock
		//TODO: PWM clock
		else if(!unused)
		{
			fprintf(
				stderr,
				"ERROR: Counter %d input from %s not implemented\n",
				m_countnum,
				m_clock->GetDescription().c_str());
			return false;
		}
		nbase += 4;
	}
	
	//others have 3-bit selector
	else
	{
		//Low-frequency oscillator
		if(dynamic_cast<Greenpak4LFOscillator*>(m_clock->GetRealEntity()) != NULL)
		{
			if(m_preDivide != 1)
			{
				fprintf(
					stderr,
					"ERROR: Counter %d does not support pre-divider values other than 1 when clocked by LF osc\n",
					m_countnum);
				return false;
			}
			
			//3'b100
			bitstream[nbase + 2] = true;
			bitstream[nbase + 1] = false;
			bitstream[nbase + 0] = false;
		}
		
		//Ring oscillator
		else if(dynamic_cast<Greenpak4RingOscillator*>(m_clock->GetRealEntity()) != NULL)
		{
			if(m_preDivide != 1)
			{
				fprintf(
					stderr,
					"ERROR: Counter %d does not support pre-divider values other than 1 when clocked by ring osc\n",
					m_countnum);
				return false;
			}
			
			//3'b110
			bitstream[nbase + 2] = true;
			bitstream[nbase + 1] = true;
			bitstream[nbase + 0] = false;
		}
		
		//TODO: RCOSC with dividers
		//TODO: cascading
		//TODO: Matrix outputs
		else if(!unused)
		{
			fprintf(
				stderr,
				"ERROR: Counter %d input from %s not implemented\n",
				m_countnum,
				m_clock->GetDescription().c_str());
			return false;
		}
	
		nbase += 3;
	}
		
	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Main counter logic
	
	//FSM capable (see CNT/DLY4)
	if(m_hasFSM)
	{			
		////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Reset mode
		
		bitstream[nbase + 1] = (m_resetMode & 2) ? true : false;
		bitstream[nbase + 0] = (m_resetMode & 1) ? true : false;
		
		////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Block function
		
		nbase += 2;
		
		if(m_hasEdgeDetect)
		{
			//if unused, go to delay mode
			if(unused)
			{
				bitstream[nbase + 1] = false;
				bitstream[nbase + 0] = false;
			}
			
			//Counter / FSM / PWM mode selected
			else
			{
				bitstream[nbase + 1] = false;
				bitstream[nbase + 0] = true;
			}
			
			nbase += 2;
		}
		
		else
		{
			//if unused, go to delay mode
			if(unused)
				bitstream[nbase + 0] = false;
			
			//Counter / FSM / PWM mode selected
			else
				bitstream[nbase + 0] = true;
				
			nbase ++;
		}
		
		////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// FSM input data source (not implemented for now)
		
		//NVM data (FSM data = max count)
		bitstream[nbase + 0] = false;
		bitstream[nbase + 1] = false;
		
		////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Value control
		
		//Reset count to zero
		//TODO: make this configurable
		bitstream[nbase + 2] = false;
	}
		
	//Not FSM capable (see CNT/DLY0)
	else
	{			
		////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Reset mode
		
		bitstream[nbase + 1] = (m_resetMode & 2) ? true : false;
		bitstream[nbase + 0] = (m_resetMode & 1) ? true : false;
		
		////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Block function
		
		//PWM mode is only 1 bit (see CNT/DLY80
		if(m_hasPWM)
		{
			//if unused, 1'b0 = delay
			if(unused)
				bitstream[nbase + 2] = false;
			
			//1'b1 = CNT
			else
				bitstream[nbase + 2] = true;
		}
		
		//Not PWM capable (see CNT/DLY0)
		else
		{		
			//if unused, 2'b00 = delay
			if(unused)
			{
				bitstream[nbase + 3] = false;
				bitstream[nbase + 2] = false;
			}
			
			//2'b01 = CNT
			else
			{
				bitstream[nbase + 3] = false;
				bitstream[nbase + 2] = true;
			}
		}
		
		////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Wake/sleep power down
		
		//For now, always run normally
		if(m_hasWakeSleepPowerDown && !unused)
			bitstream[nbase + 4] = true;
		
	}

	return true;
}
