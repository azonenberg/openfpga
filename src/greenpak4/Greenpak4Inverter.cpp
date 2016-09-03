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

Greenpak4Inverter::Greenpak4Inverter(
		Greenpak4Device* device,
		unsigned int matrix,
		unsigned int ibase,
		unsigned int oword)
		: Greenpak4BitstreamEntity(device, matrix, ibase, oword, -1)
		, m_input(device->GetGround())
{
}

Greenpak4Inverter::~Greenpak4Inverter()
{

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Accessors

string Greenpak4Inverter::GetDescription()
{
	char buf[128];
	snprintf(buf, sizeof(buf), "INV_%u", m_matrix);
	return string(buf);
}

vector<string> Greenpak4Inverter::GetInputPorts() const
{
	vector<string> r;
	r.push_back("IN");
	return r;
}

void Greenpak4Inverter::SetInput(string port, Greenpak4EntityOutput src)
{
	if(port == "IN")
		m_input = src;

	//ignore anything else silently (should not be possible since synthesis would error out)
}

vector<string> Greenpak4Inverter::GetOutputPorts() const
{
	vector<string> r;
	r.push_back("OUT");
	return r;
}

unsigned int Greenpak4Inverter::GetOutputNetNumber(string port)
{
	if(port == "OUT")
		return m_outputBaseWord;
	else
		return -1;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Serialization

void Greenpak4Inverter::CommitChanges()
{
	//No configuration
}

bool Greenpak4Inverter::Load(bool* /*bitstream*/)
{
	//TODO: Do our inputs
	LogFatal("Unimplemented\n");
}

bool Greenpak4Inverter::Save(bool* bitstream)
{
	if(!WriteMatrixSelector(bitstream, m_inputBaseWord, m_input))
		return false;

	return true;
}
