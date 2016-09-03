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

#include "Greenpak4.h"
#include <stdio.h>
#include <stdlib.h>

using namespace std;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Construction / destruction

Greenpak4Abuf::Greenpak4Abuf(
		Greenpak4Device* device)
		: Greenpak4BitstreamEntity(device, 0, -1, -1, -1)
		, m_input(device->GetGround())
{
}

Greenpak4Abuf::~Greenpak4Abuf()
{

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Accessors

string Greenpak4Abuf::GetDescription()
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

void Greenpak4Abuf::CommitChanges()
{
	//No configuration
}

bool Greenpak4Abuf::Load(bool* /*bitstream*/)
{
	//TODO: Do our inputs
	LogFatal("Unimplemented\n");
}

bool Greenpak4Abuf::Save(bool* /*bitstream*/)
{
	//no configuration, we just exist to help configure the comparator input muxes

	return true;
}
