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
 
#include "../xbpar/xbpar.h"
#include "../greenpak4/Greenpak4.h"
#include "Greenpak4PAREngine.h"

using namespace std;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Construction / destruction

Greenpak4PAREngine::Greenpak4PAREngine(PARGraph* netlist, PARGraph* device)
	: PAREngine(netlist, device)
{
	
}

Greenpak4PAREngine::~Greenpak4PAREngine()
{
	
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Print logic

void Greenpak4PAREngine::PrintUnroutes(vector<PARGraphEdge*>& unroutes)
{
	printf("\nUnroutable nets (%zu):\n", unroutes.size());
	for(auto edge : unroutes)
	{
		auto source = static_cast<Greenpak4NetlistEntity*>(edge->m_sourcenode->GetData());
		auto sport = dynamic_cast<Greenpak4NetlistPort*>(source);
		auto scell = dynamic_cast<Greenpak4NetlistCell*>(source);
		auto dest = static_cast<Greenpak4NetlistEntity*>(edge->m_destnode->GetData());
		auto dport = dynamic_cast<Greenpak4NetlistPort*>(dest);
		auto dcell = dynamic_cast<Greenpak4NetlistCell*>(dest);
		
		if(scell != NULL)
			printf("    from cell %s ", scell->m_name.c_str());
		else if(sport != NULL)
		{		
			auto iob = static_cast<Greenpak4IOB*>(edge->m_sourcenode->GetMate()->GetData());
			printf("    from port %s (mapped to IOB_%d)", sport->m_name.c_str(), iob->GetPinNumber());
		}
		else
			printf("    from [invalid] ");
		
		printf(" to ");
		if(dcell != NULL)
		{
			printf("cell %s (mapped to ", dcell->m_name.c_str());
			
			//Figure out what we got mapped to
			auto entity = static_cast<Greenpak4BitstreamEntity*>(edge->m_destnode->GetMate()->GetData());
			auto lut = dynamic_cast<Greenpak4LUT*>(entity);
			if(lut != NULL)
				printf("LUT%u_%u", lut->GetOrder(), lut->GetLutIndex());
			else
				printf("unknown site");
			
			printf(") pin %s\n", edge->m_destport.c_str());
		}
		else if(dport != NULL)
			printf("port %s\n", dport->m_name.c_str());
		else
			printf("[invalid]\n");
	}
	
	printf("\n");
}
