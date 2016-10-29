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
	if(!LockDevice())
	{
		LogError("Failed to lock dev board\n");
		return false;
	}
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
	UnlockDevice();
	return 0;
}

/**
	@brief The actual application-layer test
 */
bool RunTest(hdevice hdev)
{
	LogIndenter li;

	//Set up the I/O config we need for this test (?)
	IOConfig ioConfig;
	for(size_t i = 2; i <= 20; i++)
		ioConfig.driverConfigs[i] = TP_RESET;
	if(!SetIOConfig(hdev, ioConfig))
		return false;

	const int INPUT_PIN = 8;
	const int OUTPUT_PIN = 7;

	//Sweep input and verify output
	LogVerbose("Running PGA voltage sweep\n");
	LogVerbose("|%10s|%10s|%10s|%10s|%10s|%10s|\n", "Requested", "Actual", "Output", "Expected", "Error", "% error");
	double step = 0.05;
	for(double vtest = step; vtest < 0.51; vtest += step)
	{
		//Set up a signal generator on the input
		if(!ConfigureSiggen(hdev, INPUT_PIN, vtest))
			return false;

		//Read the input
		if(!SelectADCChannel(hdev, INPUT_PIN))
			return false;
		double vin;
		if(!ReadADC(hdev, vin))
			return false;

		//Read the output
		if(!SelectADCChannel(hdev, OUTPUT_PIN))
			return false;
		double vout;
		if(!ReadADC(hdev, vout))
			return false;

		double expected = vin * 2;
		double error = expected - vout;
		double pctError = (error * 100) / expected;

		LogVerbose("|%10.3f|%10.3f|%10.3f|%10.3f|%10.3f|%10.3f|\n", vtest, vin, vout, expected, error, pctError);

		//TODO: Decide on an appropriate tolerance
		if(pctError > 5)
		{
			LogError("Percent error is too big, test failed\n");
			return false;
		}
	}

	return true;
}
