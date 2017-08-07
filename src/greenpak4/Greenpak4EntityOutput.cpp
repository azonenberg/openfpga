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

#include <xbpar.h>
#include <Greenpak4.h>

using namespace std;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Greenpak4EntityOutput

Greenpak4EntityOutput Greenpak4EntityOutput::GetDual() const
{
	return m_src->GetDual()->GetOutput(m_port);
}

bool Greenpak4EntityOutput::IsPowerRail() const
{
	return dynamic_cast<Greenpak4PowerRail*>(m_src) != NULL;
}

bool Greenpak4EntityOutput::IsVoltageReference() const
{
	return dynamic_cast<Greenpak4VoltageReference*>(m_src) != NULL;
}

bool Greenpak4EntityOutput::IsIOB() const
{
	return dynamic_cast<Greenpak4IOB*>(m_src) != NULL;
}

bool Greenpak4EntityOutput::IsPGA() const
{
	return dynamic_cast<Greenpak4PGA*>(m_src) != NULL;
}

bool Greenpak4EntityOutput::IsDAC() const
{
	return dynamic_cast<Greenpak4DAC*>(m_src) != NULL;
}

bool Greenpak4EntityOutput::GetPowerRailValue() const
{
	if(!IsPowerRail())
		return false;
	return dynamic_cast<Greenpak4PowerRail*>(m_src)->GetDigitalValue();
}
