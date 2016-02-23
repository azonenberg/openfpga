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
	unsigned int cbase)
	: Greenpak4IOB(device, matrix, ibase, oword, cbase)
{
	
}

Greenpak4IOBTypeA::~Greenpak4IOBTypeA()
{
	
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Bitfile metadata

unsigned int Greenpak4IOBTypeA::GetConfigLen()
{
	//2 bit input mode, 2 bit output mode, 2 bit pullup value, 1 bit pullup enable
	return 7;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Serialization

bool Greenpak4IOBTypeA::Load(bool* /*bitstream*/)
{
	//TODO
	fprintf(stderr, "Greenpak4IOBTypeA::Load not implemented\n");
	exit(-1);
	
	return false;
}

bool Greenpak4IOBTypeA::Save(bool* bitstream)
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// INPUT BUS
	
	if(!WriteMatrixSelector(bitstream, m_inputBaseWord, m_outputSignal))
		return false;
	if(!WriteMatrixSelector(bitstream, m_inputBaseWord+1, m_outputEnable))
		return false;
		
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
	
	//Output drive strength 2
	switch(m_driveStrength)
	{
		case DRIVE_1X:
			bitstream[m_configBase+2] = false;
			break;
			
		case DRIVE_2X:
			bitstream[m_configBase+2] = true;
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
	
	//Pullup/down resistor strength 5:4, direction 6
	if(m_pullDirection == PULL_NONE)
	{
		bitstream[m_configBase + 4] = false;
		bitstream[m_configBase + 5] = false;
		
		//don't care, pull circuit disconnected
		bitstream[m_configBase + 6] = false;
	}
	else
	{
		switch(m_pullStrength)
		{
			case PULL_10K:
				bitstream[m_configBase + 4] = true;
				bitstream[m_configBase + 5] = false;
				break;
				
			case PULL_100K:
				bitstream[m_configBase + 4] = false;
				bitstream[m_configBase + 5] = true;
				break;
				
			case PULL_1M:
				bitstream[m_configBase + 4] = true;
				bitstream[m_configBase + 5] = true;
				break;
				
			default:
				fprintf(stderr, "ERROR: Invalid pull strength\n");
				return false;
		}
		
		switch(m_pullDirection)
		{
			case PULL_UP:
				bitstream[m_configBase + 6] = true;
				break;
			
			case PULL_DOWN:
				bitstream[m_configBase + 6] = false;
				break;
				
			default:
				fprintf(stderr, "ERROR: Invalid pull direction\n");
				return false;
		}
	}
	
	return true;
}
