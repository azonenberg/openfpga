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
	//Set up libusb and open the board (for now assume we only have one)
	USBSetup();
	hdevice hdev = OpenDevice();
	
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
	
	//Set the I/O configuration on the test points
	printf("Setting initial dummy I/O configuration\n");
	IOConfig config;
	for(int i=3; i<=10; i++)
	{
		config.driverConfigs[i] = TP_PULLUP;
		config.ledEnabled[i] = true;
	}
	SetIOConfig(hdev, config);
	
	//Wait a while
	usleep(1000 * 1000);	
	
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
	
	//Done
	printf("Cleaning up\n");
	USBCleanup(hdev);
	return 0;
}
