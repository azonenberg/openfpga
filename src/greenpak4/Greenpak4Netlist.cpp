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

Greenpak4NetlistEntity::~Greenpak4NetlistEntity()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Construction / destruction

Greenpak4Netlist::Greenpak4Netlist(std::string fname, std::string top)
	: m_topModuleName(top)
	, m_topModule(NULL)
{
	//Read the netlist
	FILE* fp = fopen(fname.c_str(), "r");
	if(fp == NULL)
	{
		fprintf(stderr, "ERROR: Failed to open netlist file %s\n", fname.c_str());
		exit(-1);
	}
	if(0 != fseek(fp, 0, SEEK_END))
	{
		fprintf(stderr, "ERROR: Failed to seek to end of netlist file %s\n", fname.c_str());
		exit(-1);
	}
	size_t len = ftell(fp);
	if(0 != fseek(fp, 0, SEEK_SET))
	{
		fprintf(stderr, "ERROR: Failed to seek to start of netlist file %s\n", fname.c_str());
		exit(-1);
	}
	char* json_string = new char[len];
	if(len != fread(json_string, 1, len, fp))
	{
		fprintf(stderr, "ERROR: Failed read contents of netlist file %s\n", fname.c_str());
		exit(-1);
	}
	fclose(fp);
	
	//Parse the JSON
	json_tokener* tok = json_tokener_new();
	if(!tok)
	{
		fprintf(stderr, "ERROR: Failed to create JSON tokenizer object\n");
		exit(-1);
	}
	json_tokener_error err;
	json_object* object = json_tokener_parse_verbose(json_string, &err);
	if(NULL == object)
	{
		const char* desc = json_tokener_error_desc(err);
		fprintf(stderr, "ERROR: JSON parsing failed (err = %s)\n", desc);
		exit(-1);
	}
	
	//Read stuff from it
	Load(object);
	
	//Clean up
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
				fprintf(stderr, "ERROR: netlist creator should be of type string but isn't\n");
				exit(-1);
			}
			m_creator = json_object_get_string(child);
			printf("Netlist creator: %s\n", m_creator.c_str());
		}
		
		//Modules in the file (expecting an object)
		else if(name == "modules")
		{
			if(!json_object_is_type(child, json_type_object))
			{
				fprintf(stderr, "ERROR: netlist modules should be of type object but isn't\n");
				exit(-1);
			}
			
			//Load them
			LoadModules(child);
		}
		
		//Something bad
		else
		{
			fprintf(stderr, "ERROR: Unknown top-level JSON object \"%s\"\n", name.c_str());
			exit(-1);
		}
	}
	
	IndexNets();
}

/**
	@brief Index the nets so that each net has a list of cell ports it connects to.
	
	Has to be done as a second pass because there may be cycles in the netlist preventing us from resolving names
	as we parse the JSON
 */
void Greenpak4Netlist::IndexNets()
{
	printf("Indexing...\n");
	
	//Loop over all of our ports and add them to the associated nets
	for(auto it = m_topModule->port_begin(); it != m_topModule->port_end(); it ++)
	{
		Greenpak4NetlistPort* port = it->second;
		//printf("    Port %s connects to:\n", it->first.c_str());
		
		for(size_t i=0; i<port->m_nodes.size(); i++)
		{
			Greenpak4NetlistNode* node = port->m_nodes[i];
			//printf("        node %s\n", node->m_name.c_str());
			node->m_ports.push_back(port);
		}
	}
	
	//Loop over all of our cells and add their connections
	for(auto it = m_topModule->cell_begin(); it != m_topModule->cell_end(); it ++)
	{
		Greenpak4NetlistCell* cell = it->second;
		//printf("    Cell %s connects to:\n", it->first.c_str());
		for(auto jt : cell->m_connections)
		{
			Greenpak4NetlistNet* net = jt.second;
			//TODO: support vectors
			Greenpak4NetlistNode* node = net->m_nodes[0];
			//printf("        %s: net %s\n", jt.first.c_str(), node->m_name.c_str());
			node->m_nodeports.push_back(Greenpak4NetlistNodePoint(cell, jt.first));
		}
	}
	
	//Make a set of the nodes to avoid duplication
	for(auto it = m_topModule->net_begin(); it != m_topModule->net_end(); it ++)
	{
		Greenpak4NetlistNet* net = it->second;
		for(auto node : net->m_nodes)
			m_nodes.insert(node);
	}
	
	//Print them out
	/*
	for(auto node : m_nodes)
	{
		printf("    Node %s connects to:\n", node->m_name.c_str());
		for(auto p : node->m_ports)
		{
			Greenpak4NetlistNet* net = m_topModule->GetNet(p->m_name);
			printf("        port %s (loc %s)\n", p->m_name.c_str(), net->m_attributes["LOC"].c_str());
		}
		for(auto c : node->m_nodeports)
			printf("        cell %s port %s\n", c.m_cell->m_name.c_str(), c.m_portname.c_str());
	}
	*/
}

/**
	@brief Module parsing routine
	
	Loads all of the modules in the netlist
 */
void Greenpak4Netlist::LoadModules(json_object* object)
{
	printf("Loading modules...\n");
	
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
			fprintf(stderr, "ERROR: netlist module entry should be of type object but isn't\n");
			exit(-1);
		}
		
		//TODO: If the child object is a standard library cell, don't bother parsing it?
		
		//Load it
		m_modules[name] = new Greenpak4NetlistModule(this, name, child);
	}
	
	//Verify we got the top-level module we expected
	if(m_modules.find(m_topModuleName) != m_modules.end())
		m_topModule = m_modules[m_topModuleName];
	else
	{
		fprintf(stderr, "ERROR: Unable to find top-level module \"%s\" in netlist\n", m_topModuleName.c_str());
		exit(-1);
	}
}
