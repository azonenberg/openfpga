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
	@brief Implementation of FCDevice
 */

#include "crowbar.h"
#include "FCDevice.h"
#include "FCCoolRunnerIIDevice.h"
#include "FCIOBank.h"

using namespace std;

FCDevice::~FCDevice()
{
	//Do not delete IOBs since the macrocells etc may own them
	
	//Clean up I/O banks
	for(size_t i=0; i<m_banks.size(); i++)
		delete m_banks[i];
	m_banks.clear();
}

/**
	@brief Creates a device given a name
 */
FCDevice* FCDevice::CreateDevice(string name)
{
	//String match the part number
	if(name.find("xc2c") != string::npos)
		return new FCCoolRunnerIIDevice(name);
	
	//Out of ideas, give up
	throw JtagExceptionWrapper(
		"Invalid device name (does not match a known part)",
		"",
		JtagException::EXCEPTION_TYPE_GIGO);
}

/**
	@brief Gets the IOB for a given pin
	
	TODO: Build a hash table or something, O(n) is silly
 */
FCIOB* FCDevice::GetIOBForPin(string name)
{
	for(size_t i=0; i<m_iobs.size(); i++)
	{
		if(m_iobs[i]->GetPin() == name)
			return m_iobs[i];
	}
	
	throw JtagExceptionWrapper(
		"Invalid IOB name",
		"",
		JtagException::EXCEPTION_TYPE_GIGO);
}
