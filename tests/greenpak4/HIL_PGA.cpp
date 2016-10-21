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

	//Do standard board bringup
	hdevice hdev = OpenBoard();
	if(!hdev)
	{
		LogError("Failed to open board\n");
		return 1;
	}
	if(!SetStatusLED(hdev, 1))
		return 1;

	//Prepare to run the test
	if(!TestSetup(hdev, argv[1], 25000, 3.3, SilegoPart::SLG46620V))
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
	LogDebug("Test complete, resetting board\n");
	SetStatusLED(hdev, 0);
	Reset(hdev);
	return 0;
}

/**
	@brief The actual application-layer test
 */
bool RunTest(hdevice hdev)
{
	LogIndenter li;

	//

	return true;
}
