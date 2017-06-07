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

#ifndef Greenpak4NetlistCell_h
#define Greenpak4NetlistCell_h

class Greenpak4NetlistModule;

//only for RTTI support and naming
class Greenpak4NetlistEntity
{
public:
	Greenpak4NetlistEntity(std::string name = "")
	: m_name(name)
	{}

	virtual ~Greenpak4NetlistEntity();

	std::string m_name;
};

//A single primitive cell in the netlist
//TODO: move to separate header
class Greenpak4NetlistCell : public Greenpak4NetlistEntity
{
public:
	Greenpak4NetlistCell(Greenpak4NetlistModule* module)
	: m_parent(module)
	{ m_parnode = NULL; }
	virtual ~Greenpak4NetlistCell();

	bool HasParameter(std::string att)
	{ return m_parameters.find(att) != m_parameters.end(); }

	bool HasAttribute(std::string att)
	{ return m_attributes.find(att) != m_attributes.end(); }

	//Indicates whether the cell is an I/O buffer
	bool IsIOB()
	{ return (m_type == "GP_IBUF") || (m_type == "GP_IOBUF") || (m_type == "GP_OBUF") || (m_type == "GP_OBUFT"); }

	//Indicates whether the cell is an input buffer
	bool IsIbuf()
	{ return (m_type == "GP_IBUF") || (m_type == "GP_IOBUF"); }

	//Indicates whether the cell is an output buffer
	bool IsObuf()
	{ return (m_type == "GP_OBUF") || (m_type == "GP_IOBUF"); }

	//Indicates whether the cell is a power rail
	bool IsPowerRail()
	{ return (m_type == "GP_VDD") || (m_type == "GP_VSS"); }

	std::string GetLOC();

	bool HasLOC()
	{ return (m_attributes.find("LOC") != m_attributes.end()); }

	///Module name
	std::string m_type;

	std::map<std::string, std::string> m_parameters;
	std::map<std::string, std::string> m_attributes;

	typedef std::vector<Greenpak4NetlistNode*> cellnet;

	/**
		@brief Map of connections to the cell

		connections[portname] = {bit2, bit1, bit0}
	 */
	std::map<std::string, cellnet > m_connections;

	PARGraphNode* m_parnode;

	//Parent module of the cell, not the module we're an instance of
	Greenpak4NetlistModule* m_parent;
};

#endif
