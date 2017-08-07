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

Greenpak4IOBTypeA::Greenpak4IOBTypeA(
	Greenpak4Device* device,
	unsigned int pin_num,
	unsigned int matrix,
	unsigned int ibase,
	unsigned int oword,
	unsigned int cbase,
	unsigned int flags)
	: Greenpak4IOB(device, pin_num, matrix, ibase, oword, cbase, flags)
{

}

Greenpak4IOBTypeA::~Greenpak4IOBTypeA()
{

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Accessors

string Greenpak4IOBTypeA::GetDescription() const
{
	char buf[128];
	snprintf(buf, sizeof(buf), "P%u", m_pinNumber);
	return string(buf);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Serialization

bool Greenpak4IOBTypeA::Load(bool* bitstream)
{
	//If we have no output pad driver, skip the driver inputs
	if(m_flags & IOB_FLAG_INPUTONLY)
	{
		m_outputEnable = m_device->GetGround();
		m_outputSignal = m_device->GetGround();
	}

	//We have an output driver, read the signals feeding it
	else
	{
		ReadMatrixSelector(bitstream, m_inputBaseWord + 0, m_matrix, m_outputSignal);
		ReadMatrixSelector(bitstream, m_inputBaseWord + 1, m_matrix, m_outputEnable);
	}

	int thresh = (bitstream[m_configBase + 1] << 1) | bitstream[m_configBase + 0];
	switch(thresh)
	{
		case 3:
			m_inputThreshold = THRESHOLD_ANALOG;
			m_schmittTrigger = false;
			break;

		case 2:
			m_inputThreshold = THRESHOLD_LOW;
			m_schmittTrigger = false;
			break;

		case 1:
			m_inputThreshold = THRESHOLD_NORMAL;
			m_schmittTrigger = true;
			break;

		case 0:
			m_inputThreshold = THRESHOLD_NORMAL;
			m_schmittTrigger = false;
			break;
	}

	//Base address for upcoming stuff, skipping output driver if not implemented
	unsigned int base = m_configBase + 2;

	//Load OBUF config if we're not input-only
	if(! (m_flags & IOB_FLAG_INPUTONLY) )
	{
		//If second driver is off, we're x1 for sure
		if(!bitstream[m_configBase + 2])
			m_driveStrength = DRIVE_1X;

		//Second driver is on. Do we have a quad?
		else
		{
			if( (m_flags & IOB_FLAG_X4DRIVE) && bitstream[m_configBase + 7] )
				m_driveStrength = DRIVE_4X;

			else
				m_driveStrength = DRIVE_2X;
		}

		if(bitstream[m_configBase + 3])
			m_driveType = DRIVE_NMOS_OPENDRAIN;
		else
			m_driveType = DRIVE_PUSHPULL;

		base += 2;
	}

	//Load pull direction (just up or down, float comes in the next block)
	if(bitstream[base + 2])
		m_pullDirection = PULL_UP;
	else
		m_pullDirection = PULL_DOWN;

	//Load pull value and float flag
	int pullVal = (bitstream[base+1] << 1) | bitstream[base];
	switch(pullVal)
	{
		case 0:
			m_pullDirection = PULL_NONE;
			m_pullStrength = PULL_1M;
			break;

		case 1:
			m_pullStrength = PULL_10K;

		case 2:
			m_pullStrength = PULL_100K;

		case 3:
			m_pullStrength = PULL_1M;
	}

	return true;
}

bool Greenpak4IOBTypeA::Save(bool* bitstream)
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// INPUT BUS

	//If we have no output pad driver, skip the driver inputs
	if(m_flags & IOB_FLAG_INPUTONLY)
	{
		//Verify that they are tied sanely (output enable is ground, signal is dontcare)
		if(!m_outputEnable.IsPowerRail())
		{
			LogError("Tried to tie OE of an input-only pin (%s) to something other than a power rail\n",
				GetDescription().c_str());
			return false;
		}
		if(m_outputEnable.GetPowerRailValue() != false)
		{
			LogError("Tried to tie OE of an input-only pin to something other than ground\n");
			return false;
		}
	}

	else
	{
		//If our output is from a Vref, special processing needed
		if(m_outputSignal.IsVoltageReference())
		{
			//Float the digital output buffer
			Greenpak4EntityOutput gnd = m_device->GetGround();
			if(!WriteMatrixSelector(bitstream, m_inputBaseWord, gnd))
				return false;
			if(!WriteMatrixSelector(bitstream, m_inputBaseWord+1, gnd))
				return false;

			//Configure the analog output
			auto vref = dynamic_cast<Greenpak4VoltageReference*>(m_outputSignal.GetRealEntity());
			unsigned int sel = vref->GetMuxSel();
			bitstream[m_analogConfigBase + 1] = (sel & 2) ? true : false;
			bitstream[m_analogConfigBase + 0] = (sel & 1) ? true : false;
		}

		//If our output is from a DAC, special processing needed
		else if(m_outputSignal.IsDAC())
		{
			//Float the digital output buffer
			Greenpak4EntityOutput gnd = m_device->GetGround();
			if(!WriteMatrixSelector(bitstream, m_inputBaseWord, gnd))
				return false;
			if(!WriteMatrixSelector(bitstream, m_inputBaseWord+1, gnd))
				return false;

			//Set the analog selector to constant 2'b11
			//SLG4662x specific!
			if(
				(m_device->GetPart() == Greenpak4Device::GREENPAK4_SLG46620) ||
				(m_device->GetPart() == Greenpak4Device::GREENPAK4_SLG46621)
			)
			{
				bitstream[m_analogConfigBase + 1] = true;
				bitstream[m_analogConfigBase + 0] = true;
			}
			else
				LogError("Greenpak4IOBTypeA: not implemented for 46140 yet\n");
		}

		//If our output is from a PGA, special processing needed
		else if(m_outputSignal.IsPGA())
		{
			//Float the digital output buffer
			Greenpak4EntityOutput gnd = m_device->GetGround();
			if(!WriteMatrixSelector(bitstream, m_inputBaseWord, gnd))
				return false;
			if(!WriteMatrixSelector(bitstream, m_inputBaseWord+1, gnd))
				return false;

			//No special configuration required
		}

		//Digital output and enable
		else
		{
			if(!WriteMatrixSelector(bitstream, m_inputBaseWord, m_outputSignal))
				return false;
			if(!WriteMatrixSelector(bitstream, m_inputBaseWord+1, m_outputEnable))
				return false;
		}
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
			LogError("Invalid IOB threshold\n");
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
					LogError("Invalid drive strength (x4 drive not present on this pin\n");
					return false;
				}
				break;

			default:
				LogError("Invalid drive strength\n");
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
				LogError("Invalid driver type\n");
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
				LogError("Invalid pull strength\n");
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
				LogError("Invalid pull direction\n");
				return false;
		}
	}

	return true;
}
