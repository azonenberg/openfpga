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

Greenpak4Bandgap::Greenpak4Bandgap(
	Greenpak4Device* device,
	unsigned int matrix,
	unsigned int ibase,
	unsigned int oword,
	unsigned int cbase)
	: Greenpak4BitstreamEntity(device, matrix, ibase, oword, cbase)
	, m_autoPowerDown(true)
	, m_chopperEn(true)
	, m_outDelay(100)
{
	m_dual = new Greenpak4DualEntity(this);
}

Greenpak4Bandgap::~Greenpak4Bandgap()
{
	
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Accessors

string Greenpak4Bandgap::GetDescription()
{
	return "BANDGAP0";
}

vector<string> Greenpak4Bandgap::GetInputPorts() const
{
	vector<string> r;
	//no inputs
	return r;
}

void Greenpak4Bandgap::SetInput(string /*port*/, Greenpak4EntityOutput /*src*/)
{
	//no inputs
}

vector<string> Greenpak4Bandgap::GetOutputPorts() const
{
	vector<string> r;
	r.push_back("OK");
	//VOUT is not general fabric routing
	return r;
}

unsigned int Greenpak4Bandgap::GetOutputNetNumber(string port)
{
	if(port == "OK")
		return m_outputBaseWord;
	//VOUT is not general fabric routing
	else
		return -1;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Serialization

void Greenpak4Bandgap::CommitChanges()
{
	//Get our cell, or bail if we're unassigned
	auto ncell = dynamic_cast<Greenpak4NetlistCell*>(GetNetlistEntity());
	if(ncell == NULL)
		return;
		
	if(ncell->HasParameter("AUTO_PWRDN"))
	{
		string p = ncell->m_parameters["AUTO_PWRDN"];
		if(p == "1")
			m_autoPowerDown = true;
		else
			m_autoPowerDown = false;
	}
	
	if(ncell->HasParameter("CHOPPER_EN"))
	{
		string p = ncell->m_parameters["CHOPPER_EN"];
		if(p == "1")
			m_chopperEn = true;
		else
			m_chopperEn = false;
	}
	
	if(ncell->HasParameter("OUT_DELAY"))
	{
		string p = ncell->m_parameters["OUT_DELAY"];
		if(p == "100")
			m_outDelay = 100;
		else if(p == "550")
			m_outDelay = 550;
		else
		{
			fprintf(
				stderr,
				"ERROR: Bandgap has illegal reset delay \"%s\" (must be 100 or 550)\n",
				p.c_str());
			exit(-1);
		}
	}
}

bool Greenpak4Bandgap::Load(bool* /*bitstream*/)
{
	printf("Greenpak4Bandgap::Load() not yet implemented\n");
	return false;
}

bool Greenpak4Bandgap::Save(bool* bitstream)
{
	//Startup delay
	if(m_outDelay == 100)
		bitstream[m_configBase + 0] = true;
	else
		bitstream[m_configBase + 0] = false;
		
	//Power-down 936
	if(m_autoPowerDown)
		bitstream[m_configBase + 13] = false;
	else
		bitstream[m_configBase + 13] = true;
		
	//Chopper enable flag
	if(m_chopperEn)
		bitstream[m_configBase + 15] = true;
	else
		bitstream[m_configBase + 15] = false;
	
	return true;
}
