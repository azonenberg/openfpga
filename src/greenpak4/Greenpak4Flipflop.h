/***********************************************************************************************************************
 * Copyright (C) 2016 Andrew Zonenberg and contributors                                                                *
 *                                                                                                                     *
 * This program is free software; you can redistribute it and/or modify it under the terms of the GNU Lesser General   *
 * Public License as published by the Free Software Foundation; either version 2.1 of the License, or (at your option) *
 * any later version.                                                                                                  *
 *                                                                                                                     *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied  *
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for     *
 * more details.                                                                                                       *
 *                                                                                                                     *
 * You should have received a copy of the GNU Lesser General Public License along with this program; if not, you may   *
 * find one here:                                                                                                      *
 * https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt                                                              *
 * or you may search the http://www.gnu.org website for the version 2.1 license, or you may write to the Free Software *
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA                                      *
 **********************************************************************************************************************/
 
#ifndef Greenpak4Flipflop_h
#define Greenpak4Flipflop_h

/**
	@brief A single flipflop/latch (TODO: support latch mode)
 */ 
class Greenpak4Flipflop : public Greenpak4BitstreamEntity
{
public:

	//Construction / destruction
	Greenpak4Flipflop(
		Greenpak4Device* device,
		unsigned int ffnum,
		bool has_sr,
		unsigned int matrix,
		unsigned int ibase,
		unsigned int oword,
		unsigned int cbase);
	virtual ~Greenpak4Flipflop();
	
	bool HasSetReset()
	{ return m_hasSR; }
	
	void SetInitValue(bool b)
	{ m_initValue = b; }
	
	//Bitfile metadata
	virtual unsigned int GetConfigLen();
	
	//Serialization
	virtual bool Load(bool* bitstream);
	virtual bool Save(bool* bitstream);
	
	//Set inputs
	void SetSRMode(bool mode)
	{ m_srmode = mode; }
	virtual void SetInputSignal(Greenpak4BitstreamEntity* sig);
	virtual void SetClockSignal(Greenpak4BitstreamEntity* sig);
	virtual void SetNSRSignal(Greenpak4BitstreamEntity* sig);
	
	unsigned int GetFlipflopIndex()
	{ return m_ffnum; }

	virtual std::string GetDescription();

protected:
	
	///Index of our flipflop
	unsigned int m_ffnum;
	
	///True if we have a set/reset input
	bool m_hasSR;
	
	///Power-on reset value
	bool m_initValue;
	
	///Set/reset mode (1=set, 0=reset)
	bool m_srmode;
	
	///Input signal
	Greenpak4BitstreamEntity* m_input;
	
	///Clock signal
	Greenpak4BitstreamEntity* m_clock;
	
	///Negative set/reset signal
	Greenpak4BitstreamEntity* m_nsr;
};

#endif
