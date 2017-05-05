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
	@brief Implementation of FCCoolRunnerIIMacrocell
 */

#include "crowbar.h"
#include "FCCoolRunnerIIFunctionBlock.h"
#include "FCCoolRunnerIIMacrocell.h"
#include "../jtaghal/JEDFileWriter.h"

using namespace std;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Construction / destruction

FCCoolRunnerIIMacrocell::FCCoolRunnerIIMacrocell(FCCoolRunnerIIFunctionBlock* fb, int n)
	: m_iob(this)
	, m_xorin(XORIN_ONE)
	, m_cellnum(n)
	, m_fb(fb)
	, m_aclk(true)
	, m_clk(CLK_GCK0)
	, m_clkEdge (CLK_EDGE_DDR)	
	, m_r(3)
	, m_p(3)
	, m_regmod(REG_MODE_DFF)
	, m_fbval(3)
	, m_inreg(INREG_XOR)
	, m_ffResetState(true)
{
	m_netlistcell = NULL;
}

FCCoolRunnerIIMacrocell::~FCCoolRunnerIIMacrocell()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Accessors

/**
	@brief Gets the index of this macrocell within its parent function block
 */
int FCCoolRunnerIIMacrocell::GetCellNumber()
{
	return m_cellnum;
}

/**
	@brief Gets the macrocell's parent function block
 */
FCCoolRunnerIIFunctionBlock* FCCoolRunnerIIMacrocell::GetFunctionBlock()
{
	return m_fb;
}

string FCCoolRunnerIIMacrocell::GetClockSourceText()
{
	switch(m_clk)
	{
	case CLK_GCK0:
		return "GCK0";
	case CLK_GCK1:
		return "GCK1";
	case CLK_GCK2:
		return "GCK2";
	case CLK_PTC:
		return "PTC";
	default:
		throw JtagExceptionWrapper(
			"Invalid clock source",
			"",
			JtagException::EXCEPTION_TYPE_GIGO);
		break;
	}
}

string FCCoolRunnerIIMacrocell::GetClockEdgeText()
{
	switch(m_clkEdge)
	{
	case CLK_EDGE_RISING:
		return "Rising";
	case CLK_EDGE_FALLING:
		return "Falling";
	case CLK_EDGE_DDR:
		return "DDR";
	default:
		throw JtagExceptionWrapper(
			"Invalid clock edge",
			"",
			JtagException::EXCEPTION_TYPE_GIGO);
		break;
	}
}

string FCCoolRunnerIIMacrocell::GetRegModeText()
{
	switch(m_regmod)
	{
	case REG_MODE_DFF:
		return "DFF";
	case REG_MODE_LATCH:
		return "Latch";
	case REG_MODE_TFF:
		return "TFF";
	case REG_MODE_DFFCE:
		return "DFFCE";
	default:
		throw JtagExceptionWrapper(
			"Invalid reg mode",
			"",
			JtagException::EXCEPTION_TYPE_GIGO);
		break;
	}
}

string FCCoolRunnerIIMacrocell::GetXorinText()
{
	switch(m_xorin)
	{
		case XORIN_ZERO:
			return "0";
		case XORIN_NPTC:
			return "~PTC";
		case XORIN_PTC:
			return "PTC";
		case XORIN_ONE:
			return "1";
		default:
			throw JtagExceptionWrapper(
				"Invalid xorin mode",
				"",
				JtagException::EXCEPTION_TYPE_GIGO);
			break;
	}
}

string FCCoolRunnerIIMacrocell::GetNamePrefix()
{
	char buf[128] = "";
	snprintf(buf, sizeof(buf), "fb%d_%d", m_fb->GetBlockNumber(), m_cellnum);
	return buf;
}

/**
	@brief Gets the net name for our flipflop
 */
string FCCoolRunnerIIMacrocell::GetTopLevelNetName()
{
	return GetNamePrefix() + "_ff";
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Debug output

void FCCoolRunnerIIMacrocell::DumpRTL()
{	
	if(m_fb->IsMacrocellOutputUsed(m_cellnum - 1))
	{
		//Determine which inputs are used
		bool orused = m_fb->IsOrTermUsed(m_cellnum - 1);
		int ptcnum = GetPtermCIndex();
		bool ptcused = m_fb->IsProductTermUsed(ptcnum);
		
		//Variable declaration
		printf("    (* LOC= FB%d_%d *) wire %s_xor = ", m_fb->GetBlockNumber(), m_cellnum, GetNamePrefix().c_str());
			
		/*
			orterm
			~orterm
			PTC valid
				orterm ^ PTC
				orterm ^ ~PTC
		*/
		if(orused)
		{
			/*
			XORIN_ZERO = 0,
			XORIN_NPTC = 1,
			XORIN_PTC = 2,
			XORIN_ONE = 3
			*/
			
			switch(m_xorin)
			{
			case XORIN_ZERO:
				printf("fb%d_%d_orterm; // slow path", m_fb->GetBlockNumber(), m_cellnum);
				break;
			case XORIN_PTC:
				if(ptcused)
					printf("fb%d_%d_orterm ^ fb%d_pterm_%d;", m_fb->GetBlockNumber(), m_cellnum, m_fb->GetBlockNumber(), ptcnum + 1);
				else
					printf("fb%d_%d_orterm; //^ unused PTC = nop", m_fb->GetBlockNumber(), m_cellnum);
				break;
			case XORIN_NPTC:
				if(ptcused)
					printf("fb%d_%d_orterm ^ ~fb%d_pterm_%d;", m_fb->GetBlockNumber(), m_cellnum, m_fb->GetBlockNumber(), ptcnum + 1);
				else
					printf("!fb%d_%d_orterm; //^ ~unused PTC = invert", m_fb->GetBlockNumber(), m_cellnum);
				break;
			case XORIN_ONE:
				printf("~fb%d_%d_orterm; // slow path", m_fb->GetBlockNumber(), m_cellnum);
				break;
			}
		}
		
		/*	
			Constant 1
			Constant 0
			PTC valid
				PTC
				~PTC
		 */
		else
		{
			switch(m_xorin)
			{
			case XORIN_ZERO:
				printf("0;");
				break;
			case XORIN_PTC:
				if(ptcused)
					printf("fb%d_pterm_%d; //fast path", m_fb->GetBlockNumber(), ptcnum + 1);
				else
					printf("0; //ptc not used");
				break;
			case XORIN_NPTC:
				if(ptcused)
					printf("!fb%d_pterm_%d; //fast path", m_fb->GetBlockNumber(), ptcnum + 1);
				else
					printf("1; //ptc not used");
				break;
			case XORIN_ONE:
				printf("1;");
				break;
			}
		}
		
		printf("\n");
		
		//TODO: Flipflop
	}
}

void FCCoolRunnerIIMacrocell::DumpHeader()
{
	printf("\n");
	printf( "   ┌───────────┬─────────────────────────────────────────────────┬────────────────────┬──────────────────┬───────────────────────────────────────────────────────────────────────────┐\n");
	printf( "   │           │  FLIPFLOP                                       │        UNKNOWN     │      IBUF        │ OBUF                                                                      │\n");
	printf( "   ├───────────┼─────────────────────────────────────────────────┼────────────────────┼──────────────────┼───────────────────────────────────────────────────────────────────────────┤\n");
	
	
	printf(	"   │%10s │ %6s   %4s   %5s   %6s   %7s   %4s │ %1s   %1s   %2s   %5s │ %7s   %6s │ %10s   %8s   %11s   %4s   %4s   %3s   %15s │\n",
			"Macrocell",
			"Mode",
			"DSrc",
			"aclk",
			"ClkSrc",
			"ClkMode",
			"Init",
			"r",
			"p",
			"fb",
			"xorin",
			"Schmitt",
			"InMode",
			"Mode",
			"Source",
			"Termination",
			"Slew",
			"Bank",
			"Pin",
			"I/O standard"
		);
}

void FCCoolRunnerIIMacrocell::Dump()
{
	printf("   │%10d │ %6s   %4s   %5d   %6s   %7s   %4s │ %1d   %1d   %2d   %5s │ %7s   %6s │ %10s   %8s   %11s   %4s   %4d   %3s   %15s │\n",
		m_cellnum,
		GetRegModeText().c_str(),
		(m_inreg == INREG_IOB) ? "IOB" : "XOR",
		m_aclk,
		GetClockSourceText().c_str(),
		GetClockEdgeText().c_str(),
		m_ffResetState ? "1'b1" : "1'b0",
		m_r,
		m_p,
		m_fbval,
		GetXorinText().c_str(),
		m_iob.m_bSchmittTriggerEnabled ? "yes" : "no",
		m_iob.GetInZText().c_str(),
		m_iob.GetOEText().c_str(),
		(m_iob.m_outmode == FCCoolRunnerIIIOB::OUTPUT_REGISTER) ? "Register" : "Direct",
		(m_iob.GetTerminationMode() == FCCoolRunnerIIIOB::TERM_FLOAT) ? "FLOAT" : "TERM",
		(m_iob.GetSlewRate() == FCCoolRunnerIIIOB::SLEW_FAST) ? "FAST" : "SLOW",
		m_iob.GetBank()->GetBankNumber(),
		m_iob.GetPin().c_str(),
		m_iob.GetIOStandardText().c_str()
		);
}

void FCCoolRunnerIIMacrocell::DumpFooter()
{
	printf( "   └───────────┴─────────────────────────────────────────────────┴────────────────────┴──────────────────┴───────────────────────────────────────────────────────────────────────────┘\n");
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Initialization
int FCCoolRunnerIIMacrocell::LoadFromBitstream(bool* bitstream, int start_address)
{
	//printf("        Loading macrocell %d, starting at fuse %d\n", m_cellnum, start_address);
	
	//TODO: support devices other than XC2C32A
	
	m_aclk  	= bitstream[start_address++];
	bool clkop 	= bitstream[start_address++];
	m_clk = (bitstream[start_address] << 1) | bitstream[start_address+1];
	start_address += 2;
	bool clkfreq 	= bitstream[start_address++];
	m_r			= (bitstream[start_address] << 1) | bitstream[start_address+1];
	start_address += 2;
	m_p			= (bitstream[start_address] << 1) | bitstream[start_address+1];
	start_address += 2;
	m_regmod	= (bitstream[start_address] << 1) | bitstream[start_address+1];
	start_address += 2;
	m_iob.SetInZ( (bitstream[start_address] << 1) | bitstream[start_address+1] );
	start_address += 2;
	m_fbval		= (bitstream[start_address] << 1) | bitstream[start_address+1];
	start_address += 2;
	m_inreg 	= bitstream[start_address++];
	m_iob.m_bSchmittTriggerEnabled = bitstream[start_address++];
	int xorin		= (bitstream[start_address] << 1) | bitstream[start_address+1];
	start_address += 2;
	m_iob.m_outmode 	= bitstream[start_address++];
	m_iob.SetOE(	(bitstream[start_address] << 3)   | (bitstream[start_address+1] << 2) |
					(bitstream[start_address+2] << 1) | bitstream[start_address+3]);
	start_address += 4;
	m_iob.SetTerminationMode(bitstream[start_address++]);
	m_iob.SetSlewRate(bitstream[start_address++]);
	m_ffResetState	 = !bitstream[start_address++];	//reset state is inverted
	
	//Decode clock mode
	if(clkfreq)
		m_clkEdge = CLK_EDGE_DDR;
	else if(clkop)
		m_clkEdge = CLK_EDGE_FALLING;
	else
		m_clkEdge = CLK_EDGE_RISING;
		
	//Decode XOR mode
	switch(xorin)
	{
	case XORIN_ZERO:
		m_xorin = XORIN_ZERO;
		break;
	case XORIN_NPTC:
		m_xorin = XORIN_NPTC;
		break;
	case XORIN_PTC:
		m_xorin = XORIN_PTC;
		break;
	case XORIN_ONE:
	default:
		m_xorin = XORIN_ONE;
		break;
	}
	 
	return start_address;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// File output

void FCCoolRunnerIIMacrocell::SaveToBitstream(CPLDSerializer* writer)
{
	//TODO: support devices other than XC2C32A
	//N Aclk ClkOp Clk:2 ClkFreq R:2 P:2 RegMod:2 INz:2 FB:2 InReg St XorIn:2 RegCom Oe:4 Tm Slw Pu*
	int inz = m_iob.GetInZ();
	int oe = m_iob.GetOE();
	bool data_out[27] =
	{
		m_aclk,															//aclk:1
		(m_clkEdge == CLK_EDGE_FALLING),								//clkop:1
		(m_clk & 2) ? true : false,	(m_clk & 1) ? true : false,			//clk:2
		(m_clkEdge == CLK_EDGE_DDR),									//clkfreq
		(m_r & 2) ? true : false,	(m_r & 1) ? true : false,			//r:2
		(m_p & 2) ? true : false,	(m_p & 1) ? true : false,			//p:2
		(m_regmod & 2) ? true : false,	(m_regmod & 1) ? true : false,	//regmod:2
		(inz & 2) ? true : false,	(inz & 1) ? true : false,			//inz:2
		(m_fbval & 2) ? true : false,	(m_fbval & 1) ? true : false,	//fb:2
		m_inreg,														//inreg
		m_iob.m_bSchmittTriggerEnabled,									//st
		(m_xorin & 2) ? true : false,	(m_xorin & 1) ? true : false,	//xorin:2
		m_iob.m_outmode,												//RegCom
		(oe & 8) ? true : false, (oe & 4) ? true : false, 				//oe:4
		(oe & 1) ? true : false, (oe & 1) ? true : false, 
		m_iob.GetTerminationMode(),										//tm
		m_iob.GetSlewRate(),											//slew
		!m_ffResetState													//pu (reset state is inverted)
	};
	
	writer->AddBodyFuseData(data_out, 27);
}
