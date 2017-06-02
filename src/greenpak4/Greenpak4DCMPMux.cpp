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

Greenpak4DCMPMux::Greenpak4DCMPMux(
	Greenpak4Device* device,
		unsigned int matrix,
		unsigned int ibase)
	: Greenpak4BitstreamEntity(device, matrix, ibase, -1, -1)
	, m_sel0(device->GetGround())
	, m_sel1(device->GetGround())
{
}

Greenpak4DCMPMux::~Greenpak4DCMPMux()
{

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Accessors

string Greenpak4DCMPMux::GetDescription() const
{
	return "DCMPMUX_0";
}

vector<string> Greenpak4DCMPMux::GetInputPorts() const
{
	vector<string> r;
	r.push_back("SEL[0]");
	r.push_back("SEL[1]");
	return r;
}

void Greenpak4DCMPMux::SetInput(string port, Greenpak4EntityOutput src)
{
	if(port == "SEL[0]")
		m_sel0 = src;
	else if(port == "SEL[1]")
		m_sel1 = src;

	//ignore anything else silently (should not be possible since synthesis would error out)
}

vector<string> Greenpak4DCMPMux::GetOutputPorts() const
{
	vector<string> r;
	//no general fabric outputs
	return r;
}

unsigned int Greenpak4DCMPMux::GetOutputNetNumber(string /*port*/)
{
	//no general fabric outputs
	return -1;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Serialization

bool Greenpak4DCMPMux::CommitChanges()
{
	//no parameters
	return true;
}

bool Greenpak4DCMPMux::Load(bool* /*bitstream*/)
{
	LogError("Unimplemented\n");
	return false;
}

bool Greenpak4DCMPMux::Save(bool* bitstream)
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// INPUT BUS

	if(!WriteMatrixSelector(bitstream, m_inputBaseWord, m_sel0))
		return false;
	if(!WriteMatrixSelector(bitstream, m_inputBaseWord + 1, m_sel1))
		return false;

	return true;
}
