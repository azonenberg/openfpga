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
	@brief Declaration of FCCoolRunnerIINetlistMacrocell
 */

#ifndef FCCoolRunnerIINetlistMacrocell_h
#define FCCoolRunnerIINetlistMacrocell_h

#include "FCCoolRunnerIINetSource.h"
#include "FCCoolRunnerIIOrTerm.h"
#include "FCCoolRunnerIIMacrocell.h"
#include "FCCoolRunnerIIIOB.h"

/**
	@brief An unplaced macrocell.
	
	During fitting, a FCCoolRunnerIINetlistMacrocell is paired up with a FCCoolRunnerIIMacrocell.
 */
class FCCoolRunnerIINetlistMacrocell : public FCCoolRunnerIINetSource
{
public:
	FCCoolRunnerIINetlistMacrocell(FCCoolRunnerIIOrTerm* orterm, FCCoolRunnerIIIOB* iob);
	virtual ~FCCoolRunnerIINetlistMacrocell();
	
	FCCoolRunnerIIOrTerm* GetOrTerm()
	{ return m_orterm; }
	
	FCCoolRunnerIIMacrocell* GetMacrocell()
	{ return m_macrocell; }
	
	void SetMacrocell(FCCoolRunnerIIMacrocell* macrocell)
	{
		m_macrocell = macrocell;
		m_iob = &macrocell->m_iob;
		macrocell->m_netlistcell = this;
	}
	
	virtual std::string GetTopLevelNetName();
	
	FCCoolRunnerIIMacrocell::XorinModes m_xorin;
	
	void WriteConfig();
	
	//TODO: Support product-term-C path
	
protected:
	FCCoolRunnerIIOrTerm* m_orterm;
	FCCoolRunnerIIIOB* m_iob;
	FCCoolRunnerIIMacrocell* m_macrocell;
};

#endif
