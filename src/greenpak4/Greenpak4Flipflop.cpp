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

Greenpak4Flipflop::Greenpak4Flipflop(
	Greenpak4Device* device,
	unsigned int ffnum,
	bool has_sr,
	unsigned int matrix,
	unsigned int ibase,
	unsigned int oword,
	unsigned int cbase)
	: Greenpak4BitstreamEntity(device, matrix, ibase, oword, cbase)
	, m_ffnum(ffnum)
	, m_hasSR(has_sr)
	, m_initValue(false)
	, m_srmode(false)
	, m_outputInvert(false)
	, m_latchMode(false)
	, m_input(device->GetGround())
	, m_clock(device->GetGround())
	, m_nsr(device->GetPower())
{
}

Greenpak4Flipflop::~Greenpak4Flipflop()
{

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Accessors

vector<string> Greenpak4Flipflop::GetInputPorts() const
{
	vector<string> r;
	r.push_back("D");
	r.push_back("CLK");
	r.push_back("nCLK");
	if(m_hasSR)
		r.push_back("nSR");
	return r;
}

void Greenpak4Flipflop::SetInput(string port, Greenpak4EntityOutput src)
{
	if( (port == "CLK") || (port == "nCLK") )
		m_clock = src;
	else if(port == "D")
		m_input = src;

	//multiple set/reset modes possible
	else if(port == "nSR")
		m_nsr = src;
	else if(port == "nSET")
	{
		m_srmode = true;
		m_nsr = src;
	}
	else if(port == "nRST")
	{
		m_srmode = false;
		m_nsr = src;
	}

	//ignore anything else silently (should not be possible since synthesis would error out)
}

Greenpak4EntityOutput Greenpak4Flipflop::GetInput(string port) const
{
	if( (port == "CLK") || (port == "nCLK") )
		return m_clock;
	else if(port == "D")
		return m_input;
	if( (port == "nSR") || (port == "nSET") || (port == "nRST") )
		return m_nsr;
	else
		return Greenpak4EntityOutput(NULL);
}

vector<string> Greenpak4Flipflop::GetOutputPorts() const
{
	vector<string> r;
	r.push_back("Q");
	r.push_back("nQ");
	return r;
}

/**
	@brief Q and nQ go to the same route and we can only use one at a time, so filter them
 */
vector<string> Greenpak4Flipflop::GetOutputPortsFiltered(bool* bitstream) const
{
	//Read the bitstream to see if we should be filtering.
	//Do not call Load() as we're const

	vector<string> r;
	if(bitstream[m_configBase + 1])
		r.push_back("nQ");
	else
		r.push_back("Q");
	return r;
}

vector<string> Greenpak4Flipflop::GetAllInputPorts() const
{
	vector<string> r;
	r.push_back("D");
	r.push_back("R");
	if(m_hasSR && !m_nsr.IsPowerRail())
	{
		if(m_srmode)
			r.push_back("nSET");
		else
			r.push_back("nRST");
	}
	if(m_latchMode)
		r.push_back("nCLK");
	else
		r.push_back("CLK");
	return r;
}

vector<string> Greenpak4Flipflop::GetAllOutputPorts() const
{
	vector<string> r;
	if(m_outputInvert)
		r.push_back("nQ");
	else
		r.push_back("Q");
	return r;
}

unsigned int Greenpak4Flipflop::GetOutputNetNumber(string port)
{
	if( (port == "Q") || (port == "nQ") )
		return m_outputBaseWord;
	else
		return -1;
}

string Greenpak4Flipflop::GetDescription() const
{
	char buf[128];
	snprintf(buf, sizeof(buf), "DFF_%u", m_ffnum);
	return string(buf);
}

string Greenpak4Flipflop::GetPrimitiveName() const
{
	string base = "GP_D";
	if(m_latchMode)
		base += "LATCH";
	else
		base += "FF";

	if(m_hasSR && !m_nsr.IsPowerRail())
	{
		if(m_srmode)
			base += "S";
		else
			base += "R";
	}

	if(m_outputInvert)
		base += "I";
	return base;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Serialization


map<string, string> Greenpak4Flipflop::GetParameters() const
{
	map<string, string> params;

	if(m_initValue)
		params["INIT"] = "1";
	else
		params["INIT"] = "0";

	return params;
}

bool Greenpak4Flipflop::CommitChanges()
{
	//Get our cell, or bail if we're unassigned
	auto ncell = dynamic_cast<Greenpak4NetlistCell*>(GetNetlistEntity());
	if(ncell == NULL)
		return true;

	//If our primitive name contains "LATCH" we're a latch, not a FF
	if(ncell->m_type.find("LATCH") != string::npos)
		m_latchMode = true;

	//If our primitive name ends in "I" we're inverting the output
	if(ncell->m_type[ncell->m_type.length()-1] == 'I')
		m_outputInvert = true;

	if(ncell->HasParameter("SRMODE"))
		m_srmode = (ncell->m_parameters["SRMODE"] == "1");

	if(ncell->HasParameter("INIT"))
		m_initValue = (ncell->m_parameters["INIT"] == "1");

	return true;
}

bool Greenpak4Flipflop::Load(bool* bitstream)
{
	//Read inputs (set/reset comes first, if present)
	int ibase = m_inputBaseWord;
	if(m_hasSR)
	{
		ReadMatrixSelector(bitstream, ibase + 0, m_matrix, m_nsr);
		ibase ++;
	}
	else
		m_nsr = m_device->GetPower();
	ReadMatrixSelector(bitstream, ibase + 0, m_matrix, m_input);
	ReadMatrixSelector(bitstream, ibase + 1, m_matrix, m_clock);

	//Read configuration
	m_latchMode = bitstream[m_configBase + 0];
	m_outputInvert = bitstream[m_configBase + 1];

	if(m_hasSR)
	{
		m_srmode = bitstream[m_configBase + 2];
		m_initValue = bitstream[m_configBase + 3];
	}
	else
		m_initValue = bitstream[m_configBase + 2];

	return true;
}

bool Greenpak4Flipflop::Save(bool* bitstream)
{
	//Sanity check: cannot have set/reset on a DFF, only a DFFSR
	bool has_sr = !m_nsr.IsPowerRail();
	if(has_sr && !m_hasSR)
		LogError("Tried to configure set/reset on a DFF cell with no S/R input\n");

	//Check if we're unused (input and clock pins are tied to ground)
	bool no_input = ( m_input.IsPowerRail() && !m_input.GetPowerRailValue() );
	bool no_clock = ( m_input.IsPowerRail() && !m_clock.GetPowerRailValue() );
	bool unused = (no_input && no_clock);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// INPUT BUS

	if(m_hasSR)
	{
		//Set/reset defaults to constant 1 if not hooked up
		//but if we have set/reset then use that.
		//If we're totally unused, hold us in reset
		Greenpak4EntityOutput sr = m_device->GetPower();
		if(unused)
			sr = m_device->GetGround();
		else if(has_sr)
			sr = m_nsr;
		if(!WriteMatrixSelector(bitstream, m_inputBaseWord + 0, sr))
			return false;

		//Hook up data and clock
		if(!WriteMatrixSelector(bitstream, m_inputBaseWord + 1, m_input))
			return false;
		if(!WriteMatrixSelector(bitstream, m_inputBaseWord + 2, m_clock))
			return false;
	}

	else
	{
		//Hook up data and clock
		if(!WriteMatrixSelector(bitstream, m_inputBaseWord + 0, m_input))
			return false;
		if(!WriteMatrixSelector(bitstream, m_inputBaseWord + 1, m_clock))
			return false;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Configuration

	//Mode select
	bitstream[m_configBase + 0] = m_latchMode;

	//Output polarity
	bitstream[m_configBase + 1] = m_outputInvert;

	if(m_hasSR)
	{
		//Set/reset mode
		bitstream[m_configBase + 2] = m_srmode;

		//Initial state
		bitstream[m_configBase + 3] = m_initValue;
	}

	else
	{
		//Initial state
		bitstream[m_configBase + 2] = m_initValue;
	}

	return true;
}

