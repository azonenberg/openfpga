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

Greenpak4Bandgap::Greenpak4Bandgap(
	Greenpak4Device* device,
	unsigned int matrix,
	unsigned int ibase,
	unsigned int oword,
	unsigned int cbase,
	unsigned int cbase_pwren,
	unsigned int cbase_chopper)
	: Greenpak4BitstreamEntity(device, matrix, ibase, oword, cbase)
	, m_autoPowerDown(true)
	, m_chopperEn(true)
	, m_outDelay(100)
	, m_cbasePowerEn(cbase_pwren)
	, m_cbaseChopper(cbase_chopper)
{
	m_dual = new Greenpak4DualEntity(this);
}

Greenpak4Bandgap::~Greenpak4Bandgap()
{

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Accessors

string Greenpak4Bandgap::GetDescription() const
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

Greenpak4EntityOutput Greenpak4Bandgap::GetInput(string /*port*/) const
{
	//no inputs
	return Greenpak4EntityOutput(NULL);
}

vector<string> Greenpak4Bandgap::GetOutputPorts() const
{
	vector<string> r;
	r.push_back("OK");
	return r;
}

unsigned int Greenpak4Bandgap::GetOutputNetNumber(string port)
{
	if(port == "OK")
		return m_outputBaseWord;
	else
		return -1;
}

string Greenpak4Bandgap::GetPrimitiveName() const
{
	return "GP_BANDGAP";
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Serialization

map<string, string> Greenpak4Bandgap::GetParameters() const
{
	map<string, string> params;

	if(m_autoPowerDown)
		params["AUTO_PWRDN"] = "1";
	else
		params["AUTO_PWRDN"] = "0";

	if(m_chopperEn)
		params["CHOPPER_EN"] = "1";
	else
		params["CHOPPER_EN"] = "0";

	char tmp[128];
	snprintf(tmp, sizeof(tmp), "%d", m_outDelay);
	params["OUT_DELAY"] = tmp;

	return params;
}

bool Greenpak4Bandgap::CommitChanges()
{
	//Get our cell, or bail if we're unassigned
	auto ncell = dynamic_cast<Greenpak4NetlistCell*>(GetNetlistEntity());
	if(ncell == NULL)
		return true;

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
			LogError("Bandgap has illegal reset delay \"%s\" (must be 100 or 550)\n",
				p.c_str());
			return false;
		}
	}

	return true;
}

bool Greenpak4Bandgap::Load(bool* bitstream)
{
	if(bitstream[m_configBase])
		m_outDelay = 100;
	else
		m_outDelay = 550;

	m_autoPowerDown = !bitstream[m_cbasePowerEn];

	if(m_cbaseChopper)
		m_chopperEn = bitstream[m_cbaseChopper];

	return true;
}

bool Greenpak4Bandgap::Save(bool* bitstream)
{
	//Startup delay
	if(m_outDelay == 100)
		bitstream[m_configBase] = true;
	else
		bitstream[m_configBase] = false;

	//Power-down 936
	if(m_autoPowerDown)
		bitstream[m_cbasePowerEn] = false;
	else
		bitstream[m_cbasePowerEn] = true;

	//Chopper enable flag
	if(m_cbaseChopper)
	{
		if(m_chopperEn)
			bitstream[m_cbaseChopper] = true;
		else
			bitstream[m_cbaseChopper] = false;
	}

	return true;
}
