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

#ifndef Greenpak4PairedEntity_h
#define Greenpak4PairedEntity_h

/**
	@brief A pair of entities muxed into one site
 */
class Greenpak4PairedEntity : public Greenpak4BitstreamEntity
{
public:

	//Construction / destruction
	Greenpak4PairedEntity(
		Greenpak4Device* device,
		unsigned int matrix,
		unsigned int select,
		Greenpak4BitstreamEntity* a,
		Greenpak4BitstreamEntity* b);
	virtual ~Greenpak4PairedEntity();

	//Serialization
	virtual bool Load(bool* bitstream);
	virtual bool Save(bool* bitstream);

	virtual std::string GetDescription() const;

	virtual void SetInput(std::string port, Greenpak4EntityOutput src);
	virtual Greenpak4EntityOutput GetInput(std::string port) const;
	virtual unsigned int GetOutputNetNumber(std::string port);

	virtual std::vector<std::string> GetInputPorts() const;
	virtual std::vector<std::string> GetOutputPorts() const;

	virtual bool CommitChanges();

	Greenpak4BitstreamEntity* GetEntity(std::string type)
	{ return m_entities[m_emap[type]]; }

	Greenpak4BitstreamEntity* GetActiveEntity() const
	{ return m_entities[m_activeEntity]; }

	void AddType(std::string type, bool entity);
	bool SetEntityType(std::string type);

protected:

	//Address of the select bit
	unsigned int m_select;

	//The currently selected entity
	bool m_activeEntity;

	//Map of HDL module names to entity types
	std::map<std::string, bool> m_emap;

	//The underlying entities
	Greenpak4BitstreamEntity* m_entities[2];
};

#endif
