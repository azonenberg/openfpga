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

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Construction / destruction

Greenpak4IOBTypeA::Greenpak4IOBTypeA(
	Greenpak4Device* device,
	unsigned int matrix,
	unsigned int ibase,
	unsigned int oword,
	unsigned int cbase,
	unsigned int flags)
	: Greenpak4IOB(device, matrix, ibase, oword, cbase, flags)
{
	
}

Greenpak4IOBTypeA::~Greenpak4IOBTypeA()
{
	
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Bitfile metadata

unsigned int Greenpak4IOBTypeA::GetConfigLen()
{
	//2 bit input mode, 2 bit output mode, 2 bit pullup value, 1 bit pullup enable.
	//Possibly one more bit for super driver.
	//Input-only pins have no config 

	if(m_flags & IOB_FLAG_INPUTONLY)
		return 5;

	if(m_flags & IOB_FLAG_X4DRIVE)
		return 8;

	return 7;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Serialization

bool Greenpak4IOBTypeA::Load(bool* /*bitstream*/)
{
	//TODO
	fprintf(stderr, "Greenpak4IOBTypeA::Load not implemented\n");	
	return false;
}

bool Greenpak4IOBTypeA::Save(bool* bitstream)
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// INPUT BUS
	
	//If we have no output pad driver, skip the driver inputs
	if(m_flags & IOB_FLAG_INPUTONLY)
	{
		//Verify that they are tied sanely (output enable is ground, signal is dontcare)
		Greenpak4PowerRail* oe = dynamic_cast<Greenpak4PowerRail*>(m_outputEnable);
		if(oe == NULL)
		{
			fprintf(stderr, "ERROR: Tried to tie OE of an input-only pin to something other than a power rail\n");
			return false;
		}
		if(oe->GetDigitalValue() != false)
		{
			fprintf(stderr, "ERROR: Tried to tie OE of an input-only pin to something other than ground\n");
			return false;
		}
	}
	
	else
	{	
		if(!WriteMatrixSelector(bitstream, m_inputBaseWord, m_outputSignal))
			return false;
		if(!WriteMatrixSelector(bitstream, m_inputBaseWord+1, m_outputEnable))
			return false;
	}
		
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// CONFIGURATION
		
	//Input threshold 1:0
	switch(m_inputThreshold)
	{ 
		case THRESHOLD_ANALOG:
			bitstream[m_configBase+0] = true;
			bitstream[m_configBase+1] = true;
			break;
			
		case THRESHOLD_LOW:
			bitstream[m_configBase+0] = false;
			bitstream[m_configBase+1] = true;
			break;
			
		case THRESHOLD_NORMAL:
			bitstream[m_configBase+1] = false;
			bitstream[m_configBase+0] = m_schmittTrigger;
			break;
			
		default:
			fprintf(stderr, "ERROR: Invalid IOB threshold\n");
			return false;
	}
	
	//Base address for upcoming stuff, skipping output driver if not implemented
	unsigned int base = m_configBase + 2;
	
	if(! (m_flags & IOB_FLAG_INPUTONLY) )
	{
		//Output drive strength 2, 7 if super driver present
		switch(m_driveStrength)
		{
			case DRIVE_1X:
				bitstream[m_configBase+2] = false;
				if(m_flags & IOB_FLAG_X4DRIVE)
					bitstream[m_configBase+7] = false;
				break;
				
			case DRIVE_2X:
				bitstream[m_configBase+2] = true;
				if(m_flags & IOB_FLAG_X4DRIVE)
					bitstream[m_configBase+7] = false;
				break;
			
			//If we have a super driver, write x4 as double x2
			case DRIVE_4X:
				bitstream[m_configBase+2] = true;
				if(m_flags & IOB_FLAG_X4DRIVE)
					bitstream[m_configBase+7] = true;
				else
				{
					fprintf(stderr, "ERROR: Invalid drive strength (x4 drive not present on this pin\n");
					return false;
				}
				break;
			
			default:
				fprintf(stderr, "ERROR: Invalid drive strength\n");
				return false;
		}
	
		//Output buffer type 3
		switch(m_driveType)
		{
			case DRIVE_PUSHPULL:
				bitstream[m_configBase+3] = false;
				break;
				
			case DRIVE_NMOS_OPENDRAIN:
				bitstream[m_configBase+3] = true;
				break;
				
			default:
				fprintf(stderr, "ERROR: Invalid driver type\n");
				return false;
		}
		
		base += 2;
	}
	
	//Pullup/down resistor strength 5:4, direction 6
	if(m_pullDirection == PULL_NONE)
	{
		bitstream[base] = false;
		bitstream[base+1] = false;
		
		//don't care, pull circuit disconnected
		bitstream[base+2] = false;
	}
	else
	{
		switch(m_pullStrength)
		{
			case PULL_10K:
				bitstream[base] = true;
				bitstream[base + 1] = false;
				break;
				
			case PULL_100K:
				bitstream[base] = false;
				bitstream[base + 1] = true;
				break;
				
			case PULL_1M:
				bitstream[base] = true;
				bitstream[base + 1] = true;
				break;
				
			default:
				fprintf(stderr, "ERROR: Invalid pull strength\n");
				return false;
		}
		
		switch(m_pullDirection)
		{
			case PULL_UP:
				bitstream[base + 2] = true;
				break;
			
			case PULL_DOWN:
				bitstream[base + 2] = false;
				break;
				
			default:
				fprintf(stderr, "ERROR: Invalid pull direction\n");
				return false;
		}
	}
	
	return true;
}
