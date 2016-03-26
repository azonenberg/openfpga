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

class Greenpak4NetlistNodePoint
{
public:

	Greenpak4NetlistNodePoint(Greenpak4NetlistCell* cell, std::string port)
		: m_cell(cell)
		, m_portname(port)
	{}

	Greenpak4NetlistCell* m_cell;
	std::string m_portname;
};

//A single node in the netlist (may be a wire or part of a bus)
class Greenpak4NetlistNode
{
public:

	Greenpak4NetlistNode()
	: m_net(NULL)
	{}

	std::string m_name;
	
	//The net we're part of
	Greenpak4NetlistNet* m_net;
	
	//List of internal points we link to (only valid after indexing)
	std::vector<Greenpak4NetlistNodePoint> m_nodeports;
	
	//List of ports we link to (only valid after indexing)
	std::vector<Greenpak4NetlistPort*> m_ports;
};

//A module port (attached to a single node)
class Greenpak4NetlistPort : public Greenpak4NetlistEntity
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
	
	Greenpak4NetlistNet* m_net;
	
	//The netlist node we're attached to
	Greenpak4NetlistNode* m_node;
	
	//The graph node for this IOB (only valid during place-and-route)
	PARGraphNode* m_parnode;
};

#endif
