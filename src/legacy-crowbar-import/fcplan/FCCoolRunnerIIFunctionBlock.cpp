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
#include "FCCoolRunnerIIFunctionBlock.h"

using namespace std;

FCCoolRunnerIIFunctionBlock::FCCoolRunnerIIFunctionBlock(
	FCCoolRunnerIIDevice* parent,
	FCCoolRunnerIIZIABlock* zia,
	bool mirror)
	: m_macrocells({
			this, this, this, this, this, this, this, this,
			this, this, this, this, this, this, this, this
		})
	, m_parent(parent)
	, m_zia(zia)
	, m_mirror(mirror)
	, m_blocknum(0)
	, m_xpos(0)
	, m_ypos(0)
{
	for(int i=0; i<16; i++)
		m_macrocells[i].SetCellIndex(i);
}

FCCoolRunnerIIFunctionBlock::~FCCoolRunnerIIFunctionBlock()
{
}

void FCCoolRunnerIIFunctionBlock::CreateModelXC2C32A()
{
	for(int i=0; i<16; i++)
		m_macrocells[i].CreateModelXC2C32A();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Rendering

pair<int, int> FCCoolRunnerIIFunctionBlock::GetSize()
{
	return pair<int, int>(
		FCCoolRunnerIIDevice::fb_width + 2*FCCoolRunnerIIDevice::fb_xmargin,
		FCCoolRunnerIIDevice::fb_height);
}

void FCCoolRunnerIIFunctionBlock::Render(const Cairo::RefPtr<Cairo::Context>& cr, float scale, int xoff, int yoff)
{
	//Skip left side margin
	cr->save();
	cr->translate(xoff, yoff);
	
	//Draw outline of the entire FB
	cr->set_source_rgb(0, 0, 1);
	cr->set_line_width(2);
	int tleft = FCCoolRunnerIIDevice::fb_xmargin;
	int tright = FCCoolRunnerIIDevice::fb_width - FCCoolRunnerIIDevice::fb_xmargin;
	cr->move_to(tleft,      0);
	cr->line_to(tright,		0);
	cr->line_to(tright,		FCCoolRunnerIIDevice::fb_height);
	cr->line_to(tleft,		FCCoolRunnerIIDevice::fb_height);
	cr->line_to(tleft,   	0);
	cr->stroke();

	//Set up left/right positions for the PLA
	int left;
	int right;
	if(!m_mirror)
	{
		left = tleft + FCCoolRunnerIIDevice::fb_mcwidth;
		right = tright - FCCoolRunnerIIDevice::fb_axmargin;
	}
	else
	{
		right = tright - FCCoolRunnerIIDevice::fb_mcwidth;
		left = tleft + FCCoolRunnerIIDevice::fb_axmargin;
	}
	
	//Draw the OR array block
	cr->set_source_rgb(0, 0, 1);
	cr->set_line_width(2);
	int top = FCCoolRunnerIIDevice::fb_axmargin + FCCoolRunnerIIDevice::fb_andblockheight;
	int bottom = top + FCCoolRunnerIIDevice::fb_orheight - FCCoolRunnerIIDevice::fb_axmargin;
	cr->set_line_width(2);
	cr->move_to(left, 	top);
	cr->line_to(right,	top);
	cr->line_to(right,	bottom);
	cr->line_to(left,	bottom);
	cr->line_to(left,  	top);
	cr->stroke();
	DrawStringCentered(left, right, top, bottom, cr, "PLA OR", "sans bold 32");

	//Draw the PLA AND array
	RenderANDArray(cr, scale, left, right);
	
	//Draw the PLA OR array
	RenderORArray(cr, scale, left, right);
	
	//Draw the macrocells
	if(!m_mirror)
		left = tleft;
	else
		left = tright - FCCoolRunnerIIDevice::fb_mcwidth;
	for(int i=0; i<16; i++)
		m_macrocells[i].Render(cr, left, i*FCCoolRunnerIIDevice::fb_mcheight);
	
	cr->restore();
}

void FCCoolRunnerIIFunctionBlock::RenderANDArray(const Cairo::RefPtr<Cairo::Context>& cr, float scale, int left, int right)
{
	//Draw the outline for the upper AND array block
	cr->set_source_rgb(0, 0, 1);
	cr->set_line_width(2);
	int top = FCCoolRunnerIIDevice::fb_axmargin;
	int bottom = FCCoolRunnerIIDevice::fb_andblockheight - FCCoolRunnerIIDevice::fb_axmargin;
	cr->move_to(left, 	top);
	cr->line_to(right,	top);
	cr->line_to(right,	bottom);
	cr->line_to(left,	bottom);
	cr->line_to(left,  	top);
	cr->stroke();
	
	//Draw the outline for the lower AND array block
	top += FCCoolRunnerIIDevice::fb_orheight + FCCoolRunnerIIDevice::fb_andblockheight;
	bottom = top + FCCoolRunnerIIDevice::fb_andblockheight - 2*FCCoolRunnerIIDevice::fb_axmargin;
	cr->move_to(left, 	top);
	cr->line_to(right,	top);
	cr->line_to(right,	bottom);
	cr->line_to(left,	bottom);
	cr->line_to(left,  	top);
	cr->stroke();
	
	//Get clip area for visibility culling
	double clip_x1, clip_y1, clip_x2, clip_y2;
	cr->get_clip_extents(clip_x1, clip_y1, clip_x2, clip_y2);
	
	//Draw outlines of columns
	for(int col=0; col<56; col++)
	{
		//See if we need to invert the column ordering
		int icol = col;
		if(m_mirror)
			icol = 55 - col;
			
		//Top block
		int xleft = left + FCCoolRunnerIIDevice::fb_placolwidth*icol + FCCoolRunnerIIDevice::fb_axmargin/2;
		int xright = xleft + FCCoolRunnerIIDevice::fb_placolwidth - FCCoolRunnerIIDevice::fb_axmargin;
		top = FCCoolRunnerIIDevice::fb_axmargin*2;
		bottom = FCCoolRunnerIIDevice::fb_andblockheight - FCCoolRunnerIIDevice::fb_axmargin;
		
		//Draw outline
		cr->set_source_rgb(0, 0, 1);
		cr->set_line_width(1);
		cr->move_to(xleft, 	top);
		cr->line_to(xright,	top);
		cr->line_to(xright,	bottom);
		cr->line_to(xleft,	bottom);
		cr->line_to(xleft,  	top);
		cr->stroke();
		
		//Bottom block
		top += FCCoolRunnerIIDevice::fb_orheight + FCCoolRunnerIIDevice::fb_andblockheight;
		bottom = top + FCCoolRunnerIIDevice::fb_andblockheight - 2*FCCoolRunnerIIDevice::fb_axmargin;
		
		//Draw outline
		cr->set_source_rgb(0, 0, 1);
		cr->set_line_width(1);
		cr->move_to(xleft, 	top);
		cr->line_to(xright,	top);
		cr->line_to(xright,	bottom);
		cr->line_to(xleft,	bottom);
		cr->line_to(xleft,  	top);
		cr->stroke();
	}
	
	//Draw low-detail version if zoomed out
	if(scale < 0.6)
	{
		//Draw outlines of columns
		for(int col=0; col<56; col++)
		{
			//See if we need to invert the column ordering
			int icol = col;
			if(m_mirror)
				icol = 55 - col;
				
			//Top block
			int xleft = left + FCCoolRunnerIIDevice::fb_placolwidth*icol + FCCoolRunnerIIDevice::fb_axmargin/2;
			int xright = xleft + FCCoolRunnerIIDevice::fb_placolwidth - FCCoolRunnerIIDevice::fb_axmargin;
			top = FCCoolRunnerIIDevice::fb_axmargin*2;
			bottom = FCCoolRunnerIIDevice::fb_andblockheight - FCCoolRunnerIIDevice::fb_axmargin;
			
			//Draw text
			char str[128];
			snprintf(str, sizeof(str), "PTERM_%d\n", col+1);
			cr->set_source_rgb(1, 1, 1);
			DrawStringCenteredVertical(xleft, xright, top, bottom, cr, str, "sans normal 24");
			
			//Bottom block
			top += FCCoolRunnerIIDevice::fb_orheight + FCCoolRunnerIIDevice::fb_andblockheight;
			bottom = top + FCCoolRunnerIIDevice::fb_andblockheight - 2*FCCoolRunnerIIDevice::fb_axmargin;
			
			//Text
			cr->set_source_rgb(1, 1, 1);
			DrawStringCenteredVertical(xleft, xright, top, bottom, cr, str, "sans normal 24");
		}
		
		return;
	}
		
	//otherwise draw full rows
	else
	{
		//Check if each pterm is used at all
		bool term_used_arr[56] = {0};
		for(int pterm=0; pterm<56; pterm++)
		{
			for(int nin=0; nin<80; nin++)
			{
				if(!GetANDArrayConfigBit(pterm, nin))
					term_used_arr[pterm] = true;
			}
		}
		
		for(int row=0; row<40; row++)
		{
			//Center of the row
			int y = FCCoolRunnerIIDevice::fb_androwheight*row + FCCoolRunnerIIDevice::fb_androwheight/2;
			if(row >= 20)
				y += FCCoolRunnerIIDevice::fb_orheight + FCCoolRunnerIIDevice::fb_androwheight;
				
			//Stop if this row is offscreen
			if( ((y - FCCoolRunnerIIDevice::fb_androwheight) > clip_y2) ||
				((y + FCCoolRunnerIIDevice::fb_androwheight) < clip_y1)
			)
			{
				continue;
			}
			
			//Draw input lines from the ZIA
			cr->set_line_width(1);
			SetColor(cr, m_zia->IsOutputUsed(m_blocknum, row));
			cr->move_to(left, y);
			cr->line_to(right, y);
			if(!m_mirror)
			{
				cr->move_to(right, y);
				cr->line_to(
					FCCoolRunnerIIDevice::fb_width + FCCoolRunnerIIDevice::fb_xmargin + FCCoolRunnerIIDevice::zia_xmargin,
					y);
			}
			else
			{
				cr->move_to(left, y);
				cr->line_to(
					-(FCCoolRunnerIIDevice::fb_xmargin + FCCoolRunnerIIDevice::zia_xmargin),
					y);
			}
			cr->stroke();
			
			for(int col=0; col<56; col++)
			{
				//See if we need to invert the column ordering
				int icol = col;
				if(m_mirror)
					icol = 55 - col;
					
				//Center of the column
				int x =
					left + 
					FCCoolRunnerIIDevice::fb_placolwidth*icol + (FCCoolRunnerIIDevice::fb_placolwidth/2);
					
				//Stop if this column is offscreen
				if( ((x - FCCoolRunnerIIDevice::fb_placolwidth) > clip_x2) ||
					((x + FCCoolRunnerIIDevice::fb_placolwidth) < clip_x1)
				)
				{
					continue;
				}
					
				cr->save();
				cr->translate(x, y);
				if(row >= 20)			//Invert upside-down rows in bottom half of PLA
					cr->rotate(M_PI);
				
				//Check if this cell is used at all
				bool cfg_and_true = !GetANDArrayConfigBit(col, row*2);
				bool cfg_and_false = !GetANDArrayConfigBit(col, row*2 + 1);
				bool cell_used = cfg_and_true || cfg_and_false;
				bool term_used = term_used_arr[col];
				
				//Prepare to draw muxes
				cr->set_line_width(1);
				
				//Lines hanging down to the mux
				int muxtop = FCCoolRunnerIIDevice::fb_muxpinlen;
				int muxbot = muxtop + FCCoolRunnerIIDevice::fb_muxheight;
				int muxbtop = FCCoolRunnerIIDevice::fb_muxpinlen - FCCoolRunnerIIDevice::fb_bubblerad*2;
				int muxinleft = - FCCoolRunnerIIDevice::fb_muxpinpitch;
				int muxinright = FCCoolRunnerIIDevice::fb_muxpinpitch;
				
				SetColor(cr, cfg_and_true);
				cr->move_to(muxinleft, 0);
				cr->line_to(muxinleft, muxtop);
				cr->stroke();
				
				SetColor(cr, cfg_and_false);
				cr->move_to(muxinright, 0);
				cr->line_to(muxinright, muxbtop);
				cr->stroke();
				
				//Inverter bubble on right side input (keep color from rightmost input)
				int muxbcent = FCCoolRunnerIIDevice::fb_muxpinlen - FCCoolRunnerIIDevice::fb_bubblerad;
				cr->set_line_width(1);
				cr->arc(muxinright, muxbcent, FCCoolRunnerIIDevice::fb_bubblerad, 0, 2*M_PI);
				cr->stroke();
				
				//The mux itself
				SetColor(cr, cell_used);
				if(cell_used)
					DrawMux(cr, 0, muxtop, cfg_and_true ? 1 : 2);
				else
					DrawMux(cr, 0, muxtop, 0);
				
				//Line from the mux to the AND gate
				int andtop = muxbot + FCCoolRunnerIIDevice::fb_andpinlen;
				cr->move_to(0, muxbot);
				cr->line_to(0, andtop);
				cr->stroke();
							
				//Input to the AND gate from the previous stage
				int andin2 = 2*FCCoolRunnerIIDevice::fb_andpinpitch;
				int androt = muxbot + FCCoolRunnerIIDevice::fb_andpinlen/2;
				SetColor(cr, cell_used);
				cr->move_to(andin2, andtop);
				cr->line_to(andin2, androt);
				cr->stroke();
				
				//Then draw the AND gate itself
				int andx = FCCoolRunnerIIDevice::fb_andpinpitch;
				DrawAND(cr, andx, andtop);
								
				//Mux to bypass the AND gate
				int andbot = andtop + FCCoolRunnerIIDevice::fb_andbodylen + 2*FCCoolRunnerIIDevice::fb_andpinpitch;
				int mux2top = andbot + FCCoolRunnerIIDevice::fb_muxpinlen;
				int mux2x = andx + FCCoolRunnerIIDevice::fb_muxpinpitch;
				int mux2rot = andbot + FCCoolRunnerIIDevice::fb_muxpinlen/2;
				int mux2x2 = mux2x + FCCoolRunnerIIDevice::fb_muxpinpitch;
				
				SetColor(cr, cell_used);
				cr->move_to(andx, andbot);
				cr->line_to(andx, mux2top);
				cr->stroke();
				SetColor(cr, term_used && !cell_used);
				cr->move_to(mux2x2, mux2rot);
				cr->line_to(mux2x2, mux2top);
				cr->stroke();
				
				SetColor(cr, term_used);
				DrawMux(cr, mux2x, mux2top, cell_used ? 1 : 2);
				
				//Output of the mux
				int mux2bot = mux2top + FCCoolRunnerIIDevice::fb_muxheight;
				int mux2zigx = mux2x + FCCoolRunnerIIDevice::fb_andzig;
				int mux2zigy = mux2bot + FCCoolRunnerIIDevice::fb_muxpinlen;
				int rowbot = FCCoolRunnerIIDevice::fb_androwheight; 
				int jumprad = 2;
				SetColor(cr, term_used);
				cr->move_to(mux2x, mux2bot);
				cr->line_to(mux2x, mux2zigy);
				cr->line_to(mux2zigx, mux2zigy);
				cr->line_to(mux2zigx, rowbot - jumprad);
				cr->stroke();
				if( (row != 19) && (row != 20) )
				{
					cr->arc(mux2zigx, rowbot, jumprad, -M_PI_2, M_PI_2);
					cr->stroke();
				}
				
				//Line from the previous mux to this stage
				if(row != 0 && row != 40)
				{
					SetColor(cr, term_used);
					cr->move_to(mux2zigx, jumprad);
					cr->line_to(mux2zigx, androt);
					cr->stroke();
					
					SetColor(cr, cell_used);
					cr->move_to(mux2zigx, androt);
					cr->line_to(andin2, androt);
					cr->stroke();
					
					SetColor(cr, term_used && !cell_used);
					cr->move_to(mux2zigx, androt);
					cr->line_to(mux2zigx, mux2rot);
					cr->line_to(mux2x2, mux2rot);
					cr->stroke();
				}

				//Draw cell name
				char str[128];
				snprintf(str, sizeof(str), "AND_FB%d_X%dY%d\n", m_blocknum, icol+1, row+1);
				cr->set_source_rgb(1, 1, 1);
				int xleft = FCCoolRunnerIIDevice::fb_axmargin - FCCoolRunnerIIDevice::fb_placolwidth/2;
				int texty = mux2zigy + FCCoolRunnerIIDevice::fb_muxpinlen/3;
				if( (row == 19) || (row== 20) )
					texty = mux2zigy - FCCoolRunnerIIDevice::fb_muxpinlen/3;
				cr->save();
					if(row >= 20)
						xleft += FCCoolRunnerIIDevice::fb_placolwidth;
					cr->translate(xleft, texty);
					if(row >= 20)			//flip text back to right side up
						cr->rotate(M_PI);
					DrawString(0, 0, cr, str, "sans normal 2.5");
				cr->restore();
				
				cr->restore();
			}
		}
	}
}

void FCCoolRunnerIIFunctionBlock::RenderORArray(
	const Cairo::RefPtr<Cairo::Context>& /*cr*/,
	float /*scale*/,
	int /*left*/,
	int /*right*/)
{
	
}

/**
	@brief Draws a 2-input mux with two inputs and no pins.
	
	The mux's inputs are centered at (x, y) and the output is below it.
	
	sel = 0 means draw no selector
	sel = 1 means draw left
	sel = 2 means draw right
 */
void FCCoolRunnerIIFunctionBlock::DrawMux(const Cairo::RefPtr<Cairo::Context>& cr, int x, int y, int sel)
{
	int muxtopleft = x - 2*FCCoolRunnerIIDevice::fb_muxpinpitch;
	int muxtopright = x + 2*FCCoolRunnerIIDevice::fb_muxpinpitch;
	int muxbotleft = x - FCCoolRunnerIIDevice::fb_muxpinpitch;
	int muxbotright = x + FCCoolRunnerIIDevice::fb_muxpinpitch;
	int muxbot = y + FCCoolRunnerIIDevice::fb_muxheight;
	int muxin1 = x - FCCoolRunnerIIDevice::fb_muxpinpitch;
	int muxin2 = x + FCCoolRunnerIIDevice::fb_muxpinpitch;
	cr->move_to(muxtopleft,  y);
	cr->line_to(muxtopright, y);
	cr->line_to(muxbotright, muxbot);
	cr->line_to(muxbotleft,  muxbot);
	cr->line_to(muxtopleft,  y);
	
	if(sel != 0)
	{
		if(sel == 1)
			cr->move_to(muxin1, y);
		else
			cr->move_to(muxin2, y);
		cr->line_to(x, muxbot);
	}
	
	cr->stroke();
}

/**
	@brief Draws an AND gate with two inputs and no pins.
	
	The gate's inputs are centered at (x, y) and the output is below it.
 */
void FCCoolRunnerIIFunctionBlock::DrawAND(const Cairo::RefPtr<Cairo::Context>& cr, int x, int y)
{
	int andleft = x - 2*FCCoolRunnerIIDevice::fb_andpinpitch;
	int andright = x + 2*FCCoolRunnerIIDevice::fb_andpinpitch;
	int andbodybot = y + FCCoolRunnerIIDevice::fb_andbodylen;
	int andrad = 2*FCCoolRunnerIIDevice::fb_andpinpitch;
	cr->move_to(andleft, andbodybot);
	cr->line_to(andleft, y);
	cr->line_to(andright, y);
	cr->line_to(andright, andbodybot);
	cr->stroke();
	cr->arc(x, andbodybot, andrad, 0, M_PI);
	cr->stroke();
}

void FCCoolRunnerIIFunctionBlock::SetColorIdle(const Cairo::RefPtr<Cairo::Context>& cr)
{
	cr->set_source_rgb(0.25, 0.25, 0.25);
}

void FCCoolRunnerIIFunctionBlock::SetColorActive(const Cairo::RefPtr<Cairo::Context>& cr)
{
	cr->set_source_rgb(0.25, 1, 0.25);
}

void FCCoolRunnerIIFunctionBlock::SetColor(const Cairo::RefPtr<Cairo::Context>& cr, bool active)
{
	if(active)
		SetColorActive(cr);
	else
		SetColorIdle(cr);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Accessors

/**
	@brief Gets a config bit at specified 2D coordinates within this function block's AND array.
	
	The column is the product term, the row is the input bit number (input N is ??, N+1 is ??).
 */
bool FCCoolRunnerIIFunctionBlock::GetANDArrayConfigBit(int pterm, int input)
{
	const int zia_row_count = 40;
	const int zia_cfg_width = m_zia->GetConfigBitWidth();	
	const int zia_cfg_count = zia_cfg_width * zia_row_count;
	
	return m_parent->GetConfigBit(m_parent->GetConfigBitBase(m_blocknum) + zia_cfg_count + zia_row_count*2*pterm + input);
}

bool FCCoolRunnerIIFunctionBlock::GetORArrayConfigBit(int /*col*/, int /*row*/)
{
	assert(false);
	return -1;
}
