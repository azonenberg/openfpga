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

#include "gp4tchar.h"

using namespace std;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Board initialization

bool PostProgramSetup(hdevice hdev, int voltage_mv)
{
	SetStatusLED(hdev, 1);

	//Clear I/Os from programming mode
	if(!IOReset(hdev))
		return false;

	//Load the new configuration
	if(!IOSetup(hdev))
		return false;
	if(!PowerSetup(hdev, voltage_mv))
		return false;

	return true;
}

bool IOReset(hdevice hdev)
{
	IOConfig ioConfig;
	for(size_t i = 2; i <= 20; i++)
		ioConfig.driverConfigs[i] = TP_RESET;
	if(!SetIOConfig(hdev, ioConfig))
		return false;

	return true;
}

bool IOSetup(hdevice hdev)
{
	//Enable the I/O pins
	unsigned int pins[] =
	{
		3, 4, 5, 12, 13, 14, 15
	};

	//Configure I/O voltage
	IOConfig config;
	for(auto npin : pins)
	{
		config.driverConfigs[npin] = TP_FLOAT;
		config.ledEnabled[npin] = true;
		config.expansionEnabled[npin] = true;
	}
	if(!SetIOConfig(hdev, config))
		return false;

	return true;
}

bool PowerSetup(hdevice hdev, int voltage_mv)
{
	float voltage = voltage_mv * 0.001f;
	if(voltage > 3.455)
	{
		LogError("Tried to set voltage to %.3f mV, disallowing for FPGA safety\n", voltage);
		return false;
	}

	return ConfigureSiggen(hdev, 1, voltage);
}

bool InitializeHardware(hdevice hdev, SilegoPart expectedPart)
{
	//Figure out what's there
	SilegoPart detectedPart = SilegoPart::UNRECOGNIZED;
	vector<uint8_t> programmedBitstream;
	BitstreamKind bitstreamKind;
	if(!DetectPart(hdev, detectedPart, programmedBitstream, bitstreamKind))
		return false;

	//Complain if we got something funny
	if(expectedPart != detectedPart)
	{
		LogError("Detected wrong part (not what we're trying to characterize)\n");
		return false;
	}

	//Get the board into a known state
	if(!ResetAllSiggens(hdev))
		return false;
	if(!IOReset(hdev))
		return false;
	if(!IOSetup(hdev))
		return false;
	if(!PowerSetup(hdev))
		return false;

	return true;
}
