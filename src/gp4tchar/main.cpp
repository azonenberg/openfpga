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

bool LoadTimingData(string fname);

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

	//Load timing data from disk
	LogNotice("Loading timing data...\n");
	string tfname = "timing.json";
	if(!LoadTimingData(tfname))
		return 1;

	//Measure delay through each element
	if(!MeasurePinToPinDelays(sock, hdev))
		return 1;
	/*
	if(!MeasureCrossConnectionDelays(sock, hdev))
		return 1;
	if(!MeasureLUTDelays(sock, hdev))
		return 1;
	*/

	/*
	if(!MeasureInverterDelays(sock, hdev))
		return 1;
	*/

	//Save to disk
	LogNotice("Saving timing data to file %s\n", tfname.c_str());
	g_calDevice.SaveTimingData(tfname.c_str());

	//Print output
	LogNotice("Dumping timing data...\n");
	{
		LogIndenter li;
		g_calDevice.PrintTimingData();
	}

	//Done
	LogNotice("Done, resetting board\n");
	SetStatusLED(hdev, 0);
	Reset(hdev);
	USBCleanup(hdev);

	return 0;
}

bool LoadTimingData(string fname)
{
	//Open the file (non-existence is a legal no-op, return success silently)
	FILE* fp = fopen(fname.c_str(), "rb");
	if(fp == NULL)
		return true;
	if(0 != fseek(fp, 0, SEEK_END))
	{
		LogError("Failed to seek to end of timing data file %s\n", fname.c_str());
		fclose(fp);
		return false;
	}
	size_t len = ftell(fp);
	if(0 != fseek(fp, 0, SEEK_SET))
	{
		LogError("Failed to seek to start of timing data file %s\n", fname.c_str());
		return false;
	}
	char* json_string = new char[len + 1];
	json_string[len] = '\0';
	if(len != fread(json_string, 1, len, fp))
	{
		LogError("Failed to read contents of timing data file %s\n", fname.c_str());
		delete[] json_string;
		fclose(fp);
		return false;
	}
	fclose(fp);

	//Parse the JSON
	json_tokener* tok = json_tokener_new();
	if(!tok)
	{
		LogError("Failed to create JSON tokenizer object\n");
		delete[] json_string;
		return false;
	}
	json_tokener_error err;
	json_object* object = json_tokener_parse_verbose(json_string, &err);
	if(NULL == object)
	{
		const char* desc = json_tokener_error_desc(err);
		LogError("JSON parsing failed (err = %s)\n", desc);
		delete[] json_string;
		return false;
	}

	//Load stuff
	if(!g_calDevice.LoadTimingData(object))
		return false;

	//Done
	json_object_put(object);
	json_tokener_free(tok);
	return true;
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
