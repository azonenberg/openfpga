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
 
#ifndef Greenpak4Counter_h
#define Greenpak4Counter_h

/**
	@brief A hard counter block (may also have alternate functions)
 */ 
class Greenpak4Counter : public Greenpak4BitstreamEntity
{
public:

	//Construction / destruction
	Greenpak4Counter(
		Greenpak4Device* device,
		unsigned int depth,
		bool has_fsm,
		bool has_wspwrdn,
		bool has_edgedetect,
		bool has_pwm,
		unsigned int countnum,
		unsigned int matrix,
		unsigned int ibase,
		unsigned int oword,
		unsigned int cbase);
	virtual ~Greenpak4Counter();
		
	//Bitfile metadata
	virtual unsigned int GetConfigLen();
	
	//Serialization
	virtual bool Load(bool* bitstream);
	virtual bool Save(bool* bitstream);
	
	virtual std::string GetDescription();
	
	unsigned int GetDepth()
	{ return m_depth; }
	
	unsigned int GetCounterIndex()
	{ return m_countnum; }
	
	void SetReset(Greenpak4BitstreamEntity* reset)
	{ m_reset = reset; }
	
	void SetClock(Greenpak4BitstreamEntity* clock)
	{ m_clock = clock; }
	
	void SetCounterValue(unsigned int val)
	{ m_countVal = val; }
	
	void SetPreDivide(unsigned int val)
	{ m_preDivide = val; }
	
	enum ResetMode
	{
		BOTH_EDGE = 0,
		FALLING_EDGE = 1,
		RISING_EDGE = 2,
		HIGH_LEVEL = 3
	};
	
	void SetResetMode(ResetMode mode)
	{ m_resetMode = mode; }
	
	virtual std::vector<std::string> GetInputPorts();
	virtual std::vector<std::string> GetOutputPorts();
	
protected:
	
	///Bit depth of this counter
	unsigned int m_depth;
	
	///Device index of this counter
	unsigned int m_countnum;
	
	///Reset input
	Greenpak4BitstreamEntity* m_reset;
	
	///Clock input
	Greenpak4BitstreamEntity* m_clock;
		
	///FSM-present flag
	bool m_hasFSM;
	
	///Counter value
	unsigned int m_countVal;
	
	///Pre-divider value (not all values legal for all clock sources)
	unsigned int m_preDivide;
	
	///Reset mode
	ResetMode m_resetMode;
	
	///Indicates if we have a wake-sleep power down mode
	bool m_hasWakeSleepPowerDown;
	
	///Indicates if we have an edge detector
	bool m_hasEdgeDetect;
	
	///Indicates if we have a PWM mode
	bool m_hasPWM;
};

#endif
