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

#define EP_OUT 0x02
#define EP_IN  0x81

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// USB command helpers

void SendInterruptTransfer(hdevice hdev, const uint8_t* buf, size_t size)
{
	int transferred;
	int err = 0;
	if(0 != (err = libusb_interrupt_transfer(hdev, EP_OUT, const_cast<uint8_t*>(buf), size, &transferred, 250)))
	{
		printf("libusb_interrupt_transfer failed (%s)\n", libusb_error_name(err));
		exit(-1);
	}
}

void ReceiveInterruptTransfer(hdevice hdev, uint8_t* buf, size_t size)
{
	int transferred;
	int err = 0;
	if(0 != (err = libusb_interrupt_transfer(hdev, EP_IN, buf, size, &transferred, 250)))
	{
		printf("libusb_interrupt_transfer failed (%s)\n", libusb_error_name(err));
		exit(-1);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Enumeration / setup helpers

//Set up USB stuff
void USBSetup()
{
	if(0 != libusb_init(NULL))
	{
		printf("libusb_init failed\n");
		exit(-1);
	}
}

void USBCleanup(hdevice hdev)
{
	libusb_close(hdev);
	libusb_exit(NULL);
}

//Gets the device handle (assume only one for now)
hdevice OpenDevice(uint16_t idVendor, uint16_t idProduct)
{
	libusb_device** list;
	ssize_t devcount = libusb_get_device_list(NULL, &list);
	if(devcount < 0)
	{
		LogError("libusb_get_device_list failed\n");
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
		if( (desc.idVendor == idVendor) && (desc.idProduct == idProduct) )
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
			LogError("libusb_open failed\n");
			exit(-1);
		}
	}
	libusb_free_device_list(list, 1);
	if(!found)
	{
		return NULL;
	}

	//Detach the kernel driver, if any
	int err = libusb_detach_kernel_driver(hdev, 0);
	if( (0 != err) && (LIBUSB_ERROR_NOT_FOUND != err) )
	{
		LogError("Can't detach kernel driver\n");
		exit(-1);
	}

	//Set the device configuration
	if(0 != (err = libusb_set_configuration(hdev, 1)))
	{
		LogError("Failed to select device configuration (err = %d)\n", err);
		exit(-1);
	}

	//Claim interface 0
	if(0 != libusb_claim_interface(hdev, 0))
	{
		LogError("Failed to claim interface\n");
		exit(-1);
	}

	return hdev;
}

//Gets a string descriptor as a STL string
string GetStringDescriptor(hdevice hdev, uint8_t index)
{
	char strbuf[128];
	if(libusb_get_string_descriptor_ascii(hdev, index, (unsigned char*)strbuf, sizeof(strbuf)) < 0)
	{
		LogError("libusb_get_string_descriptor_ascii failed\n");
		exit(-1);
	}

	return string(strbuf);
}
