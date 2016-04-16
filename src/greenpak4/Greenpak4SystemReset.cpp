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

Greenpak4SystemReset::Greenpak4SystemReset(
	Greenpak4Device* device,
	unsigned int matrix,
	unsigned int ibase,
	unsigned int oword,
	unsigned int cbase)
	: Greenpak4BitstreamEntity(device, matrix, ibase, oword, cbase)
	, m_resetMode(RISING_EDGE)
	, m_reset(device->GetGround())
{

}

Greenpak4SystemReset::~Greenpak4SystemReset()
{
	
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Accessors

string Greenpak4SystemReset::GetDescription()
{
	return "SYSRST0";
}

vector<string> Greenpak4SystemReset::GetInputPorts()
{
	vector<string> r;
	r.push_back("RST");
	return r;
}

vector<string> Greenpak4SystemReset::GetOutputPorts()
{
	vector<string> r;
	//no output ports
	return r;
}

unsigned int Greenpak4SystemReset::GetOutputNetNumber(string /*port*/)
{
	//no output ports;
	return -1;
}

void Greenpak4SystemReset::SetInput(string port, Greenpak4EntityOutput src)
{
	if(port == "RST")
		m_reset = src;
	
	//ignore anything else silently (should not be possible since synthesis would error out)
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Serialization

void Greenpak4SystemReset::CommitChanges()
{
	//Get our cell, or bail if we're unassigned
	auto ncell = dynamic_cast<Greenpak4NetlistCell*>(GetNetlistEntity());
	if(ncell == NULL)
		return;
		
	if(ncell->HasParameter("RESET_MODE"))
	{
		Greenpak4SystemReset::ResetMode mode = Greenpak4SystemReset::RISING_EDGE;
		string p = ncell->m_parameters["RESET_MODE"];
		if(p == "RISING")
			mode = Greenpak4SystemReset::RISING_EDGE;
		else if(p == "FALLING")
			mode = Greenpak4SystemReset::FALLING_EDGE;
		else if(p == "LEVEL")
			mode = Greenpak4SystemReset::HIGH_LEVEL;
		else
		{
			fprintf(
				stderr,
				"ERROR: Reset \"%s\" has illegal reset mode \"%s\" (must be RISING, FALLING, or LEVEL)\n",
				ncell->m_name.c_str(),
				p.c_str());
			exit(-1);
		}
		SetResetMode(mode);
	}
}

bool Greenpak4SystemReset::Load(bool* /*bitstream*/)
{
	printf("Greenpak4SystemReset::Load() not yet implemented\n");
	return false;
}

bool Greenpak4SystemReset::Save(bool* bitstream)
{
	//No DRC needed - cannot route anything but pin 2 to us
	//If somebody tries something stupid PAR will fail with an unroutable design
	
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// INPUT BUS
	
	//no logic needed, hard-wired to pin #2
	
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Configuration
	
	//Reset mode
	switch(m_resetMode)
	{
		case HIGH_LEVEL:
			bitstream[m_configBase + 0] = true;
			bitstream[m_configBase + 1] = false;
			break;
			
		case RISING_EDGE:
			bitstream[m_configBase + 0] = false;
			bitstream[m_configBase + 1] = false;
			break;
			
		case FALLING_EDGE:
			bitstream[m_configBase + 0] = false;
			bitstream[m_configBase + 1] = true;
			break;
	}
	
	//Reset enable if m_reset is not a power rail (ground)
	if(!m_reset.IsPowerRail())
		bitstream[m_configBase + 2] = true;
	else
		bitstream[m_configBase + 2] = false;
	
	return true;
}
