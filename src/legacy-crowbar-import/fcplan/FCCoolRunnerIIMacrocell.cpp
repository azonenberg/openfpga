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
#include "FCCoolRunnerIIMacrocell.h"

using namespace std;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Construction / destruction

FCCoolRunnerIIMacrocell::FCCoolRunnerIIMacrocell(FCCoolRunnerIIFunctionBlock* parent)
	: m_iob(parent->GetDevice(), this)
	, m_parent(parent)
	, m_cellnum(0)
	, m_cfgwidth(0)
	, m_cfgcount(0)
{
}

FCCoolRunnerIIMacrocell::~FCCoolRunnerIIMacrocell()
{
}

void FCCoolRunnerIIMacrocell::SetCellIndex(int i)
{
	m_cellnum = i;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Rendering

pair<int, int> FCCoolRunnerIIMacrocell::GetSize()
{
	return pair<int, int>(FCCoolRunnerIIDevice::fb_mcwidth, FCCoolRunnerIIDevice::fb_mcheight);
}

void FCCoolRunnerIIMacrocell::Render(const Cairo::RefPtr<Cairo::Context>& cr, int xoff, int yoff)
{
	cr->save();
	cr->translate(xoff, yoff);
	
	int left = FCCoolRunnerIIDevice::fb_axmargin;
	int right = FCCoolRunnerIIDevice::fb_mcwidth - FCCoolRunnerIIDevice::fb_axmargin;
	int top = FCCoolRunnerIIDevice::fb_mcymargin;
	int bottom = FCCoolRunnerIIDevice::fb_mcheight - FCCoolRunnerIIDevice::fb_mcymargin;
	
	//Draw outline of entire macrocell
	cr->set_source_rgb(1, 0, 1);
	cr->set_line_width(2);
	cr->move_to(left,	top);
	cr->line_to(right,	top);
	cr->line_to(right,	bottom);
	cr->line_to(left,	bottom);
	cr->line_to(left,	top);
	cr->stroke();
	
	//Print name
	char sbuf[16];
	snprintf(sbuf, sizeof(sbuf), "MC %d", m_cellnum + 1);
	
	//Draw text
	DrawStringCentered(left, right, top, bottom, cr, sbuf, "sans bold 18");
	
	cr->restore();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Initialization

void FCCoolRunnerIIMacrocell::CreateModelXC2C32A()
{
	m_cfgwidth = 9;
	m_cfgcount = 27;
}
