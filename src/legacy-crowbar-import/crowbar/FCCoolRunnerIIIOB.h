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
	@brief Declaration of FCCoolRunnerIIIOB
 */

#ifndef FCCoolRunnerIIIOB_h
#define FCCoolRunnerIIIOB_h

#include "FCIOB.h"
#include "FCCoolRunnerIINetSource.h"

class FCCoolRunnerIIMacrocell;

/**
	@brief A single I/O buffer
	
	Output enable is either dedicated product term PTB or control term CTE
 */
class FCCoolRunnerIIIOB : public FCIOB
						, public FCCoolRunnerIINetSource
{
public:
	FCCoolRunnerIIIOB(FCCoolRunnerIIMacrocell* parent);
	virtual ~FCCoolRunnerIIIOB();
	
	enum iostandard
	{
		UNCONSTRAINED,
		LVTTL,
		LVCMOS33,
		LVCMOS25,
		LVCMOS18,
		LVCMOS15,
		HSTL_1,
		SSTL2_1,
		SSTL3_1
	};

	std::string GetIOStandardText();

	enum SlewRate
	{
		SLEW_FAST = 0,
		SLEW_SLOW = 1
	};	
	bool GetSlewRate();
	void SetSlewRate(bool rate);
	
	enum TerminationMode
	{
		TERM_FLOAT = 0,
		TERM_ACTIVE = 1
	};
	bool GetTerminationMode();
	void SetTerminationMode(bool mode);
	
	///Schmitt trigger enable flag
	bool m_bSchmittTriggerEnabled;
	
	enum OutputMode
	{
		OUTPUT_REGISTER = 0,
		OUTPUT_DIRECT = 1
	};
	
	///Registered / combinatorial output
	bool m_outmode;
	
	//TODO: Separate flag for keeper/float/active/ground etc
	
	enum InZ
	{
		INZ_INPUT = 0,
		//values 1 and 2 not seen yet
		INZ_FLOAT = 3
	};
	
	void SetInZ(int z);
	int GetInZ();
	std::string GetInZText();
	
	enum OE
	{
		OE_OUTPUT    = 0,
		OE_OPENDRAIN = 1,
		OE_TRISTATE  = 8,
		OE_CGND      = 14,
		OE_FLOAT     = 15
	};
	
	void SetOE(int oe);
	int GetOE();
	std::string GetOEText();
	
	void DumpRTL();
	
	std::string GetTopLevelNetName();
	
	std::string GetNamePrefix();
	
	///I/O standard of this IOB
	iostandard m_iostandard;
	
	FCCoolRunnerIIMacrocell* GetMacrocell()
	{ return m_macrocell; }
	
protected:

	std::string GetConstraints();

	int m_oe;

	///Input mode (see InZ)
	int m_inz;

	///The macrocell we're associated with
	FCCoolRunnerIIMacrocell* m_macrocell;
	
	///Slew rate
	bool m_slewRate;
	
	///Termination mode
	bool m_termMode;
};

#endif
