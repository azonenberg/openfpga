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
// Device I/O

void SetStatusLED(hdevice hdev, bool status)
{
	//Generate the status packet
	unsigned char cmd[63];
	GeneratePacketHeader(cmd, 0x2104);
	cmd[4] = status;
	
	//and send it
	SendInterruptTransfer(hdev, cmd, sizeof(cmd));
}

void SetTestPointConfig(hdevice hdev, TestPointConfig& config)
{
	//Generate the command packet header
	unsigned char cmd[63];
	GeneratePacketHeader(cmd, 0x0439);
	
	//Offsets 04 ... 27: test point config data
	unsigned int offset = 4;
	for(unsigned int i=2; i<=20; i++)
	{
		unsigned int cfg = config.driverConfigs[i];
		cmd[offset] = cfg >> 8;
		cmd[offset + 1] = cfg & 0xff;
		
		offset += 2;
		
		//skip TP11 since that's ground, no config for it
		if(i == 10)
			i++;
	}
	
	//Offsets 28 ... 2e: unused? leave at 0 for now
	
	//Offsets 2f ... 31: expansion connector
	uint8_t expansionBitMap[21][2] =
	{
		{0x00, 0x00},		//unused
		{0x30, 0x01},		//Vdd
		
		{0x31, 0x04},		//TP2
		{0x31, 0x01},		//TP3
		{0x31, 0x10},		//TP4
		{0x31, 0x40},		//TP5
		{0x2f, 0x01},		//TP6
		{0x2f, 0x04},		//TP7
		{0x2f, 0x10},		//TP8
		{0x2f, 0x40},		//TP9
		{0x2f, 0x80},		//TP10
		
		{0x2f, 0x20},		//TP12
		{0x31, 0x08},		//TP13
		{0x31, 0x02},		//TP14
		{0x30, 0x80},		//TP15
		{0x31, 0x20},		//TP16
		{0x2f, 0x02},		//TP17
		{0x30, 0x20},		//TP18
		{0x30, 0x08},		//TP19
		{0x2f, 0x08}		//TP20
	};
	for(unsigned int i=1; i<21; i++)
	{
		if(config.expansionEnabled[i])
			cmd[expansionBitMap[i][0]] = expansionBitMap[i][1];
	}
	
	//Offsets 32 ... 34: LEDs from TP3 ... TP15
	offset = 0x32;
	unsigned int tpbase = 3;
	for(int i=0; i<3; i++)
	{
		uint8_t ledcfg = 0;
		for(int j=0; j<4; j++)
		{
			uint8_t bitmask = 1 << j;
			uint8_t tpnum = tpbase + j;
			
			if(config.ledEnabled[tpnum])
				ledcfg |= bitmask;
			if(config.ledInverted[tpnum])
				ledcfg |= (bitmask << 4);
		}
		cmd[offset] = ledcfg;
		
		//Bump pointers
		offset ++;
		tpbase += 4;
		
		//skip TP11 as it's not implemented in the hardware (ground)
		if(i == 1)
			tpbase ++;
	}
	
	//Offsets 35 ... 36: LEDs from TP16 ... TP20
	uint8_t leden = 0;
	uint8_t ledinv = 0;
	for(int i=0; i<5; i++)
	{
		uint8_t tpnum = 16 + i;
		uint8_t bitmask = 1 << i;
		
		if(config.ledEnabled[tpnum])
			leden |= bitmask;
		if(config.ledInverted[tpnum])
			ledinv |= bitmask;
	}
	cmd[0x35] = leden;
	cmd[0x36] = ledinv;
	
	//Offset 37: always constant 1. Power LED maybe?
	cmd[0x37] = 1;
	
	//Send it
	SendInterruptTransfer(hdev, cmd, sizeof(cmd));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// USB interrupt helpers

void GeneratePacketHeader(unsigned char* data, uint16_t type, uint16_t packets_left)
{
	data[0] = 0x01;
	data[1] = type >> 8;
	data[2] = type & 0xff;
	data[3] = packets_left;	//decreasing sequence number
	for(int i=4; i<62; i++)
		data[i] = 0;
}

