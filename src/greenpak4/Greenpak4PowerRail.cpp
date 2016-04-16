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

using namespace std;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Construction / destruction

Greenpak4PowerRail::Greenpak4PowerRail(
	Greenpak4Device* device,
	unsigned int matrix,
	unsigned int oword)
	: Greenpak4BitstreamEntity(device, matrix, 0, oword, 0)
	//Give garbage values to ibase and cbase since we have no inputs or configuration
{
	m_dual = new Greenpak4DualEntity(this);
}

Greenpak4PowerRail::~Greenpak4PowerRail()
{
	
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Dummy serialization placeholders (nothing to do, we have no data)

void Greenpak4PowerRail::CommitChanges()
{
	//no action needed, we have no input pins to drive and no configuration
}

bool Greenpak4PowerRail::Load(bool* /*bitstream*/)
{
	return true;
}

bool Greenpak4PowerRail::Save(bool* /*bitstream*/)
{
	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Accessors

vector<string> Greenpak4PowerRail::GetInputPorts()
{
	vector<string> r;
	//no inputs
	return r;
}

void Greenpak4PowerRail::SetInput(string port, Greenpak4EntityOutput src)
{
	//no inputs
}

vector<string> Greenpak4PowerRail::GetOutputPorts()
{
	vector<string> r;
	r.push_back("OUT");
	return r;
}

unsigned int Greenpak4PowerRail::GetOutputNetNumber(std::string port)
{
	if(port == "OUT")
		return m_outputBaseWord;
	else
		return -1;
}

string Greenpak4PowerRail::GetDescription()
{
	if(GetDigitalValue())
		return "VDD0";
	else
		return "VSS0";
}
