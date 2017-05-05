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
#include "FCCoolRunnerIIDevice.h"
#include "FCCoolRunnerIIDevice.h"

using namespace std;

FCCoolRunnerIIDevice::FCCoolRunnerIIDevice(CPLDBitstream* bitstream)
	: FCXilinxDevice(bitstream)
{	
	//Look up the package
	/*
	int package = 0;
	if(bitstream->devname.find("QF32") != string::npos)
		package = XilinxCoolRunnerIIDevice::QFG32;
	else if(bitstream->devname.find("VQ44") != string::npos)
		package = XilinxCoolRunnerIIDevice::VQG44;
	else if(bitstream->devname.find("QF48") != string::npos)
		package = XilinxCoolRunnerIIDevice::QFG48;
	else if(bitstream->devname.find("CP56") != string::npos)
		package = XilinxCoolRunnerIIDevice::CPG56;
	else if(bitstream->devname.find("VQ100") != string::npos)
		package = XilinxCoolRunnerIIDevice::VQG100;
	else if(bitstream->devname.find("CP132") != string::npos)
		package = XilinxCoolRunnerIIDevice::CPG132;
	else if(bitstream->devname.find("TQ144") != string::npos)
		package = XilinxCoolRunnerIIDevice::TQG144;
	else if(bitstream->devname.find("PQ208") != string::npos)
		package = XilinxCoolRunnerIIDevice::PQG208;
	else if(bitstream->devname.find("FT256") != string::npos)
		package = XilinxCoolRunnerIIDevice::FTG256;
	else if(bitstream->devname.find("FG324") != string::npos)
		package = XilinxCoolRunnerIIDevice::FGG324;
	else
	{
		throw JtagExceptionWrapper(
			"Invalid package name (doesn't match a known CoolRunner-II part)",
			"",
			JtagException::EXCEPTION_TYPE_GIGO);
	}
	*/
	
	//Create the device model
	if(bitstream->devname.find("XC2C32A") == 0)
		CreateModelXC2C32A();
	else
	{
		throw JtagExceptionWrapper(
			"Invalid device name (doesn't match a known CoolRunner-II part)",
			"",
			JtagException::EXCEPTION_TYPE_GIGO);
	}
	
	//TODO: Load from the bitstream
}

FCCoolRunnerIIDevice::~FCCoolRunnerIIDevice()
{
	for(auto x : m_fbpairs)
		delete x;
	m_fbpairs.clear();
	
	//Clear helpers
	m_fbs.clear();
	m_fbgrid.clear();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Device creation

void FCCoolRunnerIIDevice::CreateModelXC2C32A()
{
	//Set up shift register size
	m_shregwidth = 260;
	m_shregdepth = 48;
	
	//One FB pair, 8 bit ZIA
	auto pair = new FCCoolRunnerIIFunctionBlockPair(this, 8);
	m_fbpairs.push_back(pair);
	m_fbs.push_back(&pair->m_right);	//FB1
	m_fbs.push_back(&pair->m_left);		//FB2
	m_fbgrid[0][0] = pair;
	
	//Number all FBs
	for(size_t i=0; i<m_fbs.size(); i++)
		m_fbs[i]->SetBlockIndex(i);
		
	//Set X/Y coordinates
	//Note that by our convention, X-coordinate 0 is the first bit to go into the shift register (right side of die).
	m_fbs[0]->SetBlockPosition(0, 0);
	m_fbs[1]->SetBlockPosition(1, 0);
	
	//Create device model
	//Needs to happen after all FBs have been created
	pair->CreateModelXC2C32A();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Rendering

pair<int, int> FCCoolRunnerIIDevice::GetSize(float scale)
{
	int width = 0;
	int height = 0;

	//Look up size of one FB (assume all are equal)
	auto fbsize = m_fbgrid[0][0]->GetSize();

	//Add in device size (TODO: margins etc)
	width += m_fbgrid[0].size() * fbsize.first;
	height += m_fbgrid.size() * fbsize.second;
	
	//Everything before here is scaled
	width *= scale;
	height *= scale;
	
	//Add on margins (unscaled)
	width += 2 * device_xmargin;
	height += 2 * device_ymargin;
	
	return pair<int, int>(width, height);
}

void FCCoolRunnerIIDevice::Render(const Cairo::RefPtr<Cairo::Context>& cr, float scale)
{
	float y = device_ymargin;
	for(auto rowi : m_fbgrid)
	{
		float x = device_xmargin;
		for(auto celli : rowi.second)
		{
			celli.second->Render(cr, scale, x, y);
			x += celli.second->GetSize().first;	//TODO: spacing
		}
		y += rowi.second[0]->GetSize().second;		//TODO: spacing
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Accessors

int FCCoolRunnerIIDevice::GetShiftRegisterWidth()
{
	return m_shregwidth;
}

int FCCoolRunnerIIDevice::GetShiftRegisterDepth()
{
	return m_shregdepth;
}

int FCCoolRunnerIIDevice::GetConfigBitBase(int fb)
{
	const int pterm_count = 56;
	const int zia_row_count = 40;
	const int macrocell_count = 16;
	const int macrocell_cfg_width = m_fbs[0]->m_macrocells[0].GetConfigBitCount();
	const int macrocell_cfg_count = macrocell_cfg_width * macrocell_count;
	const int zia_cfg_width =  m_fbs[0]->GetZIA()->GetConfigBitWidth();	
	const int zia_cfg_count = zia_cfg_width * zia_row_count;
	const int and_cfg_count = zia_row_count*2*pterm_count;
	const int or_cfg_count = macrocell_count*pterm_count;
	
	const int fb_cfg_count = macrocell_cfg_count + zia_cfg_count + and_cfg_count + or_cfg_count;
	
	//Start of config bits for this block
	return fb * fb_cfg_count;
}
