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
#include "FCCoolRunnerIIZIABlock.h" 

using namespace std;
	
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Construction / destruction

FCCoolRunnerIIZIABlock::FCCoolRunnerIIZIABlock(FCCoolRunnerIIDevice* parent, int bitwidth)
	: m_parent(parent)
	, m_bitwidth(bitwidth)
	, m_inputcount(0)	//updated in CreateModel*
{
}

FCCoolRunnerIIZIABlock::~FCCoolRunnerIIZIABlock()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Initialization

void FCCoolRunnerIIZIABlock::CreateModelXC2C32A()
{
	m_inputcount = 65;		//(16 FF * 2 FB) + (16 GPIO * 2 FB) + 1 dedicated input
	
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// ZIA via matrix reverse engineered from SEM photos of M3-M4 vias
	
	static const int matrix[40][6] = 
	{
		//Rows 0-9
		{ 0, 10, 22, 34, 46, 58 },
		{ 1, 11, 23, 41, 48, 61 },
		{ 2, 12, 30, 35, 53, 60 },
		{ 3, 13, 26, 42, 47, 55 },
		{ 4, 14, 28, 38, 44, 59 },
		{ 5, 15, 31, 40, 50, 56 },
		{ 6, 16, 21, 33, 52, 62 },
		{ 7, 17, 27, 32, 45, 64 },
		{ 8, 18, 25, 39, 43, 57 },
		{ 9, 19, 24, 37, 51, 54 },
		
		//Rows 10-19
		{ 7, 20, 29, 36, 49, 63 },
		{ 0, 11, 23, 35, 47, 59 },
		{ 1, 12, 30, 37, 50, 64 },
		{ 2, 19, 24, 42, 49, 62 },
		{ 3, 15, 31, 36, 44, 61 },
		{ 4, 17, 27, 33, 48, 56 },
		{ 5, 20, 29, 39, 45, 60 },
		{ 6, 10, 22, 41, 51, 57 },
		{ 7, 16, 21, 34, 53, 63 },
		{ 8, 14, 28, 32, 46, 55 },
		
		//Rows 20-29
		{ 9, 13, 26, 40, 43, 58 },
		{ 8, 18, 25, 38, 52, 54 },
		{ 0, 12, 24, 36, 48, 60 },
		{ 1, 19, 26, 39, 53, 54 },
		{ 2, 13, 31, 38, 51, 55 },
		{ 3, 20, 25, 33, 50, 63 },
		{ 4, 16, 22, 37, 45, 62 },
		{ 5, 18, 28, 34, 49, 57 },
		{ 6, 11, 30, 40, 46, 61 },
		{ 7, 10, 23, 42, 52, 58 },
		
		//Rows 30-39
		{ 8, 17, 21, 35, 44, 64 },
		{ 9, 15, 29, 32, 47, 56 },
		{ 9, 14, 27, 41, 43, 59 },
		{ 0, 13, 25, 37, 49, 61 },
		{ 1, 15, 28, 42, 43, 60 },
		{ 2, 20, 27, 40, 44, 54 },
		{ 3, 14, 22, 39, 52, 56 },
		{ 4, 11, 26, 34, 51, 64 },
		{ 5, 17, 23, 38, 46, 63 },
		{ 6, 19, 29, 35, 50, 58 }
	};
	
	for(int row=0; row<40; row++)
	{
		for(int col=0; col<6; col++)
			m_matrix[row].push_back(matrix[row][col]);
	}
	
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Add input sources (order must not be changed, this is the actual ordering of the bus wires in the ZIA!)
	
	//FB1 input pins
	for(int i=0; i<16; i++)
		m_sources.push_back(&m_parent->GetFB(0)->m_macrocells[i].m_iob);
		
	//Dedicated input
	m_sources.push_back(NULL);
	
	//FB2 input pins
	for(int i=0; i<16; i++)
		m_sources.push_back(&m_parent->GetFB(1)->m_macrocells[i].m_iob);
		
	//FB1 and FB2 flipflops
	/*
	for(int i=0; i<16; i++)
		m_sources.push_back(&m_parent->GetFB(0)->m_macrocells[i]);
	for(int i=0; i<16; i++)
		m_sources.push_back(&m_parent->GetFB(1)->m_macrocells[i]);
	*/
	for(int i=0; i<16; i++)
		m_sources.push_back(NULL);
	for(int i=0; i<16; i++)
		m_sources.push_back(NULL);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Accessors

bool FCCoolRunnerIIZIABlock::GetZIAConfigBit(int fb, int pos, int row)
{
	return m_parent->GetConfigBit(m_parent->GetConfigBitBase(fb) + m_bitwidth*row + pos);
}

bool FCCoolRunnerIIZIABlock::IsOutputUsed(int fb, int row)
{
	switch(m_bitwidth)
	{
	//XC2C32[A]
	//Leftmost bit is active-high "pull to zero", which needs to be off.
	//TODO: Fail DRC if none of the outputs are enabled (floating input).
	case 8:
		return !GetZIAConfigBit(fb, 0, row);
	
	default:
		//die... structure for other chips hasn't yet been reversed
		assert(false);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Rendering

pair<int, int> FCCoolRunnerIIZIABlock::GetSize()
{
	return pair<int, int>(
		m_inputcount * FCCoolRunnerIIDevice::zia_colwidth + 2*FCCoolRunnerIIDevice::zia_xmargin,
		FCCoolRunnerIIDevice::fb_height);
}

void FCCoolRunnerIIZIABlock::Render(const Cairo::RefPtr<Cairo::Context>& cr, float /*scale*/, int xoff, int yoff)
{
	cr->save();
	xoff += FCCoolRunnerIIDevice::zia_xmargin;
	cr->translate(xoff, yoff);
	
	int width = m_inputcount * FCCoolRunnerIIDevice::zia_colwidth;
	
	//Draw outline of entire ZIA
	cr->set_source_rgb(1, 1, 0);
	cr->set_line_width(2);
	cr->move_to(0,      0);
	cr->line_to(width,	0);
	cr->line_to(width,	FCCoolRunnerIIDevice::fb_height);
	cr->line_to(0,		FCCoolRunnerIIDevice::fb_height);
	cr->line_to(0,       0);
	cr->stroke();
	
	//Draw columns
	for(int col=0; col<m_inputcount; col++)
	{
		//Invert column count because ZIA counts from right to left
		int icol = (m_inputcount - 1 - col);
		
		//Center of the column
		int x = FCCoolRunnerIIDevice::zia_colwidth*icol + (FCCoolRunnerIIDevice::zia_colwidth/2);
		
		//Draw the line
		cr->set_line_width(2);
		cr->set_source_rgb(0.5, 0.5, 0.5);
		cr->move_to(x, 0);
		cr->line_to(x, FCCoolRunnerIIDevice::fb_height);
		cr->stroke();
		
		/*
		//Draw net names
		FCCoolRunnerIINetSource* source = m_sources[col];
		if(source != NULL)
		{
			for(int y=0; y<FCCoolRunnerIIDevice::fb_height; y += FCCoolRunnerIIDevice::net_label_spacing)
			{
				if(y == 0)
					continue;
				
				cr->set_source_rgb(0, 0, 0);
				DrawStringCenteredVertical(
					x-2, x+2, y, y+FCCoolRunnerIIDevice::net_label_spacing,
					cr, source->GetName(), "sans bold 3");
			}
		}
		*/
	}
	
	//Draw row stuff
	for(int row=0; row<40; row++)
	{
		//Center of the row
		int y = FCCoolRunnerIIDevice::fb_androwheight*row + FCCoolRunnerIIDevice::fb_androwheight/2;
		if(row >= 20)
			y += FCCoolRunnerIIDevice::fb_orheight + FCCoolRunnerIIDevice::fb_androwheight;

		//Draw dots at intersections
		for(auto col : m_matrix[row])
		{
			//Invert column count because ZIA counts from right to left
			int icol = (m_inputcount - 1 - col);
			
			//Center of the column
			int x = FCCoolRunnerIIDevice::zia_colwidth*icol + (FCCoolRunnerIIDevice::zia_colwidth/2);
			
			//Draw the dot
			cr->set_line_width(1);
			cr->set_source_rgb(0.5, 0.5, 0.5);	//TODO: color when active
			cr->arc(x, y, FCCoolRunnerIIDevice::fb_dotrad, 0, 2*M_PI);
			cr->fill();
		}
		
		//Draw output lines feeding out to the PLA AND array.
		cr->set_line_width(1);
		cr->set_source_rgb(0.2, 0.8, 0.2);
		cr->move_to(FCCoolRunnerIIDevice::zia_colwidth/2,			y + FCCoolRunnerIIDevice::fb_netspacing);	//Right feeding
		cr->line_to(width - FCCoolRunnerIIDevice::fb_axmargin, 		y + FCCoolRunnerIIDevice::fb_netspacing);
		cr->line_to(width - FCCoolRunnerIIDevice::fb_axmargin, 		y);
		cr->line_to(width - FCCoolRunnerIIDevice::zia_xmargin,		y);		
		cr->move_to(width - FCCoolRunnerIIDevice::zia_colwidth/2,	y - FCCoolRunnerIIDevice::fb_netspacing);	//Left feeding
		cr->line_to(FCCoolRunnerIIDevice::fb_axmargin,				y - FCCoolRunnerIIDevice::fb_netspacing);
		cr->line_to(FCCoolRunnerIIDevice::fb_axmargin,				y);
		cr->line_to(-FCCoolRunnerIIDevice::zia_xmargin,				y);
		cr->stroke();
	}

	//Prepare to draw global config block
	int left = FCCoolRunnerIIDevice::fb_axmargin;
	int right = width - FCCoolRunnerIIDevice::fb_axmargin;		
	int top = FCCoolRunnerIIDevice::fb_axmargin + FCCoolRunnerIIDevice::fb_andblockheight;
	int bottom = top + FCCoolRunnerIIDevice::fb_orheight - FCCoolRunnerIIDevice::fb_axmargin;
	
	//Clear it
	cr->set_source_rgb(0, 0, 0);
	cr->move_to(left,	top);
	cr->line_to(right,	top);
	cr->line_to(right,	bottom);
	cr->line_to(left,	bottom);
	cr->line_to(left,  	top);
	cr->fill();
	
	//Fill it and draw text
	cr->set_line_width(2);
	cr->set_source_rgb(0, 1, 1);
	cr->move_to(left,	top);
	cr->line_to(right,	top);
	cr->line_to(right,	bottom);
	cr->line_to(left,	bottom);
	cr->line_to(left,  	top);
	cr->stroke();
	DrawStringCentered(left, right, top, bottom, cr, "GLOBAL CONFIG", "sans bold 24");
	
	cr->restore();
}
