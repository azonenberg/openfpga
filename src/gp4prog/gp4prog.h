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

#ifndef gp4prog_h
#define gp4prog_h

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// System / library headers

#include <stdio.h>
#include <stdlib.h>

#include <string>

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include <libusb-1.0/libusb.h>

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// USB command wrappers

typedef libusb_device_handle* hdevice;

void USBSetup();
void USBCleanup(hdevice hdev);

hdevice OpenDevice();
std::string GetStringDescriptor(hdevice hdev, uint8_t index);
void SendInterruptTransfer(hdevice hdev, unsigned char* buf, size_t size);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Board protocol stuff

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
class TestPointConfig
{
public:

	//Unused indexes waste space but we don't care
	//test point config matches with array indexes this way - much easier to code to

	//Configuration of each test pin's driver
	TPConfig driverConfigs[21];	//only [20:12] and [10:2] meaningful

	//Configuration of each test pin's LED
	bool ledEnabled[21];		//only [20:12] and [10:3] meaningful
	bool ledInverted[21];		//only [20:12] and [10:3] meaningful
	
	//Configuration of expansion connector
	bool expansionEnabled[21];	//only [20:12], [10:2] meaningful for signals
								//[1] is Vdd
								
	TestPointConfig()
	{
		for(int i=0; i<21; i++)
		{
			driverConfigs[i] = TP_NC;
			ledEnabled[i] = false;
			ledInverted[i] = false;
			expansionEnabled[i] = false;
		}
	}
};

void GeneratePacketHeader(unsigned char* data, uint16_t type);
void SetStatusLED(hdevice hdev, bool status);
void SetTestPointConfig(hdevice hdev, TestPointConfig& config);

#endif
