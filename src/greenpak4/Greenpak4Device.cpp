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
 
#include "Greenpak4.h"
#include <cassert>

using namespace std;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Construction / destruction

Greenpak4Device::Greenpak4Device(GREENPAK4_PART part)
	: m_part(part)
{
	switch(part)
	{
	case GREENPAK4_SLG46620:
		CreateDevice_SLG46620();
		break;
		
	default:
		assert(false);
		break;
	}
}

Greenpak4Device::~Greenpak4Device()
{
	for(auto x : m_bitstuff)
		delete x;
	m_bitstuff.clear();
}

void Greenpak4Device::CreateDevice_SLG46620()
{
	//64 inputs per routing matrix
	m_matrixBits = 6;
	
	//Create the LUT2s (4 per device half)
	for(int i=0; i<4; i++)
	{
		m_luts.push_back(new Greenpak4LUT(
			this,
			0,			//First half of LUT2s are attached to crossbar #0
			i*2,		//LUT2 base is row 0, then 2 inputs per LUT
			i+1,		//First mux entry is ground, then the LUT2s
			576 + i*4,	//LUT2s start at bitstream offset 576, 2^2 bits per LUT
			2));		//this is a LUT2
	}
	for(int i=0; i<4; i++)
	{
		m_luts.push_back(new Greenpak4LUT(
			this,
			1,			//Second half are attached to crossbar #1
			i*2,		//LUT2 base is row 0, then 2 inputs per LUT
			i+1,		//First mux entry is ground, then the LUT2s
			698 + i*4,	//LUT2s start at bitstream offset 698, 2^2 bits per LUT
			2));		//this is a LUT2
	}
	
	//Create the LUT3s (8 per device half)
	for(int i=0; i<8; i++)
	{
		m_luts.push_back(new Greenpak4LUT(
			this,
			0,			//First half of LUT3s are attached to crossbar #0
			i*3 + 8,	//LUT3 base is row 8, then 3 inputs per LUT
			i+5,		//we come after the last LUT2
			592 + i*8,	//LUT2s start at bitstream offset 592, 2^3 bits per LUT
			3));		//this is a LUT3
	}
	for(int i=0; i<8; i++)
	{
		m_luts.push_back(new Greenpak4LUT(
			this,
			1,			//Second half are attached to crossbar #1
			i*3 + 8,	//LUT3 base is row 8, then 3 inputs per LUT
			i+5,		//we come after the last LUT2
			714 + i*8,	//LUT3s start at bitstream offset 714, 2^3 bits per LUT
			3));		//this is a LUT3
	}
	
	//TODO: Create the LUT4s (this is special because both have alternate functions)
	
	//TODO: DFF/latches
	
	//TODO: Pipe delays
	
	//TODO: Edge detector/prog delays
	
	//TODO: Inverters
	
	//TODO: Output pins
	
	//TODO: Comparators
	
	//TODO: External clock??
	
	//TODO: Oscillators
	
	//TODO: Counters
	
	//TODO: Slave SPI
	
	//TODO: Cross-connections between matrixes
	
	//TODO: ADC
	
	//TODO: DAC
	
	//TODO: Bandgap reference
	
	//TODO: Reserved bits
	
	//TODO: Vdd bypass
	
	//TODO: Configuration/boot stuff
	
	//TODO: IO pad precharge? what does this involve?
	
	//Finally, put everything in bitstuff so we can walk the whole bitstream and not care about details
	for(auto x : m_luts)
		m_bitstuff.push_back(x);
}
