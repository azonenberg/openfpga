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

#ifndef _WIN32
#include <termios.h>
#else
#include <conio.h>
#endif

using namespace std;

bool ReadTraceDelays();
bool CalibrateTraceDelays(Socket& sock, hdevice hdev);
bool MeasureDelay(Socket& sock, int src, int dst, float& delay);
bool PromptAndMeasureDelay(Socket& sock, int src, int dst, float& delay);
bool MeasureLutDelays(Socket& sock, hdevice hdev);

bool MeasurePinToPinDelays(Socket& sock, hdevice hdev);
bool MeasureCrossConnectionDelays(Socket& sock, hdevice hdev);
bool MeasureCrossConnectionDelay(Socket& sock, hdevice hdev, unsigned int matrix, unsigned int index, float& delay);
bool ProgramAndMeasureDelay(Socket& sock, hdevice hdev, vector<uint8_t>& bitstream, int src, int dst, float& delay);

void WaitForKeyPress();

//Delays from FPGA to DUT, one way, per pin
//TODO: DelayPair for these
float	g_pinDelays[21] = {0};	//0 is unused so we can use 1-based pin numbering like the chip does
								//For now, only pins 3-4-5 and 13-14-15 are used

//Delays within the device for input/output buffers
//Map from (src, dst) to delay
map<pair<int, int>, CellDelay> g_pinToPinDelaysX1;
map<pair<int, int>, CellDelay> g_pinToPinDelaysX2;
//TODO: x4 drive strength

//Delay through each cross connection
DelayPair g_eastXconnDelays[10];
DelayPair g_westXconnDelays[10];

//Propagation delay through each LUT to the output
map<Greenpak4LUT*, CellDelay> g_lutDelays;

//Device configuration
Greenpak4Device::GREENPAK4_PART part = Greenpak4Device::GREENPAK4_SLG46620;
Greenpak4IOB::PullDirection unused_pull = Greenpak4IOB::PULL_DOWN;
Greenpak4IOB::PullStrength  unused_drive = Greenpak4IOB::PULL_1M;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Entry point

int main(int argc, char* argv[])
{
	Severity console_verbosity = Severity::NOTICE;
	unsigned int nboard = 0;

	string server;
	int port = 0;

	//Parse command-line arguments
	for(int i=1; i<argc; i++)
	{
		string s(argv[i]);

		//Let the logger eat its args first
		if(ParseLoggerArguments(i, argc, argv, console_verbosity))
			continue;

		else if(s == "--port")
				port = atoi(argv[++i]);
		else if(s == "--server")
			server = argv[++i];

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

	//Connect to the server
	if( (server == "") || (port == 0) )
	{
		LogError("No server or port name specified\n");
		return 1;
	}
	Socket sock(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
	if(!sock.Connect(server, port))
	{
		LogError("Failed to connect to GreenpakTimingTest server\n");
		return 1;
	}

	LogNotice("GreenPAK timing characterization helper v0.1 by Andrew Zonenberg\n");

	//Open the dev board
	hdevice hdev = OpenBoard(nboard);
	if(!hdev)
		return 1;

	//Light it up so we know it's busy
	SetStatusLED(hdev, 1);

	//Do initial loopback characterization on the devkit and adapters before installing the chip
	if(!ReadTraceDelays())
	{
		if(!CalibrateTraceDelays(sock, hdev))
		{
			SetStatusLED(hdev, 0);
			Reset(hdev);
			USBCleanup(hdev);
			return 1;
		}
	}

	//Make sure we have the right chip plugged in
	SilegoPart spart = SilegoPart::SLG46620V;
	if(!InitializeHardware(hdev, spart))
	{
		SetStatusLED(hdev, 0);
		Reset(hdev);
		USBCleanup(hdev);
		return 1;
	}

	//Measure delay through each element
	/*
	if(!MeasureCrossConnectionDelays(sock, hdev))
		return 1;
	if(!MeasureLutDelays(sock, hdev))
		return 1;
	*/

	//Done
	LogNotice("Done, resetting board\n");
	SetStatusLED(hdev, 0);
	Reset(hdev);
	USBCleanup(hdev);

	return 0;
}

void WaitForKeyPress()
{
	LogNotice("Press any key to continue . . .\n");

#ifndef _WIN32
	struct termios oldt, newt;
	tcgetattr ( STDIN_FILENO, &oldt );
	newt = oldt;
	newt.c_lflag &= ~( ICANON | ECHO );
	tcsetattr ( STDIN_FILENO, TCSANOW, &newt );
	getchar();
	tcsetattr ( STDIN_FILENO, TCSANOW, &oldt );
#else
	_getch();
#endif
}

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
	g_pinDelays[3] = (delay_34 + delay_35 - delay_45) / 2;
	g_pinDelays[4] = delay_34 - g_pinDelays[3];
	g_pinDelays[5] = delay_35 - g_pinDelays[3];

	//Repeat for other side
	g_pinDelays[13] = (delay_1314 + delay_1315 - delay_1415) / 2;
	g_pinDelays[14] = delay_1314 - g_pinDelays[13];
	g_pinDelays[15] = delay_1315 - g_pinDelays[13];

	//Print results
	{
		LogNotice("Calculated trace delays:\n");
		LogIndenter li2;
		for(int i=3; i<=5; i++)
			LogNotice("Pin %2d to DUT: %.3f ns\n", i, g_pinDelays[i]);
		for(int i=13; i<=15; i++)
			LogNotice("Pin %2d to DUT: %.3f ns\n", i, g_pinDelays[i]);
	}

	//Write to file
	LogNotice("Writing calibration to file pincal.csv\n");
	FILE* fp = fopen("pincal.csv", "w");
	for(int i=3; i<=5; i++)
		fprintf(fp, "%d,%.3f\n", i, g_pinDelays[i]);
	for(int i=13; i<=15; i++)
		fprintf(fp, "%d,%.3f\n", i, g_pinDelays[i]);
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

		g_pinDelays[i] = f;

		LogNotice("Pin %2d to DUT: %.3f ns\n", i, f);
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
		g_eastXconnDelays[i] = DelayPair(d, -1);

		//west
		MeasureCrossConnectionDelay(sock, hdev, 1, i, d);
		g_westXconnDelays[i] = DelayPair(d, -1);
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
