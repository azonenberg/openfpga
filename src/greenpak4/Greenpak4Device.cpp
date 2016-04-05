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

Greenpak4Device::Greenpak4Device(
	GREENPAK4_PART part,
	Greenpak4IOB::PullDirection default_pull,
	Greenpak4IOB::PullStrength default_drive)
	: m_part(part)
{
	//Initialize everything
	switch(part)
	{
	case GREENPAK4_SLG46620:
		CreateDevice_SLG46620();
		break;
		
	default:
		assert(false);
		break;
	}
	
	//Set up pullups/downs on every IOB by default
	for(auto x : m_iobs)
	{
		x.second->SetPullDirection(default_pull);
		x.second->SetPullStrength(default_drive);
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
	
	//Create power rails
	//These have to come first, since all other devices will refer to these during construction
	m_constantZero = new Greenpak4PowerRail(this, 0, 0);
	m_constantOne = new Greenpak4PowerRail(this, 0, 63);
	
	//Create the LUT2s (4 per device half)
	for(int i=0; i<4; i++)
	{
		m_lut2s.push_back(new Greenpak4LUT(
			this,
			i,
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
			i+4,
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
			i,
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
			i+8,
			1,			//Second half are attached to crossbar #1
			i*3 + 8,	//LUT3 base is row 8, then 3 inputs per LUT
			i+5,		//we come after the last LUT2
			714 + i*8,	//LUT3s start at bitstream offset 714, 2^3 bits per LUT
			3));		//this is a LUT3
	}
	
	//Create the first LUT4 (pattern generator capable)
	//For now, no PGEN support, only usable as a LUT
	m_lut4s.push_back(new Greenpak4LUTPgen(
		this,
		0,
		0,		//Attached to crossbar #0
		32,		//LUT4 base is row 32
		13,		//we come after the last LUT3
		656,	//LUT4 starts after last LUT3
		4));	//this is a LUT4
	
	//Create the second LUT4 (no special functionality)
	m_lut4s.push_back(new Greenpak4LUT(
		this,
		1,
		1,		//Attached to crossbar #1
		32,		//LUT4 base is row 32
		13,		//we come after the last LUT3
		778,	//LUT4s start at bitstream offset 778, 2^4 bits per LUT
		4));	//this is a LUT4
	
	//Create the Type-A IOBs (with output enable)
	m_iobs[2] =  new Greenpak4IOBTypeA(this, 2,  0, -1, 24, 941, Greenpak4IOB::IOB_FLAG_INPUTONLY);
	m_iobs[3] =  new Greenpak4IOBTypeA(this, 3,  0, 56, 25, 946);
	m_iobs[5] =  new Greenpak4IOBTypeA(this, 5,  0, 59, 27, 960);		
	m_iobs[7] =  new Greenpak4IOBTypeA(this, 7,  0, 62, 29, 974);
	m_iobs[9] =  new Greenpak4IOBTypeA(this, 9,  0, 65, 31, 988);
	m_iobs[10] = new Greenpak4IOBTypeA(this, 10, 0, 67, 32, 995, Greenpak4IOB::IOB_FLAG_X4DRIVE);
	m_iobs[13] = new Greenpak4IOBTypeA(this, 13, 1, 57, 25, 1919);
	m_iobs[14] = new Greenpak4IOBTypeA(this, 14, 1, 59, 26, 1926);
	m_iobs[16] = new Greenpak4IOBTypeA(this, 16, 1, 62, 28, 1940);
	m_iobs[18] = new Greenpak4IOBTypeA(this, 18, 1, 65, 30, 1954);
	m_iobs[19] = new Greenpak4IOBTypeA(this, 19, 1, 67, 31, 1961);
	
	//Create the Type-B IOBs (no output enable)
	m_iobs[4]  = new Greenpak4IOBTypeB(this,  4, 0, 58, 26, 953);
	m_iobs[6]  = new Greenpak4IOBTypeB(this,  6, 0, 61, 28, 967);
	m_iobs[8]  = new Greenpak4IOBTypeB(this,  8, 0, 64, 30, 981);
	m_iobs[12] = new Greenpak4IOBTypeB(this, 12, 1, 56, 24, 1911, Greenpak4IOB::IOB_FLAG_X4DRIVE);	
	m_iobs[15] = new Greenpak4IOBTypeB(this, 15, 1, 61, 27, 1933);
	m_iobs[17] = new Greenpak4IOBTypeB(this, 17, 1, 64, 29, 1947);
	m_iobs[20] = new Greenpak4IOBTypeB(this, 20, 1, 69, 32, 1968);
	
	//DFF/latches
	//NOTE: Datasheet bug
	//Figure 42 of SLG46620_DS_r075 (page 97) says DFF5 config range is bits 708-710
	//but this collides with LUT2_7 and does not reflect actual silicon behavior.
	//Actual range is bits 695-697 (listed on page 151)
	m_dffsr.push_back(new Greenpak4Flipflop(this, 0,  true,  0, 36, 14, 677));
	m_dffsr.push_back(new Greenpak4Flipflop(this, 1,  true,  0, 39, 15, 681));
	m_dffsr.push_back(new Greenpak4Flipflop(this, 2,  true,  0, 42, 16, 685));
	m_dffs.push_back( new Greenpak4Flipflop(this, 3,  false, 0, 45, 17, 689));
	m_dffs.push_back( new Greenpak4Flipflop(this, 4,  false, 0, 47, 18, 692));
	m_dffs.push_back( new Greenpak4Flipflop(this, 5,  false, 0, 49, 19, 695));	
	m_dffsr.push_back(new Greenpak4Flipflop(this, 6,  true,  1, 36, 14, 794));
	m_dffsr.push_back(new Greenpak4Flipflop(this, 7,  true,  1, 39, 15, 798));
	m_dffsr.push_back(new Greenpak4Flipflop(this, 8,  true,  1, 42, 16, 802));
	m_dffs.push_back( new Greenpak4Flipflop(this, 9,  false, 1, 45, 17, 806));
	m_dffs.push_back( new Greenpak4Flipflop(this, 10, false, 1, 47, 18, 809));
	m_dffs.push_back( new Greenpak4Flipflop(this, 11, false, 1, 49, 19, 812));
	
	//TODO: Pipe delays
	
	//TODO: Edge detector/prog delays
	
	//Inverters
	m_inverters.push_back(new Greenpak4Inverter(this, 0, 55, 23));
	m_inverters.push_back(new Greenpak4Inverter(this, 1, 55, 23));
	
	//TODO: Comparators
	
	//TODO: External clock??
	
	//Low-frequency oscillator
	m_lfosc = new Greenpak4LFOscillator(
		this,
		0, 		//Matrix applies to inputs, we can route output globally
		84,		//input base (single power-down input)
		50,		//output word (plus dedicated routing to counters etc)
		1652);	//bitstream location
	
	//TODO: Other oscillators
	
	//Counters
	m_counters14bit.push_back(new Greenpak4Counter(
		this,
		14,		//depth
		false,	//no FSM mode
		true,	//wake-sleep powerdown bit
		false,	//no edge detector
		false,	//no PWM
		0,		//counter number
		0,		//matrix
		74,		//ibase
		36,		//oword
		1731));	//cbase
	m_counters14bit.push_back(new Greenpak4Counter(
		this,
		14,		//depth
		false,	//no FSM mode
		false,	//no wake-sleep powerdown
		false,	//no edge detector
		false,	//no PWM
		1,		//counter number
		1,		//matrix
		75,		//ibase
		36,		//oword
		1753));	//cbase
	m_counters14bit.push_back(new Greenpak4Counter(
		this,
		14,		//depth
		true,	//we have FSM mode
		false,	//no wake-sleep powerdown
		true,	//we have edge detector
		false,	//no PWM
		2,		//counter number
		0,		//matrix
		75,		//ibase
		37,		//oword
		1774));	//cbase
	m_counters14bit.push_back(new Greenpak4Counter(
		this,
		14,		//depth
		false,	//no FSM mode
		false,	//no wake-sleep powerdown
		false,	//no edge detector
		false,	//no PWM
		3,		//counter number
		1,		//matrix
		76,		//ibase
		37,		//oword
		1799));	//cbase (datasheet has a typo, this is correct)
	m_counters8bit.push_back(new Greenpak4Counter(
		this,
		8,		//depth 
		true,	//we have FSM mode
		false,	//no wake-sleep powerdown
		false,	//no edge detector
		false,	//no PWM
		4,		//counter number
		1,		//matrix
		77,		//ibase
		38,		//oword,
		1820));	//cbase
	m_counters8bit.push_back(new Greenpak4Counter(
		this,
		8,		//depth 
		false,	//no FSM mode
		false,	//no wake-sleep powerdown
		false,	//no edge detector
		false,	//no PWM
		5,		//counter number
		0,		//matrix
		78,		//ibase
		38,		//oword,
		1838));	//cbase
	m_counters8bit.push_back(new Greenpak4Counter(
		this,
		8,		//depth 
		false,	//no FSM mode
		false,	//no wake-sleep powerdown
		false,	//no edge detector
		false,	//no PWM
		6,		//counter number
		0,		//matrix
		79,		//ibase
		39,		//oword,
		1852));	//cbase
	m_counters8bit.push_back(new Greenpak4Counter(
		this,
		8,		//depth 
		false,	//no FSM mode
		false,	//no wake-sleep powerdown
		false,	//no edge detector
		false,	//no PWM
		7,		//counter number
		1,		//matrix (datasheet has a typo, this is correct)
		80,		//ibase
		39,		//oword,
		1866));	//cbase
	m_counters8bit.push_back(new Greenpak4Counter(
		this,
		8,		//depth 
		false,	//no FSM mode
		false,	//no wake-sleep powerdown
		false,	//no edge detector
		true,	//PWM mode
		8,		//counter number
		1,		//matrix
		81,		//ibase
		40,		//oword,
		1880));	//cbase
	m_counters8bit.push_back(new Greenpak4Counter(
		this,
		8,		//depth 
		false,	//no FSM mode
		false,	//no wake-sleep powerdown
		false,	//no edge detector
		true,	//PWM mode
		9,		//counter number
		0,		//matrix
		80,		//ibase
		40,		//oword,
		1895));	//cbase
	
	//TODO: Slave SPI
	
	//TODO: Cross-connections between matrixes
	
	//TODO: ADC
	
	//TODO: DAC
	
	//Bandgap reference
	m_bandgap = new Greenpak4Bandgap(this, 0, 0, 41, 923);
	
	//TODO: Reserved bits
	
	//TODO: Vdd bypass
	
	//Power-on reset
	m_por = new Greenpak4PowerOnReset(this, 0, -1, 62, 2009);
	
	//TODO: IO pad precharge? what does this involve?
	
	//System reset BLOCK
	m_sysrst = new Greenpak4SystemReset(this, 0, 24, -1, 2018);
	
	//Total length of our bitstream
	m_bitlen = 2048;
	
	//Initialize matrix base addresses
	m_matrixBase[0] = 0;
	m_matrixBase[1] = 1024;
	
	//Create cross connections
	for(unsigned int matrix=0; matrix<2; matrix++)
	{
		for(unsigned int i=0; i<10; i++)
		{
			auto cc = new Greenpak4CrossConnection(
				this,
				1 - matrix,	//invert, since matrix is OUTPUT location
				85 + i,		//ibase
				52 + i,		//oword
				0			//cbase is invalid, we have no configuration at all
				);
			cc->SetInput(m_constantZero);
			m_crossConnections[matrix][i] = cc;
		}
	}
	
	//Do final initialization
	CreateDevice_common();	
}

void Greenpak4Device::CreateDevice_common()
{
	//Add LUT2-3-4s to the LUT list
	for(auto x : m_lut2s)
		m_luts.push_back(x);
	for(auto x : m_lut3s)
		m_luts.push_back(x);
	for(auto x : m_lut4s)
		m_luts.push_back(x);
		
	//Add both kinds of FFs to the FF list
	for(auto x : m_dffs)
		m_dffAll.push_back(x);
	for(auto x : m_dffsr)
		m_dffAll.push_back(x);
		
	//Add all counters to counter list
	for(auto x : m_counters8bit)
		m_counters.push_back(x);
	for(auto x : m_counters14bit)
		m_counters.push_back(x);
	
	//Finally, put everything in bitstuff so we can walk the whole bitstream and not care about details
	for(auto x : m_luts)
		m_bitstuff.push_back(x);
	for(auto x : m_dffAll)
		m_bitstuff.push_back(x);
	for(auto x : m_inverters)
		m_bitstuff.push_back(x);
	for(auto x : m_counters)
		m_bitstuff.push_back(x);
	for(auto x : m_iobs)
		m_bitstuff.push_back(x.second);
	m_bitstuff.push_back(m_constantZero);
	m_bitstuff.push_back(m_constantOne);
	m_bitstuff.push_back(m_lfosc);
	m_bitstuff.push_back(m_sysrst);
	m_bitstuff.push_back(m_bandgap);
	m_bitstuff.push_back(m_por);
	
	//TODO: this might be device specific - not all parts have exactly two matrices and ten cross connections
	for(unsigned int matrix=0; matrix<2; matrix++)
		for(unsigned int i=0; i<10; i++)
			m_bitstuff.push_back(m_crossConnections[matrix][i]);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Accessors

Greenpak4BitstreamEntity* Greenpak4Device::GetPowerRail(bool rail)
{
	if(rail)
		return m_constantOne;
	else
		return m_constantZero;
}

Greenpak4IOB* Greenpak4Device::GetIOB(unsigned int pin)
{
	if(m_iobs.find(pin) == m_iobs.end())
		return NULL;
	return m_iobs[pin];
}

Greenpak4LUT* Greenpak4Device::GetLUT(unsigned int i)
{
	if(i >= m_luts.size())
		return NULL;
	return m_luts[i];
}

Greenpak4LUT* Greenpak4Device::GetLUT2(unsigned int i)
{
	if(i >= m_lut2s.size())
		return NULL;
	return m_lut2s[i];
}

Greenpak4LUT* Greenpak4Device::GetLUT3(unsigned int i)
{
	if(i >= m_lut3s.size())
		return NULL;
	return m_lut3s[i];
}

Greenpak4LUT* Greenpak4Device::GetLUT4(unsigned int i)
{
	if(i >= m_lut4s.size())
		return NULL;
	return m_lut4s[i];
}

unsigned int Greenpak4Device::GetMatrixBase(unsigned int matrix)
{
	if(matrix > 1)
		return 0;
		
	return m_matrixBase[matrix];
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// File I/O

bool Greenpak4Device::WriteToFile(std::string fname)
{
	//Open the file
	FILE* fp = fopen(fname.c_str(), "w");
	if(!fp)
	{
		fprintf(stderr, "Couldn't open %s for writing\n", fname.c_str());
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
