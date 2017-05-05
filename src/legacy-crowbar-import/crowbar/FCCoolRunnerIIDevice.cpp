/***********************************************************************************************************************
*                                                                                                                      *
* ANTIKERNEL v0.1                                                                                                      *
*                                                                                                                      *
* Copyright (c) 2012-2016 Andrew D. Zonenberg                                                                          *
* All rights reserved.                                                                                                 *
*                                                                                                                      *
* Redistribution and use in source and binary forms, with or without modification, are permitted provided that the     *
* following conditions are met:                                                                                        *
*                                                                                                                      *
*    * Redistributions of source code must retain the above copyright notice, this list of conditions, and the         *
*      following disclaimer.                                                                                           *
*                                                                                                                      *
*    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the       *
*      following disclaimer in the documentation and/or other materials provided with the distribution.                *
*                                                                                                                      *
*    * Neither the name of the author nor the names of any contributors may be used to endorse or promote products     *
*      derived from this software without specific prior written permission.                                           *
*                                                                                                                      *
* THIS SOFTWARE IS PROVIDED BY THE AUTHORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED   *
* TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL *
* THE AUTHORS BE HELD LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES        *
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR       *
* BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT *
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE       *
* POSSIBILITY OF SUCH DAMAGE.                                                                                          *
*                                                                                                                      *
***********************************************************************************************************************/

/**
	@file
	@author Andrew D. Zonenberg
	@brief Implementation of CoolRunner-II device class
 */

#include "crowbar.h"
#include "../jtaghal/XilinxCoolRunnerIIDevice.h"
#include "../jtaghal/CPLDBitstream.h"
#include "../jtaghal/JEDFileWriter.h"
#include "FCCoolRunnerIIDevice.h"
#include "FCCoolRunnerIIIOBank.h"
#include "FCCoolRunnerIIMacrocell.h"

using namespace std;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Construction / destruction
FCCoolRunnerIIDevice::FCCoolRunnerIIDevice(std::string name)
	: m_devname(name)
{	
	//Parse the device name
	int density=0;
	char package[16]="";
	if(3 != sscanf(name.c_str(), "xc2c%3d%*[a-]%1d%*[-]%15s", &density, &m_speedgrade, package))
	{
		throw JtagExceptionWrapper(
			"Invalid CR-II device name (does not match a known part)",
			"",
			JtagException::EXCEPTION_TYPE_GIGO);
	}
	string spackage(package);
	
	//Look up package name
	if(spackage == "qf32" || spackage == "qfg32")
		m_package = XilinxCoolRunnerIIDevice::QFG32;
	else if(spackage == "vq44" || spackage == "vqg44")
		m_package = XilinxCoolRunnerIIDevice::VQG44; 
	else if(spackage == "cp56" || spackage == "cpg56")
		m_package = XilinxCoolRunnerIIDevice::CPG56; 
	else
	{
		throw JtagExceptionWrapper(
			"Invalid CR-II device name (does not match a known part)",
			"",
			JtagException::EXCEPTION_TYPE_GIGO);
	}
	
	//TODO: support non-A xc2c32 and xc2c64 (?)
	
	//Create the device
	switch(density)
	{
	case 32:
		CreateModelXC2C32A();
		break;
	case 64:
		CreateModelXC2C64A();
		break;
	case 128:
		CreateModelXC2C128();
		break;
	case 256:
		CreateModelXC2C256();
		break;
	case 384:
		CreateModelXC2C384();
		break;
	case 512:
		CreateModelXC2C512();
		break;
	default:
		throw JtagExceptionWrapper(
			"Invalid CR-II device density (does not match a known part)",
			"",
			JtagException::EXCEPTION_TYPE_GIGO);
	}
}

FCCoolRunnerIIDevice::~FCCoolRunnerIIDevice()
{
	for(size_t i=0; i<m_fbs.size(); i++)
		delete m_fbs[i];
	m_fbs.clear();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Model creation

/** 
	@brief Creates a model for the XC2C32A.
	
	The XC2C32 is very similar but has four less fuses and only one I/O bank. It is not supported at the moment.
	
	The device is initialized to the default (idle) state, which does not necessarily correspond to the erased state.
	
	For example, the idle state of unused pins is pull-down but the idle state in an erased chip is pull-up.
 */
void FCCoolRunnerIIDevice::CreateModelXC2C32A()
{
	m_gckMux = 0;
	m_gsrMux = 0;
	m_goeMux = 255;
	m_gterm = GLOBAL_TERM_KEEPER;
	m_dedicatedInputMode = 1;
	
	//Total number of fuses
	m_fusecount = 12278;
	
	const int fb_count = 2;
	const int zia_width = 8;
	
	//Create the function blocks and add the IOBs
	for(int i=0; i<fb_count; i++)
	{
		m_fbs.push_back(new FCCoolRunnerIIFunctionBlock(
			this,
			i+1,	//Block number, one based as per datasheet
			zia_width	//Number of mux bits in ZIA per output
			));
			
		for(int j=0; j<FCCoolRunnerIIFunctionBlock::MACROCELLS_PER_FB; j++)
			m_iobs.push_back(&m_fbs[i]->GetMacrocell(j)->m_iob);
	}
	
	//Create the I/O banks
	//Banks are named with monotonically increasing numbers
	for(int i=0; i<2; i++)
		m_banks.push_back(new FCCoolRunnerIIIOBank(i+1));
		
	//The I/O buffers already exist (part of the function blocks)
	//but we need to assign them to pins and banks
	//First, put them all in the banks
	//FB1 gets bank 2 and vice versa (see DS310 page 8)
	AddIOBSToBank(m_fbs[0], m_banks[1]);
	AddIOBSToBank(m_fbs[1], m_banks[0]);
	
	//Table of IOB-to-pin mappings (see DS310 page 8)
	const char* pin_names_qfg32[fb_count][FCCoolRunnerIIFunctionBlock::MACROCELLS_PER_FB]=
	{
		//FB 1
		{ "",   "",   "",   "P3",  "P2",  "P1",  "P32", "P31", "P30", "P29", "P28", "P24", "",    "P23",  "",    ""    },
		
		//FB 2
		{ "P5",  "",   "",   "",   "P6",  "P7",  "P8",  "P9",  "P10", "",   "",   "P13", "P17",  "P18",  "P19",  ""    }
	};
	const char* pin_names_vqg44[fb_count][FCCoolRunnerIIFunctionBlock::MACROCELLS_PER_FB]=
	{
		//FB 1
		{ "P38", "P37", "P36", "P34", "P33", "P32", "P31", "P30", "P29", "P28", "P27", "P23", "P22",  "P21",  "P20",  "P19"  },
		
		//FB 2
		{ "P39", "P40", "P41", "P42", "P43", "P44", "P1",  "P2",  "P3",  "P5",  "P6",  "P8",  "P12",  "P13",  "P14",  "P16"  }
	};
	const char* pin_names_cpg56[fb_count][FCCoolRunnerIIFunctionBlock::MACROCELLS_PER_FB]=
	{
		//FB 1
		{ "F1", "E3", "E1", "D1", "C1", "A3", "A2", "B1", "A1", "C4", "C5", "C8", "A10", "B10", "C10", "E8"  },
		
		//FB 2
		{ "G1", "F3", "H1", "G3", "J1", "K1", "K2", "K3", "H3", "K5", "H5", "H8", "K8",  "H10", "G10", "F10" }
	};
	
	//Assign IOBs to pins
	for(int i=0; i<fb_count; i++)
	{
		for(int j=0; j<FCCoolRunnerIIFunctionBlock::MACROCELLS_PER_FB; j++)
		{
			switch(m_package)
			{
			case XilinxCoolRunnerIIDevice::QFG32:
				m_fbs[i]->GetMacrocell(j)->m_iob.SetPin(pin_names_qfg32[i][j]);
				break;
			case XilinxCoolRunnerIIDevice::VQG44:
				m_fbs[i]->GetMacrocell(j)->m_iob.SetPin(pin_names_vqg44[i][j]);
				break;
			case XilinxCoolRunnerIIDevice::CPG56:
				m_fbs[i]->GetMacrocell(j)->m_iob.SetPin(pin_names_cpg56[i][j]);
				break;
			default:
				throw JtagExceptionWrapper(
					"Invalid package for XC2C32A (must be one of QF[G]32, VQ[G]44, CP[G]56)",
					"",
					JtagException::EXCEPTION_TYPE_GIGO);
				break;
			}
		}
	}
	
	//ZIA configuration for IOBs
	//Reversed by experiment
	//https://siliconpr0n.org/archive/doku.php?id=fc:xc2c#actual_zia_bits_for_xc2c32a
	struct
	{
		int fb;
		int mc;
		int row;
		const char* value;
	} zia_iob_config[]=
	{
		{  0,  0,  0, "81" },	{  0,  0, 11, "81" },	{  0,  0, 22, "81" },	{  0,  0, 33, "81" },
		{  0,  1,  1, "81" },	{  0,  1, 12, "81" },	{  0,  1, 23, "81" },	{  0,  1, 34, "81" },
		{  0,  2,  2, "81" },	{  0,  2, 13, "81" },	{  0,  2, 24, "81" },	{  0,  2, 35, "81" },
		{  0,  3,  3, "81" },	{  0,  3, 14, "81" },	{  0,  3, 25, "81" },	{  0,  3, 36, "81" },
		{  0,  4,  4, "81" },	{  0,  4, 15, "81" },	{  0,  4, 26, "81" },	{  0,  4, 37, "81" },
		{  0,  5,  5, "81" },	{  0,  5, 16, "81" },	{  0,  5, 27, "81" },	{  0,  5, 38, "81" },
		{  0,  6,  6, "81" },	{  0,  6, 17, "81" },	{  0,  6, 28, "81" },	{  0,  6, 39, "81" },
		{  0,  7,  7, "81" },	{  0,  7, 18, "81" },	{  0,  7, 29, "81" },	{  0,  7, 10, "81" },
		{  0,  8,  8, "81" },	{  0,  8, 19, "81" },	{  0,  8, 30, "81" },	{  0,  8, 21, "81" },
		{  0,  9,  9, "81" },	{  0,  9, 20, "81" },	{  0,  9, 31, "81" },	{  0,  9, 32, "81" },
		{  0, 10,  0, "82" },	{  0, 10, 17, "82" },	{  0, 10, 29, "82" },
		{  0, 11,  1, "82" },	{  0, 11, 28, "82" },	{  0, 11, 37, "82" },	{  0, 11, 11, "82" },
		{  0, 12,  2, "82" },	{  0, 12, 12, "82" },	{  0, 12, 22, "82" },
		{  0, 13,  3, "82" },	{  0, 13, 33, "82" },	{  0, 13, 20, "82" },	{  0, 13, 24, "82" },
		{  0, 14,  4, "82" },	{  0, 14, 36, "82" },	{  0, 14, 32, "82" },	{  0, 14, 19, "82" },
		{  0, 15,  5, "82" },	{  0, 15, 34, "82" },	{  0, 15, 31, "82" },	{  0, 15, 14, "82" },
		
		{  1,  0,  7, "82" },	{  1,  0, 15, "82" },	{  1,  0, 30, "82" },	{  1,  0, 38, "82" },
		{  1,  1,  8, "82" },	{  1,  1, 21, "82" },	{  1,  1, 27, "82" },
		{  1,  2,  9, "82" },	{  1,  2, 13, "82" },	{  1,  2, 23, "82" },	{  1,  2, 39, "82" },
		{  1,  3, 10, "82" },	{  1,  3, 16, "82" },	{  1,  3, 25, "82" },	{  1,  3, 35, "82" },
		{  1,  4,  6, "84" },	{  1,  4, 18, "84" },	{  1,  4, 30, "84" },
		{  1,  5,  0, "84" },	{  1,  5, 17, "84" },	{  1,  5, 26, "84" },	{  1,  5, 36, "84" },
		{  1,  6,  1, "84" },	{  1,  6, 11, "84" },	{  1,  6, 29, "84" },	{  1,  6, 38, "84" },
		{  1,  7,  9, "84" },	{  1,  7, 13, "84" },	{  1,  7, 22, "84" },
		{  1,  8,  8, "84" },	{  1,  8, 21, "84" },	{  1,  8, 25, "84" },	{  1,  8, 33, "84" },
		{  1,  9,  3, "84" },	{  1,  9, 20, "84" },	{  1,  9, 23, "84" },	{  1,  9, 37, "84" },
		{  1, 10,  7, "84" },	{  1, 10, 15, "84" },	{  1, 10, 32, "84" },	{  1, 10, 35, "84" },
		{  1, 11,  4, "84" },	{  1, 11, 19, "84" },	{  1, 11, 27, "84" },	{  1, 11, 34, "84" },
		{  1, 12, 10, "84" },	{  1, 12, 16, "84" },	{  1, 12, 31, "84" },	{  1, 12, 39, "84" },
		{  1, 13,  2, "84" },	{  1, 13, 12, "84" },	{  1, 13, 28, "84" },
		{  1, 14,  5, "84" },	{  1, 14, 14, "84" },	{  1, 14, 24, "84" },
		{  1, 15,  7, "88" },	{  1, 15, 19, "88" },	{  1, 15, 31, "88" }
	};
	
	for(size_t i=0; i<sizeof(zia_iob_config)/sizeof(zia_iob_config[0]); i++)
	{
		auto iob = zia_iob_config[i];
		m_legalZIAConnections[&m_fbs[iob.fb]->GetMacrocell(iob.mc)->m_iob].push_back(
			FCCoolRunnerIIZIAEntry(iob.row, iob.value, zia_width));
	}
	
	//TODO: Add input-only pin
	
	//ZIA configuration for macrocell flipflops
	//Reversed by experiment
	struct
	{
		int fb;
		int mc;
		int row;
		const char* value;
	} zia_ff_config[] =
	{
		{  0,  0,  6, "88" },	{  0,  0, 15, "88" },	{  0,  0, 25, "88" },
		{  0,  1,  0, "88" },	{  0,  1, 18, "88" },	{  0,  1, 27, "88" },	{  0,  1, 37, "88" },
		{  0,  2,  2, "88" },	{  0,  2, 11, "88" },	{  0,  2, 30, "88" },	{  0,  2, 39, "88" },
		{  0,  3, 10, "88" },	{  0,  3, 14, "88" },	{  0,  3, 22, "88" },
		{  0,  4,  9, "88" },	{  0,  4, 12, "88" },	{  0,  4, 26, "88" },	{  0,  4, 33, "88" },
		{  0,  5,  4, "88" },	{  0,  5, 21, "88" },	{  0,  5, 24, "88" },	{  0,  5, 38, "88" },
		{  0,  6,  8, "88" },	{  0,  6, 16, "88" },	{  0,  6, 23, "88" },	{  0,  6, 36, "88" },
		{  0,  7,  5, "88" },	{  0,  7, 20, "88" },	{  0,  7, 28, "88" },	{  0,  7, 35, "88" },
		{  0,  8,  1, "88" },	{  0,  8, 17, "88" },	{  0,  8, 32, "88" },
		{  0,  9,  3, "88" },	{  0,  9, 13, "88" },	{  0,  9, 29, "88" },	{  0,  9, 34, "88" },
		{  0, 10,  8, "90" },	{  0, 10, 20, "90" },	{  0, 10, 32, "90" },	{  0, 10, 34, "90" },
		{  0, 11,  4, "90" },	{  0, 11, 14, "90" },	{  0, 11, 30, "90" },	{  0, 11, 35, "90" },
		{  0, 12,  7, "90" },	{  0, 12, 16, "90" },	{  0, 12, 26, "90" },
		{  0, 13,  0, "90" },	{  0, 13, 19, "90" },	{  0, 13, 28, "90" },	{  0, 13, 38, "90" },
		{  0, 14,  3, "90" },	{  0, 14, 11, "90" },	{  0, 14, 31, "90" },
		{  0, 15,  1, "90" },	{  0, 15, 15, "90" },	{  0, 15, 22, "90" },
		
		{  1,  0, 10, "90" },	{  1,  0, 13, "90" },	{  1,  0, 27, "90" },	{  1,  0, 33, "90" },
		{  1,  1,  5, "90" },	{  1,  1, 12, "90" },	{  1,  1, 25, "90" },	{  1,  1, 39, "90" },
		{  1,  2,  9, "90" },	{  1,  2, 17, "90" },	{  1,  2, 24, "90" },	{  1,  2, 37, "90" },
		{  1,  3,  6, "90" },	{  1,  3, 21, "90" },	{  1,  3, 29, "90" },	{  1,  3, 36, "90" },
		{  1,  4,  2, "90" },	{  1,  4, 18, "90" },	{  1,  4, 23, "90" },
		{  1,  5,  9, "a0" },	//unknown
		{  1,  6,  3, "a0" },	//unknown
		{  1,  7,  5, "a0" },	//unknown
		{  1,  8,  8, "a0" },
		{  1,  9,  0, "a0" },
		{  1, 10,  4, "a0" },
		{  1, 11,  2, "a0" },
		{  1, 12,  1, "a0" },
		{  1, 13,  6, "a0" },
		{  1, 14, 10, "a0" },
		{  1, 15,  7, "a0" }
	};
	
	for(size_t i=0; i<sizeof(zia_ff_config)/sizeof(zia_ff_config[0]); i++)
	{
		auto ff = zia_ff_config[i];
		m_legalZIAConnections[m_fbs[ff.fb]->GetMacrocell(ff.mc)].push_back(
			FCCoolRunnerIIZIAEntry(ff.row, ff.value, zia_width));
	}
	
	//Generate reverse ZIA mapping
	//std::map< std::pair<int, std::string> >, FCCoolRunnerIINetSource*> m_ziaEntryReverseMap;
	//std::map<FCCoolRunnerIINetSource*, std::vector<FCCoolRunnerIIZIAEntry> > m_legalZIAConnections;
	for(auto it = m_legalZIAConnections.begin(); it != m_legalZIAConnections.end(); it++)
	{
		FCCoolRunnerIINetSource* net = it->first;
		vector<FCCoolRunnerIIZIAEntry>& entries = it->second;
		for(size_t i=0; i<entries.size(); i++)
		{
			FCCoolRunnerIIZIAEntry& entry = entries[i];
			m_ziaEntryReverseMap[ pair<int, string>(entry.m_row, entry.m_muxsel_str) ] = net;
		}
	}
}

void FCCoolRunnerIIDevice::CreateModelXC2C64A()
{
	throw JtagExceptionWrapper(
		"Support for this device is not implemented",
		"",
		JtagException::EXCEPTION_TYPE_UNIMPLEMENTED);
}

void FCCoolRunnerIIDevice::CreateModelXC2C128()
{
	throw JtagExceptionWrapper(
		"Support for this device is not implemented",
		"",
		JtagException::EXCEPTION_TYPE_UNIMPLEMENTED);
}

void FCCoolRunnerIIDevice::CreateModelXC2C256()
{
	throw JtagExceptionWrapper(
		"Support for this device is not implemented",
		"",
		JtagException::EXCEPTION_TYPE_UNIMPLEMENTED);
}

void FCCoolRunnerIIDevice::CreateModelXC2C384()
{
	throw JtagExceptionWrapper(
		"Support for this device is not implemented",
		"",
		JtagException::EXCEPTION_TYPE_UNIMPLEMENTED);
}

void FCCoolRunnerIIDevice::CreateModelXC2C512()
{
	throw JtagExceptionWrapper(
		"Support for this device is not implemented",
		"",
		JtagException::EXCEPTION_TYPE_UNIMPLEMENTED);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Initialization helpers

/**
	@brief Adds all of the IOBs in a function block to an I/O bank.
	
	The CR-II architecture appears to require that all IOBs associated with a single FB be in the same bank.
 */
void FCCoolRunnerIIDevice::AddIOBSToBank(FCCoolRunnerIIFunctionBlock* fb, FCIOBank* bank)
{
	for(int i=0; i<FCCoolRunnerIIFunctionBlock::MACROCELLS_PER_FB; i++)
		bank->AddIOB(&fb->GetMacrocell(i)->m_iob);
}

/**
	@brief Returns the net source for a given ZIA entry
 */
FCCoolRunnerIINetSource* FCCoolRunnerIIDevice::DecodeZIAEntry(int row, string muxsel)
{
	auto x = pair<int, string>(row,muxsel);
	if(m_ziaEntryReverseMap.find(x) == m_ziaEntryReverseMap.end())
	{
		asm("int3");
		printf("row %d muxsel %s\n", row, muxsel.c_str());
		throw JtagExceptionWrapper(
			"Invalid ZIA entry, no decode possible",
			"",
			JtagException::EXCEPTION_TYPE_GIGO);
	}
	return m_ziaEntryReverseMap[x];
}

const vector<FCCoolRunnerIIZIAEntry>& FCCoolRunnerIIDevice::GetZIAEntriesForNetSource(FCCoolRunnerIINetSource* net)
{
	return m_legalZIAConnections[net];
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Debug output

void FCCoolRunnerIIDevice::Dump()
{
	printf("Dumping device  : %6s\n", m_devname.c_str());
	printf("    Fuse count  : %6d\n", (int)m_fusecount);
	printf("    Speed grade : %6d\n", m_speedgrade);
	printf("    Package     : %6s\n", XilinxCoolRunnerIIDevice::GetPackageName(m_package).c_str());
	printf("    I/O pins    : %6d\n", (int)m_iobs.size());
	printf("    Macrocells  : %6d\n", (int)m_fbs.size() * FCCoolRunnerIIFunctionBlock::MACROCELLS_PER_FB);
	printf("    Blocks      : %6d\n", (int)m_fbs.size());
	
	//Dump the function blocks
	for(size_t i=0; i<m_fbs.size(); i++)
		m_fbs[i]->Dump();
		
	//Dump the I/O banks
	m_banks[0]->DumpHeader();
	for(size_t i=0; i<m_banks.size(); i++)
		m_banks[i]->Dump();
		
	//Global settings
	printf("\n");
	printf("    Global clock mux     : %d\n", m_gckMux);
	printf("    Global set/reset mux : %d\n", m_gsrMux);
	printf("    Global output en mux : %d\n", m_goeMux);
	printf("    Global termination   : %s\n", (m_gterm == GLOBAL_TERM_KEEPER) ? "KEEPER" : "PULLUP");
	printf("    Dedicated input mode : %d\n", m_dedicatedInputMode);
}

void FCCoolRunnerIIDevice::DumpRTL()
{
	//Print header
	printf("\n\n");
	printf("//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////\n");
	if(m_bitfile != "")
		printf("// Generated from: %-100s //\n", m_bitfile.c_str());
	printf("// Device:          %15s                                                                                     //\n", m_devname.c_str());
	printf("// Fuse count:      %15d                                                                                     //\n", (int)m_fusecount);
	printf("// Speed grade:     %15d                                                                                     //\n", m_speedgrade);
	printf("// Package:         %15s                                                                                     //\n", XilinxCoolRunnerIIDevice::GetPackageName(m_package).c_str());
	printf("// I/O pads:        %15d (incl. unbonded)                                                                    //\n", (int)m_iobs.size());
	printf("// Function blocks: %15d                                                                                     //\n", (int)m_fbs.size());
	printf("// Macrocells:      %15d                                                                                     //\n", (int)m_fbs.size() * FCCoolRunnerIIFunctionBlock::MACROCELLS_PER_FB);
	printf("//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////\n");
	printf("\n");
	
	//Module header (TODO: figure out what I/O nets are used)
	printf("module BitstreamDump(\n");
	unordered_set<string> nets;
	for(size_t i=0; i<m_fbs.size(); i++)
		m_fbs[i]->GetTopLevelNetNames(nets);
	size_t netcount = 0;
	for(auto it=nets.begin(); it!=nets.end(); it++)
	{
		netcount++;
		if( netcount < nets.size() )
			printf("    %s,\n", it->c_str());
		else
			printf("    %s\n", it->c_str());
	}
	printf("    );\n");
	printf("\n");
	
	//Dump the IOB headers for each function block
	printf("    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////\n");	
	printf("    // I/O declarations\n");
	printf("\n");
	for(size_t i=0; i<m_fbs.size(); i++)
		m_fbs[i]->DumpIOBRTL();
	printf("    // Dedicated input pin not implemented\n");
	printf("\n");
	
	//Dump the function blocks
	for(size_t i=0; i<m_fbs.size(); i++)
		m_fbs[i]->DumpRTL();
	
	/*
	//Dump the I/O banks
	m_banks[0]->DumpHeader();
	for(size_t i=0; i<m_banks.size(); i++)
		m_banks[i]->Dump();
		
	//Global settings
	printf("\n");
	printf("    Global clock mux     : %d\n", m_gckMux);
	printf("    Global set/reset mux : %d\n", m_gsrMux);
	printf("    Global output en mux : %d\n", m_goeMux);
	printf("    Global termination   : %s\n", (m_gterm == GLOBAL_TERM_KEEPER) ? "KEEPER" : "PULLUP");
	printf("    Dedicated input mode : %d\n", m_dedicatedInputMode);
	*/
	
	printf("endmodule\n");
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Initialization

void FCCoolRunnerIIDevice::LoadFromBitstream(std::string fname)
{
	/*
		CR-II JED file structure
		
		Function blocks
			
		Global stuff
			Global clock mux
			GSR mux
			GOE mux
			Global termination
			Output standard
			Input standard
			Dedicated input mode
			For each bank
				vcci
				vcco
	 */
	
	//Create the dummy device
	//TODO: pass the correct device number / package etc
	XilinxCoolRunnerIIDevice device(
		XilinxCoolRunnerIIDevice::XC2C32A,
		XilinxCoolRunnerIIDevice::VQG44,
		0,		//stepping number
		0x6E18093,
		NULL,	//no JtagInterface, just for file loading
		0);
	ProgrammableDevice* pdev = static_cast<ProgrammableDevice*>(&device);	//why is this necessary??
		
	//Load the bitstream itself
	printf("Loading device model from bitstream...\n");
	CPLDBitstream* bit = static_cast<CPLDBitstream*>(pdev->LoadFirmwareImage(fname));
	bool* fuse_data = bit->fuse_data;
	int pos = 0;
	
	//Quick sanity check
	if(bit->fuse_count != m_fusecount)
	{
		throw JtagExceptionWrapper(
			"Fuse count in bitstream does not match our device name",
			"",
			JtagException::EXCEPTION_TYPE_GIGO);
	}
	
	//Load function blocks
	for(size_t i=0; i<m_fbs.size(); i++)
		pos = m_fbs[i]->LoadFromBitstream(fuse_data, pos);
	

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Everything below here is probably somewhat XC2C32A specific and needs to get reworked at some point.

	//printf("    Loading global settings, starting at fuse %d\n", pos);

	//Global clock mux
	m_gckMux = (fuse_data[pos] << 2) | (fuse_data[pos+1] << 1) | fuse_data[pos+2];
	pos += 3;
	
	//No clock divider in 32/64 macrocell devices
	//TODO: support this for larger devices
	
	//No clock delay in 32/64 macrocell devices
	//TODO: support this for larger devices
	
	//GSR mux
	m_gsrMux = (fuse_data[pos] << 1) | fuse_data[pos+1];
	pos += 2;
	
	//Global OE
	m_goeMux = 0;
	for(int i=0; i<8; i++)
		m_goeMux = (m_goeMux << 1) | fuse_data[pos++];
		
	//Global termination
	m_gterm = fuse_data[pos++];
	
	//No DataGate in 32/64 macrocell devices
	//TODO: support this for larger devices
	
	//Global input and output voltage standards.
	//Not used in -A series devices unless being loaded with a legacy device bitstream
	//TODO: >=128 macrocell devices just write directly to the per-bank I/O standard bit
	bool globalVCCO = fuse_data[pos++];
	bool globalVCCI = fuse_data[pos++];
	
	//"Dedicated input mode" - maybe related to global set/reset?
	//This is only present in 32/64 macrocell devices.
	m_dedicatedInputMode = (fuse_data[pos] << 1) | fuse_data[pos+1];
	pos += 2;
	
	//Load I/O bank IOSTANDARDs
	for(size_t i=0; i<m_banks.size(); i++)
	{
		//AND with legacy values to get the per-bank value
		bool vcci = fuse_data[pos++] & globalVCCI;
		bool vcco = fuse_data[pos++] & globalVCCO;
		static_cast<FCCoolRunnerIIIOBank*>(m_banks[i])->SetVoltages(vcci, vcco);
	}
	
	//No Vref enable in 32/64 macrocell devices
	//TODO: support this for larger devices
		 
	 //Clean up
	delete bit;
	
	m_bitfile = fname;
}

void FCCoolRunnerIIDevice::SaveToBitstream(std::string fname)
{
	printf("Saving device model to bitstream...\n");
	JEDFileWriter writer(fname);
	SaveToBitstream(&writer);
}

void FCCoolRunnerIIDevice::SaveToBitstream(CPLDSerializer* writer)
{
	//Write some header info
	writer->AddDefaultHeaderComment();
	writer->AddHeaderComment(string("Device: ") + m_devname + "\n\n");
	
	//Get ready to write the actual bitstream
	writer->BeginFuseData(m_fusecount, GetTotalPinCount());
	
	//Write Xilinx-specific version and device headers for compatibility with iMPACT etc
	writer->AddBodyComment("Version number");
	writer->AddBodyComment("VERSION libcrowbar");
	writer->AddBodyBlankLine();
	writer->AddBodyComment("Xilinx-specific device name");
	writer->AddBodyComment(string("DEVICE ") + strtoupper(m_devname));
	
	//Write the function blocks
	writer->AddBodyBlankLine();
	writer->AddBodyComment("===========================================================================");
	writer->AddBodyComment("=                             FUNCTION BLOCKS                             =");
	writer->AddBodyComment("===========================================================================");
	writer->AddBodyComment("Function block numbers are one-based as per datasheet.");
	writer->AddBodyComment("Note that that hprep6 uses zero-based indexing when commenting output.");
	for(size_t i=0; i<m_fbs.size(); i++)
		m_fbs[i]->SaveToBitstream(writer);
		
	//Write global mux data etc
	writer->AddBodyBlankLine();
	writer->AddBodyComment("===========================================================================");
	writer->AddBodyComment("=                              GLOBAL MUXES                               =");
	writer->AddBodyComment("===========================================================================");
	
	//Global clock mux
	bool gckMux[3] =
	{
		(m_gckMux & 4) ? true : false,
		(m_gckMux & 2) ? true : false,
		(m_gckMux & 1) ? true : false
	};
	writer->AddBodyBlankLine();
	writer->AddBodyComment("Global clock mux");
	writer->AddBodyFuseData(gckMux, 3);
	
	//Global set/reset mux
	bool gsrMux[2] =
	{
		(m_gsrMux & 2) ? true : false,
		(m_gsrMux & 1) ? true : false
	};
	writer->AddBodyBlankLine();
	writer->AddBodyComment("Global set/reset mux");
	writer->AddBodyFuseData(gsrMux, 2);
	
	//Global OE mux
	bool goeMux[8] =
	{
		(m_goeMux & 128) ? true : false,
		(m_goeMux & 64) ? true : false,
		(m_goeMux & 32) ? true : false,
		(m_goeMux & 16) ? true : false,
		(m_goeMux & 8) ? true : false,
		(m_goeMux & 4) ? true : false,
		(m_goeMux & 2) ? true : false,
		(m_goeMux & 1) ? true : false
	};
	writer->AddBodyBlankLine();
	writer->AddBodyComment("Global output enable mux");
	writer->AddBodyFuseData(goeMux, 8);
	
	//Global termination
	writer->AddBodyBlankLine();
	if(m_gterm == GLOBAL_TERM_KEEPER)
		writer->AddBodyComment("Global termination mode (KEEPER)");
	else
		writer->AddBodyComment("Global termination mode (PULLUP)");
	writer->AddBodyFuseData(&m_gterm, 1);
	
	//Global I/O standards
	//TODO: If >=128 macrocells, I/O bank standards go here
	writer->AddBodyBlankLine();
	writer->AddBodyComment("Global I/O voltage standards (legacy bits for XC2C32/64 compatibility, always 1)");
	bool ones[2] = {true, true};
	writer->AddBodyFuseData(ones, 2);
	
	//Dedicated input mode (no idea what this does)
	bool dimode[2] =
	{
		(m_dedicatedInputMode & 2) ? true : false,
		(m_dedicatedInputMode & 1) ? true : false
	};
	writer->AddBodyBlankLine();
	writer->AddBodyComment("Dedicated input mode");
	writer->AddBodyFuseData(dimode, 2);
	
	//I/O banks
	//TODO: if >=128 macrocells, skip this
	for(size_t i=0; i<m_banks.size(); i++)
		static_cast<FCCoolRunnerIIIOBank*>(m_banks[i])->SaveToBitstream(writer);
	
	//Done
	writer->EndFuseData();
}

/**
	@brief Gets the total pin count on the device (including power, ground, JTAG, and I/O)
 */
int FCCoolRunnerIIDevice::GetTotalPinCount()
{
	switch(m_package)
	{
	case XilinxCoolRunnerIIDevice::QFG32:
		return 32;
	case XilinxCoolRunnerIIDevice::VQG44:
		return 44;
	case XilinxCoolRunnerIIDevice::CPG56:
		return 56;
	default:
		throw JtagExceptionWrapper(
			"Invalid CR-II device name (does not match a known part)",
			"",
			JtagException::EXCEPTION_TYPE_GIGO);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Fitting

/**
	@brief Fits a mapped configuration to the device.
 */
void FCCoolRunnerIIDevice::Fit(FCCoolRunnerIINetlist* netlist)
{
	//TODO: Make sure old config was loaded or wiped to avoid inconsistent states?
	
	//Fit the I/O banks
	printf("IO bank fitting\n");
	for(size_t i=0; i<m_banks.size(); i++)
		static_cast<FCCoolRunnerIIIOBank*>(m_banks[i])->Fit();
		
	//Fit the macrocells
	printf("Macrocell fitting\n");
	FitMacrocells(netlist);
	
	//Fit the function blocks
	for(size_t i=0; i<m_fbs.size(); i++)
		m_fbs[i]->Fit();
}

void FCCoolRunnerIIDevice::FitMacrocells(FCCoolRunnerIINetlist* netlist)
{
	//Macrocell fitting
	printf("    Pass 1: I/O macrocells                 ");
	vector<FCCoolRunnerIIMacrocell*> available_macrocells;
	int placedmc = 0;
	for(size_t i=0; i<m_fbs.size(); i++)
	{
		FCCoolRunnerIIFunctionBlock* fb = m_fbs[i];
		for(size_t j=0; j<FCCoolRunnerIIFunctionBlock::MACROCELLS_PER_FB; j++)
		{
			FCCoolRunnerIIMacrocell* mc = fb->GetMacrocell(j);
			if(mc->m_netlistcell == NULL)
				available_macrocells.push_back(mc);
			else
				placedmc++;
		}
	}
	vector<FCCoolRunnerIINetlistMacrocell*> unplaced_macrocells;
	for(size_t i=0; i<netlist->m_macrocells.size(); i++)
	{
		FCCoolRunnerIINetlistMacrocell* mc = netlist->m_macrocells[i];
		if(mc->GetMacrocell() == NULL)
			unplaced_macrocells.push_back(mc);
	}
	int freemc = (int)available_macrocells.size();
	printf("%d placed, %d unplaced, %d available\n",
		placedmc, (int)unplaced_macrocells.size(), freemc);
	if(available_macrocells.size() < unplaced_macrocells.size())
	{
		throw JtagExceptionWrapper(
			"DRC error: More macrocells requested than available on device",
			"",
			JtagException::EXCEPTION_TYPE_GIGO);
	}
	printf("    Pass 2: Unplaced macrocells            ");
	for(size_t i=0; i<unplaced_macrocells.size(); i++)
	{
		//TODO: Assign unplaced macrocells to function blocks to maximize product term sharing etc
		//For now, do sequential assignment
		unplaced_macrocells[i]->SetMacrocell(available_macrocells[i]);
		placedmc++;
		freemc--;
	}
	printf("%d placed, 0 unplaced, %d available\n",
		placedmc, freemc);
	
	//Copy macrocell configuration
	for(size_t i=0; i<netlist->m_macrocells.size(); i++)
		netlist->m_macrocells[i]->WriteConfig();
}
