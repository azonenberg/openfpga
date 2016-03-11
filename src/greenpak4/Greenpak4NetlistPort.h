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

#ifndef Greenpak4NetlistPort_h
#define Greenpak4NetlistPort_h

#include <string>
#include <vector>
#include <json-c/json.h>

//A single node in the netlist (may be a wire or part of a bus)
class Greenpak4NetlistNode
{
public:

	Greenpak4NetlistNode()
	{ m_parnode = NULL; }

	//Link to the graph node for place-and-route
	PARGraphNode* m_parnode;

	std::string m_name;
};

//A module port (may contain one or more nodes)
class Greenpak4NetlistPort
{
public:
	Greenpak4NetlistPort(Greenpak4NetlistModule* module, std::string name, json_object* object);
	virtual ~Greenpak4NetlistPort();

	enum Direction
	{
		DIR_INPUT,
		DIR_OUTPUT,
		DIR_INOUT
	};
	Direction m_direction;

	Greenpak4NetlistModule* m_module;
	
	std::string m_name;
	
	std::vector<Greenpak4NetlistNode*> m_nodes;
	
	//Assigned IOB (only valid after ParIOBs())
	//TODO: Support vectors here
	Greenpak4IOB* m_iob;
};

#endif