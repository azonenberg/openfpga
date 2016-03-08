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

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// P&R logic

void Greenpak4Netlist::PlaceAndRoute(Greenpak4Device* device)
{
	printf("\nPlace-and-route engine initializing...\n");
	
	//Verify we have a top-level module, fail if none found
	if(m_topModule == NULL)
	{
		fprintf(stderr, "INTERNAL ERROR: Cannot place a netlist without a top-level module\n");
		exit(-1);
	}
	
	//Do the IO pins first
	ParIOBs(device);	
}

/**
	@brief Place all of the IO pins at the assigned locations
 */
void Greenpak4Netlist::ParIOBs(Greenpak4Device* device)
{
	printf("I/O pin constrained placement\n");
	for(auto it = m_topModule->port_begin(); it != m_topModule->port_end(); it ++)
	{
		Greenpak4NetlistPort* port = it->second;
		
		//Sanity check
		if(!m_topModule->HasNet(it->first))
		{
			fprintf(stderr, "INTERNAL ERROR: Netlist has a port named \"%s\" but no corresponding net\n",
				it->first.c_str());
			exit(-1);
		}
		
		//Look up the net and make sure there's a LOC
		Greenpak4NetlistNet* net = m_topModule->GetNet(it->first);
		if(!net->HasAttribute("LOC"))
		{
			fprintf(
				stderr,
				"ERROR: Top-level port \"%s\" does not have a constrained location (LOC attribute).\n"
				"       In order to ensure proper device functionality all IO pins must be constrained.\n",
				it->first.c_str());
			exit(-1);
		}
		
		//Look up the matching IOB
		int pin_num;
		string sloc = net->GetAttribute("LOC");
		if(1 != sscanf(sloc.c_str(), "P%d", &pin_num))
		{
			fprintf(
				stderr,
				"ERROR: Top-level port \"%s\" has an invalid LOC constraint \"%s\" (expected P3, P5, etc)\n",
				it->first.c_str(),
				sloc.c_str());
			exit(-1);
		}
		Greenpak4IOB* iob = device->GetIOB(pin_num);
		if(iob == NULL)
		{
			fprintf(
				stderr,
				"ERROR: Top-level port \"%s\" has an invalid LOC constraint \"%s\" (no such pin, or not a GPIO)\n",
				it->first.c_str(),
				sloc.c_str());
			exit(-1);
		}
		
		//Type B IOBs cannot be used for inout
		if( (port->m_direction == Greenpak4NetlistPort::DIR_INOUT) &&
			(dynamic_cast<Greenpak4IOBTypeB*>(iob) != NULL) )
		{
			fprintf(
				stderr,
				"ERROR: Top-level inout port \"%s\" is constrained to a pin \"%s\" which does "
					"not support bidirectional IO\n",
				it->first.c_str(),
				sloc.c_str());
			exit(-1);
		}
		
		//Input-only pins cannot be used for IO or output
		if( (port->m_direction != Greenpak4NetlistPort::DIR_INPUT) &&
			iob->IsInputOnly() )
		{
			fprintf(
				stderr,
				"ERROR: Top-level port \"%s\" is constrained to an input-only pin \"%s\" but is not "
					"declared as an input\n",
				it->first.c_str(),
				sloc.c_str());
			exit(-1);
		}
		
		//If we get here, everything looks good! Pair the port wth the IOB
		//TODO: support vectors
		port->m_iob = iob;
		
		//Iterate over attributes and figure out what to do
		for(auto jt : net->m_attributes)
		{
			//do nothing, only for debugging
			if(jt.first == "src")
			{}
			
			//already handled elsewhere
			else if(jt.first == "LOC")
			{}
			
			//IO schmitt trigger
			else if(jt.first == "SCHMITT_TRIGGER")
			{
				if(jt.second == "0")
					iob->SetSchmittTrigger(false);
				else
					iob->SetSchmittTrigger(true);
			}
			
			else if(jt.first == "PULLUP")
			{
				iob->SetPullDirection(Greenpak4IOB::PULL_UP);
				if(jt.second == "10k")
					iob->SetPullStrength(Greenpak4IOB::PULL_10K);
				else if(jt.second == "100k")
					iob->SetPullStrength(Greenpak4IOB::PULL_100K);
				else if(jt.second == "1M")
					iob->SetPullStrength(Greenpak4IOB::PULL_1M);
			}
			
			else if(jt.first == "PULLDOWN")
			{
				iob->SetPullDirection(Greenpak4IOB::PULL_DOWN);
				if(jt.second == "10k")
					iob->SetPullStrength(Greenpak4IOB::PULL_10K);
				else if(jt.second == "100k")
					iob->SetPullStrength(Greenpak4IOB::PULL_100K);
				else if(jt.second == "1M")
					iob->SetPullStrength(Greenpak4IOB::PULL_1M);
			}

			else
			{
				printf("WARNING: Top-level port \"%s\" has unrecognized attribute %s, ignoring\n",
					it->first.c_str(), jt.first.c_str());
			}
		}
	}
}
