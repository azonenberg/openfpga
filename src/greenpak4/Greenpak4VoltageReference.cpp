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

Greenpak4VoltageReference::Greenpak4VoltageReference(
		Greenpak4Device* device,
		unsigned int refnum,
		unsigned int cbase,
		unsigned int vout_muxsel)
		: Greenpak4BitstreamEntity(device, 0, -1, -1, cbase)
		, m_vin(device->GetGround())
		, m_refnum(refnum)
		, m_vinDiv(1)
		, m_vref(50)	//not used; selector 5'b00000 in bitstream
		, m_voutMuxsel(vout_muxsel)
{
	//give us a dual so we can route to both left-side comparators and right-side ios
	m_dual = new Greenpak4DualEntity(this);
}

Greenpak4VoltageReference::~Greenpak4VoltageReference()
{
	
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Accessors

string Greenpak4VoltageReference::GetDescription()
{
	char buf[128];
	snprintf(buf, sizeof(buf), "VREF_%d", m_refnum);
	return string(buf);
}

vector<string> Greenpak4VoltageReference::GetInputPorts() const
{
	vector<string> r;
	r.push_back("VIN");
	return r;
}

void Greenpak4VoltageReference::SetInput(string port, Greenpak4EntityOutput src)
{
	if(port == "VIN")
		m_vin = src;
	
	//ignore anything else silently (should not be possible since synthesis would error out)
}

vector<string> Greenpak4VoltageReference::GetOutputPorts() const
{
	vector<string> r;
	//no general fabric outputs
	return r;
}

unsigned int Greenpak4VoltageReference::GetOutputNetNumber(string /*port*/)
{
	//no general fabric outputs
	return -1;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Serialization

void Greenpak4VoltageReference::CommitChanges()
{
	//Get our cell, or bail if we're unassigned
	auto ncell = dynamic_cast<Greenpak4NetlistCell*>(GetNetlistEntity());
	if(ncell == NULL)
		return;
	
	if(ncell->HasParameter("VIN_DIV"))
		m_vinDiv = atoi(ncell->m_parameters["VIN_DIV"].c_str());
		
	if(ncell->HasParameter("VREF"))
		m_vref = atoi(ncell->m_parameters["VREF"].c_str());
}

bool Greenpak4VoltageReference::Load(bool* /*bitstream*/)
{
	//TODO: Do our inputs
	LogFatal("Unimplemented\n");
}

bool Greenpak4VoltageReference::Save(bool* bitstream)
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// INPUT BUS
	
	//No matrix selectors at all
	
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Configuration

	unsigned int select = 0;

	//Constants
	if(m_vin.IsPowerRail())
	{
		//Constant voltage
		if(m_vin.GetPowerRailValue() == 0)
		{
			if(m_vref % 50)
			{
				LogError("DRC: Voltage reference %s must be set to a multiple of 50 mV (requested %d)\n",
					GetDescription().c_str(), m_vref);
				return false;
			}
			
			if(m_vref < 50 || m_vref > 1200)
			{
				LogError("DRC: Voltage reference %s must be set between 50mV and 1200 mV inclusive (requested %d)\n",
					GetDescription().c_str(), m_vref);
				return false;
			}
			
			if(m_vinDiv != 1)
			{
				LogError("DRC: Voltage reference %s must have divisor of 1 when using constant voltage\n",
					GetDescription().c_str());
				return false;
			}
			
			select = (m_vref / 50) - 1;		
		}
		
		//Divided Vdd
		else
		{
			LogError("Greenpak4VoltageReference inputs for divided Vdd not implemented yet\n");
			return false;
		}
	}
	
	//TODO: external Vref, DAC
	else
	{
		LogError("Greenpak4VoltageReference inputs other than constant not implemented yet\n");
		return false;
	}

	//Write the actual selector
	bitstream[m_configBase + 0] = (select & 1) ? true : false;
	bitstream[m_configBase + 1] = (select & 2) ? true : false;
	bitstream[m_configBase + 2] = (select & 4) ? true : false;
	bitstream[m_configBase + 3] = (select & 8) ? true : false;
	bitstream[m_configBase + 4] = (select & 16) ? true : false;
	
	return true;
}
