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

#include <cstdlib>
#include <cstring>
#include <cmath>
#include <unistd.h>
#include <log.h>
#include <gpdevboard.h>
#include <Greenpak4.h>

using namespace std;

hdevice InitializeHardware(unsigned int nboard, SilegoPart expectedPart);
bool DoInitializeHardware(hdevice hdev, SilegoPart expectedPart);
bool PostProgramSetup(hdevice hdev);
bool IOReset(hdevice hdev);
bool IOSetup(hdevice hdev);
bool PowerSetup(hdevice hdev);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Entry point

int main(int argc, char* argv[])
{
	Severity console_verbosity = Severity::NOTICE;
	unsigned int nboard = 0;

	//Parse command-line arguments
	for(int i=1; i<argc; i++)
	{
		string s(argv[i]);

		//Let the logger eat its args first
		if(ParseLoggerArguments(i, argc, argv, console_verbosity))
			continue;

		else if(s == "--device")
		{
			if(i+1 < argc)
				nboard = atoi(argv[++i]);

			else
			{
				printf("--device requires an argument\n");
				return 1;
			}
		}

		else
		{
			printf("Unrecognized command-line argument \"%s\", use --help\n", s.c_str());
			return 1;
		}
	}

	//Set up logging
	g_log_sinks.emplace(g_log_sinks.begin(), new STDLogSink(console_verbosity));

	//Hardware configuration
	Greenpak4Device::GREENPAK4_PART part = Greenpak4Device::GREENPAK4_SLG46620;
	SilegoPart spart = SilegoPart::SLG46620V;
	Greenpak4IOB::PullDirection unused_pull = Greenpak4IOB::PULL_DOWN;
	Greenpak4IOB::PullStrength  unused_drive = Greenpak4IOB::PULL_1M;

	//Print status message and set up the board
	LogNotice("GreenPAK timing characterization helper\n");
	hdevice hdev = InitializeHardware(nboard, spart);
	if(!hdev)
		return 1;

	//Create the device object
	Greenpak4Device device(part, unused_pull, unused_drive);
	device.SetIOPrecharge(false);
	device.SetDisableChargePump(false);
	device.SetLDOBypass(false);
	device.SetNVMRetryCount(1);

	//Configure pin 5 as an output and drive it high
	auto iob = device.GetIOB(5);
	auto vdd = device.GetPower();
	iob->SetInput("IN", vdd);
	iob->SetDriveType(Greenpak4IOB::DRIVE_PUSHPULL);
	iob->SetDriveStrength(Greenpak4IOB::DRIVE_1X);

	//Generate a bitstream
	vector<uint8_t> bitstream;
	device.WriteToBuffer(bitstream, 0, false);
	device.WriteToFile("/tmp/test.txt", 0, false);

	//Emulate the device
	LogVerbose("Loading new bitstream\n");
	if(!DownloadBitstream(hdev, bitstream, DownloadMode::EMULATION))
		return 1;
	if(!PostProgramSetup(hdev))
		return 1;

	//Wait a bit
	usleep(5000 * 1000);

	//Done
	LogNotice("Done, resetting board\n");
	SetStatusLED(hdev, 0);
	Reset(hdev);
	USBCleanup(hdev);

	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Board initialization

hdevice InitializeHardware(unsigned int nboard, SilegoPart expectedPart)
{
	//Open the dev board
	hdevice hdev = OpenBoard(nboard);
	if(!hdev)
		return NULL;

	//Light it up so we know it's busy
	SetStatusLED(hdev, 1);

	//Set it up
	if(!DoInitializeHardware(hdev, expectedPart))
	{
		SetStatusLED(hdev, 0);
		Reset(hdev);
		USBCleanup(hdev);
		return NULL;
	}

	return hdev;
}

bool PostProgramSetup(hdevice hdev)
{
	//Clear I/Os from programming mode
	if(!IOReset(hdev))
		return false;

	//Load the new configuration
	if(!IOSetup(hdev))
		return false;
	if(!PowerSetup(hdev))
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

bool PowerSetup(hdevice hdev)
{
	//Do not change this voltage! This is what the FPGA uses
	//TODO: sweep from 3.15 to 3.45 to do voltage corner analysis
	float voltage = 3.3;
	return ConfigureSiggen(hdev, 1, voltage);
}

bool DoInitializeHardware(hdevice hdev, SilegoPart expectedPart)
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
