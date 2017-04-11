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
#ifndef _WIN32
#include <termios.h>
#else
#include <conio.h>
#endif

using namespace std;

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

	//Print status message
	LogNotice("GreenPAK timing characterization helper\n");
	LogNotice("Right now, just enables pins used by pmod-gpdevboard and sets up IO\n");

	//Open the dev board
	hdevice hdev = OpenBoard(nboard);
	if(!hdev)
		return 1;

	//Light up the status LED
	if(!SetStatusLED(hdev, 1))
		return 1;

	//DO NOT do anything that might make the IO voltage change, or send Vpp!

	//Reset all signal generators we may have used during setup
	if(!ResetAllSiggens(hdev))
		return 1;

	//Reset I/O config
	IOConfig ioConfig;
	for(size_t i = 2; i <= 20; i++)
		ioConfig.driverConfigs[i] = TP_RESET;
	if(!SetIOConfig(hdev, ioConfig))
		return false;

	unsigned int pins[] =
	{
		2, 3, 4, 5, 12, 13, 14, 15
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
		return 1;

	//Do not change this voltage! This is what the FPGA uses
	float voltage = 3.3;
	if(!ConfigureSiggen(hdev, 1, voltage))
		return 1;

	//Hold the lock until something happens
	bool lock = true;
	if(lock)
	{
		LogNotice("Holding lock on board, press any key to exit...\n");
		SetStatusLED(hdev, 1);

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

	//Done
	LogNotice("Done, resetting board\n");
	SetStatusLED(hdev, 0);
	Reset(hdev);
	USBCleanup(hdev);
}
