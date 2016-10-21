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
#include "gpdevboard.h"
#include <unistd.h>

using namespace std;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Status check

bool CheckStatus(hdevice hdev)
{
	LogDebug("Requesting board status\n");

	BoardStatus status;
	if(!GetStatus(hdev, status))
		return false;
	LogVerbose("Board voltages: A = %.3f V, B = %.3f V\n", status.voltageA, status.voltageB);

	if(status.externalOverCurrent)
		LogError("Overcurrent condition detected on external supply\n");
	if(status.internalOverCurrent)
		LogError("Overcurrent condition detected on internal supply\n");
	if(status.internalUnderVoltage)
		LogError("Undervoltage condition detected on internal supply\n");

	return !(status.externalOverCurrent &&
			 status.internalOverCurrent &&
			 status.internalUnderVoltage);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Initialization

/**
	@brief Connect to the board, but don't change anything
 */
hdevice OpenBoard()
{
	//Set up libusb
	if(!USBSetup())
		return NULL;

	//Try opening the board in "orange" mode
	LogNotice("\nSearching for developer board\n");
	hdevice hdev = OpenDevice(0x0f0f, 0x0006);
	if(!hdev)
	{
		//Try opening the board in "white" mode
 		hdev = OpenDevice(0x0f0f, 0x8006);
		if(!hdev)
		{
			LogError("No device found, giving up\n");
			return NULL;
		}

		//Change the board into "orange" mode
		LogVerbose("Switching developer board from bootloader mode\n");
		if(!SwitchMode(hdev))
			return NULL;

		//Takes a while to switch and re-enumerate
		usleep(1200 * 1000);

		//Try opening the board in "orange" mode again
		hdev = OpenDevice(0x0f0f, 0x0006);
		if(!hdev)
		{
			LogError("Could not switch mode, giving up\n");
			return NULL;
		}
	}

	//Get string descriptors
	string name, vendor;
	if(!GetStringDescriptor(hdev, 1, name) || //board name
	   !GetStringDescriptor(hdev, 2, vendor)) //manufacturer
	{
		return NULL;
	}
	LogNotice("Found: %s %s\n", vendor.c_str(), name.c_str());
	//string 0x80 is 02 03 for this board... what does that mean? firmware rev or something?
	//it's read by emulator during startup but no "2" and "3" are printed anywhere...

	//Check that we're in good standing
	if(!CheckStatus(hdev))
	{
		LogError("Fault condition detected during initial check, exiting\n");
		return NULL;
	}

	//Done
	return hdev;
}
