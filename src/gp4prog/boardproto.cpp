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

void DataFrame::Send(hdevice hdev)
{
	unsigned char data[63] = {0};
	
	//Packet header
	data[0] = m_sequenceA;
	data[1] = m_type;
	data[2] = 3 + m_payload.size();
	data[3] = m_sequenceB;
	
	//Packet body
	for(size_t i=0; i<m_payload.size(); i++)
		data[4+i] = m_payload[i];
		
	SendInterruptTransfer(hdev, data, sizeof(data));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Device I/O

void SetStatusLED(hdevice hdev, bool status)
{
	DataFrame frame(DataFrame::SET_STATUS_LED);
	frame.push_back(status);
	frame.Send(hdev);
}

void SetIOConfig(hdevice hdev, IOConfig& config)
{
	DataFrame frame(DataFrame::CONFIG_IO);
	
	//Test point config data
	for(unsigned int i=2; i<=20; i++)
	{
		unsigned int cfg = config.driverConfigs[i];
		frame.push_back(cfg >> 8);
		frame.push_back(cfg & 0xff);
		
		//skip TP11 since that's ground, no config for it
		if(i == 10)
			i++;
	}
	
	//7 unknown bytes, leave zero for now
	for(size_t i=0; i<7; i++)
		frame.push_back(0);
	
	//Offsets 2f ... 31: expansion connector TODO
	uint8_t exp[3] = {0};
	uint8_t expansionBitMap[21][2] =
	{
		{0, 0x00},		//unused
		{1, 0x01},		//Vdd
		
		{2, 0x04},		//TP2
		{2, 0x01},		//TP3
		{2, 0x10},		//TP4
		{2, 0x40},		//TP5
		{0, 0x01},		//TP6
		{0, 0x04},		//TP7
		{0, 0x10},		//TP8
		{0, 0x40},		//TP9
		{0, 0x80},		//TP10
		
		{0, 0x20},		//TP12
		{2, 0x08},		//TP13
		{2, 0x02},		//TP14
		{1, 0x80},		//TP15
		{2, 0x20},		//TP16
		{0, 0x02},		//TP17
		{1, 0x20},		//TP18
		{1, 0x08},		//TP19
		{2, 0x08}		//TP20
	};
	for(unsigned int i=1; i<21; i++)
	{
		if(config.expansionEnabled[i])
			exp[expansionBitMap[i][0]] = expansionBitMap[i][1];
	}
	for(size_t i=0; i<7; i++)
		frame.push_back(exp[i]);
	
	//LEDs from TP3 ... TP15
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
		frame.push_back(ledcfg);
				
		//Bump pointers. skip TP11 as it's not implemented in the hardware (ground)
		tpbase += 4;
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
	frame.push_back(leden);
	frame.push_back(ledinv);
	
	//Offset 37: always constant 1, meaning unknown
	frame.push_back(0x1);
	
	//Done, send it
	frame.Send(hdev);
}


