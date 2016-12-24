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

Greenpak4SPI::Greenpak4SPI(
		Greenpak4Device* device,
		unsigned int matrix,
		unsigned int ibase,
		unsigned int obase,
		unsigned int cbase)
		: Greenpak4BitstreamEntity(device, matrix, ibase, obase, cbase)
		, m_csn(device->GetGround())
		, m_useAsBuffer(false)
		, m_cpha(false)
		, m_cpol(false)
		, m_width8Bits(false)
		, m_dirIsOutput(false)
		, m_parallelOutputToFabric(false)
{
}

Greenpak4SPI::~Greenpak4SPI()
{

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Accessors

string Greenpak4SPI::GetDescription()
{
	//only one SPI core
	return "SPI_0";
}

vector<string> Greenpak4SPI::GetInputPorts() const
{
	vector<string> r;
	//SCK must come from clock buffer
	r.push_back("CSN");
	//SDAT is dedicated routing
	return r;
}

void Greenpak4SPI::SetInput(string port, Greenpak4EntityOutput src)
{
	if(port == "CSN")
		m_csn = src;

	//Ignore inputs from dedicated routing as there's not much muxing going on

	//ignore anything else silently (should not be possible since synthesis would error out)
}

vector<string> Greenpak4SPI::GetOutputPorts() const
{
	vector<string> r;
	r.push_back("INT");
	return r;
}

unsigned int Greenpak4SPI::GetOutputNetNumber(string port)
{
	if(port == "INT")
		return m_outputBaseWord;
	else
		return -1;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Serialization

bool Greenpak4SPI::CommitChanges()
{
	//Get our cell, or bail if we're unassigned
	auto ncell = dynamic_cast<Greenpak4NetlistCell*>(GetNetlistEntity());
	if(ncell == NULL)
		return true;

	if(ncell->HasParameter("DATA_WIDTH"))
	{
		int width = atoi(ncell->m_parameters["DATA_WIDTH"].c_str());
		if(width == 8)
			m_width8Bits = true;
		else if(width == 16)
			m_width8Bits = false;
		else
		{
			LogError("Greenpak4SPI: only supported data widths are 8 and 16\n");
			return false;
		}
	}

	if(ncell->HasParameter("SPI_CPHA"))
		m_cpha = atoi(ncell->m_parameters["SPI_CPHA"].c_str()) ? true : false;

	if(ncell->HasParameter("SPI_CPOL"))
		m_cpol = atoi(ncell->m_parameters["SPI_CPOL"].c_str()) ? true : false;

	if(ncell->HasParameter("DIRECTION"))
	{
		auto s = ncell->m_parameters["DIRECTION"];
		if(s == "INPUT")
			m_dirIsOutput = false;
		else if(s == "OUTPUT")
			m_dirIsOutput = true;
		else
		{
			LogError("Greenpak4SPI: Direction must be OUTPUT or INPUT\n");
			return false;
		}
	}

	return true;
}

bool Greenpak4SPI::Load(bool* /*bitstream*/)
{
	//TODO: Do our inputs
	LogError("Unimplemented\n");
	return false;
}

bool Greenpak4SPI::Save(bool* bitstream)
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// INPUT BUS

	if(!WriteMatrixSelector(bitstream, m_inputBaseWord, m_csn))
		return false;

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// CONFIGURATION

	//SPI as ADC buffer
	bitstream[m_configBase + 0] = m_useAsBuffer;

	//+1 = input source (FSMs or ADC)

	//Clock phase/polarity
	bitstream[m_configBase + 2] = m_cpha;
	bitstream[m_configBase + 3] = m_cpol;

	//Width selector
	bitstream[m_configBase + 4] = m_width8Bits;

	//Direction
	bitstream[m_configBase + 5] = m_dirIsOutput;

	//Parallel output enable
	bitstream[m_configBase + 6] = m_parallelOutputToFabric;

	//TODO: SDIO mux selector

	return true;
}
