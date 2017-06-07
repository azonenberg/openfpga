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

#ifndef CombinatorialDelay_h
#define CombinatorialDelay_h

/**
	@brief Delay down a combinatorial path (at externally specified conditions)
 */
class CombinatorialDelay
{
public:
	CombinatorialDelay(float r = 0, float f = 0)
	: m_rising(r)
	, m_falling(f)
	{  }

	float m_rising;
	float m_falling;

	//Gets the worst-case delay for this path, not caring about the edge direction
	float GetWorst()
	{
		if(m_rising > m_falling)
			return m_rising;
		else
			return m_falling;
	}

	//Delay adding
	CombinatorialDelay operator+(const CombinatorialDelay& rhs) const
	{ return CombinatorialDelay(m_rising + rhs.m_rising, m_falling + rhs.m_falling); }

	CombinatorialDelay& operator=(const CombinatorialDelay& rhs)
	{
		m_rising = rhs.m_rising;
		m_falling = rhs.m_falling;
		return *this;
	}

	CombinatorialDelay& operator+=(const CombinatorialDelay& rhs)
	{
		m_rising += rhs.m_rising;
		m_falling += rhs.m_falling;
		return *this;
	}

	CombinatorialDelay& operator-=(const CombinatorialDelay& rhs)
	{
		m_rising -= rhs.m_rising;
		m_falling -= rhs.m_falling;
		return *this;
	}
};

#endif
