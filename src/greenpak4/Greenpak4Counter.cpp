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
	, m_reset(device->GetGround())	//default reset is ground
	, m_clock(device->GetGround())
	, m_up(device->GetGround())
	, m_keep(device->GetGround())
	, m_hasFSM(has_fsm)
	, m_countVal(0)
	, m_preDivide(1)
	, m_resetMode(BOTH_EDGE)						//default reset mode is both edges
	, m_resetValue(ZERO)
	, m_hasWakeSleepPowerDown(has_wspwrdn)
	, m_hasEdgeDetect(has_edgedetect)
	, m_hasPWM(has_pwm)
{

}

Greenpak4Counter::~Greenpak4Counter()
{
	
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Accessors

string Greenpak4Counter::GetDescription()
{
	char buf[128];
	if(m_hasFSM)
		snprintf(buf, sizeof(buf), "COUNT%u_ADV_%u", m_depth, m_countnum);
	else
		snprintf(buf, sizeof(buf), "COUNT%u_%u", m_depth, m_countnum);
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
		string p = ncell->m_parameters["RESET_MODE"];
		if(p == "RISING")
			m_resetMode = Greenpak4Counter::RISING_EDGE;
		else if(p == "FALLING")
			m_resetMode = Greenpak4Counter::FALLING_EDGE;
		else if(p == "BOTH")
			m_resetMode = Greenpak4Counter::BOTH_EDGE;
		else if(p == "LEVEL")
			m_resetMode = Greenpak4Counter::HIGH_LEVEL;
		else
		{
			LogError(
				"Counter \"%s\" has illegal reset mode \"%s\" "
				"(must be RISING, FALLING, BOTH, or LEVEL)\n",
				ncell->m_name.c_str(),
				p.c_str());
			exit(-1);
		}
	}

	if(ncell->HasParameter("RESET_VALUE"))
	{
		string p = ncell->m_parameters["RESET_VALUE"];
		if(p == "ZERO")
			m_resetValue = ZERO;
		else if(p == "COUNT_TO")
			m_resetValue = COUNT_TO;
		else
		{
			LogError(
				"Counter \"%s\" has illegal reset value \"%s\" "
				"(must be ZERO or COUNT_TO)\n",
				ncell->m_name.c_str(),
				p.c_str());
			exit(-1);
		}
	}
	
	if(ncell->HasParameter("COUNT_TO"))
		m_countVal = (atoi(ncell->m_parameters["COUNT_TO"].c_str()));
	
	if(ncell->HasParameter("CLKIN_DIVIDE"))
		m_preDivide = (atoi(ncell->m_parameters["CLKIN_DIVIDE"].c_str()));
}

vector<string> Greenpak4Counter::GetInputPorts() const
{
	vector<string> r;
	r.push_back("RST");
	if(m_hasFSM) {
		r.push_back("UP");
		r.push_back("KEEP");
	}
	return r;
}

void Greenpak4Counter::SetInput(string port, Greenpak4EntityOutput src)
{
	if(port == "RST")
		m_reset = src;
	else if(port == "CLK")
		m_clock = src;
	else if(port == "UP")
		m_up = src;
	else if(port == "KEEP")
		m_keep = src;
	
	//ignore anything else silently (should not be possible since synthesis would error out)
}

vector<string> Greenpak4Counter::GetOutputPorts() const
{
	vector<string> r;
	r.push_back("OUT");
	return r;
}

unsigned int Greenpak4Counter::GetOutputNetNumber(string port)
{
	if(port == "OUT")
		return m_outputBaseWord;
	else
		return -1;
}

bool Greenpak4Counter::Load(bool* /*bitstream*/)
{
	LogFatal("Unimplemented\n");
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
				
			//Keep FSM input
			if(!WriteMatrixSelector(bitstream, m_inputBaseWord + 1, m_keep))
				return false;
				
			//Up FSM input
			if(!WriteMatrixSelector(bitstream, m_inputBaseWord + 2, m_up))
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
	
	//Get the bitstream node for RTTI checks
	Greenpak4BitstreamEntity* clk = m_clock.GetRealEntity();
	bool unused = false;
	
	//Check if we're unused
	if(m_clock.IsPowerRail())
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
				LogError(
					"Counter %d does not support pre-divider values other than 1 when clocked by LF osc\n",
					m_countnum);
				return false;
			}
			
			//4'b1010
			bitstream[nbase + 3] = true;
			bitstream[nbase + 2] = false;
			bitstream[nbase + 1] = true;
			bitstream[nbase + 0] = false;
		}

		//TODO: Matrix outputs
		
		//Ring oscillator
		else if(dynamic_cast<Greenpak4RingOscillator*>(clk) != NULL)
		{
			if(m_preDivide != 1)
			{
				LogError(
					"Counter %d does not support pre-divider values other than 1 when clocked by ring osc\n",
					m_countnum);
				return false;
			}
			
			//4'b1000
			bitstream[nbase + 3] = true;
			bitstream[nbase + 2] = false;
			bitstream[nbase + 1] = false;
			bitstream[nbase + 0] = false;
		}
		
		//RC oscillator
		//TODO: 12 is a legal value for some counters but not others, need to consider this during placement??
		else if(dynamic_cast<Greenpak4RCOscillator*>(clk) != NULL)
		{
			switch(m_preDivide)
			{
				//4'b0000
				case 1:
					bitstream[nbase + 3] = false;
					bitstream[nbase + 2] = false;
					bitstream[nbase + 1] = false;
					bitstream[nbase + 0] = false;
					break;
				
				//4'b0001
				case 4:
					bitstream[nbase + 3] = false;
					bitstream[nbase + 2] = false;
					bitstream[nbase + 1] = false;
					bitstream[nbase + 0] = true;
					break;
				
				//4'b0010
				case 12:
					bitstream[nbase + 3] = false;
					bitstream[nbase + 2] = false;
					bitstream[nbase + 1] = true;
					bitstream[nbase + 0] = false;
					break;
				
				//4'b0011
				case 24:
					bitstream[nbase + 3] = false;
					bitstream[nbase + 2] = false;
					bitstream[nbase + 1] = true;
					bitstream[nbase + 0] = true;
					break;
				
				//4'b0100
				case 64:
					bitstream[nbase + 3] = false;
					bitstream[nbase + 2] = true;
					bitstream[nbase + 1] = false;
					bitstream[nbase + 0] = false;
					break;
					
				default:
					LogError(
						"Counter %d does not support pre-divider values other than 1/4/12/24/64 "
						"when clocked by ring osc\n",
						m_countnum);
					return false;
			}
		}
		
		//TODO: SPI clock
		//TODO: FSM clock
		//TODO: PWM clock
		else if(!unused)
		{
			LogError("Counter %d input from %s not implemented\n",
				m_countnum,
				m_clock.GetDescription().c_str());
			return false;
		}
		nbase += 4;
	}
	
	//others have 3-bit selector
	else
	{
		//Low-frequency oscillator
		if(dynamic_cast<Greenpak4LFOscillator*>(clk) != NULL)
		{
			if(m_preDivide != 1)
			{
				LogError(
					"Counter %d does not support pre-divider values other than 1 when clocked by LF osc\n",
					m_countnum);
				return false;
			}
			
			//3'b100
			bitstream[nbase + 2] = true;
			bitstream[nbase + 1] = false;
			bitstream[nbase + 0] = false;
		}
		
		//Ring oscillator
		else if(dynamic_cast<Greenpak4RingOscillator*>(clk) != NULL)
		{
			if(m_preDivide != 1)
			{
				LogError(
					"Counter %d does not support pre-divider values other than 1 when clocked by ring osc\n",
					m_countnum);
				return false;
			}
			
			//3'b110
			bitstream[nbase + 2] = true;
			bitstream[nbase + 1] = true;
			bitstream[nbase + 0] = false;
		}
		
		//RC oscillator
		else if(dynamic_cast<Greenpak4RCOscillator*>(clk) != NULL)
		{
			switch(m_preDivide)
			{
				//3'b000
				case 1:
					bitstream[nbase + 2] = false;
					bitstream[nbase + 1] = false;
					bitstream[nbase + 0] = false;
					break;
					
				//3'b001
				case 4:
					bitstream[nbase + 2] = false;
					bitstream[nbase + 1] = false;
					bitstream[nbase + 0] = true;
					break;
					
				//3'b010
				case 24:
					bitstream[nbase + 2] = false;
					bitstream[nbase + 1] = true;
					bitstream[nbase + 0] = false;
					break;
					
				//3'b011
				case 64:
					bitstream[nbase + 2] = false;
					bitstream[nbase + 1] = true;
					bitstream[nbase + 0] = true;
					break;
					
				default:
					LogError(
						"Counter %d does not support pre-divider values other than 1/4/24/64 "
							"when clocked by RC osc\n",
						m_countnum);
					return false;
			}
		}
		
		//TODO: cascading
		//TODO: Matrix outputs
		
		else if(!unused)
		{
			LogError("Counter %d input from %s not implemented\n",
				m_countnum,
				m_clock.GetDescription().c_str());
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
		// FSM input data source
		
		//NVM data (FSM data = max count)
		//NB: on SLG46620, FSM0 and FSM1 encoding for this register does not match
		bitstream[nbase + 0] = false;
		bitstream[nbase + 1] = false;

		////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Value control
		
		//If unused, reset to zero
		if(unused)
			bitstream[nbase + 2] = false;

		else
		{
			//Set (to FSM data source)
			if(m_resetValue == COUNT_TO)
				bitstream[nbase + 2] = true;

			//Reset (to zero)
			else if(m_resetValue == ZERO)
				bitstream[nbase + 2] = false;
		}
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
