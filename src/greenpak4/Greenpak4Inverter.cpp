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

#include "Greenpak4.h"
#include <stdio.h>
#include <stdlib.h>

using namespace std;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Construction / destruction

Greenpak4Inverter::Greenpak4Inverter(
		Greenpak4Device* device,
		unsigned int matrix,
		unsigned int ibase,
		unsigned int oword)
		: Greenpak4BitstreamEntity(device, matrix, ibase, oword, -1)
		, m_input(device->GetPowerRail(matrix, 0))
{
}

Greenpak4Inverter::~Greenpak4Inverter()
{
	
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Bitfile metadata

unsigned int Greenpak4Inverter::GetConfigLen()
{
	//no configuration other than the inputs
	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Accessors

string Greenpak4Inverter::GetDescription()
{
	char buf[128];
	snprintf(buf, sizeof(buf), "INV_%d_%d", m_matrix, m_inputBaseWord);
	return string(buf);
}

void Greenpak4Inverter::SetInput(Greenpak4BitstreamEntity* input)
{
	m_input = input;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Serialization of the truth table

bool Greenpak4Inverter::Load(bool* /*bitstream*/)
{
	//TODO: Do our inputs
	fprintf(stderr, "unimplemented\n");
	return false;
}

bool Greenpak4Inverter::Save(bool* bitstream)
{
	if(!WriteMatrixSelector(bitstream, m_inputBaseWord, m_input))
		return false;
		
	return true;
}
