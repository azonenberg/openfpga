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

#ifndef PTVCorner_h
#define PTVCorner_h

#include <string>

/**
	@brief A single point in the (process, temperature, voltage) space
 */
class PTVCorner
{
public:
	enum ProcessSpeed
	{
		SPEED_SLOW,
		SPEED_TYPICAL,	//TODO: average across all tested dies to find this? or just skip and specify slow/fast?
		SPEED_FAST
	};

	PTVCorner(ProcessSpeed s, int t, int v)
	: m_speed(s)
	, m_dieTemp(t)
	, m_voltage(v)
	{}

	ProcessSpeed GetSpeed() const
	{ return m_speed; }

	int GetTemp() const
	{ return m_dieTemp; }

	int GetVoltage() const
	{ return m_voltage; }

	std::string toString() const;

	//Comparison operator for STL collections
	bool operator<(const PTVCorner& rhs) const;
	bool operator!=(const PTVCorner& rhs) const;

protected:

	/**
		@brief Where does this die fall in the process spectrum? Can be at either extreme or somewhere in the middle
	 */
	ProcessSpeed m_speed;

	/**
		@brief Die temperature, in degC
	 */
	int	m_dieTemp;

	/**
		@brief Supply voltage, in mV

		Note, this is *cell* supply voltage which may not be the Vcore rail (e.g. if we're querying an I/O cell)
	 */
	int m_voltage;
};

#endif
