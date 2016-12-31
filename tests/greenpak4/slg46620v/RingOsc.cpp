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
#include <unistd.h>
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

	//Set up the test
	hdevice hdev = MultiBoardTestSetup(argv[1], 25000, 3.3, SilegoPart::SLG46620V);
	if(!hdev)
	{
		LogError("Failed to open board\n");
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
	ResetAllSiggens(hdev);
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

	//Set up the I/O config we need for this test
	IOConfig ioConfig;
	for(size_t i = 2; i <= 20; i++)
		ioConfig.driverConfigs[i] = TP_RESET;
	if(!SetIOConfig(hdev, ioConfig))
		return false;

	//Measure the oscillator frequency
	unsigned int freq;
	if(!MeasureOscillatorFrequency(hdev, freq))
		return false;

	unsigned int expected = 13500000;
	LogVerbose("Measured ring oscillator frequency (with divide-by-2): %u Hz (expected %u)\n",
		freq, expected);

	int err = freq - expected;
	LogVerbose("    Error: %d Hz\n", err);
	int tolerance = 5000;
	if(abs(err) > tolerance)
	{
		LogError("Error is too big (more than %d Hz)\n", tolerance);
		return false;
	}

	return true;
}
