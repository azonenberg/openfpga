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
{
}

Greenpak4DCMPMux::~Greenpak4DCMPMux()
{

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Accessors

string Greenpak4DCMPMux::GetDescription()
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

void Greenpak4DCMPMux::SetInput(string /*port*/, Greenpak4EntityOutput /*src*/)
{
	//TODO hook up
}

vector<string> Greenpak4DCMPMux::GetOutputPorts() const
{
	vector<string> r;
	//r.push_back("VDD_LOW");
	return r;
}

unsigned int Greenpak4DCMPMux::GetOutputNetNumber(string port)
{
	/*if(port == "VDD_LOW")
		return m_outputBaseWord;
	else*/
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

bool Greenpak4DCMPMux::Save(bool* /*bitstream*/)
{
	//no configuration - output only
	return true;
}
