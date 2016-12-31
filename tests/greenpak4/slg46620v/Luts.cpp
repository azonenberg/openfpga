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
	hdevice hdev = MultiBoardTestSetup(argv[1], 25000, 1.8, SilegoPart::SLG46620V);
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

	//Loop over all values for the 4 inputs and see what we get
	bool ok = true;
	for(int i=0; i<16; i++)
	{
		bool a = (i & 1) ? true : false;
		bool b = (i & 2) ? true : false;
		bool c = (i & 4) ? true : false;
		bool d = (i & 8) ? true : false;

		bool nand1_expected = !a;
		bool nand2_expected = !(a & b);
		bool nand3_expected = !(a & b & c);
		bool nand4_expected = !(a & b & c & d);

		bool expected[8] =
		{
			nand1_expected, nand2_expected, nand3_expected, nand4_expected,
			nand1_expected, nand2_expected, nand3_expected, nand4_expected
		};

		//Drive the inputs
		LogVerbose("Testing %d %d %d %d\n", a, b, c, d);
		LogVerbose("Expected: %d %d %d %d\n", nand1_expected, nand2_expected, nand3_expected, nand4_expected);
		ioConfig.driverConfigs[2] = a ? TP_VDD : TP_GND;
		ioConfig.driverConfigs[3] = b ? TP_VDD : TP_GND;
		ioConfig.driverConfigs[4] = c ? TP_VDD : TP_GND;
		ioConfig.driverConfigs[5] = d ? TP_VDD : TP_GND;
		if(!SetIOConfig(hdev, ioConfig))
			return false;

		//Read the outputs
		int pin_nums[8] = {6, 7, 8, 9, 12, 13, 14, 15};
		bool results[8];
		for(int i=0; i<8; i++)
		{
			double value;
			if(!SingleReadADC(hdev, pin_nums[i], value))
				return false;
			results[i] = (value > 0.5);
		}

		//Sanity check the results
		for(int j=0; j<8; j++)
		{
			if(expected[j] != results[j])
			{
				LogError("input (%d %d %d %d), output pin %d, got %d instead of %d\n",
					a, b, c, d,
					pin_nums[j],
					results[j],
					expected[j]);
				ok = false;
			}
		}
	}

	return ok;
}
