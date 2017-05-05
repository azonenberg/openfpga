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
	@brief Declaration of FCCoolRunnerIIDevice
 */
#ifndef FCCoolRunnerIIDevice_h
#define FCCoolRunnerIIDevice_h

#include <vector>

#include "FCDevice.h"
#include "FCCoolRunnerIIFunctionBlock.h"
#include "FCCoolRunnerIIProductTerm.h"
#include "FCCoolRunnerIINetlist.h"
#include "FCCoolRunnerIIZIAEntry.h"

/**
	@brief A generic CoolRunner-II CPLD device
	
	A CR-II device contains an even number of function blocks (from 2 to 32) and some other stuff (TODO)
 */
class FCCoolRunnerIIDevice : public FCDevice
{
public:
	FCCoolRunnerIIDevice(std::string name);
	virtual ~FCCoolRunnerIIDevice();
	
	virtual void LoadFromBitstream(std::string fname);
	virtual void SaveToBitstream(std::string fname);
	virtual void SaveToBitstream(CPLDSerializer* writer);

	virtual int GetTotalPinCount();
	
	size_t GetBlockCount()
	{ return m_fbs.size(); }
	
	FCCoolRunnerIIFunctionBlock* GetFunctionBlock(size_t i)
	{ return m_fbs[i]; }
	
	FCCoolRunnerIINetSource* DecodeZIAEntry(int row, std::string muxsel);
	
	void Fit(FCCoolRunnerIINetlist* netlist);

	const std::vector<FCCoolRunnerIIZIAEntry>& GetZIAEntriesForNetSource(FCCoolRunnerIINetSource* net);

protected:
	void FitMacrocells(FCCoolRunnerIINetlist* netlist);
	
	/*
		ZIA stuff
		
			Map of FCCoolRunnerIINetSource*'s to std::vector<int> indicating legal rows
			Each function block has an array of 40 FCCoolRunnerIINetSource*s 
	 */

protected:	
	//Model creation
	void CreateModelXC2C32A();
	void CreateModelXC2C64A();
	void CreateModelXC2C128();
	void CreateModelXC2C256();
	void CreateModelXC2C384();
	void CreateModelXC2C512();
	
	//Initialization helpers
	void AddIOBSToBank(FCCoolRunnerIIFunctionBlock* fb, FCIOBank* bank);
	
public:
	//Print a human-readable representation to stdout
	virtual void Dump();
	
	//Print a synthesizeable RTL representation to stdout
	virtual void DumpRTL();
	
protected:
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Device properties which aren't directly included in the bitstream
	
	///@brief Number of fuses
	size_t m_fusecount;
	
	///@brief Speed grade of the device
	int m_speedgrade;
	
	///@brief Human-readable device name
	std::string m_devname;
	
	///@brief Package ID (from XilinxCoolRunnerIIDevice::packages enum)
	int m_package;
	
	///@brief Array of function blocks
	std::vector<FCCoolRunnerIIFunctionBlock*> m_fbs;
	
	///@brief Name of bitfile we were loaded from
	std::string m_bitfile;
		
protected:	
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Global config settings (included verbatim in bitstream)

	///Global clock (exact bit meaning unknown)
	int m_gckMux;
	
	///Global set/reset (exact bit meaning unknown)
	int m_gsrMux;
	
	///Global output enable (exact bit meaning unknown)
	int m_goeMux;
	
	///Global termination. Toggles between keeper and pullup mode.
	enum global_term_modes
	{
		GLOBAL_TERM_KEEPER = 0,
		GLOBAL_TERM_PULLUP = 1
	};
	bool m_gterm;
	
	///"Dedicated input mode". Set to 1 to terminate unused pins, 0 to float or CGND
	///Maybe this is the mode of the input-only pin???
	int m_dedicatedInputMode;
	
	///Map indicating legal ZIA connections
	std::map<FCCoolRunnerIINetSource*, std::vector<FCCoolRunnerIIZIAEntry> > m_legalZIAConnections;
	
	///Converse of m_legalZIAConnections
	std::map< std::pair<int, std::string> , FCCoolRunnerIINetSource*> m_ziaEntryReverseMap;
};

#endif
