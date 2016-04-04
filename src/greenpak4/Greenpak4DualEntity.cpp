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

using namespace std;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Construction / destruction

Greenpak4DualEntity::Greenpak4DualEntity(Greenpak4BitstreamEntity* dual)
	: Greenpak4BitstreamEntity(
		dual->GetDevice(),
		1 - dual->GetMatrix(),
		0,	//no inputs
		dual->GetOutputBase(),
		0)	//no configuration
	, m_dual(dual)
{

}

Greenpak4DualEntity::~Greenpak4DualEntity()
{
	
}

Greenpak4BitstreamEntity* Greenpak4DualEntity::GetRealEntity()
{
	return m_dual;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Bitfile metadata

unsigned int Greenpak4DualEntity::GetConfigLen()
{
	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Accessors

string Greenpak4DualEntity::GetDescription()
{
	return m_dual->GetDescription();
}

vector<string> Greenpak4DualEntity::GetInputPorts()
{
	return m_dual->GetInputPorts();
}

vector<string> Greenpak4DualEntity::GetOutputPorts()
{
	return m_dual->GetOutputPorts();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Serialization

void Greenpak4DualEntity::CommitChanges()
{
	//no action needed, we have no input pins to drive and no configuration
}

bool Greenpak4DualEntity::Load(bool* /*bitstream*/)
{
	return true;
}

bool Greenpak4DualEntity::Save(bool* /*bitstream*/)
{
	return true;
}
