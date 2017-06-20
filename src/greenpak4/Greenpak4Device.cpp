/***********************************************************************************************************************
 * Copyright (C) 2016-2017 Andrew Zonenberg and contributors                                                           *
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

#include <cassert>
#include <log.h>
#include <Greenpak4.h>

using namespace std;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Construction / destruction

Greenpak4Device::Greenpak4Device(
	GREENPAK4_PART part,
	Greenpak4IOB::PullDirection default_pull,
	Greenpak4IOB::PullStrength default_drive)
	: m_part(part)
	, m_ioPrecharge(false)
	, m_disableChargePump(false)
	, m_ldoBypass(false)
	, m_nvmLoadRetryCount(1)
	, m_hasTimingData(false)
{
	//Create power rails
	//These have to come first, since all other nodes will refer to these during construction
	m_constantZero = new Greenpak4PowerRail(this, 0, 0);
	m_constantOne = new Greenpak4PowerRail(this, 0, 63);

	//NULL out all of the non-array objects in case we don't have them in the device being created
	m_lfosc = NULL;
	m_rcosc = NULL;
	m_ringosc = NULL;
	m_abuf = NULL;
	m_sysrst = NULL;
	m_bandgap = NULL;
	m_pga = NULL;
	m_por = NULL;
	m_pwrdet = NULL;
	m_dcmpmux = NULL;
	m_spi = NULL;
	for(int i=0; i<2; i++)
	{
		for(int j=0; j<10; j++)
			m_crossConnections[i][j] = NULL;
	}

	//Initialize everything
	switch(part)
	{
	case GREENPAK4_SLG46140:
		CreateDevice_SLG46140();
		break;

	case GREENPAK4_SLG46620:
		CreateDevice_SLG4662x(false);
		break;

	case GREENPAK4_SLG46621:
		CreateDevice_SLG4662x(true);
		break;

	default:
		LogFatal("Invalid part number requested\n");
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

void Greenpak4Device::CreateDevice_SLG46140()
{
	//64 inputs per routing matrix
	m_matrixBits = 6;

	//Create the dedicated LUT2s (4 total)
	for(int i=0; i<4; i++)
	{
		m_lut2s.push_back(new Greenpak4LUT(
			this,
			i,
			0,			//everything is in matrix 0
			i*2,		//LUT2 base is row 0, then 2 inputs per LUT
			i+1,		//First mux entry is ground, then the LUT2s
			848 + i*4,	//LUT2s start at bitstream offset 848, 2^2 bits per LUT
			2));		//this is a LUT2
	}

	//Create the dedicated LUT3s (4 total)
	for(int i=0; i<4; i++)
	{
		m_lut2s.push_back(new Greenpak4LUT(
			this,
			i,
			0,			//everything is in matrix 0
			i*3 + 12,	//LUT3 base is row 12, then 3 inputs per LUT
			i+7,		//we come after the last LUT3
			874 + i*8,	//LUT3s start at bitstream offset 874, 2^3 bits per LUT
			3));		//this is a LUT3
	}

	/*
	//Create the first LUT, and its pattern-generator alter ego
	auto lut40 = new Greenpak4LUT(
		this,
		0,
		0,		//Attached to crossbar #0
		32,		//LUT4 base is row 32
		13,		//we come after the last LUT3
		656,	//LUT4 starts after last LUT3
		4);	//this is a LUT4
	auto pgen = new Greenpak4PatternGenerator(
		this,
		0,
		32,
		13,
		656);

	//Create the paired cell for them
	auto lpgen = new Greenpak4PairedEntity(
		this,
		0,		//Attached to crossbar #0
		676,	//Selector bit for LUT or PGEN mode
		lut40,	//select=0 means we're a LUT
		pgen);	//select=1 means we're a PGEN

	lpgen->AddType("GP_INV", 0);	//Combinatorial logic all uses the LUT
	lpgen->AddType("GP_2LUT", 0);
	lpgen->AddType("GP_3LUT", 0);
	lpgen->AddType("GP_4LUT", 0);
	lpgen->AddType("GP_PGEN", 1);	//Pattern generator is the only block mapped to the PGEN

	m_lut4s.push_back(lpgen);
	*/

	//Create the Type-A IOBs (with output enable).
	m_iobs[2] =  new Greenpak4IOBTypeA(this, 2,  0, -1, 22, 761, Greenpak4IOB::IOB_FLAG_INPUTONLY);
	m_iobs[3] =  new Greenpak4IOBTypeA(this, 3,  0, 44, 23, 766);
	m_iobs[4] =  new Greenpak4IOBTypeA(this, 4,  0, 46, 24, 773);
	m_iobs[5] =  new Greenpak4IOBTypeA(this, 5,  0, 48, 25, 780);
	m_iobs[7] =  new Greenpak4IOBTypeA(this, 7,  0, 51, 27, 795);
	m_iobs[9] =  new Greenpak4IOBTypeA(this, 9,  0, 53, 28, 802);
	m_iobs[12] = new Greenpak4IOBTypeA(this, 12, 0, 57, 31, 827);
	m_iobs[13] = new Greenpak4IOBTypeA(this, 13, 0, 59, 32, 834);
	m_iobs[14] = new Greenpak4IOBTypeA(this, 14, 0, 61, 33, 841);

	//m_iobs[19]->SetAnalogConfigBase(878);
	//m_iobs[18]->SetAnalogConfigBase(876);

	//Create the Type-B IOBs (no output enable)
	m_iobs[6]  = new Greenpak4IOBTypeB(this,  6, 0, 50, 26, 788);
	m_iobs[10] = new Greenpak4IOBTypeB(this, 10, 0, 55, 29, 811, Greenpak4IOB::IOB_FLAG_X4DRIVE);
	m_iobs[11] = new Greenpak4IOBTypeB(this, 11, 0, 56, 30, 820);

	//Dedicated DFF/latches (2 total)
	//m_dffsr.push_back(new Greenpak4Flipflop(this, 2,  true,  0, 42, 16, 685));
	//m_dffs.push_back( new Greenpak4Flipflop(this, 3,  false, 0, 45, 17, 689));

	//Shift registers
	//m_shregs.push_back(new Greenpak4ShiftRegister(this, 0, 40, 19, 750));

	//Edge detector/prog delay line
	m_delays.push_back(new Greenpak4Delay(this, 0, 43, 21, 486));

	//No discrete inverters

	//TODO: External clocks??

	//Low-frequency oscillator
	m_lfosc = new Greenpak4LFOscillator(
		this,
		0, 		//everything is matrix 0
		66,		//input base (single power-down input)
		36,		//output word (plus dedicated routing to counters etc)
		562,	//bitstream location of power management stuff
		560);	//bitstream location of clock divider

	/*
	//Ring oscillator
	//TODO: Addresses are right, but bitstream coding is different
	m_ringosc = new Greenpak4RingOscillator(
		this,
		0,		//everything is matrix 0
		66,		//input base (single power-down input)
		34,		//output word (plus dedicated routing to counters etc)
		574);	//bitstream location
	*/

	/*
	//RC oscillator
	m_rcosc = new Greenpak4RCOscillator(
		this,
		0,		//Matrix applies to inputs, we can route output globally
		84,		//input base (single power-down input)
		49,		//output word (plus dedicated routing to counters etc)
		1642);	//bitstream location
	*/

	//Counters
	/*
	m_counters8bit.push_back(new Greenpak4Counter(
		this,
		8,		//depth
		false,	//no FSM mode
		true,	//we have wake-sleep powerdown
		true,	//we have edge detector
		false,	//no PWM
		0,		//counter number
		0,		//matrix
		68,		//ibase
		37,		//oword,
		722));	//cbase
	*/

	/*
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
	*/

	//TODO: Slave SPI

	//TODO: ADC

	/*
	//The DACs
	m_dacs.push_back(new Greenpak4DAC(this, 844, 840, 843, 840, 0));
	m_dacs.push_back(new Greenpak4DAC(this, 823, 834, 883, 840, 1));
	*/
	//Bandgap reference
	m_bandgap = new Greenpak4Bandgap(this, 0, 0, 51, 507, 512, 0);
	/*
	//Voltage references for comparators
	m_vrefs.push_back(new Greenpak4VoltageReference(this, 0, 1));
	m_vrefs.push_back(new Greenpak4VoltageReference(this, 1, 2));
	m_vrefs.push_back(new Greenpak4VoltageReference(this, 2, 1));
	m_vrefs.push_back(new Greenpak4VoltageReference(this, 3, 2));
	m_vrefs.push_back(new Greenpak4VoltageReference(this, 4));
	m_vrefs.push_back(new Greenpak4VoltageReference(this, 5));
	*/
	//Extra voltage references for the DACs (always 1.0V but having them declared as GP_VREF makes HDL cleaner)
	m_vrefs.push_back(new Greenpak4VoltageReference(this, 2));
	m_vrefs.push_back(new Greenpak4VoltageReference(this, 3));
	/*
	//Analog comparators
	//TODO speed doubler for ACMP5? Need to double check latest datasheet, this may have been changed
	m_acmps.push_back(new Greenpak4Comparator(this, 0, 0, 69, 33, 832, 852, 853, 855, 934, 892));
	m_acmps.push_back(new Greenpak4Comparator(this, 1, 1, 70, 33, 831, 861, 857, 859, 932, 897));
	m_acmps.push_back(new Greenpak4Comparator(this, 2, 1, 71, 34,  0,  862, 864, 863, 930, 902));
	m_acmps.push_back(new Greenpak4Comparator(this, 3, 1, 72, 35,  0,  866, 867, 869, 928, 907));
	m_acmps.push_back(new Greenpak4Comparator(this, 4, 0, 70, 34,  0,  875, 871, 873, 926, 912));
	m_acmps.push_back(new Greenpak4Comparator(this, 5, 0, 71, 35,  0,  880,  0,   0,  924, 917));

	//PGA
	m_pga = new Greenpak4PGA(this, 815);
	*/
	//No analog buffer in SLG46140
	/*
	//Comparator input routing
	auto pin3 = m_iobs[3]->GetOutput("OUT");
	auto pin4 = m_iobs[4]->GetOutput("OUT");
	auto pin6 = m_iobs[6]->GetOutput("OUT");
	auto pin12 = m_iobs[12]->GetOutput("OUT");
	auto pin13 = m_iobs[13]->GetOutput("OUT");
	auto pin15 = m_iobs[15]->GetOutput("OUT");
	auto vdd = m_constantOne->GetOutput("OUT");
	auto pga = m_pga->GetOutput("VOUT");
	auto pin6_buf = m_abuf->GetOutput("OUT");
	m_acmps[0]->AddInputMuxEntry(pin6, 0);
	m_acmps[0]->AddInputMuxEntry(pin6_buf, 1);
	m_acmps[0]->AddInputMuxEntry(vdd, 2);
	m_acmps[1]->AddInputMuxEntry(pin12, 0);
	m_acmps[1]->AddInputMuxEntry(pga, 1);
	m_acmps[1]->AddInputMuxEntry(pin6, 2);
	m_acmps[1]->AddInputMuxEntry(pin6_buf, 2);
	m_acmps[1]->AddInputMuxEntry(vdd, 2);
	*/
	//TODO: Vdd bypass

	//Power-on reset
	m_por = new Greenpak4PowerOnReset(this, 0, -1, 62, 1004);

	//Power detector
	m_pwrdet = new Greenpak4PowerDetector(this, 0, 52);

	//System reset
	m_sysrst = new Greenpak4SystemReset(this, 0, 22, -1, 1000);

	//Total length of our bitstream
	m_bitlen = 1024;

	//Initialize matrix base addresses
	m_matrixBase[0] = 0;
	m_matrixBase[1] = 0;		//initialize for completeness's sake, but there is no matrix 1

	//no cross connections

	//Do final initialization
	CreateDevice_common();
}

void Greenpak4Device::CreateDevice_SLG4662x(bool dual_rail)
{
	//64 inputs per routing matrix
	m_matrixBits = 6;

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

	//Create the second LUT4 (no special functionality)
	//Put it FIRST in the array so that initial placement prefers it
	//and doesn't take up the PGEN site right away
	m_lut4s.push_back(new Greenpak4LUT(
		this,
		1,
		1,		//Attached to crossbar #1
		32,		//LUT4 base is row 32
		13,		//we come after the last LUT3
		778,	//LUT4s start at bitstream offset 778, 2^4 bits per LUT
		4));	//this is a LUT4

	//Create the first LUT, and its pattern-generator alter ego
	auto lut40 = new Greenpak4LUT(
		this,
		0,
		0,		//Attached to crossbar #0
		32,		//LUT4 base is row 32
		13,		//we come after the last LUT3
		656,	//LUT4 starts after last LUT3
		4);	//this is a LUT4
	auto pgen = new Greenpak4PatternGenerator(
		this,
		0,
		32,
		13,
		656);

	//Create the paired cell for them
	auto lpgen = new Greenpak4PairedEntity(
		this,
		0,		//Attached to crossbar #0
		676,	//Selector bit for LUT or PGEN mode
		lut40,	//select=0 means we're a LUT
		pgen);	//select=1 means we're a PGEN

	lpgen->AddType("GP_INV", 0);	//Combinatorial logic all uses the LUT
	lpgen->AddType("GP_2LUT", 0);
	lpgen->AddType("GP_3LUT", 0);
	lpgen->AddType("GP_4LUT", 0);
	lpgen->AddType("GP_PGEN", 1);	//Pattern generator is the only block mapped to the PGEN

	m_lut4s.push_back(lpgen);

	//Create the Type-A IOBs (with output enable).
	//Pin 14 is a used as VCCIO in the SLG46621, the GPIO driver is not bonded out
	m_iobs[2] =  new Greenpak4IOBTypeA(this, 2,  0, -1, 24, 941, Greenpak4IOB::IOB_FLAG_INPUTONLY);
	m_iobs[3] =  new Greenpak4IOBTypeA(this, 3,  0, 56, 25, 946);
	m_iobs[5] =  new Greenpak4IOBTypeA(this, 5,  0, 59, 27, 960);
	m_iobs[7] =  new Greenpak4IOBTypeA(this, 7,  0, 62, 29, 974);
	m_iobs[9] =  new Greenpak4IOBTypeA(this, 9,  0, 65, 31, 988);
	m_iobs[10] = new Greenpak4IOBTypeA(this, 10, 0, 67, 32, 995, Greenpak4IOB::IOB_FLAG_X4DRIVE);
	m_iobs[13] = new Greenpak4IOBTypeA(this, 13, 1, 57, 25, 1919);
	if(!dual_rail)
		m_iobs[14] = new Greenpak4IOBTypeA(this, 14, 1, 59, 26, 1926);
	m_iobs[16] = new Greenpak4IOBTypeA(this, 16, 1, 62, 28, 1940);
	m_iobs[18] = new Greenpak4IOBTypeA(this, 18, 1, 65, 30, 1954);
	m_iobs[19] = new Greenpak4IOBTypeA(this, 19, 1, 67, 31, 1961);

	m_iobs[19]->SetAnalogConfigBase(878);
	m_iobs[18]->SetAnalogConfigBase(876);

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

	//Shift registers
	m_shregs.push_back(new Greenpak4ShiftRegister(this, 0, 51, 20, 1610));
	m_shregs.push_back(new Greenpak4ShiftRegister(this, 1, 51, 20, 1619));

	//Edge detector/prog delays
	m_delays.push_back(new Greenpak4Delay(this, 0, 54, 22, 1600));
	m_delays.push_back(new Greenpak4Delay(this, 1, 54, 22, 1605));

	//Inverters
	m_inverters.push_back(new Greenpak4Inverter(this, 0, 55, 23));
	m_inverters.push_back(new Greenpak4Inverter(this, 1, 55, 23));

	//Low-frequency oscillator
	m_lfosc = new Greenpak4LFOscillator(
		this,
		0, 		//Matrix applies to inputs, we can route output globally
		84,		//input base (single power-down input)
		50,		//output word (plus dedicated routing to counters etc)
		1652,	//bitstream location of power management stuff
		1654);	//bitstream location of clock divider

	//Ring oscillator
	m_ringosc = new Greenpak4RingOscillator(
		this,
		0,		//Matrix applies to inputs, we can route output globally
		84,		//input base (single power-down input)
		48,		//output word (plus dedicated routing to counters etc)
		1630);	//bitstream location

	//RC oscillator
	m_rcosc = new Greenpak4RCOscillator(
		this,
		0,		//Matrix applies to inputs, we can route output globally
		84,		//input base (single power-down input)
		49,		//output word (plus dedicated routing to counters etc)
		1642);	//bitstream location

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

	//Slave SPI
	m_spi = new Greenpak4SPI(this, 0, 82, 44, 1656);

	//TODO: ADC

	//The DACs
	m_dacs.push_back(new Greenpak4DAC(this, 844, 840, 843, 840, 0));
	m_dacs.push_back(new Greenpak4DAC(this, 823, 834, 883, 840, 1));

	//Bandgap reference
	m_bandgap = new Greenpak4Bandgap(this, 0, 0, 41, 923, 936, 938);

	//Voltage references for comparators
	m_vrefs.push_back(new Greenpak4VoltageReference(this, 0, 1));
	m_vrefs.push_back(new Greenpak4VoltageReference(this, 1, 2));
	m_vrefs.push_back(new Greenpak4VoltageReference(this, 2, 1));
	m_vrefs.push_back(new Greenpak4VoltageReference(this, 3, 2));
	m_vrefs.push_back(new Greenpak4VoltageReference(this, 4));
	m_vrefs.push_back(new Greenpak4VoltageReference(this, 5));

	//Extra voltage references for the DACs (always 1.0V but having them declared as GP_VREF makes HDL cleaner)
	m_vrefs.push_back(new Greenpak4VoltageReference(this, 6));
	m_vrefs.push_back(new Greenpak4VoltageReference(this, 7));

	//Analog comparators
	//TODO speed doubler for ACMP5? Need to double check latest datasheet, this may have been changed
	m_acmps.push_back(new Greenpak4Comparator(this, 0, 0, 69, 33, 832, 852, 853, 855, 934, 892));
	m_acmps.push_back(new Greenpak4Comparator(this, 1, 1, 70, 33, 831, 861, 857, 859, 932, 897));
	m_acmps.push_back(new Greenpak4Comparator(this, 2, 1, 71, 34,  0,  862, 864, 863, 930, 902));
	m_acmps.push_back(new Greenpak4Comparator(this, 3, 1, 72, 35,  0,  866, 867, 869, 928, 907));
	m_acmps.push_back(new Greenpak4Comparator(this, 4, 0, 70, 34,  0,  875, 871, 873, 926, 912));
	m_acmps.push_back(new Greenpak4Comparator(this, 5, 0, 71, 35,  0,  880,  0,   0,  924, 917));

	//Digital comparators
	m_dcmps.push_back(new Greenpak4DigitalComparator(this, 0, 1, 82, 42, 1670));
	m_dcmps.push_back(new Greenpak4DigitalComparator(this, 1, 1, 82, 44, 1691));
	m_dcmps.push_back(new Greenpak4DigitalComparator(this, 2, 1, 82, 46, 1711));

	//Digital comparator references
	//SLG46620 datasheet r100 page 140-141 fig 94-95 are wrong.
	//reg0 base is 1723, not 1725
	m_dcmprefs.push_back(new Greenpak4DCMPRef(this, 0, 1723));
	m_dcmprefs.push_back(new Greenpak4DCMPRef(this, 1, 1703));
	m_dcmprefs.push_back(new Greenpak4DCMPRef(this, 2, 1683));
	m_dcmprefs.push_back(new Greenpak4DCMPRef(this, 3, 1662));

	//Digital comparator input mux
	m_dcmpmux = new Greenpak4DCMPMux(this, 1, 83);

	//Clock buffers
	m_clkbufs.push_back(new Greenpak4ClockBuffer(this, 0, 0, 72));	//clk_matrix0
	m_clkbufs.push_back(new Greenpak4ClockBuffer(this, 1, 0, 73));	//clk_matrix1
	m_clkbufs.push_back(new Greenpak4ClockBuffer(this, 2, 1, 73));	//clk_matrix2
	m_clkbufs.push_back(new Greenpak4ClockBuffer(this, 3, 1, 74));	//clk_matrix3
	m_clkbufs.push_back(new Greenpak4ClockBuffer(this, 4, 0, 83));	//SPI SCK

	//Clock mux buffer for ADC and PWM
	auto mbuf = new Greenpak4MuxedClockBuffer(this, 5, 0, 1628);
	m_clkbufs.push_back(mbuf);
	mbuf->AddInputMuxEntry(m_ringosc->GetOutput("CLKOUT_HARDIP"), 0);
	mbuf->AddInputMuxEntry(m_clkbufs[2]->GetOutput("OUT"), 1);
	mbuf->AddInputMuxEntry(m_rcosc->GetOutput("CLKOUT_HARDIP"), 2);
	mbuf->AddInputMuxEntry(m_clkbufs[4]->GetOutput("OUT"), 3);

	//inp 0 is ADC, not implemented
	m_dcmps[0]->AddInputPMuxEntry(m_spi->GetOutput("RXD_HIGH"), 1);
	m_dcmps[0]->AddInputPMuxEntry(m_counters14bit[2]->GetOutput("POUT"), 2);
	m_dcmps[0]->AddInputPMuxEntry(m_dcmpmux->GetOutput("OUTA"), 3);

	m_dcmps[0]->AddInputNMuxEntry(m_counters8bit[4]->GetOutput("POUT"), 0);
	m_dcmps[0]->AddInputNMuxEntry(m_dcmprefs[0]->GetOutput("OUT"), 1);
	m_dcmps[0]->AddInputNMuxEntry(m_spi->GetOutput("RXD_LOW"), 2);
	m_dcmps[0]->AddInputNMuxEntry(m_counters8bit[0]->GetOutput("POUT"), 3);

	//in0 is ADC, not implemented
	m_dcmps[1]->AddInputPMuxEntry(m_spi->GetOutput("RXD_LOW"), 1);
	m_dcmps[1]->AddInputPMuxEntry(m_counters8bit[0]->GetOutput("POUT"), 2);
	m_dcmps[1]->AddInputPMuxEntry(m_dcmprefs[1]->GetOutput("OUT"), 3);

	m_dcmps[1]->AddInputNMuxEntry(m_counters8bit[5]->GetOutput("POUT"), 0);
	m_dcmps[1]->AddInputNMuxEntry(m_dcmpmux->GetOutput("OUTB"), 1);
	m_dcmps[1]->AddInputNMuxEntry(m_spi->GetOutput("RXD_LOW"), 2);
	m_dcmps[1]->AddInputNMuxEntry(m_counters14bit[2]->GetOutput("POUT"), 3);

	//in0 is ADC, not implemented
	m_dcmps[2]->AddInputPMuxEntry(m_spi->GetOutput("RXD_HIGH"), 1);
	m_dcmps[2]->AddInputPMuxEntry(m_counters8bit[0]->GetOutput("POUT"), 2);
	m_dcmps[2]->AddInputPMuxEntry(m_dcmprefs[3]->GetOutput("OUT"), 3);

	m_dcmps[2]->AddInputNMuxEntry(m_counters8bit[4]->GetOutput("POUT"), 0);
	m_dcmps[2]->AddInputNMuxEntry(m_dcmprefs[2]->GetOutput("OUT"), 1);
	m_dcmps[2]->AddInputNMuxEntry(m_spi->GetOutput("RXD_LOW"), 2);
	m_dcmps[2]->AddInputNMuxEntry(m_counters14bit[2]->GetOutput("POUT"), 3);

	//PGA
	m_pga = new Greenpak4PGA(this, 815);

	//Analog buffer
	m_abuf = new Greenpak4Abuf(this, 836);

	//Comparator input routing
	auto pin3 = m_iobs[3]->GetOutput("OUT");
	auto pin4 = m_iobs[4]->GetOutput("OUT");
	auto pin6 = m_iobs[6]->GetOutput("OUT");
	auto pin12 = m_iobs[12]->GetOutput("OUT");
	auto pin13 = m_iobs[13]->GetOutput("OUT");
	auto pin15 = m_iobs[15]->GetOutput("OUT");
	auto vdd = m_constantOne->GetOutput("OUT");
	auto pga = m_pga->GetOutput("VOUT");
	auto pin6_buf = m_abuf->GetOutput("OUT");
	m_acmps[0]->AddInputMuxEntry(pin6, 0);
	m_acmps[0]->AddInputMuxEntry(pin6_buf, 1);
	m_acmps[0]->AddInputMuxEntry(vdd, 2);
	m_acmps[1]->AddInputMuxEntry(pin12, 0);
	m_acmps[1]->AddInputMuxEntry(pga, 1);
	m_acmps[1]->AddInputMuxEntry(pin6, 2);
	m_acmps[1]->AddInputMuxEntry(pin6_buf, 2);
	m_acmps[1]->AddInputMuxEntry(vdd, 2);
	m_acmps[2]->AddInputMuxEntry(pin13, 0);
	m_acmps[2]->AddInputMuxEntry(pin6, 1);
	m_acmps[2]->AddInputMuxEntry(pin6_buf, 1);
	m_acmps[2]->AddInputMuxEntry(vdd, 1);
	m_acmps[3]->AddInputMuxEntry(pin15, 0);
	m_acmps[3]->AddInputMuxEntry(pin13, 1);
	m_acmps[3]->AddInputMuxEntry(pin6, 2);
	m_acmps[3]->AddInputMuxEntry(pin6_buf, 2);
	m_acmps[3]->AddInputMuxEntry(vdd, 2);
	m_acmps[4]->AddInputMuxEntry(pin3, 0);
	m_acmps[4]->AddInputMuxEntry(pin15, 1);
	m_acmps[4]->AddInputMuxEntry(pin6, 2);
	m_acmps[4]->AddInputMuxEntry(pin6_buf, 2);
	m_acmps[4]->AddInputMuxEntry(vdd, 2);
	m_acmps[5]->AddInputMuxEntry(pin4, 0);

	//Power-on reset
	m_por = new Greenpak4PowerOnReset(this, 0, -1, 62, 2009);

	//Power detector
	m_pwrdet = new Greenpak4PowerDetector(this, 0, 42);

	//System reset
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
	for(auto x : m_counters14bit)
		m_counters.push_back(x);
	for(auto x : m_counters8bit)
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
	for(auto x : m_shregs)
		m_bitstuff.push_back(x);
	for(auto x : m_vrefs)
		m_bitstuff.push_back(x);
	for(auto x : m_acmps)
		m_bitstuff.push_back(x);
	for(auto x : m_dcmps)
		m_bitstuff.push_back(x);
	for(auto x : m_dcmprefs)
		m_bitstuff.push_back(x);
	for(auto x : m_dacs)
		m_bitstuff.push_back(x);
	for(auto x : m_delays)
		m_bitstuff.push_back(x);
	for(auto x : m_clkbufs)
		m_bitstuff.push_back(x);
	m_bitstuff.push_back(m_constantZero);
	m_bitstuff.push_back(m_constantOne);

	//Add global objects if we have them
	if(m_lfosc)
		m_bitstuff.push_back(m_lfosc);
	if(m_ringosc)
		m_bitstuff.push_back(m_ringosc);
	if(m_rcosc)
		m_bitstuff.push_back(m_rcosc);
	if(m_sysrst)
		m_bitstuff.push_back(m_sysrst);
	if(m_bandgap)
		m_bitstuff.push_back(m_bandgap);
	if(m_por)
		m_bitstuff.push_back(m_por);
	if(m_pwrdet)
		m_bitstuff.push_back(m_pwrdet);
	if(m_pga)
		m_bitstuff.push_back(m_pga);
	if(m_abuf)
		m_bitstuff.push_back(m_abuf);
	if(m_dcmpmux)
		m_bitstuff.push_back(m_dcmpmux);
	if(m_spi)
		m_bitstuff.push_back(m_spi);

	//Add cross connections iff we have them
	if(m_matrixBase[0] != m_matrixBase[1])
	{
		for(unsigned int matrix=0; matrix<2; matrix++)
			for(unsigned int i=0; i<10; i++)
				m_bitstuff.push_back(m_crossConnections[matrix][i]);
	}
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

Greenpak4BitstreamEntity* Greenpak4Device::GetLUT(unsigned int i)
{
	if(i >= m_luts.size())
		return NULL;
	return m_luts[i];
}

Greenpak4BitstreamEntity* Greenpak4Device::GetLUT2(unsigned int i)
{
	if(i >= m_lut2s.size())
		return NULL;
	return m_lut2s[i];
}

Greenpak4BitstreamEntity* Greenpak4Device::GetLUT3(unsigned int i)
{
	if(i >= m_lut3s.size())
		return NULL;
	return m_lut3s[i];
}

Greenpak4BitstreamEntity* Greenpak4Device::GetLUT4(unsigned int i)
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

void Greenpak4Device::SetIOPrecharge(bool precharge)
{
	m_ioPrecharge = precharge;
}

void Greenpak4Device::SetDisableChargePump(bool disable)
{
	m_disableChargePump = disable;
}

void Greenpak4Device::SetLDOBypass(bool bypass)
{
	m_ldoBypass = bypass;
}

void Greenpak4Device::SetNVMRetryCount(int count)
{
	m_nvmLoadRetryCount = count;
}

string Greenpak4Device::GetPartAsString()
{
	switch(m_part)
	{
		case GREENPAK4_SLG46140:
			return "SLG46140";

		case GREENPAK4_SLG46620:
			return "SLG46620";

		case GREENPAK4_SLG46621:
			return "SLG46621";
	}
	return "(unknown)\n";
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// File I/O

/**
	@brief Writes the bitstream to a file

	@param fname		Name of the file to write to
	@param userid		ID code to write to the "user ID" area of the bitstream
	@param readProtect	True to disable readout of the design
 */
bool Greenpak4Device::WriteToFile(string fname, uint8_t userid, bool readProtect)
{
	//Open the file
	FILE* fp = fopen(fname.c_str(), "w");
	if(!fp)
	{
		LogError("Couldn't open %s for writing\n", fname.c_str());
		return false;
	}

	//Allocate the bitstream and initialize to zero
	//According to phone conversation w Silego FAE, 0 is legal default state for everything incl reserved bits
	//All IOs will be floating digital inputs
	bool* bitstream = new bool[m_bitlen];
	for(unsigned int i=0; i<m_bitlen; i++)
		bitstream[i] = false;

	//Generate the bitstream, then write to file if successful
	bool ok = true;
	if(GenerateBitstream(bitstream, userid, readProtect))
	{
		fprintf(fp, "index\t\tvalue\t\tcomment\n");
		for(unsigned int i=0; i<m_bitlen; i++)
			fprintf(fp, "%u\t\t%d\t\t//\n", i, (int)bitstream[i]);
	}
	else
		ok = false;

	//Done
	delete[] bitstream;
	fclose(fp);
	return ok;
}

/**
	@brief Writes a bitstream to an in-memory netlist
 */
bool Greenpak4Device::WriteToBuffer(vector<uint8_t>& bitstream, uint8_t userid, bool readProtect)
{
	bool* rawbits = new bool[m_bitlen];
	for(unsigned int i=0; i<m_bitlen; i++)
		rawbits[i] = false;

	//Generate the bitstream, abort if it fails
	if(!GenerateBitstream(rawbits, userid, readProtect))
	{
		delete[] rawbits;
		return false;
	}

	for(size_t index=0; index<m_bitlen; index++)
	{
		int byteindex = index / 8;
		if(byteindex >= (int)bitstream.size())
			bitstream.resize(byteindex + 1, 0);
		bool value = rawbits[index];
		bitstream[byteindex] |= (value << (index % 8));
	}

	//Clean up
	delete[] rawbits;
	return true;
}

/**
	@brief Generates an in-memory bitstream image

	@param bitstream	Raw bit array
	@param userid		ID code to write to the "user ID" area of the bitstream
	@param readProtect	True to disable readout of the design
 */
bool Greenpak4Device::GenerateBitstream(bool* bitstream, uint8_t userid, bool readProtect)
{
	bool ok = true;

	//Get the config data from each of our blocks
	for(auto x : m_bitstuff)
	{
		if(!x->Save(bitstream))
		{
			LogError("Bitstream node %s failed to save\n", x->GetDescription().c_str());
			ok = false;
		}
	}

	//Write chip-wide tuning data and ID code
	switch(m_part)
	{
		case GREENPAK4_SLG46621:

			//Tie the unused on-die IOB for pin 14 to ground
			//TODO: warn if anything tried to use pin 14?
			for(int i=1378; i<=1389; i++)
				bitstream[i] = false;

			//Fall through to 46620 for shared config.
			//Other than the bondout for this IOB the devices are identical.

		case GREENPAK4_SLG46620:

			//FIXME: Disable ADC block (until we have the logic for that implemented)
			bitstream[486] = true;
			bitstream[487] = true;
			bitstream[488] = true;
			bitstream[489] = true;
			bitstream[490] = true;
			bitstream[491] = true;

			//Force ADC block speed to 100 kHz (all other speeds not supported according to GreenPAK Designer)
			//TODO: Do this in the ADC class once that exists
			bitstream[838] = 0;
			bitstream[839] = 1;

			//Vref fine tune, magic value from datasheet (TODO do calibration?)
			//Seems to have been removed from most recent datasheet
			/*
			bitstream[891] = true;
			bitstream[890] = false;
			bitstream[889] = false;
			bitstream[888] = true;
			bitstream[887] = false;
			*/
			bitstream[891] = false;
			bitstream[890] = false;
			bitstream[889] = false;
			bitstream[888] = false;
			bitstream[887] = false;

			//I/O precharge
			bitstream[940] = m_ioPrecharge;

			//Device ID; immutable on the device but added to aid verification
			//5A: more data to follow
			bitstream[1016] = false;
			bitstream[1017] = true;
			bitstream[1018] = false;
			bitstream[1019] = true;
			bitstream[1020] = true;
			bitstream[1021] = false;
			bitstream[1022] = true;
			bitstream[1023] = false;

			if(m_nvmLoadRetryCount != 1)
				LogWarning("NVM retry count values other than 1 are not currently supported for SLG4662x\n");

			//Internal LDO disable
			bitstream[2008] = m_ldoBypass;

			//Charge pump disable
			bitstream[2010] = m_disableChargePump;

			//User ID of the bitstream
			bitstream[2031] = (userid & 0x01) ? true : false;
			bitstream[2032] = (userid & 0x02) ? true : false;
			bitstream[2033] = (userid & 0x04) ? true : false;
			bitstream[2034] = (userid & 0x08) ? true : false;
			bitstream[2035] = (userid & 0x10) ? true : false;
			bitstream[2036] = (userid & 0x20) ? true : false;
			bitstream[2037] = (userid & 0x40) ? true : false;
			bitstream[2038] = (userid & 0x80) ? true : false;

			//Read protection flag
			bitstream[2039] = readProtect;

			//A5: end of bitstream
			bitstream[2040] = true;
			bitstream[2041] = false;
			bitstream[2042] = true;
			bitstream[2043] = false;
			bitstream[2044] = false;
			bitstream[2045] = true;
			bitstream[2046] = false;
			bitstream[2047] = true;

			break;

		case GREENPAK4_SLG46140:

			//FIXME: Disable ADC block (until we have the logic for that implemented)
			bitstream[378] = true;
			bitstream[379] = true;
			bitstream[380] = true;
			bitstream[381] = true;
			bitstream[383] = true;
			bitstream[383] = true;

			//Force ADC block speed to 100 kHz (all other speeds not supported according to GreenPAK Designer)
			//Note that the SLG46140V datasheet r100 still lists the other speed values, but SLG46620 rev 100 does not.
			//Furthermore, GreenPAK Designer v6.02 complains about bitstreams generated using 2'b11.
			//It says "setting speed to 100 kHz" and writes to 2'b10 instead. One of these is wrong, need to ask Silego.
			//TODO: Do this in the ADC class once that exists
			bitstream[542] = 1;
			bitstream[543] = 1;

			//Vref fine tune, magic value from datasheet (TODO do calibration?)
			//Seems to have been removed from most recent datasheet, used rev 079 for this
			/*
			bitstream[495] = true;
			bitstream[494] = false;
			bitstream[493] = false;
			bitstream[492] = true;
			bitstream[491] = false;
			*/
			bitstream[495] = false;
			bitstream[494] = false;
			bitstream[493] = false;
			bitstream[492] = false;
			bitstream[491] = false;

			//I/O precharge
			bitstream[760] = m_ioPrecharge;

			//NVM boot retry
			switch(m_nvmLoadRetryCount)
			{
				case 1:
					bitstream[995] = false;
					bitstream[994] = false;
					break;

				case 2:
					bitstream[995] = false;
					bitstream[994] = true;
					break;

				case 3:
					bitstream[995] = true;
					bitstream[994] = false;
					break;

				case 4:
					bitstream[995] = true;
					bitstream[994] = true;
					break;

				default:
					LogError("NVM retry count for SLG46140 must be 1...4\n");
					ok = false;
			}

			//Internal LDO disable
			bitstream[1003] = m_ldoBypass;

			//Charge pump disable
			bitstream[1005] = m_disableChargePump;

			//User ID of the bitstream
			bitstream[1007] = (userid & 0x01) ? true : false;
			bitstream[1008] = (userid & 0x02) ? true : false;
			bitstream[1009] = (userid & 0x04) ? true : false;
			bitstream[1010] = (userid & 0x08) ? true : false;
			bitstream[1011] = (userid & 0x10) ? true : false;
			bitstream[1012] = (userid & 0x20) ? true : false;
			bitstream[1013] = (userid & 0x40) ? true : false;
			bitstream[1014] = (userid & 0x80) ? true : false;

			//Device ID; immutable on the device but added to aid verification
			//A5: end of bitstream
			bitstream[1016] = true;
			bitstream[1017] = false;
			bitstream[1018] = true;
			bitstream[1019] = false;
			bitstream[1020] = false;
			bitstream[1021] = true;
			bitstream[1022] = false;
			bitstream[1023] = true;

			break;

		//Invalid device
		default:
			LogError("Greenpak4Device: WriteToFile(): unknown device\n");
			ok = false;
	}

	return ok;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Timing analysis

void Greenpak4Device::PrintTimingData() const
{
	for(auto b : m_bitstuff)
		b->PrintTimingData();
}

void Greenpak4Device::SaveTimingData(string fname)
{
	FILE* fp = fopen(fname.c_str(), "w");
	if(!fp)
	{
		LogError("Couldn't open timing data file %s\n", fname.c_str());
		return;
	}

	//Header
	fprintf(fp, "{\n");
	fprintf(fp, "    \"part\" : \"%s\",\n", GetPartAsString().c_str());

	//Timing data for each IP block
	for(size_t i=0; i<m_bitstuff.size(); i++)
		m_bitstuff[i]->SaveTimingData(fp, (i+1) == m_bitstuff.size());

	//Footer
	fprintf(fp, "}\n");
	fclose(fp);
}

bool Greenpak4Device::LoadTimingData(string fname)
{
	//Read it
	FILE* fp = fopen(fname.c_str(), "rb");
	if(fp == NULL)
		return false;
	if(0 != fseek(fp, 0, SEEK_END))
	{
		LogError("Failed to seek to end of timing data file %s\n", fname.c_str());
		fclose(fp);
		return false;
	}
	size_t len = ftell(fp);
	if(0 != fseek(fp, 0, SEEK_SET))
	{
		LogError("Failed to seek to start of timing data file %s\n", fname.c_str());
		fclose(fp);
		return false;
	}
	char* json_string = new char[len + 1];
	json_string[len] = '\0';
	if(len != fread(json_string, 1, len, fp))
	{
		LogError("Failed to read contents of timing data file %s\n", fname.c_str());
		delete[] json_string;
		fclose(fp);
		return false;
	}
	fclose(fp);

	//Parse the JSON
	json_tokener* tok = json_tokener_new();
	if(!tok)
	{
		LogError("Failed to create JSON tokenizer object\n");
		delete[] json_string;
		return false;
	}
	json_tokener_error err;
	json_object* object = json_tokener_parse_verbose(json_string, &err);
	if(NULL == object)
	{
		const char* desc = json_tokener_error_desc(err);
		LogError("JSON parsing failed (err = %s)\n", desc);
		delete[] json_string;
		return false;
	}

	//Load stuff
	if(!LoadTimingData(object))
		return false;

	//Done
	json_object_put(object);
	json_tokener_free(tok);
	return true;
}

bool Greenpak4Device::LoadTimingData(json_object* object)
{
	//Make a map of description -> entity
	map<string, Greenpak4BitstreamEntity*> bmap;
	for(auto p : m_bitstuff)
		bmap[p->GetDescription()] = p;

	json_object_iterator end = json_object_iter_end(object);
	for(json_object_iterator it = json_object_iter_begin(object);
		!json_object_iter_equal(&it, &end);
		json_object_iter_next(&it))
	{
		//See what we got
		string name = json_object_iter_peek_name(&it);
		json_object* child = json_object_iter_peek_value(&it);

		//Part should be first key. If it doesn't match our part, complain
		if(name == "part")
		{
			if(!json_object_is_type(child, json_type_string))
			{
				LogError("timing data part should be of type string but isn't\n");
				return false;
			}
			string expected_part = json_object_get_string(child);

			if(GetPartAsString() != expected_part)
			{
				LogError("Timing data is for part %s but we're a %s\n",
					expected_part.c_str(),
					GetPartAsString().c_str());
				return false;
			}
			continue;
		}

		//Anything else should be an IP block
		//If it doesn't exist, complain
		if(bmap.find(name) == bmap.end())
		{
			LogError("Site \"%s\" is in timing data file, but not this device\n",
				name.c_str());
			return false;
		}

		//Child should be an array of process corner, info tuples
		if(!json_object_is_type(child, json_type_array))
		{
			LogError("ip block should be of type array but isn't\n");
			return false;
		}

		//It exists, load it
		if(!bmap[name]->LoadTimingData(child))
			return false;
	}

	m_hasTimingData = true;

	return true;
}
