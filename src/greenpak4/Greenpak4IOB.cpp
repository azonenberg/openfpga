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

Greenpak4IOB::Greenpak4IOB(
	Greenpak4Device* device,
	unsigned int pin_num,
	unsigned int matrix,
	unsigned int ibase,
	unsigned int oword,
	unsigned int cbase,
	unsigned int flags)
	: Greenpak4BitstreamEntity(device, matrix, ibase, oword, cbase)
	, m_pinNumber(pin_num)
	, m_schmittTrigger(false)
	, m_pullStrength(PULL_10K)
	, m_pullDirection(PULL_NONE)
	, m_driveStrength(DRIVE_1X)
	, m_driveType(DRIVE_PUSHPULL)
	, m_inputThreshold(THRESHOLD_NORMAL)
	, m_outputEnable(device->GetGround())
	, m_outputSignal(device->GetGround())
	, m_flags(flags)
	, m_analogConfigBase(0)
{
	
}

Greenpak4IOB::~Greenpak4IOB()
{
	
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Accessors

void Greenpak4IOB::CommitChanges()
{
	//Get our cell, or bail if we're unassigned
	auto niob = dynamic_cast<Greenpak4NetlistPort*>(GetNetlistEntity());
	if(niob == NULL)
		return;
	
	//TODO: support array nets
	auto net = niob->m_net;
			
	//printf("    Configuring IOB %d\n", iob->GetPinNumber());

	//Apply attributes to configure the net
	for(auto x : net->m_attributes)
	{
		//do nothing, only for debugging
		if(x.first == "src")
		{}
		
		//already PAR'd, useless now
		else if(x.first == "LOC")
		{}
		
		//IO schmitt trigger
		else if(x.first == "SCHMITT_TRIGGER")
		{
			if(x.second == "0")
				m_schmittTrigger = false;
			else
				m_schmittTrigger = true;
		}
		
		//Pullup strength/direction
		else if(x.first == "PULLUP")
		{
			m_pullDirection = Greenpak4IOB::PULL_UP;
			if(x.second == "10k")
				m_pullStrength = Greenpak4IOB::PULL_10K;
			else if(x.second == "100k")
				m_pullStrength = Greenpak4IOB::PULL_100K;
			else if(x.second == "1M")
				m_pullStrength = Greenpak4IOB::PULL_1M;
		}
		
		//Pulldown strength/direction
		else if(x.first == "PULLDOWN")
		{
			m_pullDirection = Greenpak4IOB::PULL_DOWN;
			if(x.second == "10k")
				m_pullStrength = Greenpak4IOB::PULL_10K;
			else if(x.second == "100k")
				m_pullStrength = Greenpak4IOB::PULL_100K;
			else if(x.second == "1M")
				m_pullStrength = Greenpak4IOB::PULL_1M;
		}
		
		//Driver configuration
		else if(x.first == "DRIVE_TYPE")
		{
			m_driveType = Greenpak4IOB::DRIVE_PUSHPULL;
			if(x.second == "PUSHPULL")
				m_driveType = Greenpak4IOB::DRIVE_PUSHPULL;
			else if(x.second == "NMOS_OD")
				m_driveType = Greenpak4IOB::DRIVE_NMOS_OPENDRAIN;
			else if(x.second == "PMOS_OD")
				m_driveType = Greenpak4IOB::DRIVE_PMOS_OPENDRAIN;
		}
		
		//Input buffer configuration
		else if(x.first == "IBUF_TYPE")
		{
			m_inputThreshold = Greenpak4IOB::THRESHOLD_NORMAL;
			if(x.second == "NORMAL")
				m_inputThreshold = Greenpak4IOB::THRESHOLD_NORMAL;
			else if(x.second == "LOW_VOLTAGE")
				m_inputThreshold = Greenpak4IOB::THRESHOLD_LOW;
			else if(x.second == "ANALOG")
				m_inputThreshold = Greenpak4IOB::THRESHOLD_ANALOG;
		}
		
		//Ignore flipflop initialization, that's handled elsewhere
		else if(x.first == "init")
		{
		}
		
		//TODO other configuration
		
		else
		{
			printf("WARNING: Top-level port \"%s\" has unrecognized attribute %s, ignoring\n",
				niob->m_name.c_str(), x.first.c_str());
		}
		
		//printf("        %s = %s\n", x.first.c_str(), x.second.c_str());
	}
	
	//Configure output enable
	switch(niob->m_direction)
	{
		case Greenpak4NetlistPort::DIR_OUTPUT:
			m_outputEnable = m_device->GetPower();
			break;
			
		case Greenpak4NetlistPort::DIR_INPUT:
			m_outputEnable = m_device->GetGround();
			break;
			
		case Greenpak4NetlistPort::DIR_INOUT:
		default:
			printf("ERROR: Requested invalid output configuration (or inout, which isn't implemented)\n");
			break;
	}
}

void Greenpak4IOB::SetInput(string port, Greenpak4EntityOutput src)
{
	//nameless port for now (TODO proper techmapping)
	if(port == "")
		m_outputSignal = src;
	
	//ignore anything else silently (should not be possible since synthesis would error out)
}

unsigned int Greenpak4IOB::GetOutputNetNumber(string port)
{
	//nameless port for now (TODO proper techmapping)
	if(port == "")
		return m_outputBaseWord;
	else
		return -1;
}

vector<string> Greenpak4IOB::GetInputPorts()
{
	vector<string> r;
	//r.push_back("I");
	r.push_back("");		//for now, input port has no name (TODO: do proper IOB techmapping)
	return r;
}

vector<string> Greenpak4IOB::GetOutputPorts()
{
	vector<string> r;
	//r.push_back("I");
	r.push_back("");		//for now, input port has no name (TODO: do proper IOB techmapping)
	return r;
}
