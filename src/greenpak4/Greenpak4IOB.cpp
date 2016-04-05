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
	, m_outputEnable(device->GetPowerRail(0))
	, m_outputSignal(device->GetPowerRail(0))
	, m_flags(flags)
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
				SetSchmittTrigger(false);
			else
				SetSchmittTrigger(true);
		}
		
		//Pullup strength/direction
		else if(x.first == "PULLUP")
		{
			SetPullDirection(Greenpak4IOB::PULL_UP);
			if(x.second == "10k")
				SetPullStrength(Greenpak4IOB::PULL_10K);
			else if(x.second == "100k")
				SetPullStrength(Greenpak4IOB::PULL_100K);
			else if(x.second == "1M")
				SetPullStrength(Greenpak4IOB::PULL_1M);
		}
		
		//Pulldown strength/direction
		else if(x.first == "PULLDOWN")
		{
			SetPullDirection(Greenpak4IOB::PULL_DOWN);
			if(x.second == "10k")
				SetPullStrength(Greenpak4IOB::PULL_10K);
			else if(x.second == "100k")
				SetPullStrength(Greenpak4IOB::PULL_100K);
			else if(x.second == "1M")
				SetPullStrength(Greenpak4IOB::PULL_1M);
		}
		
		//Ignore flipflop initialization, that's handled elsewhere
		else if(x.first == "init")
		{
		}
		
		//TODO: 
		
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
			SetOutputEnable(true);
			break;
			
		case Greenpak4NetlistPort::DIR_INPUT:
			SetOutputEnable(false);
			break;
			
		case Greenpak4NetlistPort::DIR_INOUT:
		default:
			printf("ERROR: Requested invalid output configuration (or inout, which isn't implemented)\n");
			break;
	}
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

void Greenpak4IOB::SetSchmittTrigger(bool enabled)
{
	m_schmittTrigger = enabled;
}

bool Greenpak4IOB::GetSchmittTrigger()
{
	return m_schmittTrigger;
}

void Greenpak4IOB::SetPullStrength(PullStrength strength)
{
	m_pullStrength = strength;
}

bool Greenpak4IOB::GetPullStrength()
{
	return m_pullStrength;
}

void Greenpak4IOB::SetPullDirection(PullDirection direction)
{
	m_pullDirection = direction;
}

bool Greenpak4IOB::GetPullDirection()
{
	return m_pullDirection;
}

void Greenpak4IOB::SetDriveStrength(DriveStrength strength)
{
	m_driveStrength = strength;
}

bool Greenpak4IOB::GetDriveStrength()
{
	return m_driveStrength;
}

void Greenpak4IOB::SetDriveType(DriveType type)
{
	m_driveType = type;
}

bool Greenpak4IOB::GetDriveType()
{
	return m_driveType;
}

void Greenpak4IOB::SetInputThreshold(InputThreshold thresh)
{
	m_inputThreshold = thresh;
}

bool Greenpak4IOB::GetInputThreshold()
{
	return m_inputThreshold;
}

void Greenpak4IOB::SetOutputEnable(Greenpak4BitstreamEntity* oe)
{
	m_outputEnable = oe;
}

Greenpak4BitstreamEntity* Greenpak4IOB::GetOutputEnable()
{
	return m_outputEnable;
}

void Greenpak4IOB::SetOutputSignal(Greenpak4BitstreamEntity* sig)
{
	m_outputSignal = sig;
}

Greenpak4BitstreamEntity* Greenpak4IOB::GetOutputSignal()
{
	return m_outputSignal;
}

void Greenpak4IOB::SetOutputEnable(bool b)
{
	m_outputEnable = m_device->GetPowerRail(b);
}

void Greenpak4IOB::SetOutputSignal(bool b)
{
	m_outputSignal = m_device->GetPowerRail(b);
}
