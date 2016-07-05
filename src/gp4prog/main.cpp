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

libusb_device_handle* OpenDevice();
string GetStringDescriptor(libusb_device_handle* hdev, uint8_t index);

void GeneratePacketHeader(unsigned char* data, uint16_t type);

void SetStatusLED(libusb_device_handle* hdev, bool status);

//Test point config (actual bitstream coding)
enum TPConfig
{
	//Types of driver
	TP_FLOAT			= 0x0200,	//Driver not hooked up at all
	TP_1				= 0x0001,	//Constant 1
	TP_0				= 0x0000,	//Constant 0
	TP_SIGGEN			= 0x0003,	//Signal generator
	
	//Drive strength
	TP_STRONG			= 0x0c00,	//Strong push-pull driver
	TP_WEAK				= 0x0e00,	//Weak push-pull  driver
	TP_OD_PU			= 0x0400,	//Open drain NMOS driver with opposing pullup
	TP_OD_PD			= 0x0600,	//Open drain PMOS driver with opposing pulldown
	TP_OD_PMOS			= 0x0a00,	//Open drain PMOS driver
	TP_OD_NMOS			= 0x0800,	//Open drain NMOS driver

	//Final combinations observed in Silego code
	TP_NC				= TP_FLOAT,					//Pad not used
	TP_VDD				= TP_STRONG | TP_1,			//Strong 1
	TP_GND				= TP_STRONG | TP_0,			//Strong 0
	TP_PULLUP			= TP_WEAK | TP_1,			//Weak 1
	TP_PULLDOWN			= TP_WEAK | TP_0,			//Weak 0
	TP_LOGIC_PP			= TP_STRONG | TP_SIGGEN,	//Strong signal generator
	TP_LOGIC_OD_PU		= TP_OD_PU | TP_SIGGEN,		//Open drain NMOS signal generator with opposing pullup
	TP_LOGIC_OD_PD		= TP_OD_PD | TP_SIGGEN,		//Open drain PMOS signal generator with opposing pulldown
	TP_LOGIC_OD_PMOS	= TP_OD_PMOS | TP_SIGGEN,	//Open drain PMOS signal generator
	TP_LOGIC_OD_NMOS	= TP_OD_NMOS | TP_SIGGEN,	//Open drain PMOS signal generator
	TP_LOGIC_WEAK_PP	= TP_WEAK | TP_SIGGEN		//Weak signal generator
};

//Helper struct for test point configuration 
//Not actual bitstream ordering, but contains all the data
struct TestPointConfig
{
public:
	TPConfig testpoint_configs[21];	//only [20:12] and [10:2] meaningful
									//[1:0] and 11 are just there so indexes match up with test point names

	//TODO expansion config
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Entry point

#define INT_ENDPOINT 2

int main(int /*argc*/, char* /*argv*/[])
{
	//Set up libusb
	if(0 != libusb_init(NULL))
	{
		printf("libusb_init failed\n");
		exit(-1);
	}
	
	//See what's out there
	libusb_device_handle* hdev = OpenDevice();
	
	//Get string descriptors
	string name = GetStringDescriptor(hdev, 1);			//board name
	string vendor = GetStringDescriptor(hdev, 2);		//manufacturer
	printf("Found: %s %s\n", vendor.c_str(), name.c_str());
	//string 0x80 is 02 03 for this board... what does that mean? firmware rev or something?
	
	//Blink the LED
	for(int i=0; i<5; i++)
	{
		SetStatusLED(hdev, 1);
		usleep(250 * 1000);
		
		SetStatusLED(hdev, 0);
		usleep(250 * 1000);
	}
	
	//Done
	libusb_close(hdev);
	libusb_exit(NULL);
	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Debug I/O

void SetStatusLED(libusb_device_handle* hdev, bool status)
{
	//Generate the status packet
	unsigned char cmd[63];
	GeneratePacketHeader(cmd, 0x2104);
	cmd[4] = status;
	
	//and send it
	int transferred;
	int err = 0;
	if(0 != (err = libusb_interrupt_transfer(hdev, INT_ENDPOINT, cmd, sizeof(cmd), &transferred, 250)))
	{
		printf("libusb_interrupt_transfer failed (err=%d)\n", err);
		exit(-1);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// USB interrupt packet header generation

void GeneratePacketHeader(unsigned char* data, uint16_t type)
{
	data[0] = 0x01;
	data[1] = type >> 8;
	data[2] = type & 0xff;
	data[3] = 0;
	for(int i=4; i<62; i++)
		data[i] = 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Enumeration / setup helpers

//Gets the device handle (assume only one for now)
libusb_device_handle* OpenDevice()
{
	libusb_device** list;
	ssize_t devcount = libusb_get_device_list(NULL, &list);
	if(devcount < 0)
	{
		printf("libusb_get_device_list failed\n");
		exit(-1);
	}
	libusb_device* device = NULL;
	bool found = false;
	for(ssize_t i=0; i<devcount; i++)
	{
		device = list[i];
		
		libusb_device_descriptor desc;
		if(0 != libusb_get_device_descriptor(device, &desc))
			continue;
		
		//Silego devkit
		if( (desc.idVendor == 0x0f0f) && (desc.idProduct == 0x0006) )
		{
			found = true;
			break;
		}
	}
	libusb_device_handle* hdev;
	if(found)
	{
		if(0 != libusb_open(device, &hdev))
		{
			printf("libusb_open failed\n");
			exit(-1);
		}
	}	
	libusb_free_device_list(list, 1);
	if(!found)
	{
		printf("No device found, giving up\n");
		exit(-1);
	}
	
	//Detach the kernel driver, if any
	int err = libusb_detach_kernel_driver(hdev, 0);
	if( (0 != err) && (LIBUSB_ERROR_NOT_FOUND != err) )
	{
		printf("Can't detach kernel driver\n");
		exit(-1);
	}
	
	//Set the device configuration
	if(0 != (err = libusb_set_configuration(hdev, 1)))
	{
		printf("Failed to select device configuration (err = %d)\n", err);
		exit(-1);
	}
	
	//Claim interface 0
	if(0 != libusb_claim_interface(hdev, 0))
	{
		printf("Failed to claim interface\n");
		exit(-1);
	}
	
	return hdev;
}

//Gets a string descriptor as a STL string
string GetStringDescriptor(libusb_device_handle* hdev, uint8_t index)
{
	char strbuf[128];
	if(libusb_get_string_descriptor_ascii(hdev, index, (unsigned char*)strbuf, sizeof(strbuf)) < 0)
	{
		printf("libusb_get_string_descriptor_ascii failed\n");
		exit(-1);
	}
	
	return string(strbuf);
}
