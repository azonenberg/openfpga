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
	@brief Declaration of FCCoolRunnerIIMacrocell
 */ 

#ifndef FCCoolRunnerIIMacrocell_h
#define FCCoolRunnerIIMacrocell_h

#include "FCCoolRunnerIIIOB.h"

class CPLDSerializer;
class FCCoolRunnerIINetlistMacrocell;
class FCCoolRunnerIIFunctionBlock;

/**
	@brief A single CoolRunner-II macrocell.
	
	Each macrocell takes the OR term from the PLA and XORs it with one of the following:
		* One product term expression PTA (hard-wired to a specific row)
		* A constant "1" (to invert the term)
		* A constant "0" (to leave it unchanged)
		
	The macrocell also contains a flipflop. The flipflop's data input may come from the output of the XOR gate or
	directly from the IOB.
	
	The flipflop's clock enable is hard-wired to the PTC input of the macrocell.
	
	The clock input to the flipflop may come from one of the following sources:
		* CTC (control term clock, a product term shared by all macrocells)
		* PTC (product term clock, hard-wired to a specific row)
		* GCK0 (global clock net)
		* GCK1 (global clock net)
		* GCK2 (global clock net)
		
	The clock may optionally be inverted before reaching the flipflop.
	
	The set/reset input to the flipflop may come from one of the following sources:
		* PTA
		* CTR (control term reset, a product term shared by all macrocells)
		* CTS (control term set, a product term shared by all macrocells)
		* GSR (global set/reset)
		* Constant '0'
		
	The output of the flipflop, as well as that of the XOR gate, are fed back to the AIM as well as to the macrocell's
	associated IOB.
 */
class FCCoolRunnerIIMacrocell : public FCCoolRunnerIINetSource
{
public:
	FCCoolRunnerIIMacrocell(FCCoolRunnerIIFunctionBlock* fb, int n);
	~FCCoolRunnerIIMacrocell();

	FCCoolRunnerIIIOB m_iob;
	
	FCCoolRunnerIIFunctionBlock* GetFunctionBlock();
	int GetCellNumber();
	
	void DumpHeader();
	void Dump();
	void DumpFooter();
	
	void DumpRTL();
	
	void DumpIOBRTL()
	{ m_iob.DumpRTL(); }
	
	std::string GetTopLevelNetName();
	
	void SaveToBitstream(CPLDSerializer* writer);
	
	int LoadFromBitstream(bool* bitstream, int start_address);
	
	enum ClockSources
	{
		CLK_GCK0 = 0,
		CLK_GCK1 = 2,
		CLK_GCK2 = 1,
		CLK_PTC = 3
	};
	std::string GetClockSourceText();
	
	enum ClockEdges
	{
		CLK_EDGE_RISING = 0,
		CLK_EDGE_FALLING = 1,
		CLK_EDGE_DDR = 2
	};
	
	std::string GetClockEdgeText();
	
	enum RegisterModes
	{
		///The register is a D flipflop
		REG_MODE_DFF = 0,
		
		///The register is a transparent latch
		REG_MODE_LATCH = 1,
		
		///The register is a T flipflop
		REG_MODE_TFF = 2,
		
		///The register is a D flipflop with clock enable
		REG_MODE_DFFCE = 3
	};
	std::string GetRegModeText();
	
	enum InregModes
	{
		INREG_IOB = 0,
		INREG_XOR = 1
	};
	
	/**
		@brief XOR mux

		Product term C for macrocell N (zero based) is product term (10 + 3n).
	 */
	enum XorinModes
	{
		//Hard-wired 0 (zero ^ unused OR term = zero)
		XORIN_ZERO = 0,
		
		//~PTC
		XORIN_NPTC = 1,
		
		//PTC
		XORIN_PTC = 2,
		
		//Hard-wired 1 (one ^ unused OR term = 1)
		XORIN_ONE = 3
	} m_xorin;
	
	std::string GetXorinText();
	
	/// @brief Returns the zero-based product term index of product term C
	int GetPtermCIndex()
	{ return (3 * (m_cellnum - 1)) + 10; }
	
	std::string GetNamePrefix();
	
	FCCoolRunnerIINetlistMacrocell* m_netlistcell;
	
protected:
	///Index of this macrocell within the FB (one based as per datasheet)
	int m_cellnum;
	
	///The function block we belong to
	FCCoolRunnerIIFunctionBlock* m_fb;
	
	///unknown
	bool m_aclk;
	
	///@brief Clock source
	int m_clk;
	
	///@brief Clock edge (combination of clkop and clkfreq bits)
	int m_clkEdge;
	
	///unknown
	int m_r;
	
	///unknown
	int m_p;
	
	///@brief Register mode (see RegisterModes enum)
	int m_regmod;
	
	/**
		@brief Unknown value
		Observed in several different values but not yet known what the conditions are.
	 */
	int m_fbval;
	
	///@brief Controls whether FF D input comes from IOB or XOR output (see InregModes enum)
	bool m_inreg;
	
	/**
		@brief Flipflop power-up value
		
		This is the logical value (true = 1'b1, false = 1'b0). The value in the bitstream is complemented for
		some reason.
	 */
	bool m_ffResetState;
};

#endif
