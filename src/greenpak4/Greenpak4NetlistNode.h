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

#ifndef Greenpak4NetlistNode_h
#define Greenpak4NetlistNode_h

class Greenpak4NetlistCell;
class Greenpak4NetlistPort;

///A single scalar wire in the netlist
class Greenpak4NetlistNodePoint
{
public:

	Greenpak4NetlistNodePoint(Greenpak4NetlistCell* cell, std::string port, unsigned int nbit, bool vector)
		: m_cell(cell)
		, m_portname(port)
		, m_nbit(nbit)
		, m_vector(vector)
	{}

	bool IsNull()
	{ return (m_cell == NULL); }

	Greenpak4NetlistCell* m_cell;
	std::string m_portname;
	unsigned int m_nbit;
	bool m_vector;
};

//A single named node in the netlist (may be a wire or part of a bus)
class Greenpak4NetlistNode
{
public:

	Greenpak4NetlistNode();

	std::string m_name;

	//Attributes
	std::map<std::string, std::string> m_attributes;

	bool HasAttribute(std::string name)
	{ return (m_attributes.find(name) != m_attributes.end() ); }

	std::string GetAttribute(std::string name)
	{ return m_attributes[name]; }

	//Source file locations
	std::vector<std::string> m_src_locations;

	//Net source (only valid after indexing)
	Greenpak4NetlistNodePoint m_driver;

	//List of internal points we link to (only valid after indexing)
	std::vector<Greenpak4NetlistNodePoint> m_nodeports;

	//List of ports we link to (only valid after indexing)
	std::vector<Greenpak4NetlistPort*> m_ports;
};

#endif
