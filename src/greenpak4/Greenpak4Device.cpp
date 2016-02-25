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
	//Delete everything
	for(auto x : m_bitstuff)
		delete x;
	m_bitstuff.clear();
}

void Greenpak4Device::CreateDevice_SLG46620()
{
	//64 inputs per routing matrix
	m_matrixBits = 6;
	
	//Create power rails (need one for each matrix).
	//These have to come first, since all other devices will refer to these during construction
	for(int i=0; i<2; i++)
	{
		m_constantZero[i] = new Greenpak4PowerRail(this, i, 0);
		m_constantOne[i] = new Greenpak4PowerRail(this, i, 63);
	}
	
	//Create the LUT2s (4 per device half)
	for(int i=0; i<4; i++)
	{
		m_lut2s.push_back(new Greenpak4LUT(
			this,
			0,			//First half of LUT2s are attached to crossbar #0
			i*2,		//LUT2 base is row 0, then 2 inputs per LUT
			i+1,		//First mux entry is ground, then the LUT2s
			576 + i*4,	//LUT2s start at bitstream offset 576, 2^2 bits per LUT
			2));		//this is a LUT2
	}
	for(int i=0; i<4; i++)
	{
		m_lut2s.push_back(new Greenpak4LUT(
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
		m_lut3s.push_back(new Greenpak4LUT(
			this,
			0,			//First half of LUT3s are attached to crossbar #0
			i*3 + 8,	//LUT3 base is row 8, then 3 inputs per LUT
			i+5,		//we come after the last LUT2
			592 + i*8,	//LUT2s start at bitstream offset 592, 2^3 bits per LUT
			3));		//this is a LUT3
	}
	for(int i=0; i<8; i++)
	{
		m_lut3s.push_back(new Greenpak4LUT(
			this,
			1,			//Second half are attached to crossbar #1
			i*3 + 8,	//LUT3 base is row 8, then 3 inputs per LUT
			i+5,		//we come after the last LUT2
			714 + i*8,	//LUT3s start at bitstream offset 714, 2^3 bits per LUT
			3));		//this is a LUT3
	}
	
	//TODO: Create the LUT4s (this is special because both have alternate functions)
	
	//Add LUT2-3-4s to the LUT list
	for(auto x : m_lut2s)
		m_luts.push_back(x);
	for(auto x : m_lut3s)
		m_luts.push_back(x);
	for(auto x : m_lut4s)
		m_luts.push_back(x);
	
	//Create the Type-A IOBs (with output enable)
	m_iobs[2] =  new Greenpak4IOBTypeA(this, 0, -1, 24, 941, Greenpak4IOB::IOB_FLAG_INPUTONLY);
	m_iobs[3] =  new Greenpak4IOBTypeA(this, 0, 56, 25, 946);
	m_iobs[5] =  new Greenpak4IOBTypeA(this, 0, 59, 27, 960);		
	m_iobs[7] =  new Greenpak4IOBTypeA(this, 0, 62, 29, 974);
	m_iobs[9] =  new Greenpak4IOBTypeA(this, 0, 65, 31, 988);
	m_iobs[10] = new Greenpak4IOBTypeA(this, 0, 67, 32, 995, Greenpak4IOB::IOB_FLAG_X4DRIVE);
	m_iobs[13] = new Greenpak4IOBTypeA(this, 1, 57, 25, 1919);
	m_iobs[14] = new Greenpak4IOBTypeA(this, 1, 59, 26, 1926);
	m_iobs[16] = new Greenpak4IOBTypeA(this, 1, 62, 28, 1940);
	m_iobs[18] = new Greenpak4IOBTypeA(this, 1, 65, 30, 1954);
	m_iobs[19] = new Greenpak4IOBTypeA(this, 1, 67, 31, 1961);
	
	//TODO: DFF/latches
	
	//TODO: Pipe delays
	
	//TODO: Edge detector/prog delays
	
	//TODO: Inverters
	
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
	for(auto x : m_iobs)
		m_bitstuff.push_back(x.second);
	for(unsigned int i=0; i<2; i++)
	{
		m_bitstuff.push_back(m_constantZero[i]);
		m_bitstuff.push_back(m_constantOne[i]);
	}
	
	//Total length of our bitstream
	m_bitlen = 2048;
	
	//Initialize matrix base addresses
	m_matrixBase[0] = 0;
	m_matrixBase[1] = 1024;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Accessors

Greenpak4BitstreamEntity* Greenpak4Device::GetPowerRail(unsigned int matrix, bool rail)
{
	if(matrix > 1)
		return NULL;
	
	if(rail)
		return m_constantOne[matrix];
	else
		return m_constantZero[matrix];
}

Greenpak4IOB* Greenpak4Device::GetIOB(unsigned int pin)
{
	if(m_iobs.find(pin) == m_iobs.end())
		return NULL;
	return m_iobs[pin];
}

Greenpak4LUT* Greenpak4Device::GetLUT2(unsigned int i)
{
	if(i >= m_lut2s.size())
		return NULL;
	return m_lut2s[i];
}

unsigned int Greenpak4Device::GetMatrixBase(unsigned int matrix)
{
	if(matrix > 1)
		return 0;
		
	return m_matrixBase[matrix];
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// File I/O

bool Greenpak4Device::WriteToFile(const char* fname)
{
	//Open the file
	FILE* fp = fopen(fname, "w");
	if(!fp)
	{
		fprintf(stderr, "Couldn't open %s for writing\n", fname);
		return false;
	}
	
	//Allocate the bitstream and initialize to zero
	//According to phone conversation w Silego FAE, 0 is legal default state for everything incl reserved bits
	//All IOs will be floating digital inputs
	bool* bitstream = new bool[m_bitlen];
	for(unsigned int i=0; i<m_bitlen; i++)
		bitstream[i] = false;
	
	//Get the config data from each of our blocks
	for(auto x : m_bitstuff)
	{
		if(!x->Save(bitstream))
			return false;
	}
		
	//Write the bitfile (TODO comment more meaningfully?)
	fprintf(fp, "index		value		comment\n");
	for(unsigned int i=0; i<m_bitlen; i++)
		fprintf(fp, "%d		%d		//\n", i, bitstream[i]);
	
	//Done
	delete[] bitstream;
	fclose(fp);
	return true;
}
