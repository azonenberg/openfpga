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

#include <string.h>
#include "gp4prog.h"

using namespace std;

vector<uint8_t> ReadBitstream(string fname);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Entry point

vector<uint8_t> ReadBitstream(string fname)
{	
	//Open the file
	FILE* fp = fopen(fname.c_str(), "r");
	if(!fp)
	{
		printf("Couldn't open %s for reading\n", fname.c_str());
		exit(-1);
	}
	
	char signature[64];
	fgets(signature, sizeof(signature), fp);
	if(strcmp(signature, "index\t\tvalue\t\tcomment\n")) 
	{
		printf("%s is not a GreenPAK bitstream", fname.c_str());
		exit(-1);
	}

	vector<uint8_t> bitstream(2048/8);
	while(!feof(fp))
	{
		int index;
		int value;
		fscanf(fp, "%d\t\t%d\t\t//\n", &index, &value);

		int byteindex = index / 8;
		if((byteindex < 0 || byteindex > (int)bitstream.size()) || (value != 0 && value != 1))
		{
			printf("%s contains a malformed GreenPAK bitstream\n", fname.c_str());
			exit(-1);
		}

		bitstream[byteindex] |= (value << (index % 8));
	}

	fclose(fp);

	return bitstream;
}

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
	
	//Light up the status LED
	SetStatusLED(hdev, 1);
	
	//Configure the signal generator for Vdd
	printf("Configuring Vdd signal generator\n");
	ConfigureSiggen(hdev, 1);
	SetSiggenStatus(hdev, 1, SIGGEN_START);

	//Select part
	printf("Selecting part\n");
	SetPart(hdev, SLG46620V);

	//Load bitstream
	printf("Loading bitstream into SRAM\n");
	LoadBitstream(hdev, ReadBitstream("test.txt"));
	
	//Set the I/O configuration on the test points
	//Expects a bitstream that does assign TP4=TP3;
	printf("Setting testing I/O configuration\n");
	IOConfig config;
	config.driverConfigs[3] = TP_GND;
	config.ledEnabled[3] = true;
	config.driverConfigs[4] = TP_PULLUP;
	config.ledEnabled[4] = true;
	SetIOConfig(hdev, config);
	
	//Blink TP3
	usleep(300 * 1000);
	config.driverConfigs[3] = TP_VDD;
	SetIOConfig(hdev, config);
	
	usleep(300 * 1000);
	config.driverConfigs[3] = TP_GND;
	SetIOConfig(hdev, config);
	
	usleep(300 * 1000);
	config.driverConfigs[3] = TP_VDD;
	SetIOConfig(hdev, config);
	
	usleep(300 * 1000);
	config.driverConfigs[3] = TP_GND;
	SetIOConfig(hdev, config);
	
	//Wipe I/O config to floating
	printf("Restoring I/O configuration\n");
	for(int i=0; i<=20; i++)
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
	SetStatusLED(hdev, 0);
	USBCleanup(hdev);
	return 0;
}
