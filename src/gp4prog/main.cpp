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

#include "gp4prog.h"

using namespace std;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Entry point

int main(int /*argc*/, char* /*argv*/[])
{
	//Set up libusb
	USBSetup();
	
	// Try opening the board in "orange" mode
	hdevice hdev = OpenDevice(0x0f0f, 0x0006);
	if(!hdev) {
		// Try opening the board in "white" mode
 		hdev = OpenDevice(0x0f0f, 0x8006);
		if(!hdev) {
			printf("No device found, giving up\n");
			exit(-1);
		}

		// Change the board into "orange" mode
		printf("Switching board mode\n");
		SwitchMode(hdev);

		// Takes a while to switch and re-enumerate
		usleep(1000 * 1000);

		// Try opening the board in "orange" mode again
		hdev = OpenDevice(0x0f0f, 0x0006);
		if(!hdev) {
			printf("Could not switch mode, giving up\n");
			exit(-1);
		}
	}
	
	//Get string descriptors
	string name = GetStringDescriptor(hdev, 1);			//board name
	string vendor = GetStringDescriptor(hdev, 2);		//manufacturer
	printf("Found: %s %s\n", vendor.c_str(), name.c_str());
	//string 0x80 is 02 03 for this board... what does that mean? firmware rev or something?
	//it's read by emulator during startup but no "2" and "3" are printed anywhere...
	
	//Blink the LED a few times
	printf("Blinking LED for sanity check\n");
	for(int i=0; i<3; i++)
	{
		SetStatusLED(hdev, 1);
		usleep(250 * 1000);
		
		SetStatusLED(hdev, 0);
		usleep(250 * 1000);
	}
	
	//Configure the signal generator for Vdd
	printf("Configuring Vdd signal generator\n");
	ConfigureSiggen(hdev, 1);
	bool enable[19] = { true, false };
	SetSiggenStatus(hdev, 1, SIGGEN_START);
	
	//Set the I/O configuration on the test points
	printf("Setting initial dummy I/O configuration\n");
	IOConfig config;
	for(int i=3; i<=5; i++)
	{
		config.driverConfigs[i] = TP_PULLUP;
		config.ledEnabled[i] = true;
	}
	SetIOConfig(hdev, config);
	
	//Wait a while
	usleep(2 * 1000 * 1000);	
	
	//Wipe I/O config to floating
	printf("Restoring I/O configuration\n");
	for(int i=0; i<=21; i++)
	{
		config.driverConfigs[i] = TP_FLOAT;
		config.ledEnabled[i] = false;
		config.ledInverted[i] = false;
		config.expansionEnabled[i] = false;
	}
	SetIOConfig(hdev, config);
	
	//Kill power
	SetSiggenStatus(hdev, 1, SIGGEN_STOP);
	
	//Done
	printf("Cleaning up\n");
	USBCleanup(hdev);
	return 0;
}