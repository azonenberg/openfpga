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

Greenpak4ClockBuffer::Greenpak4ClockBuffer(
	Greenpak4Device* device,
	unsigned int bufnum,
	unsigned int matrix,
	unsigned int ibase,
	unsigned int cbase)
	: Greenpak4BitstreamEntity(device, matrix, ibase, -1, cbase)
	, m_input(device->GetGround())
	, m_bufferNum(bufnum)
{
}

Greenpak4ClockBuffer::~Greenpak4ClockBuffer()
{

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Accessors

string Greenpak4ClockBuffer::GetDescription() const
{
	char buf[128];
	snprintf(buf, sizeof(buf), "CLKBUF_%u", m_bufferNum);
	return string(buf);
}

vector<string> Greenpak4ClockBuffer::GetInputPorts() const
{
	vector<string> r;
	r.push_back("IN");
	return r;
}

void Greenpak4ClockBuffer::SetInput(string port, Greenpak4EntityOutput src)
{
	if(port == "IN")
		m_input = src;

	//ignore anything else silently (should not be possible since synthesis would error out)
}

Greenpak4EntityOutput Greenpak4ClockBuffer::GetInput(string port) const
{
	if(port == "IN")
		return m_input;
	else
		return Greenpak4EntityOutput(NULL);
}

vector<string> Greenpak4ClockBuffer::GetOutputPorts() const
{
	vector<string> r;
	//no general fabric outputs
	return r;
}

unsigned int Greenpak4ClockBuffer::GetOutputNetNumber(string /*port*/)
{
	return -1;
}

string Greenpak4ClockBuffer::GetPrimitiveName() const
{
	return "GP_CLKBUF";
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Serialization

bool Greenpak4ClockBuffer::CommitChanges()
{
	//No configuration
	return true;
}

bool Greenpak4ClockBuffer::Load(bool* bitstream)
{
	//Load our input
	ReadMatrixSelector(bitstream, m_inputBaseWord, m_matrix, m_input);
	return true;
}

bool Greenpak4ClockBuffer::Save(bool* bitstream)
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// INPUT BUS

	if(!WriteMatrixSelector(bitstream, m_inputBaseWord, m_input))
		return false;

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// CONFIGURATION

	//none

	return true;
}
