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

DevkitCalibration g_devkitCal;
DeviceProperties g_deviceProperties;

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
	g_log_sinks.emplace(g_log_sinks.begin(), new ColoredSTDLogSink(console_verbosity));

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
	if(!MeasurePinToPinDelays(sock, hdev))
		return 1;

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
