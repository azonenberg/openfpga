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

//Device configuration
Greenpak4Device::GREENPAK4_PART part = Greenpak4Device::GREENPAK4_SLG46620;
Greenpak4IOB::PullDirection unused_pull = Greenpak4IOB::PULL_DOWN;
Greenpak4IOB::PullStrength  unused_drive = Greenpak4IOB::PULL_1M;

bool PromptAndMeasureDelay(Socket& sock, int src, int dst, float& delay);
bool MeasureCrossConnectionDelay(Socket& sock, hdevice hdev, unsigned int matrix, unsigned int index, float& delay);

bool ProgramAndMeasureDelay(
	Socket& sock,
	hdevice hdev,
	vector<uint8_t>& bitstream,
	int src,
	int dst,
	float& delay);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Initial measurement calibration

bool CalibrateTraceDelays(Socket& sock, hdevice hdev)
{
	LogNotice("Calibrate FPGA-to-DUT trace delays\n");
	LogIndenter li;

	if(!IOSetup(hdev))
		return false;

	float delay_34;
	float delay_35;
	float delay_45;

	float delay_1314;
	float delay_1315;
	float delay_1415;

	//Measure A+B, A+C, B+C delays for one side of chip
	if(!PromptAndMeasureDelay(sock, 3, 4, delay_34))
		return false;
	if(!PromptAndMeasureDelay(sock, 3, 5, delay_35))
		return false;
	if(!PromptAndMeasureDelay(sock, 4, 5, delay_45))
		return false;

	//and for the other side
	if(!PromptAndMeasureDelay(sock, 13, 14, delay_1314))
		return false;
	if(!PromptAndMeasureDelay(sock, 13, 15, delay_1315))
		return false;
	if(!PromptAndMeasureDelay(sock, 14, 15, delay_1415))
		return false;

	//Pin 3 delay = A = ( (A+B) + (A+C) - (B+C) ) / 2 = (delay_34 + delay_35 - delay_45) / 2
	g_devkitCal.pinDelays[3] = DelayPair((delay_34 + delay_35 - delay_45) / 2, -1);
	g_devkitCal.pinDelays[4] = DelayPair(delay_34 - g_devkitCal.pinDelays[3].rising, -1);
	g_devkitCal.pinDelays[5] = DelayPair(delay_35 - g_devkitCal.pinDelays[3].rising, -1);

	//Repeat for other side
	g_devkitCal.pinDelays[13] = DelayPair((delay_1314 + delay_1315 - delay_1415) / 2, -1);
	g_devkitCal.pinDelays[14] = DelayPair(delay_1314 - g_devkitCal.pinDelays[13].rising, -1);
	g_devkitCal.pinDelays[15] = DelayPair(delay_1315 - g_devkitCal.pinDelays[13].rising, -1);

	//TODO: Falling edges

	//Print results
	{
		LogNotice("Calculated trace delays:\n");
		LogIndenter li2;
		for(int i=3; i<=5; i++)
			LogNotice("Pin %2d to DUT rising: %.3f ns\n", i, g_devkitCal.pinDelays[i].rising);
		for(int i=13; i<=15; i++)
			LogNotice("Pin %2d to DUT rising: %.3f ns\n", i, g_devkitCal.pinDelays[i].rising);
	}

	//Write to file
	LogNotice("Writing calibration to file pincal.csv\n");
	FILE* fp = fopen("pincal.csv", "w");
	for(int i=3; i<=5; i++)
		fprintf(fp, "%d,%.3f\n", i, g_devkitCal.pinDelays[i].rising);
	for(int i=13; i<=15; i++)
		fprintf(fp, "%d,%.3f\n", i, g_devkitCal.pinDelays[i].rising);
	fclose(fp);

	//Prompt to put the actual DUT in
	LogNotice("Insert a SLG46620 into the socket\n");
	WaitForKeyPress();

	return true;
}

bool ReadTraceDelays()
{
	FILE* fp = fopen("pincal.csv", "r");
	if(!fp)
		return false;

	int i;
	float f;
	LogNotice("Reading pin calibration from pincal.csv (delete this file to force re-calibration)...\n");
	LogIndenter li;
	while(2 == fscanf(fp, "%d, %f", &i, &f))
	{
		if( (i > 20) || (i < 1) )
			continue;

		g_devkitCal.pinDelays[i] = DelayPair(f, -1);

		LogNotice("Pin %2d to DUT rising: %.3f ns\n", i, f);
	}

	return true;
}

bool PromptAndMeasureDelay(Socket& sock, int src, int dst, float& delay)
{
	LogNotice("Use a jumper to short pins %d and %d on the ZIF header\n", src, dst);

	LogIndenter li;

	WaitForKeyPress();
	if(!MeasureDelay(sock, src, dst, delay))
		return false;
	LogNotice("Measured pin-to-pin delay: %.3f ns\n", delay);
	return true;
}

bool MeasureDelay(Socket& sock, int src, int dst, float& delay)
{
	//Send test parameters
	uint8_t		drive = src;
	uint8_t		sample = dst;
	if(!sock.SendLooped(&drive, 1))
		return false;
	if(!sock.SendLooped(&sample, 1))
		return false;

	//Read the results back
	uint8_t ok;
	if(!sock.RecvLooped(&ok, 1))
		return false;
	if(!sock.RecvLooped((uint8_t*)&delay, sizeof(delay)))
		return false;

	if(!ok)
	{
		LogError("Couldn't measure delay (open circuit?)\n");
		return false;
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Load the bitstream onto the device and measure a pin-to-pin delay

bool ProgramAndMeasureDelay(Socket& sock, hdevice hdev, vector<uint8_t>& bitstream, int src, int dst, float& delay)
{
	//Emulate the device
	LogVerbose("Loading new bitstream\n");
	if(!DownloadBitstream(hdev, bitstream, DownloadMode::EMULATION))
		return false;
	if(!PostProgramSetup(hdev))
		return false;

	//Measure delay betweens pin 3 and 5

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Characterize I/O buffers

bool MeasurePinToPinDelays(Socket& sock, hdevice hdev)
{

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Characterize cross-connections

bool MeasureCrossConnectionDelays(Socket& sock, hdevice hdev)
{
	LogNotice("Measuring cross-connection delays...\n");
	LogIndenter li;

	float d;
	for(int i=0; i<10; i++)
	{
		//east
		MeasureCrossConnectionDelay(sock, hdev, 0, i, d);
		//g_eastXconnDelays[i] = DelayPair(d, -1);

		//west
		MeasureCrossConnectionDelay(sock, hdev, 1, i, d);
		//g_westXconnDelays[i] = DelayPair(d, -1);
	}
}

bool MeasureCrossConnectionDelay(Socket& sock, hdevice hdev, unsigned int matrix, unsigned int index, float& delay)
{
	delay = -1;

	//Create the device

	//Done
	string dir = (matrix == 0) ? "east" : "west";
	LogNotice("%s %d: %.3f ns\n", dir.c_str(), index, delay);
	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Characterize LUTs

bool MeasureLutDelays(Socket& sock, hdevice hdev)
{
	//LogNotice("Measuring LUT delays...\n");
	//LogIndenter li;

	/*
	//Hardware configuration


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


	*/
	return true;
}

