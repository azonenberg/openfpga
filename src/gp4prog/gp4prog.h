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
#include <vector>

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include <libusb-1.0/libusb.h>

#include "../log/log.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// USB command wrappers

typedef libusb_device_handle* hdevice;

void USBSetup();
void USBCleanup(hdevice hdev);

hdevice OpenDevice(uint16_t idVendor, uint16_t idProduct);
std::string GetStringDescriptor(hdevice hdev, uint8_t index);
void SendInterruptTransfer(hdevice hdev, const uint8_t* buf, size_t size);
void ReceiveInterruptTransfer(hdevice hdev, uint8_t* buf, size_t size);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Board protocol stuff

//Part numbers (actual bitstream coding)
enum SilegoPart
{
	SLG46140V = 0x14,
	SLG46620V = 0x62,
};

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
	TP_REALLY_WEAK      = 0x0000,   //Very weak push-pull driver
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
	TP_FLIMSY_PULLUP	= TP_REALLY_WEAK | TP_1,	//Very weak 1
	TP_FLIMSY_PULLDOWN	= TP_REALLY_WEAK | TP_0,	//Very weak 0
	TP_LOGIC_PP			= TP_STRONG | TP_SIGGEN,	//Strong signal generator
	TP_LOGIC_OD_PU		= TP_OD_PU | TP_SIGGEN,		//Open drain NMOS signal generator with opposing pullup
	TP_LOGIC_OD_PD		= TP_OD_PD | TP_SIGGEN,		//Open drain PMOS signal generator with opposing pulldown
	TP_LOGIC_OD_PMOS	= TP_OD_PMOS | TP_SIGGEN,	//Open drain PMOS signal generator
	TP_LOGIC_OD_NMOS	= TP_OD_NMOS | TP_SIGGEN,	//Open drain PMOS signal generator
	TP_LOGIC_WEAK_PP	= TP_WEAK | TP_SIGGEN,		//Weak signal generator

	TP_RESET            = TP_FLIMSY_PULLUP,			//Used to unstuck pins after SRAM upload
};

//Helper for test point configuration
//Not actual bitstream ordering, but contains all the data
class IOConfig
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

	IOConfig()
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

class BoardStatus
{
public:
	bool internalOverCurrent = false;
	bool externalOverCurrent = false;
	bool internalUnderVoltage = false;

	// double current = 0.0;
	double voltageA = 0.0;
	double voltageB = 0.0;
};

//Logical view of a data packet on the wire
//Not actual bitstream ordering, but contains all the data
class DataFrame
{
public:
	DataFrame()
		: m_type(0)
		, m_sequenceA(0)
		, m_sequenceB(0)
	{
	}

	DataFrame(uint8_t type)
		: m_type(type)
		, m_sequenceA(1)
		, m_sequenceB(0)
	{
	}

	DataFrame(const char *ascii);

	enum PacketTypes
	{
		READ_BITSTREAM_START		= 0x02,
		WRITE_BITSTREAM_SRAM		= 0x03,
		CONFIG_IO					= 0x04,
		RESET						= 0x05,
		//6 so far unobserved
		READ_BITSTREAM_CONT			= 0x07,
		WRITE_BITSTREAM_SRAM_ACK1   = 0x07,
		CONFIG_SIGGEN				= 0x08,
		ENABLE_SIGGEN				= 0x09,
		GET_STATUS                  = 0x0a,
		READ_BITSTREAM_ACK          = 0x13,
		WRITE_BITSTREAM_SRAM_ACK2   = 0x1a,
		SET_STATUS_LED				= 0x21,
		SET_PART                	= 0x25,
		CONFIG_ADC_MUX              = 0x33,
		GET_OSC_FREQ				= 0x42,
		READ_ADC                    = 0x47,
		TRIM_OSC					= 0x49
	};

	void Send(hdevice hdev);
	void Receive(hdevice hdev);
	void Roundtrip(hdevice hdev);
	void Roundtrip(hdevice hdev, uint8_t ack_type);

	bool IsEmpty()
	{ return m_payload.size() == 0; }

	bool IsFull()
	{ return m_payload.size() == 60; }

	void push_back(uint8_t b)
	{ m_payload.push_back(b); }

	DataFrame Next()
	{
		DataFrame next_frame(m_type);
		next_frame.m_sequenceA = m_sequenceA + 1;
		next_frame.m_sequenceB = m_sequenceB - 1;
		return next_frame;
	}

public:
	uint8_t m_type;
	uint8_t m_sequenceA;
	uint8_t m_sequenceB;
	std::vector<uint8_t> m_payload;
};

void SwitchMode(hdevice hdev);

void SetPart(hdevice hdev, SilegoPart part);

void SetStatusLED(hdevice hdev, bool status);
void SetIOConfig(hdevice hdev, IOConfig& config);

enum SiggenStatus
{
	SIGGEN_PAUSE	= 0x00,
	SIGGEN_START	= 0x01,
	SIGGEN_STOP		= 0x02,
	SIGGEN_NOP		= 0x03,
	SIGGEN_RESET	= 0x07
};

void ConfigureSiggen(hdevice hdev, uint8_t channel, double voltage);
void ResetAllSiggens(hdevice hdev);
void SetSiggenStatus(hdevice hdev, unsigned int chan, unsigned int status);

std::vector<uint8_t> UploadBitstream(hdevice hdev, size_t octets);
void DownloadBitstream(hdevice hdev, std::vector<uint8_t> bitstream);

void SelectADCChannel(hdevice hdev, unsigned int chan);
double ReadADC(hdevice hdev);

BoardStatus GetStatus(hdevice hdev);

#endif
