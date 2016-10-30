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

	double tolerance_trip = 0.05;
	double tolerance_vref = 0.15;

	//Check the bandgap outputs
	LogVerbose("Testing bandgap status\n");
	double v;
	if(!SingleReadADC(hdev, 20, v))
		return false;
	if(v < 0.5)
	{
		LogError("Bandgap status should be high\n");
		return false;
	}

	//Check the Vref outputs
	//TODO: Why is the error so wide here?
	LogVerbose("Checking ref outputs\n");
	{
		LogIndenter li;

		if(!SingleReadADC(hdev, 19, v))
			return false;
		double err = 0.8 - v;
		LogVerbose("Vref_800 = %.3f V (err = %.3f)\n", v, err);
		if(fabs(err) > tolerance_vref)
		{
			LogError("Error is too big\n");
			return false;
		}

		if(!SingleReadADC(hdev, 18, v))
			return false;
		err = 0.6 - v;
		LogVerbose("Vref_600 = %.3f V (err = %.3f)\n", v, err);
		if(fabs(err) > tolerance_vref)
		{
			LogError("Error is too big\n");
			return false;
		}
	}

	//(pin num, threshold in mV)
	const int acmps[6][2] =
	{
		{12, 200},
		{13, 400},
		{14, 600},
		{15, 800},
		{16, 1000},
		{17, 1000}
	};

	//Sweep input and verify output
	LogVerbose("Running comparator voltage sweep\n");
	double step = 0.025;
	bool pin_states[6] = {false};
	for(double vtest = step; vtest < 1.1; vtest += step)
	{
		LogIndenter li;

		//Set up a signal generator on the inputs
		//TODO: check 6 and 4 independently?
		if(!ConfigureSiggen(hdev, 6, vtest))
			return false;
		if(!ConfigureSiggen(hdev, 4, vtest))
			return false;

		//Read the outputs
		for(int i=0; i<6; i++)
		{
			double v;
			int npin = acmps[i][0];
			if(!SingleReadADC(hdev, npin, v))
				return false;

			bool b = (v > 0.5);

			//If we went LOW, something is wrong. We should never turn off on a rising edge
			if(pin_states[i] && !b)
			{
				LogError("Pin %d went low at %.3f V on a rising input waveform - something is wrong\n",
					npin, vtest);
				return false;
			}

			//If we went HIGH, log the trip point and check if it's sane
			if(!pin_states[i] && b)
			{
				double expected_trip = acmps[i][1] * 0.001f;
				double delta = vtest - expected_trip;

				LogVerbose("Pin %d went high at %.3f V (err = %.3f V)\n", npin, vtest, delta);

				if(fabs(delta) > tolerance_trip)
				{
					LogError("Error is too big\n");
					return false;
				}
			}

			//Save pin state
			pin_states[i] = b;
		}

		/*
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
		}*/
	}

	return true;
}
