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
#include "solver.h"

using namespace std;

//Device configuration
Greenpak4Device::GREENPAK4_PART part = Greenpak4Device::GREENPAK4_SLG46620;
Greenpak4IOB::PullDirection unused_pull = Greenpak4IOB::PULL_DOWN;
Greenpak4IOB::PullStrength  unused_drive = Greenpak4IOB::PULL_1M;

bool PromptAndMeasureDelay(Socket& sock, int src, int dst, float& value);
bool MeasureCrossConnectionDelay(Socket& sock, hdevice hdev, unsigned int matrix, unsigned int index, float& delay);

bool MeasurePinToPinDelay(
	Socket& sock,
	hdevice hdev,
	int src,
	int dst,
	Greenpak4IOB::DriveStrength drive,
	float& delay);

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

	//Set up the variables
	KnapsackVariable p3("FPGA pin 3 to DUT rising");
	KnapsackVariable p4("FPGA pin 4 to DUT rising");
	KnapsackVariable p5("FPGA pin 5 to DUT rising");
	KnapsackVariable p13("FPGA pin 13 to DUT rising");
	KnapsackVariable p14("FPGA pin 14 to DUT rising");
	KnapsackVariable p15("FPGA pin 15 to DUT rising");

	//Measure each pair of pins individually
	float delay;
	if(!PromptAndMeasureDelay(sock, 3, 4, delay))
		return false;
	KnapsackEquation e1(delay);
	e1.AddVariable(p3);
	e1.AddVariable(p4);

	if(!PromptAndMeasureDelay(sock, 3, 5, delay))
		return false;
	KnapsackEquation e2(delay);
	e2.AddVariable(p3);
	e2.AddVariable(p5);

	if(!PromptAndMeasureDelay(sock, 4, 5, delay))
		return false;
	KnapsackEquation e3(delay);
	e3.AddVariable(p4);
	e3.AddVariable(p5);

	if(!PromptAndMeasureDelay(sock, 13, 14, delay))
		return false;
	KnapsackEquation e4(delay);
	e4.AddVariable(p13);
	e4.AddVariable(p14);

	if(!PromptAndMeasureDelay(sock, 13, 15, delay))
		return false;
	KnapsackEquation e5(delay);
	e5.AddVariable(p13);
	e5.AddVariable(p15);

	if(!PromptAndMeasureDelay(sock, 14, 15, delay))
		return false;
	KnapsackEquation e6(delay);
	e6.AddVariable(p14);
	e6.AddVariable(p15);

	//Solve the system
	KnapsackProblem p;
	p.AddEquation(e1);
	p.AddEquation(e2);
	p.AddEquation(e3);
	p.AddEquation(e4);
	p.AddEquation(e5);
	p.AddEquation(e6);
	if(!p.Solve())
		return false;

	//Extract the values
	g_devkitCal.pinDelays[3] = p3.m_value;
	g_devkitCal.pinDelays[4] = p4.m_value;
	g_devkitCal.pinDelays[5] = p5.m_value;
	g_devkitCal.pinDelays[13] = p13.m_value;
	g_devkitCal.pinDelays[14] = p14.m_value;
	g_devkitCal.pinDelays[15] = p15.m_value;

	//Print results
	{
		LogNotice("Calculated trace delays:\n");
		LogIndenter li2;
		for(int i=3; i<=5; i++)
			LogNotice("FPGA pin %2d to DUT rising: %.3f ns\n", i, g_devkitCal.pinDelays[i].rising);
		for(int i=13; i<=15; i++)
			LogNotice("FPGA pin %2d to DUT rising: %.3f ns\n", i, g_devkitCal.pinDelays[i].rising);
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
	LogNotice("Reading devkit calibration from pincal.csv (delete this file to force re-calibration)...\n");
	LogIndenter li;
	while(2 == fscanf(fp, "%d, %f", &i, &f))
	{
		if( (i > 20) || (i < 1) )
			continue;

		g_devkitCal.pinDelays[i] = DelayPair(f, -1);

		LogNotice("FPGA pin %2d to DUT rising: %.3f ns\n", i, f);
	}

	return true;
}

bool PromptAndMeasureDelay(Socket& sock, int src, int dst, float& value)
{
	LogNotice("Use a jumper to short pins %d and %d on the ZIF header\n", src, dst);

	LogIndenter li;

	WaitForKeyPress();
	if(!MeasureDelay(sock, src, dst, value))
		return false;
	LogNotice("Measured pin-to-pin: %.3f ns\n", value);
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

	usleep (1000 * 10);

	//Measure delay between the affected pins
	return MeasureDelay(sock, src, dst, delay);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Characterize I/O buffers

bool MeasurePinToPinDelays(Socket& sock, hdevice hdev)
{
	LogNotice("Measuring pin-to-pin delays (through same crossbar)...\n");
	LogIndenter li;

	int pins[] = {3, 4, 5, 13, 14, 15};
	Greenpak4IOB::DriveStrength drives[] = {Greenpak4IOB::DRIVE_1X, Greenpak4IOB::DRIVE_2X};

	float delay;
	LogNotice("+------+------+--------+--------+--------+--------+----------+----------+\n");
	LogNotice("| From |  To  |  x1 ↑  |  x1 ↓  |  x2 ↑  |  x2 ↓  | Delta ↑  | Delta ↓  |\n");
	LogNotice("+------+------+--------+--------+--------+--------+----------+----------+\n");
	for(auto src : pins)
	{
		for(auto dst : pins)
		{
			//Can't do loopback
			if(src == dst)
				continue;

			//If the pins are on opposite sides of the device, skip it
			//(no cross connections, we only want matrix + buffer delay)
			if(src < 10 && dst > 10)
				continue;
			if(src > 10 && dst < 10)
				continue;

			auto sdpair = PinPair(src, dst);
			for(auto drive : drives)
			{
				//Do the measurement
				if(!MeasurePinToPinDelay(sock, hdev, src, dst, drive, delay))
					return false;
				g_deviceProperties.ioDelays[drive][sdpair] = DelayPair(delay, -1);
			}

			auto x1 = g_deviceProperties.ioDelays[drives[0]][sdpair];
			auto x2 = g_deviceProperties.ioDelays[drives[1]][sdpair];
			LogNotice("| %4d |  %2d  | %6.3f | %6.3f | %6.3f | %6.3f | %8.3f | %8.3f |\n",
				src,
				dst,
				x1.rising,
				x1.falling,
				x2.rising,
				x2.falling,
				x1.rising - x2.rising,
				x1.falling - x2.falling
				);
		}
	}
	LogNotice("+------+------+--------+--------+--------+--------+----------+----------+\n");

	return true;
}

/**
	@brief Measure the delay for a single (src, dst) pin tuple
 */
bool MeasurePinToPinDelay(
	Socket& sock,
	hdevice hdev,
	int src,
	int dst,
	Greenpak4IOB::DriveStrength drive,
	float& delay)
{
	delay = -1;

	//Create the device object
	Greenpak4Device device(part, unused_pull, unused_drive);
	device.SetIOPrecharge(false);
	device.SetDisableChargePump(false);
	device.SetLDOBypass(false);
	device.SetNVMRetryCount(1);

	//Configure the input pin
	auto vss = device.GetGround();
	auto srciob = device.GetIOB(src);
	srciob->SetInput("OE", vss);
	auto din = srciob->GetOutput("OUT");

	//Configure the output pin
	auto vdd = device.GetPower();
	auto dstiob = device.GetIOB(dst);
	dstiob->SetInput("IN", din);
	dstiob->SetInput("OE", vdd);
	dstiob->SetDriveType(Greenpak4IOB::DRIVE_PUSHPULL);
	dstiob->SetDriveStrength(drive);

	//Generate a bitstream
	vector<uint8_t> bitstream;
	device.WriteToBuffer(bitstream, 0, false);
	device.WriteToFile("/tmp/test.txt", 0, false);			//for debug in case of failure

	//Get the delay
	if(!ProgramAndMeasureDelay(sock, hdev, bitstream, src, dst, delay))
		return false;

	//Subtract the PCB trace delay at each end of the line
	delay -= g_devkitCal.pinDelays[src].rising;
	delay -= g_devkitCal.pinDelays[dst].rising;

	return true;
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

	return true;
}

bool MeasureCrossConnectionDelay(
	Socket& /*sock*/,
	hdevice /*hdev*/,
	unsigned int matrix,
	unsigned int index,
	float& delay)
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

bool MeasureLutDelays(Socket& /*sock*/, hdevice /*hdev*/)
{
	//LogNotice("Measuring LUT delays...\n");
	//LogIndenter li;
	return true;
}

