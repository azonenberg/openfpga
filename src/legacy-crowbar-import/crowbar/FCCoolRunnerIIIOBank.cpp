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
	@brief Implementation of FCCoolRunnerIIIOBank
 */

#include "crowbar.h"
#include "FCCoolRunnerIIIOBank.h"
#include "FCCoolRunnerIIIOB.h"
#include "../jtaghal/JEDFileWriter.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Construction / destruction

FCCoolRunnerIIIOBank::FCCoolRunnerIIIOBank(int banknum)
	: FCIOBank(banknum)
	, m_ivoltage(true)
	, m_ovoltage(true)
{
	
}

FCCoolRunnerIIIOBank::~FCCoolRunnerIIIOBank()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Accessors

void FCCoolRunnerIIIOBank::SetVoltages(bool ivoltage, bool ovoltage)
{
	m_ivoltage = ivoltage;
	m_ovoltage = ovoltage;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Debug output

void FCCoolRunnerIIIOBank::Dump()
{
	const char* ranges[] =
	{
		"HIGH",
		"LOW"
	};
	const char* iostandards[] =
	{
		"LVTTL/LVCMOS33/LVCMOS25",
		"LVCMOS18/LVCMOS15"
	};
	printf("    %4d    %15s    %30s    %15s    %30s\n",
		m_banknum,
		ranges[m_ovoltage],
		iostandards[m_ovoltage],
		ranges[m_ivoltage],
		iostandards[m_ivoltage]);
}

void FCCoolRunnerIIIOBank::DumpHeader()
{
	printf("\n    %4s    %15s    %30s    %15s    %30s\n",
		"Bank",
		"Output range",
		"Allowed output std",
		"Input range",
		"Allowed input std");
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Output

void FCCoolRunnerIIIOBank::SaveToBitstream(CPLDSerializer* writer)
{
	char buf[32];
	
	snprintf(buf, sizeof(buf), "Bank %d Vcci", m_banknum);
	writer->AddBodyBlankLine();
	writer->AddBodyComment(buf);
	writer->AddBodyFuseData(&m_ivoltage, 1);
	
	snprintf(buf, sizeof(buf), "Bank %d Vcco", m_banknum);
	writer->AddBodyBlankLine();
	writer->AddBodyComment(buf);
	writer->AddBodyFuseData(&m_ovoltage, 1);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Fitting

void FCCoolRunnerIIIOBank::Fit()
{
	//DRC: Verify all pins are compatible I/O standards
	
	//Default to high voltage if not specified
	m_ivoltage = HIGH_VOLTAGE;
	m_ovoltage = HIGH_VOLTAGE;
	bool highRequested = false;
	
	for(size_t i=0; i<m_iobs.size(); i++)
	{
		FCCoolRunnerIIIOB* iob = static_cast<FCCoolRunnerIIIOB*>(m_iobs[i]);
		
		switch(iob->m_iostandard)
		{
		
		//No constraints? Use default
		case FCCoolRunnerIIIOB::UNCONSTRAINED:
			break;
		
		//High voltage
		case FCCoolRunnerIIIOB::LVTTL:
		case FCCoolRunnerIIIOB::LVCMOS33:
		case FCCoolRunnerIIIOB::LVCMOS25:
			if(m_ivoltage == LOW_VOLTAGE)
			{
				throw JtagExceptionWrapper(
					"DRC error: Cannot mix high and low voltage I/O standards in the same bank",
					"",
					JtagException::EXCEPTION_TYPE_GIGO);
			}			
			highRequested = true;
			break;
			
		//Low voltage
		case FCCoolRunnerIIIOB::LVCMOS18:
		case FCCoolRunnerIIIOB::LVCMOS15:
			if(highRequested)
			{
				throw JtagExceptionWrapper(
					"DRC error: Cannot mix high and low voltage I/O standards in the same bank",
					"",
					JtagException::EXCEPTION_TYPE_GIGO);
			}
			
			m_ivoltage = LOW_VOLTAGE;
			m_ovoltage = LOW_VOLTAGE;			
			break;
			
		//Reference-oriented
		case FCCoolRunnerIIIOB::HSTL_1:
		case FCCoolRunnerIIIOB::SSTL2_1:
		case FCCoolRunnerIIIOB::SSTL3_1:
			throw JtagExceptionWrapper(
				"HSTL/SSTL I/O standards not implemented yet",
				"",
				JtagException::EXCEPTION_TYPE_UNIMPLEMENTED);
			break;
		}
	}
}
