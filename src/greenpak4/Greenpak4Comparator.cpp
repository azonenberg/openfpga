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

Greenpak4Comparator::Greenpak4Comparator(
		Greenpak4Device* device,
		unsigned int cmpnum,
		unsigned int matrix,
		unsigned int ibase,
		unsigned int oword,
		unsigned int cbase_isrc,
		unsigned int cbase_bw,
		unsigned int cbase_gain,
		unsigned int cbase_vin,
		unsigned int cbase_hyst
		)
		: Greenpak4BitstreamEntity(device, matrix, ibase, oword, -1)
		, m_pwren(device->GetGround())
		, m_vin(device->GetGround())
		, m_vref(device->GetGround())
		, m_cmpNum(cmpnum)
		, m_cbaseIsrc(cbase_isrc)
		, m_cbaseBw(cbase_bw)
		, m_cbaseGain(cbase_gain)
		, m_cbaseVin(cbase_vin)
		, m_cbaseHyst(cbase_hyst)
		, m_bandwidthHigh(true)
		, m_vinAtten(1)
		, m_isrcEn(false)
		, m_hysteresis(0)
{
}

Greenpak4Comparator::~Greenpak4Comparator()
{
	
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Accessors

string Greenpak4Comparator::GetDescription()
{
	char buf[128];
	snprintf(buf, sizeof(buf), "ACMP_%u", m_cmpNum);
	return string(buf);
}

vector<string> Greenpak4Comparator::GetInputPorts() const
{
	vector<string> r;
	r.push_back("PWREN");
	return r;
}

void Greenpak4Comparator::SetInput(string port, Greenpak4EntityOutput src)
{
	if(port == "PWREN")
		m_pwren = src;
	if(port == "VIN")
		m_vin = src;
	if(port == "VREF")
		m_vref = src;
	
	//ignore anything else silently (should not be possible since synthesis would error out)
}

vector<string> Greenpak4Comparator::GetOutputPorts() const
{
	vector<string> r;
	r.push_back("OUT");
	return r;
}

unsigned int Greenpak4Comparator::GetOutputNetNumber(string port)
{
	if(port == "OUT")
		return m_outputBaseWord;
	else
		return -1;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Serialization

void Greenpak4Comparator::CommitChanges()
{
	//Get our cell, or bail if we're unassigned
	auto ncell = dynamic_cast<Greenpak4NetlistCell*>(GetNetlistEntity());
	if(ncell == NULL)
		return;
		
	if(ncell->HasParameter("BANDWIDTH"))
		m_bandwidthHigh = (ncell->m_parameters["BANDWIDTH"] == "HIGH") ? true : false;
		
	if(ncell->HasParameter("VIN_ATTEN"))
		m_vinAtten = (atoi(ncell->m_parameters["VIN_ATTEN"].c_str()));
		
	if(ncell->HasParameter("VIN_ISRC_EN"))
		m_isrcEn = (ncell->m_parameters["VIN_ISRC_EN"] == "1") ? true : false;
		
	if(ncell->HasParameter("HYSTERESIS"))
		m_hysteresis = (atoi(ncell->m_parameters["HYSTERESIS"].c_str()));
}

bool Greenpak4Comparator::Load(bool* /*bitstream*/)
{
	//TODO: Do our inputs
	LogFatal("Unimplemented\n");
}

bool Greenpak4Comparator::Save(bool* bitstream)
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// INPUT BUS
	
	if(!WriteMatrixSelector(bitstream, m_inputBaseWord, m_pwren))
		return false;
	
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// CONFIGURATION
	
	//Input current source (iff present)
	if(m_cbaseIsrc > 0)
		bitstream[m_cbaseIsrc] = m_isrcEn;
		
	//Low bandwidth selector
	if(m_cbaseBw > 0)
		bitstream[m_cbaseBw] = !m_bandwidthHigh;
		
	//Gain selector
	if(m_cbaseGain > 0)
	{
		switch(m_vinAtten)
		{
			case 1:
				bitstream[m_cbaseGain + 1] = false;
				bitstream[m_cbaseGain + 0] = false;
				break;
			
			case 2:
				bitstream[m_cbaseGain + 1] = false;
				bitstream[m_cbaseGain + 0] = true;
				break;
			
			case 3:
				bitstream[m_cbaseGain + 1] = true;
				bitstream[m_cbaseGain + 0] = false;
				break;
			
			case 4:
				bitstream[m_cbaseGain + 1] = true;
				bitstream[m_cbaseGain + 0] = true;
				break;
				
				
			default:
				LogError("Invalid ACMP attenuation (must be 1/2/3/4)\n");
				return false;
		}
	}
	else
	{
		if(m_vinAtten != 1)
		{
			LogError("Invalid ACMP attenuation (must be 1 for %s)\n",
			         GetDescription().c_str());
		}
	}
	
	//Hysteresis
	if(m_cbaseHyst > 0)
	{
		switch(m_hysteresis)
		{
			case 0:
				bitstream[m_cbaseHyst + 1] = false;
				bitstream[m_cbaseHyst + 0] = false;
				break;
			
			case 25:
				bitstream[m_cbaseHyst + 1] = false;
				bitstream[m_cbaseHyst + 0] = true;
				break;
			
			case 50:
				bitstream[m_cbaseHyst + 1] = true;
				bitstream[m_cbaseHyst + 0] = false;
				break;
			
			case 200:
				bitstream[m_cbaseHyst + 1] = true;
				bitstream[m_cbaseHyst + 0] = true;
				break;
				
				
			default:
				LogError("Invalid ACMP hysteresis (must be 0/25/50/200)\n");
				return false;
		}
	}
	else
	{
		if(m_hysteresis != 0)
		{
			LogError("Invalid ACMP hysteresis (must be 0 for %s)\n",
			         GetDescription().c_str());
		}
	}
	
	//If input is ground, then don't hook it up (we're not active)
	if(m_vin.IsPowerRail() && !m_vin.GetPowerRailValue())
	{}
	
	//Invalid input
	else if(m_muxsels.find(m_vin) == m_muxsels.end())
	{
		LogError("Invalid ACMP input (tried to feed %s to %s)\n",
			m_vin.GetDescription().c_str(), GetDescription().c_str());
		return false;
	}
	
	//Valid input, hook it up
	else
	{
		unsigned int sel = m_muxsels[m_vin];
		
		//Bitstream is zero-cleared at start of the writing process so high zero bits don't need to be written
		
		//2-bit mux selector? Write the high bit
		if(sel & 2)
			bitstream[m_cbaseVin + 1] = true;
			
		//Write the low bit
		bitstream[m_cbaseVin] = (sel & 1) ? true : false;
	}
	
	return true;
}
