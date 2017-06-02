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

#include <log.h>
#include <Greenpak4.h>

using namespace std;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Construction / destruction

Greenpak4Delay::Greenpak4Delay(
		Greenpak4Device* device,
		unsigned int matrix,
		unsigned int ibase,
		unsigned int obase,
		unsigned int cbase)
		: Greenpak4BitstreamEntity(device, matrix, ibase, obase, cbase)
		, m_input(device->GetGround())
		, m_delayTap(1)
		, m_mode(DELAY)
		, m_glitchFilter(false)
{
}

Greenpak4Delay::~Greenpak4Delay()
{

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Accessors

string Greenpak4Delay::GetDescription() const
{
	char buf[128];
	snprintf(buf, sizeof(buf), "DELAY_%u", m_matrix);
	return string(buf);
}

vector<string> Greenpak4Delay::GetInputPorts() const
{
	vector<string> r;
	r.push_back("IN");
	return r;
}

void Greenpak4Delay::SetInput(string port, Greenpak4EntityOutput src)
{
	if(port == "IN")
		m_input = src;

	//ignore anything else silently (should not be possible since synthesis would error out)
}

vector<string> Greenpak4Delay::GetOutputPorts() const
{
	vector<string> r;
	r.push_back("OUT");
	return r;
}

unsigned int Greenpak4Delay::GetOutputNetNumber(string port)
{
	if(port == "OUT")
		return m_outputBaseWord;
	else
		return -1;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Serialization

bool Greenpak4Delay::CommitChanges()
{
	//Get our cell, or bail if we're unassigned
	auto ncell = dynamic_cast<Greenpak4NetlistCell*>(GetNetlistEntity());
	if(ncell == NULL)
		return true;

	//Delay line
	if(ncell->m_type == "GP_DELAY")
		m_mode = DELAY;

	//Edge detector
	else
	{
		m_mode = RISING_EDGE;

		if(ncell->HasParameter("EDGE_DIRECTION"))
		{
			string dir = ncell->m_parameters["EDGE_DIRECTION"];
			if(dir == "RISING")
				m_mode = RISING_EDGE;
			else if(dir == "FALLING")
				m_mode = FALLING_EDGE;
			else if(dir == "BOTH")
				m_mode = BOTH_EDGE;
			else
			{
				LogError("Invalid delay specifier %s (must be one of RISING, FALLING, BOTH)\n", dir.c_str());
				return false;
			}
		}
	}

	if(ncell->HasParameter("DELAY_STEPS"))
		m_delayTap = atoi(ncell->m_parameters["DELAY_STEPS"].c_str());

	if(ncell->HasParameter("GLITCH_FILTER"))
		m_glitchFilter = atoi(ncell->m_parameters["GLITCH_FILTER"].c_str()) ? true : false;

	return true;
}

bool Greenpak4Delay::Load(bool* /*bitstream*/)
{
	//TODO: Do our inputs
	LogError("Unimplemented\n");
	return false;
}

bool Greenpak4Delay::Save(bool* bitstream)
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// INPUT BUS

	if(!WriteMatrixSelector(bitstream, m_inputBaseWord, m_input))
		return false;

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// CONFIGURATION

	//Mode selector
	switch(m_mode)
	{
		case RISING_EDGE:
			bitstream[m_configBase + 0] = false;
			bitstream[m_configBase + 1] = false;
			break;

		case FALLING_EDGE:
			bitstream[m_configBase + 0] = true;
			bitstream[m_configBase + 1] = false;
			break;

		case BOTH_EDGE:
			bitstream[m_configBase + 0] = false;
			bitstream[m_configBase + 1] = true;
			break;

		case DELAY:
			bitstream[m_configBase + 0] = true;
			bitstream[m_configBase + 1] = true;
			break;
	}

	//Select the number of delay taps
	int ntap = m_delayTap - 1;
	if( (ntap < 0) || (ntap > 3) )
	{
		LogError("DRC: GP_DELAY must have 1-4 steps of delay\n");
		return false;
	}

	//Number of taps
	bitstream[m_configBase + 2] = (ntap & 1) ? true : false;
	bitstream[m_configBase + 3] = (ntap & 2) ? true : false;

	//Glitch filter
	bitstream[m_configBase + 4] = m_glitchFilter;

	return true;
}
