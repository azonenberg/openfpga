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

#include "xbpar.h"

using namespace std;

/**
	@brief Gets a human-readable description of this PTV corner
 */
string PTVCorner::toString() const
{
	char tmp[128];
	snprintf(tmp, sizeof(tmp), "%d °C, %.3f V, %s process corner",
		m_dieTemp,
		m_voltage * 0.001f,
		GetSpeedAsString().c_str());

	return string(tmp);
}

string PTVCorner::GetSpeedAsString() const
{
	const char* speed;
	switch(m_speed)
	{
		case SPEED_SLOW:
			speed = "slow";
			break;
		case SPEED_FAST:
			speed = "fast";
			break;
		case SPEED_TYPICAL:
		default:
			speed = "typical";
			break;
	}

	return string(speed);
}

//Comparison operator for STL collections
bool PTVCorner::operator<(const PTVCorner& rhs) const
{
	if(m_speed < rhs.m_speed)
		return true;
	if(m_dieTemp < rhs.m_dieTemp)
		return true;
	if(m_voltage < rhs.m_voltage)
		return true;
	return false;
}

bool PTVCorner::operator!=(const PTVCorner& rhs) const
{
	if(m_speed != rhs.m_speed)
		return true;
	if(m_dieTemp != rhs.m_dieTemp)
		return true;
	if(m_voltage != rhs.m_voltage)
		return true;
	return false;
}

bool PTVCorner::operator==(const PTVCorner& rhs) const
{
	if(m_speed != rhs.m_speed)
		return false;
	if(m_dieTemp != rhs.m_dieTemp)
		return false;
	if(m_voltage != rhs.m_voltage)
		return false;
	return true;
}
