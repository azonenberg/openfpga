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

#ifndef Greenpak4Netlist_h
#define Greenpak4Netlist_h

#include <string>
#include <map>
#include <set>

#include <json-c/json.h>

#include "Greenpak4NetlistModule.h"

/**
	@brief An UNPLACED netlist for a Greenpak4 device
 */
class Greenpak4Netlist
{
public:
	Greenpak4Netlist(std::string fname, std::string top);
	virtual ~Greenpak4Netlist();
	
	Greenpak4NetlistModule* GetTopModule()
	{ return m_topModule; }
	
	typedef std::set<Greenpak4NetlistNode*> nodeset;
	
	nodeset::iterator nodebegin()
	{ return m_nodes.begin(); }
	
	nodeset::iterator nodeend()
	{ return m_nodes.end(); }
	
	Greenpak4NetlistModule* GetModule(std::string name)
	{ return m_modules[name]; }
	
protected:
	
	void IndexNets();
	
	//Init helpers
	void Load(json_object* object);
	void LoadModules(json_object* object);
		
	std::string m_topModuleName;
	std::string m_creator;
	
	//All of the modules in the netlist
	std::map<std::string, Greenpak4NetlistModule*> m_modules;
	
	//The top-level module
	Greenpak4NetlistModule* m_topModule;
	
	//All of the nets in the netlist
	nodeset m_nodes;
};

#endif
