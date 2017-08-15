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

Greenpak4PatternGenerator::Greenpak4PatternGenerator(
	Greenpak4Device* device,
	unsigned int matrix,
	unsigned int ibase,
	unsigned int oword,
	unsigned int cbase)
	: Greenpak4BitstreamEntity(device,  matrix, ibase, oword, cbase)
	, m_clk(device->GetGround())
	, m_reset(device->GetGround())
	, m_patternLen(2)
{
	for(unsigned int i=0; i<16; i++)
		m_truthtable[i] = false;
}

Greenpak4PatternGenerator::~Greenpak4PatternGenerator()
{

}

string Greenpak4PatternGenerator::GetDescription() const
{
	return "PGEN_0";
}

unsigned int Greenpak4PatternGenerator::GetOutputNetNumber(string port)
{
	if(port == "OUT")
		return m_outputBaseWord;
	else
		return -1;
}

string Greenpak4PatternGenerator::GetPrimitiveName() const
{
	return "GP_PGEN";
}

map<string, string> Greenpak4PatternGenerator::GetParameters() const
{
	map<string, string> params;

	int table = 0;
	for(int i=0; i<16; i++)
		table |= (m_truthtable[i] << i);

	char tmp[128];
	snprintf(tmp, sizeof(tmp), "%d", table);
	params["PATTERN_DATA"] = tmp;

	snprintf(tmp, sizeof(tmp), "%d", m_patternLen);
	params["PATTERN_LEN"] = tmp;

	return params;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Pattern generator specific logic

bool Greenpak4PatternGenerator::CommitChanges()
{
	//Get our cell, or bail if we're unassigned
	auto ncell = dynamic_cast<Greenpak4NetlistCell*>(GetNetlistEntity());
	if(ncell == NULL)
		return true;

	for(auto x : ncell->m_parameters)
	{
		if(x.first == "PATTERN_DATA")
		{
			//convert to bit array format
			uint32_t truth_table = atoi(x.second.c_str());
			for(unsigned int i=0; i<16; i++)
			{
				bool a3 = (i & 8) ? true : false;
				bool a2 = (i & 4) ? true : false;
				bool a1 = (i & 2) ? true : false;
				bool a0 = (i & 1) ? true : false;
				m_truthtable[a3*8 | a2*4 | a1*2 | a0] = (truth_table & (1 << i)) ? true : false;
			}
		}

		else if(x.first == "PATTERN_LEN")
		{
			m_patternLen = atoi(x.second.c_str());
			if( (m_patternLen < 2) || (m_patternLen > 16) )
			{
				LogError("GP_PGEN PATTERN_LEN must be between 2 and 16 (requested %d)\n", m_patternLen);
				return false;
			}
		}

		else
		{
			LogWarning("Cell \"%s\" has unrecognized parameter %s, ignoring\n",
				ncell->m_name.c_str(), x.first.c_str());
		}
	}

	return true;
}

vector<string> Greenpak4PatternGenerator::GetOutputPorts() const
{
	vector<string> ret;
	ret.push_back("OUT");
	return ret;
}


vector<string> Greenpak4PatternGenerator::GetInputPorts() const
{
	vector<string> ret;
	ret.push_back("nRST");
	ret.push_back("CLK");
	return ret;
}

void Greenpak4PatternGenerator::SetInput(string port, Greenpak4EntityOutput src)
{
	if(port == "CLK")
		m_clk = src;

	else if(port == "nRST")
		m_reset = src;
}

Greenpak4EntityOutput Greenpak4PatternGenerator::GetInput(string port) const
{
	if(port == "CLK")
		return m_clk;
	else if(port == "nRST")
		return m_reset;
	else
		return Greenpak4EntityOutput(NULL);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Serialization

bool Greenpak4PatternGenerator::Load(bool* bitstream)
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// INPUT BUS

	ReadMatrixSelector(bitstream, 2, m_matrix, m_clk);
	ReadMatrixSelector(bitstream, 3, m_matrix, m_reset);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// CONFIGURATION

	//The pattern we generate
	for(unsigned int i=0; i<16; i++)
		m_truthtable[i] = bitstream[m_configBase + i];

	//4-bit counter data
	m_patternLen =
		(
			bitstream[m_configBase + 16] |
			(bitstream[m_configBase + 17] << 1) |
			(bitstream[m_configBase + 18] << 2) |
			(bitstream[m_configBase + 19] << 3)
		) + 1;

	return true;
}

bool Greenpak4PatternGenerator::Save(bool* bitstream)
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// INPUT BUS

	//0/1 are unused in PGEN mode, tie them to ground
	if(!WriteMatrixSelector(bitstream, m_inputBaseWord + 0, m_device->GetGround()))
		return false;
	if(!WriteMatrixSelector(bitstream, m_inputBaseWord + 1, m_device->GetGround()))
		return false;

	//Write clock and reset
	if(!WriteMatrixSelector(bitstream, m_inputBaseWord + 2, m_clk))
		return false;
	if(!WriteMatrixSelector(bitstream, m_inputBaseWord + 3, m_reset))
		return false;

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// CONFIGURATION

	//The pattern we generate
	for(unsigned int i=0; i<16; i++)
		bitstream[m_configBase + i] = m_truthtable[i];

	//4-bit counter data
	int len = m_patternLen - 1;
	bitstream[m_configBase + 16] = (len & 1) ? true : false;
	bitstream[m_configBase + 17] = (len & 2) ? true : false;
	bitstream[m_configBase + 18] = (len & 4) ? true : false;
	bitstream[m_configBase + 19] = (len & 8) ? true : false;

	return true;
}
