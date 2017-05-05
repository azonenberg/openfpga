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
	@brief Declaration of FCCoolRunnerIIFunctionBlock
 */

#ifndef FCCoolRunnerIIFunctionBlock_h
#define FCCoolRunnerIIFunctionBlock_h

#include "FCCoolRunnerIIMacrocell.h"
#include "FCCoolRunnerIIZIAEntry.h"
#include "FCCoolRunnerIIProductTerm.h"

class CPLDBitstream;
class CPLDSerializer;

#include <unordered_set>

class FCCoolRunnerIIDevice;

/**
	@brief A single function block within a CoolRunner-II CPLD.
	
	Each function block consists of the following:
	* 40 input signals
	* An 80x56 AND array
		Input is 40 original and 40 inverted inputs
		Output is 56 product terms
	* An 56x16 OR array
		Input is 56 product terms
		Output is 16 OR terms to macrocells
	* 16 macrocells
	
	Of the 56 p-terms, some are special:
		* 16 go to PTA inputs of each macrocell
		* 16 go to PTB inputs of each macrocell
		* 16 go to PTC inputs of each macrocell
		* 1 goes to CTC (control term clock = function block level clock)
		* 1 goes to CTR (control term reset = function block level FF reset)
		* 1 goes to CTS (control term set = function block level FF set)
		* 1 goes to CTE (control term enable = function block level OE)
	This is a total of 52 special p-terms. The other 3  seem to have no specific allocation (so far).
	
	The locations of these p-terms within the array are not yet known.
	
	All p-terms can, if not being used for special purposes, be used for general purpose logic.
	
	PTC can bypass the PLA OR term for a slight (300ps) speed boost if only one product term is used.
	
	Each function block has a "ZIA" attached to the input which appears to be the AIM. In the XC2C32A this is an
	8x40 block. The output is the 40 input signals going to the PLA. While the 8 input bits presumably control muxes of
	some sort it's not known exactly what's going on.
	
	Input signals to AIM (XC2C32A):
		32 function block outputs
		32 pin inputs
		
	This is 64 signals total and we have 8 bits.
 */
class FCCoolRunnerIIFunctionBlock
{
public:
	FCCoolRunnerIIFunctionBlock(FCCoolRunnerIIDevice* device, int blocknum, int zia_width);
	virtual ~FCCoolRunnerIIFunctionBlock();
	
	enum
	{
		MACROCELLS_PER_FB 	= 16,
		INPUT_TERM_COUNT	= 40,
		PLA_INPUT_WIDTH 	= INPUT_TERM_COUNT*2,
		PLA_PRODUCT_TERMS	= 56
	};
	
	int GetBlockNumber();
	FCCoolRunnerIIMacrocell* GetMacrocell(int i);
	
	virtual void Dump();
	virtual void DumpIOBRTL();
	virtual void DumpRTL();
	virtual void GetTopLevelNetNames(std::unordered_set<std::string>& names);
	
	int LoadFromBitstream(bool* bitstream, int start_address);
	virtual void SaveToBitstream(CPLDSerializer* writer);
	
	bool IsOrTermUsed(int macrocell);
	bool IsProductTermUsed(int pterm);
	bool IsMacrocellOutputUsed(int macrocell);
	
	void Fit();
	
	int GetZiaWidth()
	{ return m_ziaWidth; }
	
protected:
	FCCoolRunnerIIDevice* m_device;
	
	void GlobalRouting(std::map<FCCoolRunnerIINetSource*, FCCoolRunnerIIZIAEntry>& net_assignments);
	void FitProductTerms(std::map<FCCoolRunnerIIProductTerm*, int>& pterm_assignments);

	///Number of this function block within the parent device (one based as per datasheet)
	int m_blocknum;

	///The macrocells
	std::vector<FCCoolRunnerIIMacrocell*> m_macrocells;
	
	/**
		@brief The PLA AND array.
		
		Generates 56 product terms from 40 true and 40 inverted inputs.
		
		The convention used here is that a "true" value means the idle, or unprogrammed state (no connection) and a
		"false" value means the active, or programmed state (connection present).
		
		This is the same convention used in the actual EEPROM image and was chosen for consistency with the physical
		device, although it may be somewhat counterintuitive.
	 */
	bool m_plaAnd[PLA_PRODUCT_TERMS][PLA_INPUT_WIDTH];
	
	/**
		@brief The PLA OR array.
		
		Generates 16 output terms from 56 product terms.
		
		The convention used here is that a "true" value means the idle, or unprogrammed state (no connection) and a
		"false" value means the active, or programmed state (connection present).
		
		This is the same convention used in the actual EEPROM image and was chosen for consistency with the physical
		device, although it may be somewhat counterintuitive.
	 */
	bool m_plaOr[PLA_PRODUCT_TERMS][MACROCELLS_PER_FB];
	
	/**
		@brief The ZIA (input switch)
		
		Exact structure is not known at this time.
	 */
	bool* m_zia[INPUT_TERM_COUNT];
	
	int m_ziaWidth;
};

#endif
