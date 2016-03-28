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
	unsigned int countnum,
	unsigned int matrix,
	unsigned int ibase,
	unsigned int oword,
	unsigned int cbase)
	: Greenpak4BitstreamEntity(device, matrix, ibase, oword, cbase)	
	, m_depth(depth)
	, m_countnum(countnum)
	, m_reset(device->GetPowerRail(matrix, false))
	, m_clock(device->GetPowerRail(matrix, false))
	, m_hasFSM(has_fsm)
	, m_countVal(0)
	, m_preDivide(1)
	, m_resetMode(RISING_EDGE)
{

}

Greenpak4Counter::~Greenpak4Counter()
{
	
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Bitfile metadata

unsigned int Greenpak4Counter::GetConfigLen()
{
	return 0;
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
			if(!WriteMatrixSelector(bitstream, m_inputBaseWord + 1, m_device->GetPowerRail(m_matrix, false)))
				return false;
				
			//UP (ignored)
			if(!WriteMatrixSelector(bitstream, m_inputBaseWord + 1, m_device->GetPowerRail(m_matrix, false)))
				return false;
		}
	}
	
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Configuration
	
	//Count value (the same in all modes)
	//TODO: 14-bit mode
	if(m_depth > 8)
	{
		fprintf( stderr, "ERROR: 14-bit counters not implemented yet\n");
		return false;
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
	
	//COUNTER MODE
	if(true)
	{
		//Special bits if we have FSM
		if(m_hasFSM)
		{
			////////////////////////////////////////////////////////////////////////////////////////////////////////////
			// Input clock
			
			/*
			printf("clock = %s %p\n",
				m_clock->GetDescription().c_str(),
				dynamic_cast<Greenpak4LFOscillator*>(m_clock)
				);
			asm("int3");
			*/
			
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
				
				//4'b1010
				bitstream[nbase + 3] = true;
				bitstream[nbase + 2] = false;
				bitstream[nbase + 1] = true;
				bitstream[nbase + 0] = false;
			}
			else
			{
				fprintf(
					stderr,
					"ERROR: Counter %d input from %s not implemented\n",
					m_countnum,
					m_clock->GetDescription().c_str());
				return false;
			}
			
			////////////////////////////////////////////////////////////////////////////////////////////////////////////
			// Reset mode
			
			bitstream[nbase + 5] = (m_resetMode & 2) ? true : false;
			bitstream[nbase + 4] = (m_resetMode & 1) ? true : false;
			
			////////////////////////////////////////////////////////////////////////////////////////////////////////////
			// Block function
			
			//Counter / FSM mode selected
			bitstream[nbase + 6] = true;
			
			////////////////////////////////////////////////////////////////////////////////////////////////////////////
			// FSM input data source (not implemented for now)
			
			//NVM data (reset to max count)
			bitstream[nbase + 7] = false;
			bitstream[nbase + 8] = false;
			
			////////////////////////////////////////////////////////////////////////////////////////////////////////////
			// Value control
			
			//Reset to count=0
			//TODO: make this configurable
			bitstream[nbase + 9] = false;
		}
		
		else
		{
			fprintf(
				stderr,
				"ERROR: Counter %d non-FSM bitstream not implemented\n",
				m_countnum);
			return false;
		}
	}

	return true;
}
