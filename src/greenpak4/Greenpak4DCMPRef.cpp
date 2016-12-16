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

Greenpak4DCMPRef::Greenpak4DCMPRef(
	Greenpak4Device* device,
		unsigned int blocknum,
		unsigned int cbase)
	: Greenpak4BitstreamEntity(device, 1, -1, -1, cbase)
	, m_blocknum(blocknum)
{
}

Greenpak4DCMPRef::~Greenpak4DCMPRef()
{

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Accessors

string Greenpak4DCMPRef::GetDescription()
{
	char buf[128];
	snprintf(buf, sizeof(buf), "DCMPREF_%u", m_blocknum);
	return string(buf);
}

vector<string> Greenpak4DCMPRef::GetInputPorts() const
{
	vector<string> r;
	//no inputs
	return r;
}

void Greenpak4DCMPRef::SetInput(string /*port*/, Greenpak4EntityOutput /*src*/)
{
	//no inputs
}

vector<string> Greenpak4DCMPRef::GetOutputPorts() const
{
	vector<string> r;
	//r.push_back("VDD_LOW");
	return r;
}

unsigned int Greenpak4DCMPRef::GetOutputNetNumber(string port)
{
	/*if(port == "VDD_LOW")
		return m_outputBaseWord;
	else*/
		return -1;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Serialization

bool Greenpak4DCMPRef::CommitChanges()
{
	//no parameters
	return true;
}

bool Greenpak4DCMPRef::Load(bool* /*bitstream*/)
{
	LogError("Unimplemented\n");
	return false;
}

bool Greenpak4DCMPRef::Save(bool* /*bitstream*/)
{
	//no configuration - output only
	return true;
}
