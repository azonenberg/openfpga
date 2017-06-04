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

bool Greenpak4IOB::CommitChanges()
{
	//Get our IOB cell
	auto cell = dynamic_cast<Greenpak4NetlistCell*>(GetNetlistEntity());
	if(cell == NULL)
		return true;

	//Get the net
	Greenpak4NetlistNode* net = NULL;
	if(cell->m_type == "GP_IBUF")
		net = cell->m_connections["IN"][0];
	else if(cell->m_type == "GP_OBUF")
		net = cell->m_connections["OUT"][0];
	else if(cell->m_type == "GP_IOBUF")
		net = cell->m_connections["IO"][0];
	if(net == NULL)
		return true;

	//LogNotice("    Configuring IOB %d\n", m_pinNumber);
	//If we get here, we're not unused.
	//Default for USED nets, unless otherwise specced, is to float
	m_pullDirection = PULL_NONE;

	//Apply attributes to configure the net
	for(auto x : net->m_attributes)
	{
		bool bad_value = false;

		//do nothing, only for debugging
		if(x.first == "src")
		{}

		//IO schmitt trigger
		else if(x.first == "SCHMITT_TRIGGER")
		{
			if(x.second == "0")
				m_schmittTrigger = false;
			else if(x.second == "1" || x.second == "")
				m_schmittTrigger = true;
			else
				bad_value = true;
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
			else
				bad_value = true;
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
			else
				bad_value = true;
		}

		//Driver configuration
		else if(x.first == "DRIVE_STRENGTH")
		{
			if(x.second == "1X")
				m_driveStrength = Greenpak4IOB::DRIVE_1X;
			else if(x.second == "2X")
				m_driveStrength = Greenpak4IOB::DRIVE_2X;
			else if(x.second == "4X")
				m_driveStrength = Greenpak4IOB::DRIVE_4X;
			else
				bad_value = true;
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
			else
				bad_value = true;
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
			else
				bad_value = true;
		}

		//Ignore flipflop initialization, that's handled elsewhere
		else if(x.first == "init")
		{
		}

		//TODO other configuration

		else
		{
			LogWarning("Top-level port \"%s\" has unrecognized attribute %s, ignoring\n",
				cell->m_name.c_str(), x.first.c_str());
		}

		if(bad_value)
		{
			LogError("Top-level port \"%s\" has attribute %s with unrecognized value \"%s\"\n",
				cell->m_name.c_str(), x.first.c_str(), x.second.c_str());
			return false;
		}

		//LogNotice("        %s = %s\n", x.first.c_str(), x.second.c_str());
	}

	//Configure output enable
	if(cell->m_type == "GP_OBUF")
		m_outputEnable = m_device->GetPower();
	else if(cell->m_type == "GP_IBUF")
		m_outputEnable = m_device->GetGround();
	else if(cell->m_type == "GP_IOBUF")
	{
		//output enable will be hooked up by SetInput()
	}
	else
	{
		LogError("Invalid cell type for IOB\n");
		return false;
	}

	return true;
}

void Greenpak4IOB::SetInput(string port, Greenpak4EntityOutput src)
{
	if(port == "IN")
		m_outputSignal = src;
	else if(port == "OE")
		m_outputEnable = src;

	//ignore anything else silently (should not be possible since synthesis would error out)
}

unsigned int Greenpak4IOB::GetOutputNetNumber(string port)
{
	if(port == "OUT")
		return m_outputBaseWord;
	else
		return -1;
}

vector<string> Greenpak4IOB::GetInputPorts() const
{
	vector<string> r;
	r.push_back("IN");
	r.push_back("OE");
	return r;
}

vector<string> Greenpak4IOB::GetOutputPorts() const
{
	vector<string> r;
	r.push_back("OUT");
	return r;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Timing analysis

void Greenpak4IOB::PrintExtraTimingData(PTVCorner corner) const
{
	//Schmitt trigger delay
	auto sd = m_schmittTriggerDelays.find(corner);
	if(sd != m_schmittTriggerDelays.end())
	{
		auto time = sd->second;

		//Look up normal buffer delay and add
		CombinatorialDelay bd;
		if(GetCombinatorialDelay("IO", "OUT", corner, bd))
		{
			LogNotice("%10s to %10s: %.3f ns rising, %.3f ns falling\n",
				"IO (Sch)",
				"OUT",
				time.m_rising + bd.m_rising,
				time.m_falling + bd.m_falling);
		}
	}

	//Output buffer delays
	for(auto it : m_outputDelays)
	{
		//Skip results for other process corners
		if(it.first.second != corner)
			continue;

		string drive;
		switch(it.first.first)
		{
			case DRIVE_4X:
				drive = "4x";
				break;

			case DRIVE_2X:
				drive = "2x";
				break;

			case DRIVE_1X:
			default:
				drive = "1x";
				break;
		}
		string ioname = string("IO (") + drive + ")";

		LogNotice("%10s to %10s: %.3f ns rising, %.3f ns falling\n",
			"IN",
			ioname.c_str(),
			it.second.m_rising,
			it.second.m_falling);
	}
}

bool Greenpak4IOB::GetCombinatorialDelay(
		string srcport,
		string dstport,
		PTVCorner corner,
		CombinatorialDelay& delay) const
{
	//INPUT path: enable Schmitt trigger if needed
	if( (srcport == "IO") && (dstport == "OUT") )
	{
		//Get the baseline IO to OUT delay from the base class
		if(!Greenpak4BitstreamEntity::GetCombinatorialDelay(srcport, dstport, corner, delay))
		{
			LogWarning("Greenpak4IOB::GetCombinatorialDelay: no delay for process corner %s\n",
				corner.toString().c_str());
			return false;
		}

		//Add Schmitt trigger delay if needed
		if(m_schmittTrigger)
		{
			auto it = m_schmittTriggerDelays.find(corner);
			if(it == m_schmittTriggerDelays.end())
				return false;
			delay += it->second;
		}
		return true;
	}

	//OUTPUT path: look up drive strength
	else if( (srcport == "IN") && (dstport == "IO") )
	{
		auto it = m_outputDelays.find(DriveCondition(m_driveStrength, corner));
		if(it == m_outputDelays.end())
		{
			LogWarning("Greenpak4IOB::GetCombinatorialDelay: no delay for drive condition (strength=%d, corner=%s)\n",
				m_driveStrength,
				corner.toString().c_str());
			return false;
		}

		delay = it->second;
		return true;
	}

	//Default: return base class info
	return Greenpak4BitstreamEntity::GetCombinatorialDelay(srcport, dstport, corner, delay);
}

void Greenpak4IOB::SaveTimingData(FILE* fp, PTVCorner corner)
{
	if(m_schmittTriggerDelays.find(corner) != m_schmittTriggerDelays.end())
	{
		//Schmitt trigger delays
		fprintf(fp, "                {\n");
		fprintf(fp, "                    \"type\" : \"schmitt\",\n");
		auto sd = m_schmittTriggerDelays[corner];
		fprintf(fp, "                    \"rising\" : \"%f\",\n", sd.m_rising);
		fprintf(fp, "                    \"falling\" : \"%f\"\n", sd.m_falling);
		fprintf(fp, "                },\n");
	}

	//Output buffer delays
	for(auto it : m_outputDelays)
	{
		auto cond = it.first;
		auto delay = it.second;
		if(cond.second != corner)
			continue;

		auto drive = cond.first;
		int d;
		switch(drive)
		{
			case DRIVE_4X:
				d = 4;
				break;

			case DRIVE_2X:
				d = 2;
				break;

			case DRIVE_1X:
			default:
				d = 1;
				break;
		}

		fprintf(fp, "                {\n");
		fprintf(fp, "                    \"type\" : \"obuf\",\n");
		fprintf(fp, "                    \"drive\" : \"%d\",\n", d);
		fprintf(fp, "                    \"rising\" : \"%f\",\n", delay.m_rising);
		fprintf(fp, "                    \"falling\" : \"%f\"\n", delay.m_falling);
		fprintf(fp, "                },\n");
	}

	//do base class at end
	Greenpak4BitstreamEntity::SaveTimingData(fp, corner);
}

bool Greenpak4IOB::LoadExtraTimingData(PTVCorner corner, string delaytype, json_object* object)
{
	/*
	//Pull out all of the json stuff
	json_object* from;
	if(!json_object_object_get_ex(object, "from", &from))
	{
		LogError("No source for this delay\n");
		return false;
	}
	string sfrom = json_object_get_string(from);

	json_object* to;
	if(!json_object_object_get_ex(object, "to", &to))
	{
		LogError("No dest for this delay\n");
		return false;
	}
	string sto = json_object_get_string(to);

	json_object* rising;
	if(!json_object_object_get_ex(object, "rising", &rising))
	{
		LogError("No rising info for this corner\n");
		return false;
	}
	float nrising = json_object_get_double(rising);

	json_object* falling;
	if(!json_object_object_get_ex(object, "falling", &falling))
	{
		LogError("No falling info for this corner\n");
		return false;
	}
	float nfalling = json_object_get_double(falling);

	//Finally, we can actually save the delay!
	m_pinToPinDelays[corner][PinPair(sfrom, sto)] = CombinatorialDelay(nrising, nfalling);
	*/

	//no need to call base class, it's an empty stub
	return true;
}
