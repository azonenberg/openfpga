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

#include <Greenpak4.h>

using namespace std;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Construction / destruction

Greenpak4DualEntity::Greenpak4DualEntity(Greenpak4BitstreamEntity* dual)
	: Greenpak4BitstreamEntity(
		dual->GetDevice(),
		1 - dual->GetMatrix(),
		0,	//no inputs
		dual->GetInternalOutputBase(),
		0)	//no configuration
{
	m_dual = dual;
	m_dualMaster = false;
}

Greenpak4DualEntity::~Greenpak4DualEntity()
{

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Accessors

string Greenpak4DualEntity::GetDescription() const
{
	return m_dual->GetDescription();
}

vector<string> Greenpak4DualEntity::GetInputPorts() const
{
	return m_dual->GetInputPorts();
}

vector<string> Greenpak4DualEntity::GetOutputPorts() const
{
	return m_dual->GetOutputPorts();
}

void Greenpak4DualEntity::SetInput(string port, Greenpak4EntityOutput src)
{
	m_dual->SetInput(port, src);
}

Greenpak4EntityOutput Greenpak4DualEntity::GetInput(string port) const
{
	return m_dual->GetInput(port);
}

Greenpak4EntityOutput Greenpak4DualEntity::GetOutput(std::string port)
{
	return Greenpak4EntityOutput(m_dual, port, m_matrix);
}

unsigned int Greenpak4DualEntity::GetOutputNetNumber(string port)
{
	return m_dual->GetOutputNetNumber(port);
}

string Greenpak4DualEntity::GetPrimitiveName() const
{
	return m_dual->GetPrimitiveName();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Serialization

bool Greenpak4DualEntity::CommitChanges()
{
	//no action needed, we have no input pins to drive and no configuration
	return true;
}

bool Greenpak4DualEntity::Load(bool* /*bitstream*/)
{
	return true;
}

bool Greenpak4DualEntity::Save(bool* /*bitstream*/)
{
	return true;
}
