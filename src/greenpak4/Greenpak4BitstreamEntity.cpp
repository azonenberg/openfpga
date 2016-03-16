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

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Construction / destruction

Greenpak4BitstreamEntity::Greenpak4BitstreamEntity(
	Greenpak4Device* device,
	unsigned int matrix,
	unsigned int ibase,
	unsigned int obase,
	unsigned int cbase
	)
	: m_device(device)
	, m_matrix(matrix)
	, m_inputBaseWord(ibase)
	, m_outputBaseWord(obase)
	, m_configBase(cbase)
	, m_parnode(NULL)
{
	
}

Greenpak4BitstreamEntity::~Greenpak4BitstreamEntity()
{
	
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Load/save helpers

bool Greenpak4BitstreamEntity::WriteMatrixSelector(
	bool* bitstream,
	unsigned int wordpos,
	Greenpak4BitstreamEntity* signal,
	bool cross_matrix)
{
	//SANITY CHECK - must be attached to the same matrix
	//cross connections use opposite, though
	if(cross_matrix)
	{
		if(m_matrix == signal->GetMatrix())
		{
			fprintf(stderr, "DRC fail: tried to write signal from same matrix through a cross connection\n");
			return false;
		}
	}
	else
	{
		if(m_matrix != signal->GetMatrix())
		{
			fprintf(stderr, "DRC fail: tried to write signal from opposite matrix without using a cross connection\n");
			return false;
		}
	}
	
	//Good to go, write it
	unsigned int sel = signal->GetOutputBase();
	
	//Calculate right matrix for cross connections etc
	unsigned int matrix = m_matrix;
	if(cross_matrix)
		matrix = 1 - matrix;
	
	unsigned int nbits = m_device->GetMatrixBits();
	unsigned int startbit = m_device->GetMatrixBase(matrix) + wordpos * nbits;
	
	//Need to flip bit ordering since lowest array index is the MSB
	for(unsigned int i=0; i<nbits; i++)
	{
		if( (sel >> i) & 1 )
			bitstream[startbit + i] = true;
		else
			bitstream[startbit + i] = false;
	}
	
	return true;
}
