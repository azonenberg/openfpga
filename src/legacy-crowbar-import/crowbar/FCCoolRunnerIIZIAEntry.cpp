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
	@brief Implementation of FCCoolRunnerIIZIAEntry
 */

#include "crowbar.h"
#include "FCCoolRunnerIIZIAEntry.h"

using namespace std;

FCCoolRunnerIIZIAEntry::FCCoolRunnerIIZIAEntry(int row, string muxsel_hex, int zia_bits)
	: m_row(row)
	, m_muxsel_str(muxsel_hex)
{
	//Dummy for containers, never used
	if(zia_bits == 0)
		return;
	
	//Read bytes, right to left
	if(zia_bits != 8)
	{	
		throw JtagExceptionWrapper(
			"Not implemented",
			"",
			JtagException::EXCEPTION_TYPE_UNIMPLEMENTED);
	}
	
	//Temporary easy code for width=8
	int val;
	sscanf(muxsel_hex.c_str(), "%2x", &val);
	m_muxsel.push_back((val >> 7) & 1);
	m_muxsel.push_back((val >> 6) & 1);
	m_muxsel.push_back((val >> 5) & 1);
	m_muxsel.push_back((val >> 4) & 1);
	m_muxsel.push_back((val >> 3) & 1);
	m_muxsel.push_back((val >> 2) & 1);
	m_muxsel.push_back((val >> 1) & 1);
	m_muxsel.push_back((val >> 0) & 1);
}

FCCoolRunnerIIZIAEntry::~FCCoolRunnerIIZIAEntry()
{
}
