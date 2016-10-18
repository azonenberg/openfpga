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

string Greenpak4VoltageReference::GetDescription()
{
	char buf[128];
	snprintf(buf, sizeof(buf), "VREF_%u", m_refnum);
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
	//TODO: how do we do this?
	LogError("Unimplemented\n");
	return false;
}

bool Greenpak4VoltageReference::Save(bool* /*bitstream*/)
{
	//no configuration, everything is in the downstream logic
	return true;
}

unsigned int Greenpak4VoltageReference::GetACMPMuxSel()
{
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
			return 0;
		}
	}

	//See if it's a DAC
	else if(m_vin.IsDAC())
	{
		auto num = dynamic_cast<Greenpak4DAC*>(m_vin.GetRealEntity())->GetDACNum();

		switch(num)
		{
			//Constant for SLG46620V, TODO check other parts?
			case 0:
				return 0x1F;
			case 1:
				return 0x1E;

			default:
				LogError("Greenpak4VoltageReference: invalid DAC\n");
				break;
		}
	}

	//TODO: external Vref
	else
	{
		LogError("Greenpak4VoltageReference inputs other than constant not implemented yet\n");
		return false;
	}

	return select;
}
