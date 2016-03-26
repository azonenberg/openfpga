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

#include "Greenpak4CrossConnection.h"
#include <stdio.h>
#include <stdlib.h>

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
		, m_input(NULL)
{
}

Greenpak4CrossConnection::~Greenpak4CrossConnection()
{
	
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Bitfile metadata

unsigned int Greenpak4CrossConnection::GetConfigLen()
{
	//no configuration other than the inputs
	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Accessors

string Greenpak4CrossConnection::GetDescription()
{
	char buf[128];
	snprintf(buf, sizeof(buf), "XCONN_%d_%d", m_matrix, m_inputBaseWord);
	return string(buf);
}

void Greenpak4CrossConnection::SetInput(Greenpak4BitstreamEntity* input)
{
	if(input->GetMatrix() == m_matrix)
	{
		fprintf(stderr, "INTERNAL ERROR: tried to set cross-connection input from wrong matrix\n");
		exit(-1);
	}
	
	m_input = input;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Serialization of the truth table

bool Greenpak4CrossConnection::Load(bool* /*bitstream*/)
{
	//TODO: Do our inputs
	fprintf(stderr, "unimplemented\n");
	return false;
}

bool Greenpak4CrossConnection::Save(bool* bitstream)
{
	if(!WriteMatrixSelector(bitstream, m_inputBaseWord, m_input, true))
		return false;
		
	return true;
}
