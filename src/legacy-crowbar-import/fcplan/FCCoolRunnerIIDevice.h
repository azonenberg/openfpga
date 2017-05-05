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

#ifndef FCCoolRunnerIIDevice_h
#define FCCoolRunnerIIDevice_h

#include "FCCoolRunnerIIFunctionBlockPair.h"

/**
	@brief Base class for CoolRunner-II devices devices in FC
 */
class FCCoolRunnerIIDevice : public FCXilinxDevice
{
public:
	FCCoolRunnerIIDevice(CPLDBitstream* bitstream);
	virtual ~FCCoolRunnerIIDevice();
	
	//Rendering
	virtual std::pair<int, int> GetSize(float scale);
	virtual void Render(const Cairo::RefPtr<Cairo::Context>& cr, float scale);
	
	FCCoolRunnerIIFunctionBlock* GetFB(size_t i)
	{ return m_fbs[i]; }
	
	int GetShiftRegisterWidth();
	int GetShiftRegisterDepth();
	
	int GetConfigBitBase(int fb);
		
protected:

	//Initialization
	void CreateModelXC2C32A();
	
	//Device data
	std::vector<FCCoolRunnerIIFunctionBlockPair*> m_fbpairs;
	
	//Secondary indexes of stuff for easy access
	std::vector<FCCoolRunnerIIFunctionBlock*> m_fbs;
	std::map<int, std::map<int, FCCoolRunnerIIFunctionBlockPair*> > m_fbgrid;

public:	
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Dimensions for display

	enum dimensions
	{
		device_xmargin		= 10,
		device_ymargin		= 10,
		
		fb_mcwidth			= 300,
		fb_placolwidth		= 48,
		fb_xmargin			= 2,
		fb_axmargin			= 5,
		fb_plawidth			= 56 * fb_placolwidth + 2*fb_axmargin,			//56 product terms
		fb_width 			= fb_mcwidth + fb_plawidth,
		fb_dotrad			= 3,
		fb_orheight			= 400,
		fb_androwheight		= 64,
		fb_andblockheight	= fb_androwheight * 20 + fb_androwheight/2,		//20 inputs per half of the AND array
		fb_height			= fb_andblockheight * 2 + fb_orheight,
		fb_mcheight			= fb_height / 16,								//16 macrocells per FB
		fb_mcymargin		= 5,
		fb_muxpinlen		= 8,
		fb_muxpinpitch		= 4,
		fb_bubblerad		= 2,
		fb_muxheight		= 5,
		fb_andpinlen		= 8,
		fb_andpinpitch		= 4,
		fb_andbodylen		= 6,
		fb_andzig			= 8,
		
		zia_xmargin			= 2,
		zia_colwidth		= 16,
		fb_netspacing		= 7,
		
		iob_width			= 32,
		iob_height			= 32,
		
		net_label_spacing	= 100
	};
	
	int m_shregwidth;
	int m_shregdepth;
};

#endif

