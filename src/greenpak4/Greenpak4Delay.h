/***********************************************************************************************************************
 * Copyright (C) 2016-2017 Andrew Zonenberg and contributors                                                           *
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

#ifndef Greenpak4Delay_h
#define Greenpak4Delay_h

#include "Greenpak4BitstreamEntity.h"

class Greenpak4Delay : public Greenpak4BitstreamEntity
{
public:

	//Construction / destruction
	Greenpak4Delay(
		Greenpak4Device* device,
		unsigned int matrix,
		unsigned int ibase,
		unsigned int obase,
		unsigned int cbase);

	//Serialization
	virtual bool Load(bool* bitstream);
	virtual bool Save(bool* bitstream);

	virtual ~Greenpak4Delay();

	virtual std::string GetDescription() const;

	virtual void SetInput(std::string port, Greenpak4EntityOutput src);
	virtual unsigned int GetOutputNetNumber(std::string port);

	virtual std::vector<std::string> GetInputPorts() const;
	virtual std::vector<std::string> GetOutputPorts() const;

	virtual bool CommitChanges();

	void SetTap(int tap)
	{ m_delayTap = tap; }

	void SetGlitchFilter(bool enable)
	{ m_glitchFilter = enable; }

	typedef std::pair<int, PTVCorner> TimingCondition;

	void SetUnfilteredDelay(int ntap, PTVCorner c, CombinatorialDelay d)
	{ m_unfilteredDelays[TimingCondition(ntap, c)] = d; }

	void SetFilteredDelay(int ntap, PTVCorner c, CombinatorialDelay d)
	{ m_filteredDelays[TimingCondition(ntap, c)] = d; }

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Timing stuff

	virtual void PrintTimingData() const;
	virtual void PrintExtraTimingData(PTVCorner corner) const;

	virtual void SaveTimingData(FILE* fp, bool last);

	virtual bool GetCombinatorialDelay(
		std::string srcport,
		std::string dstport,
		PTVCorner corner,
		CombinatorialDelay& delay) const;

protected:
	Greenpak4EntityOutput m_input;

	int m_delayTap;

	enum modes
	{
		DELAY,
		RISING_EDGE,
		FALLING_EDGE,
		BOTH_EDGE
	} m_mode;

	bool m_glitchFilter;

	//Timing data
	std::map<TimingCondition, CombinatorialDelay > m_unfilteredDelays;	//no glitch filter
	std::map<TimingCondition, CombinatorialDelay > m_filteredDelays;	//with glitch filter

	virtual void SaveTimingData(FILE* fp, PTVCorner corner);
	virtual bool LoadExtraTimingData(PTVCorner corner, std::string delaytype, json_object* object);
};

#endif	//Greenpak4Delay_h
