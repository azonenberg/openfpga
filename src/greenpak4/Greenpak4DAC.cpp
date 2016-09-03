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

Greenpak4DAC::Greenpak4DAC(
	Greenpak4Device* device,
	unsigned int cbase_reg,
	unsigned int cbase_pwr,
	unsigned int cbase_insel,
	unsigned int dacnum)
	: Greenpak4BitstreamEntity(device, 0, -1, -1, -1)
		, m_dacnum(dacnum)
		, m_cbaseReg(cbase_reg)
		, m_cbasePwr(cbase_pwr)
		, m_cbaseInsel(cbase_insel)
{
}

Greenpak4DAC::~Greenpak4DAC()
{

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Accessors

string Greenpak4DAC::GetDescription()
{
	char buf[128];
	snprintf(buf, sizeof(buf), "DAC_%u", m_dacnum);
	return string(buf);
}

vector<string> Greenpak4DAC::GetInputPorts() const
{
	vector<string> r;
	//no general fabric inputs
	return r;
}

void Greenpak4DAC::SetInput(string port, Greenpak4EntityOutput src)
{
	//if(port == "IN")
	//	m_input = src;

	//ignore anything else silently (should not be possible since synthesis would error out)
}

vector<string> Greenpak4DAC::GetOutputPorts() const
{
	vector<string> r;
	//no general fabric outputs
	return r;
}

unsigned int Greenpak4DAC::GetOutputNetNumber(string /*port*/)
{
	return -1;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Serialization

void Greenpak4DAC::CommitChanges()
{
	//No configuration
}

bool Greenpak4DAC::Load(bool* /*bitstream*/)
{
	//TODO: Do our inputs
	LogFatal("Unimplemented\n");
}

bool Greenpak4DAC::Save(bool* /*bitstream*/)
{
	//no configuration, we just exist to help configure the comparator input muxes

	return true;
}
