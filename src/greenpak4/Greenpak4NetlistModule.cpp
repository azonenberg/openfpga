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

#define VDD_NETNUM 1
#define VSS_NETNUM 0

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Construction / destruction

Greenpak4NetlistModule::Greenpak4NetlistModule(Greenpak4Netlist* parent, std::string name, json_object* object)
	: m_parent(parent)
	, m_name(name)
{
	CreatePowerNets();
	
	printf("    %s\n", name.c_str());
	
	json_object_iterator end = json_object_iter_end(object);
	for(json_object_iterator it = json_object_iter_begin(object);
		!json_object_iter_equal(&it, &end);
		json_object_iter_next(&it))
	{
		//See what we got
		string name = json_object_iter_peek_name(&it);
		json_object* child = json_object_iter_peek_value(&it);
		
		//Whatever it is, it should be an object
		if(!json_object_is_type(child, json_type_object))
		{
			fprintf(stderr, "ERROR: module child should be of type object but isn't\n");
			exit(-1);
		}
		
		//Go over the children's children and process it
		json_object_iterator end = json_object_iter_end(child);
		for(json_object_iterator it2 = json_object_iter_begin(child);
			!json_object_iter_equal(&it2, &end);
			json_object_iter_next(&it2))
		{
			//See what we got
			string cname = json_object_iter_peek_name(&it2);
			json_object* cobject = json_object_iter_peek_value(&it2);
			
			//Whatever it is, it should be an object
			if(!json_object_is_type(cobject, json_type_object))
			{
				fprintf(stderr, "ERROR: module child should be of type object but isn't\n");
				exit(-1);
			}
			
			//Load ports
			if(name == "ports")
			{
				//Make sure it doesn't exist
				if(m_ports.find(cname) != m_ports.end())
				{
					fprintf(stderr, "ERROR: Attempted redeclaration of module port \"%s\"\n", name.c_str());
					exit(-1);
				}
				
				//Create the port
				Greenpak4NetlistPort* port = new Greenpak4NetlistPort(this, cname, cobject);
				m_ports[cname] = port;
			}
			
			//Load cells
			else if(name == "cells")
				LoadCell(cname, cobject);
			
			//Load net names
			else if(name == "netnames")
				LoadNetName(cname, cobject);
			
			//Whatever it is, we don't want it
			else
			{
				fprintf(stderr, "ERROR: Unknown top-level JSON object \"%s\"\n", name.c_str());
				exit(-1);
			}
		}
	}
	
	//Assign port nets
	for(auto it : m_ports)
		it.second->m_net = m_nets[it.first];
}

Greenpak4NetlistModule::~Greenpak4NetlistModule()
{
	//Clean up in reverse order
	
	//cells depend on everything
	for(auto x : m_cells)
		delete x.second;
	m_cells.clear();
	
	//nets depends on nodes but nothing depends on them after loading
	for(auto x : m_nets)
		delete x.second;
	m_nets.clear();
	
	//ports don't depend on anything but nodes
	for(auto x : m_ports)
		delete x.second;
	m_ports.clear();
	
	//then nodes at end
	for(auto x : m_nodes)
		delete x.second;
	m_nodes.clear();
}

void Greenpak4NetlistModule::CreatePowerNets()
{
	string vdd = "GP_VDD";
	string vss = "GP_VSS";
	
	//Create power/ground nets
	m_vdd = new Greenpak4NetlistNode;
	m_vdd->m_name = vdd;
	
	m_vss = new Greenpak4NetlistNode;
	m_vss->m_name = vss;
	
	m_nodes[VDD_NETNUM] = m_vdd;
	m_nodes[VSS_NETNUM] = m_vss;
	
	m_nets[vdd] = m_vdd;
	m_nets[vss] = m_vss;
	
	//Create driver cells for them
	Greenpak4NetlistCell* vcell = new Greenpak4NetlistCell(this);
	vcell->m_name = vdd;
	vcell->m_type = vdd;
	vcell->m_connections["OUT"] = m_vdd;
	m_cells[vdd] = vcell;
	
	Greenpak4NetlistCell* gcell = new Greenpak4NetlistCell(this);
	gcell->m_name = vss;
	gcell->m_type = vss;
	gcell->m_connections["OUT"] = m_vss;
	m_cells[vss] = gcell;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Loading

Greenpak4NetlistNode* Greenpak4NetlistModule::GetNode(int32_t netnum)
{
	//See if we already have a node with this number.
	//If not, create it
	if(m_nodes.find(netnum) == m_nodes.end())
		m_nodes[netnum] = new Greenpak4NetlistNode;
		
	return m_nodes[netnum];
}

void Greenpak4NetlistModule::LoadCell(std::string name, json_object* object)
{
	Greenpak4NetlistCell* cell = new Greenpak4NetlistCell(this);
	cell->m_name = name;
	m_cells[name] = cell;
	
	json_object_iterator end = json_object_iter_end(object);
	for(json_object_iterator it = json_object_iter_begin(object);
		!json_object_iter_equal(&it, &end);
		json_object_iter_next(&it))
	{
		//See what we got
		string cname = json_object_iter_peek_name(&it);
		json_object* child = json_object_iter_peek_value(&it);
		
		//Ignore hide_name request for now
		if(cname == "hide_name")
		{
			
		}
		
		//Type of cell
		else if(cname == "type")
		{
			if(!json_object_is_type(child, json_type_string))
			{
				fprintf(stderr, "ERROR: Cell type should be of type string but isn't\n");
				exit(-1);
			}
			
			cell->m_type = json_object_get_string(child);
		}
		
		else if(cname == "attributes")
			LoadCellAttributes(cell, child);
		
		else if(cname == "parameters")
			LoadCellParameters(cell, child);
		
		else if(cname == "connections")
			LoadCellConnections(cell, child);
		
		//redundant, we can look this up from the module
		else if(cname == "port_directions")
		{
		}
		
		//Unsupported
		else
		{
			fprintf(stderr, "ERROR: Unknown cell child object \"%s\"\n", cname.c_str());
			exit(-1);
		}
	}
}

void Greenpak4NetlistModule::LoadNetName(std::string name, json_object* object)
{	
	//Create the named net
	if(m_nets.find(name) != m_nets.end())
	{
		fprintf(stderr, "ERROR: Attempted redeclaration of net \"%s\" \n", name.c_str());
		exit(-1);
	}
	
	Greenpak4NetlistNode* node = NULL;
	
	json_object_iterator end = json_object_iter_end(object);
	for(json_object_iterator it = json_object_iter_begin(object);
		!json_object_iter_equal(&it, &end);
		json_object_iter_next(&it))
	{
		string cname = json_object_iter_peek_name(&it);
		json_object* child = json_object_iter_peek_value(&it);
		
		//Ignore hide_name request for now
		if(cname == "hide_name")
		{
		}
		
		//Bits - list of nets this name is assigned to
		else if(cname == "bits")
		{
			if(!json_object_is_type(child, json_type_array))
			{
				fprintf(stderr, "ERROR: Net name bits should be of type array but isn't\n");
				exit(-1);
			}

			//Walk the array
			int len = json_object_array_length(child);
			if(len != 1)
			{
				fprintf(stderr, "ERROR: Vectors must be split during synthesis; PAR cannot handle vector nets\n");
				exit(-1);
			}
			
			json_object* jnode = json_object_array_get_idx(child, 0);
			if(!json_object_is_type(jnode, json_type_int))
			{
				fprintf(stderr, "ERROR: Net number in module should be of type integer but isn't\n");
				exit(-1);
			}

			//How to handle multiple names for the same net??
			node = GetNode(json_object_get_int(jnode));
		
			//Set up name etc
			node->m_name = name;
			m_nets[name] = node;
		}
		
		//Attributes - array of name-value pairs
		else if(cname == "attributes")
		{
			if(!json_object_is_type(child, json_type_object))
			{
				fprintf(stderr, "ERROR: Net attributes should be of type object but isn't\n");
				exit(-1);
			}
			
			LoadNetAttributes(node, child);
		}
		
		//Unsupported
		else
		{
			fprintf(stderr, "ERROR: Unknown netname child object \"%s\"\n", cname.c_str());
			exit(-1);
		}
	}
}

void Greenpak4NetlistModule::LoadNetAttributes(Greenpak4NetlistNode* net, json_object* object)
{
	json_object_iterator end = json_object_iter_end(object);
	for(json_object_iterator it = json_object_iter_begin(object);
		!json_object_iter_equal(&it, &end);
		json_object_iter_next(&it))
	{
		string cname = json_object_iter_peek_name(&it);
		json_object* child = json_object_iter_peek_value(&it);
				
		//no type check, convert whatever it is to a string
				
		//Make sure we don't have it already
		if(net->m_attributes.find(cname) != net->m_attributes.end())
		{
			fprintf(stderr, "ERROR: Attempted redeclaration of net attribute \"%s\"\n", cname.c_str());
			exit(-1);
		}
		
		//printf("    net %s attribute %s = %s\n", net->m_name.c_str(), cname.c_str(), json_object_get_string(child));
		
		//Save the attribute
		net->m_attributes[cname] = json_object_get_string(child);
	}
}

void Greenpak4NetlistModule::LoadCellAttributes(Greenpak4NetlistCell* cell, json_object* object)
{
	json_object_iterator end = json_object_iter_end(object);
	for(json_object_iterator it = json_object_iter_begin(object);
		!json_object_iter_equal(&it, &end);
		json_object_iter_next(&it))
	{
		string cname = json_object_iter_peek_name(&it);
		json_object* child = json_object_iter_peek_value(&it);
				
		//Make sure we don't have it already
		if(cell->m_attributes.find(cname) != cell->m_attributes.end())
		{
			fprintf(stderr, "ERROR: Attempted redeclaration of cell attribute \"%s\"\n", cname.c_str());
			exit(-1);
		}
		
		//Save the attribute
		cell->m_attributes[cname] = json_object_get_string(child);
	}
}

void Greenpak4NetlistModule::LoadCellParameters(Greenpak4NetlistCell* cell, json_object* object)
{
	json_object_iterator end = json_object_iter_end(object);
	for(json_object_iterator it = json_object_iter_begin(object);
		!json_object_iter_equal(&it, &end);
		json_object_iter_next(&it))
	{
		string cname = json_object_iter_peek_name(&it);
		json_object* child = json_object_iter_peek_value(&it);
		
		//No type check, just convert back to string
		
		//Make sure we don't have it already
		if(cell->m_parameters.find(cname) != cell->m_parameters.end())
		{
			fprintf(stderr, "ERROR: Attempted redeclaration of cell parameter \"%s\"\n", cname.c_str());
			exit(-1);
		}
		
		//Save the attribute
		cell->m_parameters[cname] = json_object_get_string(child);
	}
}

void Greenpak4NetlistModule::LoadCellConnections(Greenpak4NetlistCell* cell, json_object* object)
{
	json_object_iterator end = json_object_iter_end(object);
	for(json_object_iterator it = json_object_iter_begin(object);
		!json_object_iter_equal(&it, &end);
		json_object_iter_next(&it))
	{
		string cname = json_object_iter_peek_name(&it);
		json_object* child = json_object_iter_peek_value(&it);
		
		if(!json_object_is_type(child, json_type_array))
		{
			fprintf(stderr, "ERROR: Cell connection value should be of type array but isn't\n");
			exit(-1);
		}

		//Sanity check the length. If empty, bail without creating a floating net.
		//Otherwise should be 1 bit
		int len = json_object_array_length(child);
		if(len == 0)
			continue;		
		if(len != 1)
		{
			fprintf(stderr, "ERROR: Arrays not implemented in cell connections\n");
			exit(-1);
		}
		
		Greenpak4NetlistNode* node = NULL;
		
		json_object* jnode = json_object_array_get_idx(child, 0);
		
		//If it's a string, it's a constant one or zero
		if(json_object_is_type(jnode, json_type_string))
		{
			string s = json_object_get_string(jnode);
			if(s == "1")
				node = m_vdd;
			else
				node = m_vss;
		}
		
		//Otherwise it has to be an integer
		else if(!json_object_is_type(jnode, json_type_int))
		{
			fprintf(stderr, "ERROR: Net number for cell should be of type integer but isn't\n");
			exit(-1);
		}
		
		else
			node = GetNode(json_object_get_int(jnode));
			
		//Name the net
		cell->m_connections[cname] = node;
	}
}
