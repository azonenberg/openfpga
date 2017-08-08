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

Greenpak4PairedEntity::Greenpak4PairedEntity(
	Greenpak4Device* device,
	unsigned int matrix,
	unsigned int select,
	Greenpak4BitstreamEntity* a,
	Greenpak4BitstreamEntity* b)
	: Greenpak4BitstreamEntity(device, matrix, -1, -1, select)
	, m_select(select)
	, m_activeEntity(false)
{
	m_entities[0] = a;
	m_entities[1] = b;
}

Greenpak4PairedEntity::~Greenpak4PairedEntity()
{
	//Delete both entities (we own them)
	delete m_entities[0];
	delete m_entities[1];
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Accessors

string Greenpak4PairedEntity::GetDescription() const
{
	return GetActiveEntity()->GetDescription();
}

vector<string> Greenpak4PairedEntity::GetAllInputPorts() const
{
	return GetActiveEntity()->GetAllInputPorts();
}

vector<string> Greenpak4PairedEntity::GetInputPorts() const
{
	auto pa = m_entities[0]->GetInputPorts();
	auto pb = m_entities[1]->GetInputPorts();

	//return the union of both devices' ports
	set<string> p;
	for(auto s : pa)
		p.emplace(s);
	for(auto s : pb)
		p.emplace(s);

	vector<string> r;
	for(auto s : p)
		r.push_back(s);
	return r;
}

void Greenpak4PairedEntity::SetInput(string port, Greenpak4EntityOutput src)
{
	GetActiveEntity()->SetInput(port, src);
}

Greenpak4EntityOutput Greenpak4PairedEntity::GetInput(string port) const
{
	return GetActiveEntity()->GetInput(port);
}

vector<string> Greenpak4PairedEntity::GetOutputPorts() const
{
	auto pa = m_entities[0]->GetOutputPorts();
	auto pb = m_entities[1]->GetOutputPorts();

	//return the union of both devices' ports
	set<string> p;
	for(auto s : pa)
		p.emplace(s);
	for(auto s : pb)
		p.emplace(s);

	vector<string> r;
	for(auto s : p)
		r.push_back(s);
	return r;
}

unsigned int Greenpak4PairedEntity::GetOutputNetNumber(string port)
{
	return GetActiveEntity()->GetOutputNetNumber(port);
}

string Greenpak4PairedEntity::GetPrimitiveName() const
{
	return GetActiveEntity()->GetPrimitiveName();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Module selection

void Greenpak4PairedEntity::AddType(string type, bool entity)
{
	m_emap[type] = entity;
}

bool Greenpak4PairedEntity::SetEntityType(string type)
{
	if(m_emap.find(type) == m_emap.end())
		return false;

	m_activeEntity = m_emap[type];

	//Need to copy the PAR node over so it can reference things
	GetActiveEntity()->SetPARNode(m_parnode);

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Serialization

bool Greenpak4PairedEntity::CommitChanges()
{
	//Get our cell, or bail if we're unassigned
	auto ncell = dynamic_cast<Greenpak4NetlistCell*>(GetNetlistEntity());
	if(ncell == NULL)
		return true;

	//Select the right entity based on which type we're using
	if(!SetEntityType(ncell->m_type))
		return false;

	//and commit those changes
	return GetActiveEntity()->CommitChanges();
}

bool Greenpak4PairedEntity::Load(bool* bitstream)
{
	m_activeEntity = bitstream[m_configBase];
	return GetActiveEntity()->Load(bitstream);
}

bool Greenpak4PairedEntity::Save(bool* bitstream)
{
	//Write the select bit
	bitstream[m_configBase] = m_activeEntity;

	//and the config data
	return GetActiveEntity()->Save(bitstream);
}
