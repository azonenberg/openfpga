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

Greenpak4NetlistPort::Greenpak4NetlistPort(Greenpak4NetlistModule* module, std::string name, json_object* object)
	: m_direction(DIR_INPUT)
	, m_module(module)
	, m_name(name)
{	
	json_object_iterator end = json_object_iter_end(object);
	for(json_object_iterator it = json_object_iter_begin(object);
		!json_object_iter_equal(&it, &end);
		json_object_iter_next(&it))
	{
		//See what we got
		string name = json_object_iter_peek_name(&it);
		json_object* child = json_object_iter_peek_value(&it);
		
		//Direction should be a string from the enumerated list
		if(name == "direction")
		{
			if(!json_object_is_type(child, json_type_string))
			{
				fprintf(stderr, "ERROR: Port direction should be of type string but isn't\n");
				exit(-1);
			}
			
			//See what the direction is
			string str = json_object_get_string(child);
			if(str == "input")
				m_direction = Greenpak4NetlistPort::DIR_INPUT;
			else if(str == "output")
				m_direction = Greenpak4NetlistPort::DIR_OUTPUT;
			else if(str == "inout")
				m_direction = Greenpak4NetlistPort::DIR_INOUT;
			else
			{
				fprintf(stderr, "ERROR: Invalid port direction \"%s\"\n", str.c_str());
				exit(-1);
			}
		}
		
		//List of nodes in the object (should be an array)
		else if(name == "bits")
		{
			if(!json_object_is_type(child, json_type_array))
			{
				fprintf(stderr, "ERROR: Port bits should be of type array but isn't\n");
				exit(-1);
			}

			//Walk the array
			//TODO: verify bit ordering is correct (does this even matter as long as we're consistent?)
			int len = json_object_array_length(child);
			for(int i=0; i<len; i++)
			{
				json_object* jnode = json_object_array_get_idx(child, i);
				if(!json_object_is_type(jnode, json_type_int))
				{
					fprintf(stderr, "ERROR: Net number should be of type integer but isn't\n");
					exit(-1);
				}
				
				//Add the net				
				m_nodes.push_back(module->GetNode(json_object_get_int(jnode)));
			}
		}
		
		//Garbage
		else
		{
			fprintf(stderr, "ERROR: Unknown JSON blob \"%s\" under module port list\n", name.c_str());
			exit(-1);
		}
	}
}

Greenpak4NetlistPort::~Greenpak4NetlistPort()
{
	
}
