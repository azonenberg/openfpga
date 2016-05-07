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

Greenpak4NetlistCell::~Greenpak4NetlistCell()
{
	//do not delete wires, module dtor handles that
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Constraint helpers

void Greenpak4NetlistCell::FindLOC()
{
	//Look up our module
	auto module = m_parent->GetNetlist()->GetModule(m_type);

	//Look at our attributes
	if(m_attributes.find("LOC") != m_attributes.end())
		m_loc = m_attributes["LOC"];

	//Constraints go on the cell's output port(s)
	//so look at all outbound edges
	string loc = "";
	for(auto it : m_connections)
	{
		//If not an output, ignore it
		//EXCEPTION: GP_IBUF is constrained on the INPUT instead!
		auto port = module->GetPort(it.first);
		if(m_type == "GP_IBUF")
		{
			if(port->m_direction != Greenpak4NetlistPort::DIR_INPUT)
				continue;
		}
		else if(port->m_direction == Greenpak4NetlistPort::DIR_INPUT)
			continue;
			
		//See if this net has a LOC constraint
		auto net = it.second;
		if(!net->HasAttribute("LOC"))
			continue;

		string newloc = net->GetAttribute("LOC");

		//Multiple constraints are legal iff they have the same value
		if( (m_loc != "") && (m_loc != newloc) )
		{
			fprintf(
				stderr,
				"ERROR: Multiple conflicting LOC constraints (%s, %s) are attached to cell \"%s\".\n"
				"       Please remove one or more of the constraints to allow the design to be placed.\n",
				m_loc.c_str(),
				newloc.c_str(),
				m_name.c_str());
			exit(-1);
		}
		
		m_loc = newloc;
	}
}
