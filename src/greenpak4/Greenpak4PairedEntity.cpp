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

Greenpak4PairedEntity::Greenpak4PairedEntity(
	Greenpak4Device* device,
	unsigned int matrix,
	unsigned int select,
	Greenpak4BitstreamEntity* a,
	Greenpak4BitstreamEntity* b)
	: Greenpak4BitstreamEntity(device, matrix, -1, -1, select)
{
}

Greenpak4PairedEntity::~Greenpak4PairedEntity()
{

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Accessors

string Greenpak4PairedEntity::GetDescription()
{
	return "";
}

vector<string> Greenpak4PairedEntity::GetInputPorts() const
{
	vector<string> r;
	//no inputs
	return r;
}

void Greenpak4PairedEntity::SetInput(string /*port*/, Greenpak4EntityOutput /*src*/)
{
	//no inputs
}

vector<string> Greenpak4PairedEntity::GetOutputPorts() const
{
	vector<string> r;
	//r.push_back("OK");
	return r;
}

unsigned int Greenpak4PairedEntity::GetOutputNetNumber(string port)
{
	/*if(port == "OK")
		return m_outputBaseWord;
	else*/
		return -1;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Serialization

bool Greenpak4PairedEntity::CommitChanges()
{
	//Get our cell, or bail if we're unassigned
	auto ncell = dynamic_cast<Greenpak4NetlistCell*>(GetNetlistEntity());
	if(ncell == NULL)
		return true;


	return true;
}

bool Greenpak4PairedEntity::Load(bool* /*bitstream*/)
{
	LogError("Unimplemented\n");
	return false;
}

bool Greenpak4PairedEntity::Save(bool* bitstream)
{
	//bitstream[m_configBase + 15] = false;

	return true;
}
