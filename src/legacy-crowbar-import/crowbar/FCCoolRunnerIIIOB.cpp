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
	@brief Implementation of FCCoolRunnerIIIOB
 */

#include "crowbar.h"
#include "FCCoolRunnerIIFunctionBlock.h"
#include "FCCoolRunnerIIIOB.h"
#include "FCCoolRunnerIIMacrocell.h"
#include "FCCoolRunnerIIIOBank.h"

using namespace std;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Construction / destruction

FCCoolRunnerIIIOB::FCCoolRunnerIIIOB(FCCoolRunnerIIMacrocell* parent)
	: m_bSchmittTriggerEnabled(true)
	, m_outmode(OUTPUT_DIRECT)
	, m_iostandard(UNCONSTRAINED)
	, m_oe(OE_FLOAT)
	, m_inz(INZ_FLOAT)
	, m_macrocell(parent)
	, m_slewRate(SLEW_FAST)
	, m_termMode(TERM_ACTIVE)
{
}

FCCoolRunnerIIIOB::~FCCoolRunnerIIIOB()
{
	
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Accessors

string FCCoolRunnerIIIOB::GetNamePrefix()
{
	return m_macrocell->GetNamePrefix();
}

string FCCoolRunnerIIIOB::GetIOStandardText()
{
	switch(m_iostandard)
	{
		case UNCONSTRAINED:
			return "UNCONSTRAINED";
		case LVTTL:
			return "LVTTL";
		case LVCMOS33:
			return "LVCMOS33";
		case LVCMOS25:
			return "LVCMOS25";
		case LVCMOS18:
			return "LVCMOS18";
		case LVCMOS15:
			return "LVCMOS15";
		case HSTL_1:
			return "HSTL_1";
		case SSTL2_1:
			return "SSTL2_1";
		case SSTL3_1:
			return "SSTL3_1";
		default:
			throw JtagExceptionWrapper(
				"Invalid I/O standard",
				"",
				JtagException::EXCEPTION_TYPE_GIGO);
	}
}

bool FCCoolRunnerIIIOB::GetSlewRate()
{
	return m_slewRate;
}

void FCCoolRunnerIIIOB::SetSlewRate(bool rate)
{
	m_slewRate = rate;
}

bool FCCoolRunnerIIIOB::GetTerminationMode()
{
	return m_termMode;
}

void FCCoolRunnerIIIOB::SetTerminationMode(bool mode)
{
	m_termMode = mode;
}

void FCCoolRunnerIIIOB::SetInZ(int z)
{
	switch(z)
	{
	case INZ_INPUT:
	case INZ_FLOAT:
		m_inz = z;
		break;
	default:
		throw JtagExceptionWrapper(
			"Invalid InZ",
			"",
			JtagException::EXCEPTION_TYPE_GIGO);
	}
}

int FCCoolRunnerIIIOB::GetInZ()
{
	return m_inz;
}

std::string FCCoolRunnerIIIOB::GetInZText()
{
	switch(m_inz)
	{
	case INZ_INPUT:
		return "Input";
	case INZ_FLOAT:
		return "Float";
	default:
		throw JtagExceptionWrapper(
			"Invalid InZ",
			"",
			JtagException::EXCEPTION_TYPE_GIGO);
	}
}

void FCCoolRunnerIIIOB::SetOE(int oe)
{
	switch(oe)
	{
	case OE_OUTPUT:
	case OE_OPENDRAIN:
	case OE_CGND:
	case OE_TRISTATE:
	case OE_FLOAT:
		m_oe = oe;
		break;
	default:
		throw JtagExceptionWrapper(
			"Invalid OE",
			"",
			JtagException::EXCEPTION_TYPE_GIGO);
	}
}

int FCCoolRunnerIIIOB::GetOE()
{
	return m_oe;
}

std::string FCCoolRunnerIIIOB::GetOEText()
{
	switch(m_oe)
	{
	case OE_OUTPUT:
		return "Output";
	case OE_OPENDRAIN:
		return "Open drain";
	case OE_CGND:
		return "CGND";
	case OE_FLOAT:
		return "Float";
	case OE_TRISTATE:
		return "Tristate";
	default:
		throw JtagExceptionWrapper(
			"Invalid OE",
			"",
			JtagException::EXCEPTION_TYPE_GIGO);
	}
}

string FCCoolRunnerIIIOB::GetConstraints()
{
	//Location
	char buf[128];
	snprintf(buf, sizeof(buf), "(* LOC=\"FB%d_%d\" *)",
		m_macrocell->GetFunctionBlock()->GetBlockNumber(), m_macrocell->GetCellNumber());
	string s(buf);
	
	//I/O standard
	//Use the bank's I/O standard for now
	FCCoolRunnerIIIOBank* bank = dynamic_cast<FCCoolRunnerIIIOBank*>(m_bank);
	if(bank != NULL)
	{
		bool ivolt = bank->GetIVoltage();
		bool ovolt = bank->GetOVoltage();
		
		bool volt = ovolt;
		if(GetInZ() == INZ_INPUT)
			volt = ivolt;
		
		else if(volt == FCCoolRunnerIIIOBank::HIGH_VOLTAGE)
			s += " (* IOSTANDARD = \"LVCMOS33\" *) ";
		else if(volt == FCCoolRunnerIIIOBank::LOW_VOLTAGE)
			s += " (* IOSTANDARD = \"LVCMOS18\" *) ";
	}
	
	//Slew rate
	if(m_slewRate == SLEW_FAST)
		s += " (* SLEW = \"FAST\" *)";
	else
		s += " (* SLEW = \"SLOW\" *)";
	
	//Schmitt trigger
	if(m_bSchmittTriggerEnabled)
		s += " (* SCHMITT_TRIGGER = \"yes\" *)";
		
	//TODO: Keeper / pullup
		
	return s;
}

std::string FCCoolRunnerIIIOB::GetTopLevelNetName()
{
	char buf[128] = "";
	switch(m_oe)
	{
		case OE_OUTPUT:
		case OE_OPENDRAIN:
			snprintf(buf, sizeof(buf), "%s_obuf", GetNamePrefix().c_str());
			break;
			
		case OE_TRISTATE:
			snprintf(buf, sizeof(buf), "%s_iobuf", GetNamePrefix().c_str());
			break;
			
		case OE_CGND:
			//CGND
			break;
			
		case OE_FLOAT:
			//Output driver not connnected... is input buffer connected?
			switch(m_inz)
			{
				case INZ_INPUT:
					snprintf(buf, sizeof(buf), "%s", GetNamePrefix().c_str());
					break;
				case INZ_FLOAT:
					break;
				default:
					printf("    // WARNING: Don't know what InZ value %d for %s IOB means\n", m_inz, GetNamePrefix().c_str());
					break;
			}
			break;
			
		default:
			printf("    // WARNING: Don't know what OE value %d for %s IOB means\n", m_oe, GetNamePrefix().c_str());
			break;
	}
	return buf;
}

void FCCoolRunnerIIIOB::DumpRTL()
{
	switch(m_oe)
	{
		case OE_OUTPUT:
		case OE_OPENDRAIN:
			printf("    %s\n        output wire %s_obuf;\n", GetConstraints().c_str(), GetNamePrefix().c_str());
			printf("        wire %s_output;\n",  GetNamePrefix().c_str());
			printf("        OBUF obuf_%s_inst(.O(%s_obuf), .I(%s_output));\n",
				GetNamePrefix().c_str(),  GetNamePrefix().c_str(),  GetNamePrefix().c_str());
			break;
			
		case OE_TRISTATE:
			printf("    %s\n        inout wire %s_iobuf;\n", GetConstraints().c_str(), GetNamePrefix().c_str());
			printf("    // WARNING: Generation of tristate buffers not implemented yet\n");
			break;
			
		case OE_CGND:
			printf("    // %s IOB is CGND\n", GetNamePrefix().c_str());
			break;
			
		case OE_FLOAT:
			//Output driver not connnected... is input buffer connected?
			switch(m_inz)
			{
				case INZ_INPUT:
					printf("    %s\n        input wire %s;\n", GetConstraints().c_str(), GetNamePrefix().c_str());
					printf("        wire %s_ibuf;\n",  GetNamePrefix().c_str());
					printf("        IBUF ibuf_%s_inst(.O(%s), .I(%s_ibuf));\n",
						GetNamePrefix().c_str(),  GetNamePrefix().c_str(),  GetNamePrefix().c_str());
					break;
				case INZ_FLOAT:
					break;
				default:
					printf("    // WARNING: Don't know what InZ value %d for %s IOB means\n", m_inz, GetNamePrefix().c_str());
					break;
			}
			break;
			
		default:
			printf("    // WARNING: Don't know what OE value %d for %s IOB means\n", m_oe, GetNamePrefix().c_str());
			break;
	}
}
