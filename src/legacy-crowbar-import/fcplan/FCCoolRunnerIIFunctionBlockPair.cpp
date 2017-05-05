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

#include "fcplan.h"
#include "FCCoolRunnerIIFunctionBlockPair.h"

using namespace std;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Construction / destruction

FCCoolRunnerIIFunctionBlockPair::FCCoolRunnerIIFunctionBlockPair(FCCoolRunnerIIDevice* parent, int ziawidth)
	: m_left(parent, &m_zia, false)
	, m_zia(parent, ziawidth)
	, m_right(parent, &m_zia, true)
	, m_parent(parent)
{
}

FCCoolRunnerIIFunctionBlockPair::~FCCoolRunnerIIFunctionBlockPair()
{
	
}

void FCCoolRunnerIIFunctionBlockPair::CreateModelXC2C32A()
{
	m_zia.CreateModelXC2C32A();
	m_left.CreateModelXC2C32A();
	m_right.CreateModelXC2C32A();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Rendering

pair<int, int> FCCoolRunnerIIFunctionBlockPair::GetSize()
{
	auto sl = m_left.GetSize();
	auto sz = m_zia.GetSize();
	auto sr = m_right.GetSize();
	
	return pair<int, int>(sl.first + sz.first + sr.first, sl.second);
}

void FCCoolRunnerIIFunctionBlockPair::Render(const Cairo::RefPtr<Cairo::Context>& cr, float scale, int xoff, int yoff)
{
	auto sl = m_left.GetSize();
	auto sz = m_zia.GetSize();
	
	m_left.Render(cr, scale, xoff, yoff);
	xoff += sl.first;
	m_zia.Render(cr, scale, xoff, yoff);
	xoff += sz.first;
	m_right.Render(cr, scale, xoff, yoff);
}
