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

#ifndef Greenpak4NetlistModule_h
#define Greenpak4NetlistModule_h

#include <string>
#include <vector>
#include <json-c/json.h>

class Greenpak4Netlist;
class Greenpak4NetlistPort;
class Greenpak4NetlistNode;

//A single named net in the netlist (may be one or multiple bits)
class Greenpak4NetlistNet
{
public:
	std::string m_name;
	std::vector<Greenpak4NetlistNode*> m_nodes;
	std::map<std::string, std::string> m_attributes;
	
	bool HasAttribute(std::string name)
	{ return (m_attributes.find(name) != m_attributes.end() ); }
	
	std::string GetAttribute(std::string name)
	{ return m_attributes[name]; }
};

//A single primitive cell in the netlist
class Greenpak4NetlistCell
{
public:
	virtual ~Greenpak4NetlistCell();

	std::string m_name;
	std::string m_type;
	
	std::map<std::string, std::string> m_parameters;
	std::map<std::string, std::string> m_attributes;
	
	std::map<std::string, Greenpak4NetlistNet* > m_connections;
};

/**
	@brief A single module in a Greenpak4Netlist
 */
class Greenpak4NetlistModule
{
public:
	Greenpak4NetlistModule(Greenpak4Netlist* parent, std::string name, json_object* object);
	virtual ~Greenpak4NetlistModule();
	
	Greenpak4NetlistNode* GetNode(int32_t netnum);
	
	std::string GetName()
	{ return m_name; }
	
	typedef std::map<std::string, Greenpak4NetlistPort*> portmap;
	typedef std::map<std::string, Greenpak4NetlistCell*> cellmap;
	
	portmap::iterator port_begin()
	{ return m_ports.begin(); }
	
	portmap::iterator port_end()
	{ return m_ports.end(); }
	
	cellmap::iterator cell_begin()
	{ return m_cells.begin(); }
	
	cellmap::iterator cell_end()
	{ return m_cells.end(); }
	
	bool HasNet(std::string name)
	{ return (m_nets.find(name) != m_nets.end()); }
	
	Greenpak4NetlistNet* GetNet(std::string name)
	{ return m_nets[name]; }
	
protected:
	Greenpak4Netlist* m_parent;
	
	std::string m_name;
	
	void LoadNetName(std::string name, json_object* object);
	void LoadNetAttributes(Greenpak4NetlistNet* net, json_object* object);
	void LoadCell(std::string name, json_object* object);
	void LoadCellAttributes(Greenpak4NetlistCell* cell, json_object* object);
	void LoadCellParameters(Greenpak4NetlistCell* cell, json_object* object);
	void LoadCellConnections(Greenpak4NetlistCell* cell, json_object* object);
	
	std::map<int32_t, Greenpak4NetlistNode*> m_nodes;
	portmap m_ports;
	std::map<std::string, Greenpak4NetlistNet*> m_nets;
	cellmap m_cells;
};

#endif
