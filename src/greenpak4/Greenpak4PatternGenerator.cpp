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

}

Greenpak4PatternGenerator::~Greenpak4PatternGenerator()
{

}

string Greenpak4PatternGenerator::GetDescription()
{
	return "PGEN0";
}

unsigned int Greenpak4PatternGenerator::GetOutputNetNumber(string port)
{
	if(port == "OUT")
		return m_outputBaseWord;
	else
		return -1;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Pattern generator specific logic

bool Greenpak4PatternGenerator::CommitChanges()
{
	/*
	//Get our cell, or bail if we're unassigned
	auto ncell = dynamic_cast<Greenpak4NetlistCell*>(GetNetlistEntity());
	if(ncell == NULL)
		return true;

	//It's a pattern generator
	if(ncell->m_type == "GP_PGEN")
	{
		m_pgenMode = true;

		for(auto x : ncell->m_parameters)
		{
			if(x.first == "PATTERN_DATA")
			{
				//convert to bit array format
				uint32_t truth_table = atoi(x.second.c_str());
				unsigned int nbits = 1 << m_order;
				for(unsigned int i=0; i<nbits; i++)
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

	//Nope, it's a LUT (or remapped INV etc)
	else
	{
		m_pgenMode = false;
		return Greenpak4LUT::CommitChanges();
	}
	*/
	return false;
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

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Serialization

bool Greenpak4PatternGenerator::Load(bool* bitstream)
{
	LogError("unimplemented\n");
	return false;
}

bool Greenpak4PatternGenerator::Save(bool* bitstream)
{
	/*
	//Save the mode regardless
	bitstream[m_configBase + 20] = m_pgenMode;

	//Save pattern gen stuff
	if(m_pgenMode)
	{
		//Pattern data is stored in LUT truth table

		//4-bit counter data
		int len = m_patternLen - 1;
		bitstream[m_configBase + 16] = (len & 1) ? true : false;
		bitstream[m_configBase + 17] = (len & 2) ? true : false;
		bitstream[m_configBase + 18] = (len & 4) ? true : false;
		bitstream[m_configBase + 19] = (len & 8) ? true : false;
	}

	//Save LUT stuff (input bus etc) regardless
	return Greenpak4LUT::Save(bitstream);
	*/

	return false;
}
