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

#include <log.h>
#include <gpdevboard.h>
#include <math.h>

using namespace std;

bool RunTest(hdevice hdev);

int main(int argc, char* argv[])
{
	g_log_sinks.emplace(g_log_sinks.begin(), new STDLogSink(Severity::VERBOSE));

	//expect one arg: the bitstream
	if(argc != 2)
	{
		LogNotice("Usage: [testcase] bitstream.txt\n");
		return 1;
	}

	//Do standard board bringup
	hdevice hdev = OpenBoard(0);
	if(!hdev)
	{
		LogError("Failed to open board\n");
		return 1;
	}
	if(!SetStatusLED(hdev, 1))
		return 1;

	//Prepare to run the test
	if(!TestSetup(hdev, argv[1], 25000, 1.8, SilegoPart::SLG46140V))
	{
		SetStatusLED(hdev, 0);
		Reset(hdev);
		return 1;
	}

	//Run the actual test case
	LogNotice("\n");
	LogNotice("Running application test case\n");
	if(!RunTest(hdev))
	{
		SetStatusLED(hdev, 0);
		Reset(hdev);
		return 1;
	}

	//Turn off the LED before declaring success
	LogVerbose("\n");
	LogVerbose("Test complete, resetting board\n");
	SetStatusLED(hdev, 0);
	Reset(hdev);
	USBCleanup(hdev);
	return 0;
}

/**
	@brief The actual application-layer test
 */
bool RunTest(hdevice hdev)
{
	LogIndenter li;

	//Wipe all I/O config
	IOConfig ioConfig;
	for(size_t i = 2; i <= 20; i++)
		ioConfig.driverConfigs[i] = TP_RESET;
	if(!SetIOConfig(hdev, ioConfig))
		return false;

	//Drive signals to pin 2
	const int WRITE_PIN = 2;

	//Read signals from everything else
	//Need to translate between pin and socket numbers... TODO move this to gpdevboard?
	const int board_pins[11] = {3, 4, 5, 6, 7, 12, 13, 14, 15, 16, 17};
	const int chip_pins[11]  = {3, 4, 5, 6, 7, 9,  10, 11, 12, 13, 14};

	//Digital 0 and 1
	tuple<TPConfig, TPConfig, bool> sequence[] =
	{
		make_tuple(TP_GND, TP_FLIMSY_PULLUP, false),
		make_tuple(TP_VDD, TP_FLIMSY_PULLDOWN, true)
	};

	//Send a few cycles of each value
	for(auto config : sequence)
	{
		//Write the test bit
		bool bval = get<2>(config);
		LogNotice("Setting pin %d to %d\n", WRITE_PIN, bval);
		LogIndenter li;

		//Set up the I/O configuration
		ioConfig.driverConfigs[2] = get<0>(config);
		for(size_t i = 3; i <= 20; i++)
			ioConfig.driverConfigs[i] = get<1>(config);
		if(!SetIOConfig(hdev, ioConfig))
			return false;

		//Read every output pin
		float vexpected = bval ? 1.0 : 0;
		for(int j=0; j<11; j++)
		{
			if(!SelectADCChannel(hdev, board_pins[j]))
				return false;
			double vin;
			if(!ReadADC(hdev, vin))
				return false;

			LogNotice("Read chip pin %2d (board pin %2d): %.3f V\n", chip_pins[j], board_pins[j], vin);

			//Allow 50 mV noise tolerance
			float delta = fabs(vexpected - vin);
			if(delta > 0.05)
			{
				LogError("Out of tolerance (delta = %.3f V)\n", delta);
				return false;
			}
		}
	}

	return true;
}
