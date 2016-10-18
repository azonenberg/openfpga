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

Greenpak4CrossConnection::Greenpak4CrossConnection(
		Greenpak4Device* device,
		unsigned int matrix,
		unsigned int ibase,
		unsigned int oword,
		unsigned int cbase)
		: Greenpak4BitstreamEntity(device, matrix, ibase, oword, cbase)
		, m_input(device->GetGround())
{
}

Greenpak4CrossConnection::~Greenpak4CrossConnection()
{

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Accessors

string Greenpak4CrossConnection::GetDescription()
{
	char buf[128];
	snprintf(buf, sizeof(buf), "XCONN_%u_%u", m_matrix, m_inputBaseWord);
	return string(buf);
}

void Greenpak4CrossConnection::SetInput(std::string /*port*/, Greenpak4EntityOutput input)
{
	//Don't complain if input is a power rail, those are the sole exception
	if(input.IsPowerRail())
	{}

	//Complain if input has a dual, they should never go through the cross connections
	else if(input.HasDual())
	{
		LogError("Tried to set cross-connection input from node with dual. This is probably a bug.\n");
		return;
	}

	else if(input.GetMatrix() == m_matrix)
	{
		LogError("Tried to set cross-connection input from wrong matrix. This is probably a bug.\n");
		return;
	}

	m_input = input;
}

bool Greenpak4CrossConnection::CommitChanges()
{
	//nothing to do here, we have no configuration to modify
	return true;
}

vector<string> Greenpak4CrossConnection::GetInputPorts() const
{
	vector<string> r;
	r.push_back("I");
	return r;
}

vector<string> Greenpak4CrossConnection::GetOutputPorts() const
{
	vector<string> r;
	r.push_back("O");
	return r;
}

unsigned int Greenpak4CrossConnection::GetOutputNetNumber(string /*port*/)
{
	//we respond to any net name for convenience
	return m_outputBaseWord;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Load/save logic

bool Greenpak4CrossConnection::Load(bool* /*bitstream*/)
{
	//TODO: Do our inputs
	LogError("Unimplemented\n");
	return false;
}

bool Greenpak4CrossConnection::Save(bool* bitstream)
{
	if(!WriteMatrixSelector(bitstream, m_inputBaseWord, m_input, true))
		return false;

	return true;
}
