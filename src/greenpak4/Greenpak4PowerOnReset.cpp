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

Greenpak4PowerOnReset::Greenpak4PowerOnReset(
	Greenpak4Device* device,
	unsigned int matrix,
	unsigned int ibase,
	unsigned int oword,
	unsigned int cbase)
	: Greenpak4BitstreamEntity(device, matrix, ibase, oword, cbase)
	, m_resetDelay(500)
{
	m_dual = new Greenpak4DualEntity(this);
}

Greenpak4PowerOnReset::~Greenpak4PowerOnReset()
{

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Accessors

string Greenpak4PowerOnReset::GetDescription()
{
	return "POR0";
}

vector<string> Greenpak4PowerOnReset::GetInputPorts() const
{
	vector<string> r;
	//no inputs
	return r;
}

void Greenpak4PowerOnReset::SetInput(string /*port*/, Greenpak4EntityOutput /*src*/)
{
	//no inputs
}

vector<string> Greenpak4PowerOnReset::GetOutputPorts() const
{
	vector<string> r;
	r.push_back("RST_DONE");
	return r;
}

unsigned int Greenpak4PowerOnReset::GetOutputNetNumber(string port)
{
	if(port == "RST_DONE")
		return m_outputBaseWord;
	else
		return -1;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Serialization

bool Greenpak4PowerOnReset::CommitChanges()
{
	//Get our cell, or bail if we're unassigned
	auto ncell = dynamic_cast<Greenpak4NetlistCell*>(GetNetlistEntity());
	if(ncell == NULL)
		return true;

	if(ncell->HasParameter("POR_TIME"))
	{
		string p = ncell->m_parameters["POR_TIME"];
		if(p == "4")
			m_resetDelay = 4;
		else if(p == "500")
			m_resetDelay = 500;
		else
		{
			LogError(
				"Power-on reset \"%s\" has illegal reset delay \"%s\" (must be 4 or 500)\n",
				ncell->m_name.c_str(),
				p.c_str());
			return false;
		}
	}

	return true;
}

bool Greenpak4PowerOnReset::Load(bool* /*bitstream*/)
{
	LogError("Unimplemented\n");
	return false;
}

bool Greenpak4PowerOnReset::Save(bool* bitstream)
{
	if(m_resetDelay == 4)
		bitstream[m_configBase] = false;
	else if(m_resetDelay == 500)
		bitstream[m_configBase] = true;

	return true;
}
