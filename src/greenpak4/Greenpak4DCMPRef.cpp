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

Greenpak4DCMPRef::Greenpak4DCMPRef(
	Greenpak4Device* device,
		unsigned int blocknum,
		unsigned int cbase)
	: Greenpak4BitstreamEntity(device, 1, -1, -1, cbase)
	, m_blocknum(blocknum)
	, m_referenceValue(0)
{
}

Greenpak4DCMPRef::~Greenpak4DCMPRef()
{

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Accessors

string Greenpak4DCMPRef::GetDescription() const
{
	char buf[128];
	snprintf(buf, sizeof(buf), "DCMPREF_%u", m_blocknum);
	return string(buf);
}

vector<string> Greenpak4DCMPRef::GetInputPorts() const
{
	vector<string> r;
	//no inputs
	return r;
}

void Greenpak4DCMPRef::SetInput(string /*port*/, Greenpak4EntityOutput /*src*/)
{
	//no inputs
}

Greenpak4EntityOutput Greenpak4DCMPRef::GetInput(string /*port*/) const
{
	//no inputs
	return Greenpak4EntityOutput(NULL);
}

vector<string> Greenpak4DCMPRef::GetOutputPorts() const
{
	vector<string> r;
	//no general fabric outputs
	return r;
}

unsigned int Greenpak4DCMPRef::GetOutputNetNumber(string /*port*/)
{
	//no general fabric outputs
	return -1;
}

string Greenpak4DCMPRef::GetPrimitiveName() const
{
	return "GP_DMCPREF";
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Serialization

bool Greenpak4DCMPRef::CommitChanges()
{
	//Get our cell, or bail if we're unassigned
	auto ncell = dynamic_cast<Greenpak4NetlistCell*>(GetNetlistEntity());
	if(ncell == NULL)
		return true;

	if(ncell->HasParameter("REF_VAL"))
		m_referenceValue = (atoi(ncell->m_parameters["REF_VAL"].c_str()));

	return true;
}

bool Greenpak4DCMPRef::Load(bool* bitstream)
{
	m_referenceValue = bitstream[m_configBase + 0];
	for(int i=1; i<8; i++)
		m_referenceValue |= bitstream[m_configBase + i] << 1;
	return true;
}

bool Greenpak4DCMPRef::Save(bool* bitstream)
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// CONFIGURATION

	if(m_referenceValue > 0xff)
	{
		LogError("GP_DCMPREF reference value must be less than 0xFF (got 0x%x\n", m_referenceValue);
		return false;
	}

	bitstream[m_configBase + 0] = (m_referenceValue & 1) ? true : false;
	bitstream[m_configBase + 1] = (m_referenceValue & 2) ? true : false;
	bitstream[m_configBase + 2] = (m_referenceValue & 4) ? true : false;
	bitstream[m_configBase + 3] = (m_referenceValue & 8) ? true : false;
	bitstream[m_configBase + 4] = (m_referenceValue & 16) ? true : false;
	bitstream[m_configBase + 5] = (m_referenceValue & 32) ? true : false;
	bitstream[m_configBase + 6] = (m_referenceValue & 64) ? true : false;
	bitstream[m_configBase + 7] = (m_referenceValue & 128) ? true : false;

	return true;
}
