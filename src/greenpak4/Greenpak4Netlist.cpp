/***********************************************************************************************************************
 * Copyright (C) 2017 Andrew Zonenberg and contributors                                                                *
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

Greenpak4NetlistEntity::~Greenpak4NetlistEntity()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Construction / destruction

Greenpak4Netlist::Greenpak4Netlist(string fname, string constraint_file)
	: m_topModule(NULL)
	, m_constraintFname(constraint_file)
	, m_parseOK(true)
{
	//Read the netlist
	FILE* fp = fopen(fname.c_str(), "rb");
	if(fp == NULL)
	{
		LogError("Failed to open netlist file %s\n", fname.c_str());
		m_parseOK = false;
		return;
	}
	if(0 != fseek(fp, 0, SEEK_END))
	{
		LogError("Failed to seek to end of netlist file %s\n", fname.c_str());
		m_parseOK = false;
		fclose(fp);
		return;
	}
	size_t len = ftell(fp);
	if(0 != fseek(fp, 0, SEEK_SET))
	{
		LogError("Failed to seek to start of netlist file %s\n", fname.c_str());
		m_parseOK = false;
		fclose(fp);
		return;
	}
	char* json_string = new char[len + 1];
	json_string[len] = '\0';
	if(len != fread(json_string, 1, len, fp))
	{
		LogError("Failed to read contents of netlist file %s\n", fname.c_str());
		m_parseOK = false;
		delete[] json_string;
		fclose(fp);
		return;
	}
	fclose(fp);

	//Parse the JSON
	json_tokener* tok = json_tokener_new();
	if(!tok)
	{
		LogError("Failed to create JSON tokenizer object\n");
		m_parseOK = false;
		delete[] json_string;
		return;
	}
	json_tokener_error err;
	json_object* object = json_tokener_parse_verbose(json_string, &err);
	if(NULL == object)
	{
		const char* desc = json_tokener_error_desc(err);
		LogError("JSON parsing failed (err = %s)\n", desc);
		m_parseOK = false;
		delete[] json_string;
		return;
	}

	//Read stuff from it
	Load(object);

	//Clean up
	json_object_put(object);
	json_tokener_free(tok);
	delete[] json_string;
}

Greenpak4Netlist::~Greenpak4Netlist()
{
	//Delete modules
	for(auto x : m_modules)
		delete x.second;
	m_modules.clear();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Parsing stuff

void Greenpak4Netlist::LoadConstraints()
{
	//Read the constraint file (if we have one)
	if(m_constraintFname == "")
		return;
	FILE* fp = fopen(m_constraintFname.c_str(), "r");
	if(fp == NULL)
	{
		LogError("Failed to open constraint file %s\n", m_constraintFname.c_str());
		m_parseOK = false;
		return;
	}
	LoadConstraints(fp);
	fclose(fp);
}

/**
	@brief Parsing for a PCF file
 */
void Greenpak4Netlist::LoadConstraints(FILE* fp)
{
	char line[1024];
	while(NULL != fgets(line, sizeof(line), fp))
		LoadConstraint(line);
}

void Greenpak4Netlist::LoadConstraint(const char* line)
{
	//Remove leading spaces
	while(isspace(line[0]))
		line++;

	//Skip empty lines and comments
	if(line[0] == '\0')
		return;
	if(line[0] == '#')
		return;

	//Parse the constraint
	char target[512];
	char name[512];
	char value[512];
	if(3 != sscanf(line, "set_%511s %511s %511s", name, target, value))
	{
		LogError("Ignoring malformed constraint %s\n", line);
		return;
	}

	//Convert the name to uppercase
	for(int i=0; i<512 && name[i]; i++)
		name[i] = toupper(name[i]);

	//Apply it
	m_topModule->AddWireAttribute(target, name, value);
}

/**
	@brief Top-level parsing routine

	Should only have creator and modules
 */
void Greenpak4Netlist::Load(json_object* object)
{
	json_object_iterator end = json_object_iter_end(object);
	for(json_object_iterator it = json_object_iter_begin(object);
		!json_object_iter_equal(&it, &end);
		json_object_iter_next(&it))
	{
		//See what we got
		string name = json_object_iter_peek_name(&it);
		json_object* child = json_object_iter_peek_value(&it);

		//Creator of the file (expecting a string)
		if(name == "creator")
		{
			if(!json_object_is_type(child, json_type_string))
			{
				LogError("netlist creator should be of type string but isn't\n");
				m_parseOK = false;
				return;
			}
			m_creator = json_object_get_string(child);
			LogNotice("Netlist creator: %s\n", m_creator.c_str());
		}

		//Modules in the file (expecting an object)
		else if(name == "modules")
		{
			if(!json_object_is_type(child, json_type_object))
			{
				LogError("netlist modules should be of type object but isn't\n");
				m_parseOK = false;
				return;
			}

			//Load them
			LoadModules(child);
		}

		//Something bad
		else
		{
			LogError("Unknown top-level JSON object \"%s\"\n", name.c_str());
			m_parseOK = false;
			return;
		}
	}

	IndexNets();
}

/**
	@brief Destroy all index data.
 */
void Greenpak4Netlist::ClearIndexes()
{
	for(auto node : m_nodes)
	{
		node->m_nodeports.clear();
		node->m_ports.clear();
	}

	m_nodes.clear();
}

/**
	@brief Force a re-index after changing the netlist (by PAR-level optimizations etc)
 */
void Greenpak4Netlist::Reindex()
{
	ClearIndexes();
	IndexNets();
}

/**
	@brief Index the nets so that each net has a list of cell ports it connects to.

	Has to be done as a second pass because there may be cycles in the netlist preventing us from resolving names
	as we parse the JSON
 */
void Greenpak4Netlist::IndexNets()
{
	LogTrace("Indexing...\n");

	LogIndenter li;

	//Loop over all of our ports and add them to the associated nets
	for(auto it = m_topModule->port_begin(); it != m_topModule->port_end(); it ++)
	{
		Greenpak4NetlistPort* port = it->second;
		LogTrace("Port %s connects to:\n", it->first.c_str());
		LogIndenter li;

		for(unsigned int i=0; i<port->m_nodes.size(); i++)
		{
			auto x = port->m_nodes[i];
			LogTrace("bit %u: node %s\n", i, port->m_nodes[i]->m_name.c_str());
			x->m_ports.push_back(port);
		}
	}

	//Loop over all of our cells and add their connections
	for(auto it = m_topModule->cell_begin(); it != m_topModule->cell_end(); it ++)
	{
		Greenpak4NetlistCell* cell = it->second;
		LogTrace("Cell %s connects to:\n", it->first.c_str());
		LogIndenter li;
		for(auto jt : cell->m_connections)
		{
			string cellname = jt.first;
			auto net = jt.second;
			bool vector = false;
			if(net.size() != 1)
				vector = true;
			for(unsigned int i=0; i<net.size(); i++)
			{
				Greenpak4NetlistNode* node = net[i];
				if(vector)
					LogTrace("%s[%u]: net %s\n", cellname.c_str(), i, node->m_name.c_str());
				else
					LogTrace("%s: net %s\n", cellname.c_str(), node->m_name.c_str());
				node->m_nodeports.push_back(Greenpak4NetlistNodePoint(cell, cellname, i, vector));
			}
		}
	}

	//Make a set of the nodes to avoid duplication.
	//Note that NULL is legal in vector nets if some bits were optimized out
	for(auto it = m_topModule->net_begin(); it != m_topModule->net_end(); it ++)
	{
		if(it->second != NULL)
			m_nodes.insert(it->second);
	}

	//Print them out
	for(auto node : m_nodes)
	{
		LogTrace("Node %s connects to:\n", node->m_name.c_str());
		LogIndenter li;
		for(auto p : node->m_ports)
			LogTrace("port %s\n", p->m_name.c_str());
		for(auto c : node->m_nodeports)
			LogTrace("cell %s port %s\n", c.m_cell->m_name.c_str(), c.m_portname.c_str());
	}
}

/**
	@brief Module parsing routine

	Loads all of the modules in the netlist
 */
void Greenpak4Netlist::LoadModules(json_object* object)
{
	LogNotice("\nLoading modules...\n");
	LogIndenter li;

	json_object_iterator end = json_object_iter_end(object);
	for(json_object_iterator it = json_object_iter_begin(object);
		!json_object_iter_equal(&it, &end);
		json_object_iter_next(&it))
	{
		//See what we got
		string name = json_object_iter_peek_name(&it);
		json_object* child = json_object_iter_peek_value(&it);

		//Verify it's an object
		if(!json_object_is_type(child, json_type_object))
		{
			LogError("netlist module entry should be of type object but isn't\n");
			m_parseOK = false;
			return;
		}

		//TODO: If the child object is a standard library cell, don't bother parsing it?

		//Load it
		Greenpak4NetlistModule *module = new Greenpak4NetlistModule(this, name, child);
		if(!module->Validate())
		{
			delete module;
			m_parseOK = false;
			return;
		}
		m_modules[name] = module;

		//Did we get a top-level module?
		if(module->m_attributes.find("top") != module->m_attributes.end())
		{
			if(m_topModule)
			{
				LogError("More than one top-level module in netlist\n");
				m_parseOK = false;
				return;
			}
			m_topModule = module;
		}
	}

	//Verify we got the top-level module we expected
	if(m_topModule == NULL)
	{
		LogError("Unable to find a top-level module in netlist\n");
		m_parseOK = false;
		return;
	}
}
