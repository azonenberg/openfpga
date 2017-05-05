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

#ifndef FCCoolRunnerIIFunctionBlock_h
#define FCCoolRunnerIIFunctionBlock_h

class FCCoolRunnerIIDevice;
class FCCoolRunnerIIZIABlock;
#include "FCCoolRunnerIIMacrocell.h"

/**
	@brief A function block
 */
class FCCoolRunnerIIFunctionBlock
{
public:
	FCCoolRunnerIIFunctionBlock(FCCoolRunnerIIDevice* parent, FCCoolRunnerIIZIABlock* zia, bool mirror);
	virtual ~FCCoolRunnerIIFunctionBlock();
	
	FCCoolRunnerIIDevice* GetDevice()
	{ return m_parent; }
	
	void SetBlockIndex(int i)
	{ m_blocknum = i; }
	
	int GetBlockIndex()
	{ return m_blocknum; }
	
	virtual std::pair<int, int> GetSize();
	virtual void Render(const Cairo::RefPtr<Cairo::Context>& cr, float scale, int xoff, int yoff);
	
	FCCoolRunnerIIMacrocell m_macrocells[16];
	
	void SetBlockPosition(int xpos, int ypos)
	{
		m_xpos = xpos;
		m_ypos = ypos;
	}

	bool GetANDArrayConfigBit(int pterm, int input);
	bool GetORArrayConfigBit(int col, int row);
	
	void CreateModelXC2C32A();
	
	FCCoolRunnerIIZIABlock* GetZIA()
	{ return m_zia; }
	
protected:
	FCCoolRunnerIIDevice* m_parent;
	
	int GetConfigBitBase();
	
	void RenderANDArray(const Cairo::RefPtr<Cairo::Context>& cr, float scale, int left, int right);
	void RenderORArray(const Cairo::RefPtr<Cairo::Context>& cr, float scale, int left, int right);
	
	void DrawMux(const Cairo::RefPtr<Cairo::Context>& cr, int x, int y, int sel);
	void DrawAND(const Cairo::RefPtr<Cairo::Context>& cr, int x, int y);
	
	FCCoolRunnerIIZIABlock* m_zia;
	
	void SetColorIdle(const Cairo::RefPtr<Cairo::Context>& cr);
	void SetColorActive(const Cairo::RefPtr<Cairo::Context>& cr);
	void SetColor(const Cairo::RefPtr<Cairo::Context>& cr, bool active);
	
	/**
		@brief Indicates if the function block is mirrored.
		
		If true, addresses and indexes generally increase left-to-right. If false, they decrease.
	 */
	bool m_mirror;
	int m_blocknum;
	
	int m_xpos;
	int m_ypos;
};

#endif


