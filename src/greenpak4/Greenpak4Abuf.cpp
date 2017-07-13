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

Greenpak4Abuf::Greenpak4Abuf(Greenpak4Device* device, unsigned int cbase)
		: Greenpak4BitstreamEntity(device, 0, -1, -1, cbase)
		, m_input(device->GetGround())
		, m_bufferBandwidth(1)
{
}

Greenpak4Abuf::~Greenpak4Abuf()
{

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Accessors

string Greenpak4Abuf::GetDescription() const
{
	return "ABUF0";	//only one of us for now
}

vector<string> Greenpak4Abuf::GetInputPorts() const
{
	vector<string> r;
	//no general fabric inputs
	return r;
}

void Greenpak4Abuf::SetInput(string port, Greenpak4EntityOutput src)
{
	if(port == "IN")
		m_input = src;

	//ignore anything else silently (should not be possible since synthesis would error out)
}

Greenpak4EntityOutput Greenpak4Abuf::GetInput(string port) const
{
	if(port == "IN")
		return m_input;
	else
		return Greenpak4EntityOutput(NULL);
}

vector<string> Greenpak4Abuf::GetOutputPorts() const
{
	vector<string> r;
	//no general fabric outputs
	return r;
}

unsigned int Greenpak4Abuf::GetOutputNetNumber(string /*port*/)
{
	return -1;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Serialization

bool Greenpak4Abuf::CommitChanges()
{
	//Get our cell, or bail if we're unassigned
	auto ncell = dynamic_cast<Greenpak4NetlistCell*>(GetNetlistEntity());
	if(ncell == NULL)
		return true;

	if(ncell->HasParameter("BANDWIDTH_KHZ"))
		m_bufferBandwidth = atoi(ncell->m_parameters["BANDWIDTH_KHZ"].c_str());

	//No configuration
	return true;
}

bool Greenpak4Abuf::Load(bool* bitstream)
{
	//TODO: set input as coming from the one pin it can come from?

	//Input buffer bandwidth
	int bw = 0;
	if(bitstream[m_configBase + 0])
		bw |= 1;
	if(bitstream[m_configBase + 1])
		bw |= 2;

	switch(bw)
	{
		case 0:
			m_bufferBandwidth = 1;
			break;

		case 1:
			m_bufferBandwidth = 5;
			break;

		case 2:
			m_bufferBandwidth = 20;
			break;

		case 3:
		default:
			m_bufferBandwidth = 50;
			break;
	}

	return true;
}

bool Greenpak4Abuf::Save(bool* bitstream)
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// INPUT BUS

	//none

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// CONFIGURATION

	switch(m_bufferBandwidth)
	{
		case 1:
			bitstream[m_configBase + 1] = false;
			bitstream[m_configBase + 0] = false;
			break;

		case 5:
			bitstream[m_configBase + 1] = false;
			bitstream[m_configBase + 0] = true;
			break;

		case 20:
			bitstream[m_configBase + 1] = true;
			bitstream[m_configBase + 0] = false;
			break;

		case 50:
			bitstream[m_configBase + 1] = true;
			bitstream[m_configBase + 0] = true;
			break;

		default:
			LogError("GP_ABUF buffer bandwidth must be one of 1, 5, 20, 50\n");
			return false;
	}

	return true;
}
