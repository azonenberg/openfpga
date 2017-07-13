/***********************************************************************************************************************
 * Copyright (C) 2016-2017 Andrew Zonenberg and contributors                                                           *
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

Greenpak4VoltageReference::Greenpak4VoltageReference(
		Greenpak4Device* device,
		unsigned int refnum,
		unsigned int vout_muxsel)
		: Greenpak4BitstreamEntity(device, 0, -1, -1, -1)
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

string Greenpak4VoltageReference::GetDescription() const
{
	char buf[128];
	snprintf(buf, sizeof(buf), "VREF_%u", m_refnum);
	return string(buf);
}

vector<string> Greenpak4VoltageReference::GetInputPorts() const
{
	vector<string> r;
	return r;
}

void Greenpak4VoltageReference::SetInput(string port, Greenpak4EntityOutput src)
{
	if(port == "VIN")
		m_vin = src;

	//ignore anything else silently (should not be possible since synthesis would error out)
}

Greenpak4EntityOutput Greenpak4VoltageReference::GetInput(string port) const
{
	if(port == "VIN")
		return m_vin;
	else
		return Greenpak4EntityOutput(NULL);
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

bool Greenpak4VoltageReference::CommitChanges()
{
	//Get our cell, or bail if we're unassigned
	auto ncell = dynamic_cast<Greenpak4NetlistCell*>(GetNetlistEntity());
	if(ncell == NULL)
		return true;

	if(ncell->HasParameter("VIN_DIV"))
		m_vinDiv = atoi(ncell->m_parameters["VIN_DIV"].c_str());

	if(ncell->HasParameter("VREF"))
		m_vref = atoi(ncell->m_parameters["VREF"].c_str());

	return true;
}

bool Greenpak4VoltageReference::Load(bool* /*bitstream*/)
{
	//TODO: how do we do this? Vref is attached to the comparator, so we have to steal config from that...
	LogWarning("Greenpak4VoltageReference::Load needs to be figured out\n");
	return true;
}

bool Greenpak4VoltageReference::Save(bool* /*bitstream*/)
{
	//no configuration, everything is in the downstream logic
	return true;
}

unsigned int Greenpak4VoltageReference::GetACMPMuxSel()
{
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
				return 0xff;
			}

			if(m_vref < 50 || m_vref > 1200)
			{
				LogError("DRC: Voltage reference %s must be set between 50mV and 1200 mV inclusive (requested %d)\n",
					GetDescription().c_str(), m_vref);
				return 0xff;
			}

			if(m_vinDiv != 1)
			{
				LogError("DRC: Voltage reference %s must have divisor of 1 when using constant voltage\n",
					GetDescription().c_str());
				return 0xff;
			}

			return (m_vref / 50) - 1;
		}

		//Divided Vdd
		else
		{
			switch(m_vinDiv)
			{
				case 3:
					return 0x18;

				case 4:
					return 0x19;

				default:
					LogError("Greenpak4VoltageReference: Only legal divisors for Vdd are 3 and 4\n");
					return 0xff;
			}
		}
	}

	//See if it's a DAC
	else if(m_vin.IsDAC())
	{
		auto num = dynamic_cast<Greenpak4DAC*>(m_vin.GetRealEntity())->GetDACNum();

		switch(num)
		{
			case 0:
				return 0x1F;
			case 1:
				return 0x1E;

			default:
				LogError("Greenpak4VoltageReference: invalid DAC\n");
				return 0xff;
		}
	}

	//See if it's an IOB
	else if(m_vin.IsIOB())
	{
		auto iob = dynamic_cast<Greenpak4IOB*>(m_vin.GetRealEntity());
		auto pin = iob->GetPinNumber();

		//SLG46140
		auto part = m_device->GetPart();
		if(part == Greenpak4Device::GREENPAK4_SLG46140)
		{
			if( (pin == 4) && (m_vinDiv == 2) )
				return 0x1d;
			else if( (pin == 5) && (m_vinDiv == 2) )
				return 0x1c;
			else if( (pin == 4) && (m_vinDiv == 1) )
				return 0x1b;
			else if( (pin == 5) && (m_vinDiv == 1) )
				return 0x1a;
			else
			{
				LogError("Invalid voltage reference input (pin %d, divisor %d)\n", pin, m_vinDiv);
				return 0xff;
			}
		}

		//SLG4662x
		else if( (part == Greenpak4Device::GREENPAK4_SLG46620) || (part == Greenpak4Device::GREENPAK4_SLG46621) )
		{
			//Same mux selector for all vrefs
			if(pin == 10)
			{
				if(m_vinDiv == 2)
					return 0x1c;
				else if(m_vinDiv == 1)
					return 0x1a;
				else
				{
					LogError("Invalid voltage reference input (pin %d, divisor %d)\n", pin, m_vinDiv);
					return 0xff;
				}
			}

			//All other pins use the same mux setting (different pins for each vref though)
			//No need to check for invalid pins as those will fail routing due to missing paths
			else
			{
				if(m_vinDiv == 2)
					return 0x1d;
				else if(m_vinDiv == 1)
					return 0x1b;
				else
				{
					LogError("Invalid voltage reference input (pin %d, divisor %d)\n", pin, m_vinDiv);
					return 0xff;
				}

			}
		}

		LogError("Invalid voltage reference input (pin %d)\n", iob->GetPinNumber());
		return 0xff;
	}

	//should never happen, we'd fail routing before we get here
	else
	{
		LogError("Invalid voltage reference input\n");
		return 0xff;
	}
}
