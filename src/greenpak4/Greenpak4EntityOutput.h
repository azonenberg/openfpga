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

#ifndef Greenpak4EntityOutput_h
#define Greenpak4EntityOutput_h

/**
	@brief A single output from a general fabric signal
 */
class Greenpak4EntityOutput
{
public:
	Greenpak4EntityOutput(Greenpak4BitstreamEntity* src=NULL, std::string port="", unsigned int matrix=0)
	: m_src(src)
	, m_port(port)
	, m_matrix(matrix)
	{}

	//Equality test. Do NOT check for matrix equality
	//as both outputs of a dual-matrix node are considered equal
	bool operator==(const Greenpak4EntityOutput& rhs) const
	{ return (m_src == rhs.m_src) && (m_port == rhs.m_port); }

	bool operator!=(const Greenpak4EntityOutput& rhs) const
	{ return !(rhs == *this); }

	std::string GetDescription() const
	{ return m_src->GetDescription(); }

	std::string GetOutputName() const
	{ return m_src->GetDescription() + " port " + m_port; }

	Greenpak4EntityOutput GetDual();

	Greenpak4BitstreamEntity* GetRealEntity()
	{ return m_src->GetRealEntity(); }

	bool IsPGA();
	bool IsVoltageReference();
	bool IsPowerRail();
	bool IsDAC();
	bool GetPowerRailValue();

	bool HasDual()
	{ return m_src->GetDual() != NULL; }

	unsigned int GetMatrix()
	{ return m_matrix; }

	unsigned int GetNetNumber()
	{ return m_src->GetOutputNetNumber(m_port); }

	//comparison operator for std::map
	bool operator<(const Greenpak4EntityOutput& rhs) const
	{ return GetOutputName() < rhs.GetOutputName(); }

public:
	Greenpak4BitstreamEntity* m_src;
	std::string m_port;
	unsigned int m_matrix;
};

#endif
