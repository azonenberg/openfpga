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

#ifndef FCCoolRunnerIIMacrocell_h
#define FCCoolRunnerIIMacrocell_h

#include "FCCoolRunnerIIIOB.h"

class FCCoolRunnerIIFunctionBlock;

/*
	@brief Base class for all CR-II macrocells
	
	TODO: derive classes or have flags for the different types.
	
	Right now it seems there are three kinds of macrocell:
		* Low-end devices, all MCs: 27 bits
		* High-end devices, buried MCs: 15 bits
		* High-end devices, bonded MCs: 29 bits
		
	Each support different subsets of the available features.
 */
class FCCoolRunnerIIMacrocell
{
public:
	FCCoolRunnerIIMacrocell(FCCoolRunnerIIFunctionBlock* parent);
	virtual ~FCCoolRunnerIIMacrocell();

	void SetCellIndex(int i);
	
	int GetCellIndex()
	{ return m_cellnum; }
	
	FCCoolRunnerIIFunctionBlock* GetFB()
	{ return m_parent; }

	virtual std::pair<int, int> GetSize();
	virtual void Render(const Cairo::RefPtr<Cairo::Context>& cr, int xoff, int yoff);
	
	FCCoolRunnerIIIOB m_iob;
	
	void CreateModelXC2C32A();
	
	int GetConfigBitWidth()
	{ return m_cfgwidth; }
	
	int GetConfigBitCount()
	{ return m_cfgcount; }
	
protected:
	FCCoolRunnerIIFunctionBlock* m_parent;
	
	int m_cellnum;
	
	/**
		@brief Width of our config SRAM, in bits.
		
		Note that the config SRAM is always three bits high in all devices (3*16 = 48 config rows per FB).
		
		Sometimes there are unused cells, meaning m_cfgcount may be slightly less than m_cfgwidth*3.
	 */
	int m_cfgwidth;
	
	/**
		@brief Total number of *used* bits in our config SRAM.
	 */
	int m_cfgcount;
};

#endif
