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
	@brief Declaration of FCCoolRunnerIINetlist
 */

#ifndef FCCoolRunnerIINetlist_h
#define FCCoolRunnerIINetlist_h

class FCCoolRunnerIIDevice;

#include "FCCoolRunnerIIOrTerm.h"
#include "FCCoolRunnerIIProductTerm.h"
#include "FCCoolRunnerIINetlistMacrocell.h"

/**
	@brief A technology-dependent, mapped, netlist
 */
class FCCoolRunnerIINetlist
{
public:
	FCCoolRunnerIINetlist(FCCoolRunnerIIDevice* device);
	
	virtual ~FCCoolRunnerIINetlist();
	
	FCCoolRunnerIIProductTerm* CreateProductTerm()
	{
		FCCoolRunnerIIProductTerm* pterm = new FCCoolRunnerIIProductTerm;
		m_pterms.push_back(pterm);
		return pterm;
	}
	
	FCCoolRunnerIIOrTerm* CreateOrTerm()
	{
		FCCoolRunnerIIOrTerm* oterm = new FCCoolRunnerIIOrTerm;
		m_orterms.push_back(oterm);
		return oterm;
	}
	
	FCCoolRunnerIINetlistMacrocell* CreateMacrocell(FCCoolRunnerIIOrTerm* orterm, FCCoolRunnerIIIOB* iob)
	{
		FCCoolRunnerIINetlistMacrocell* cell = new FCCoolRunnerIINetlistMacrocell(orterm, iob);
		m_macrocells.push_back(cell);
		return cell;
	}

	std::vector<FCCoolRunnerIIProductTerm*> m_pterms;
	std::vector<FCCoolRunnerIIOrTerm*> m_orterms;
	std::vector<FCCoolRunnerIINetlistMacrocell*> m_macrocells;
};

#endif

