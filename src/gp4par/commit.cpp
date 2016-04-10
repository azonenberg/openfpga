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

#include "gp4par.h"

using namespace std;

/**
	@brief After a successful PAR, copy all of the data from the unplaced to placed nodes
 */
void CommitChanges(PARGraph* device, Greenpak4Device* pdev, unsigned int* num_routes_used)
{
	printf("\nBuilding final post-route netlist...\n");
	
	//Go over all of the nodes in the graph and configure the nodes themselves
	//Net routing will come later!
	for(uint32_t i=0; i<device->GetNumNodes(); i++)
	{
		//If no node in the netlist is assigned to us, leave us at default configuration
		PARGraphNode* node = device->GetNodeByIndex(i);
		PARGraphNode* mate = node->GetMate();
		if(mate == NULL)
			continue;
		
		static_cast<Greenpak4BitstreamEntity*>(node->GetData())->CommitChanges();
	}
	
	//Done configuring all of the nodes!
	//Configure routes between them
	CommitRouting(device, pdev, num_routes_used);
}

/**
	@brief Commit post-PAR results from the netlist to the routing matrix
	
	TODO: refactor a lot of this stuff to be in the BitstreamEntity derived class
 */
void CommitRouting(PARGraph* device, Greenpak4Device* pdev, unsigned int* num_routes_used)
{
	num_routes_used[0] = 0;
	num_routes_used[1] = 0;
		
	//Map of source node to cross-connection output
	std::map<Greenpak4BitstreamEntity*, Greenpak4BitstreamEntity*> nodemap;
	
	for(uint32_t i=0; i<device->GetNumNodes(); i++)
	{
		//If no node in the netlist is assigned to us, nothing to do
		PARGraphNode* node = device->GetNodeByIndex(i);
		PARGraphNode* netnode = node->GetMate();
		if(netnode == NULL)
			continue;
			
		//Commit changes to all edges.
		//Iterate over the NETLIST graph, not the DEVICE graph, but then transfer to the device graph
		for(uint32_t i=0; i<netnode->GetEdgeCount(); i++)
		{
			auto edge = netnode->GetEdgeByIndex(i);
			auto src = static_cast<Greenpak4BitstreamEntity*>(edge->m_sourcenode->GetMate()->GetData());
			auto dst = static_cast<Greenpak4BitstreamEntity*>(edge->m_destnode->GetMate()->GetData());
			
			//Try casting to every primitive type known to mankind!
			auto iob = dynamic_cast<Greenpak4IOB*>(dst);
			auto lut = dynamic_cast<Greenpak4LUT*>(dst);			
			auto ff = dynamic_cast<Greenpak4Flipflop*>(dst);
			auto pwr = dynamic_cast<Greenpak4PowerRail*>(dst);
			auto osc = dynamic_cast<Greenpak4LFOscillator*>(dst);
			auto rosc = dynamic_cast<Greenpak4RingOscillator*>(dst);
			auto rcosc = dynamic_cast<Greenpak4RCOscillator*>(dst);
			auto count = dynamic_cast<Greenpak4Counter*>(dst);
			auto rst = dynamic_cast<Greenpak4SystemReset*>(dst);
			auto inv = dynamic_cast<Greenpak4Inverter*>(dst);
				
			//If the source node has a dual, use the secondary output if needed
			//so we don't waste cross connections
			if(src->GetDual())
			{
				if(dst->GetMatrix() != src->GetMatrix())
					src = src->GetDual();
			}
			
			//Cross connections
			unsigned int srcmatrix = src->GetMatrix();
			if(srcmatrix != dst->GetMatrix())
			{
				//Reuse existing connections, if any
				if(nodemap.find(src) != nodemap.end())
					src = nodemap[src];

				//Allocate a new cross-connection
				else
				{				
					//We need to jump from one matrix to another!
					//Make sure we have a free cross-connection to use
					if(num_routes_used[srcmatrix] >= 10)
					{
						printf(
							"ERROR: More than 100%% of device resources are used "
							"(cross connections from matrix %d to %d)\n",
								src->GetMatrix(),
								dst->GetMatrix());
					}
					
					//Save our cross-connection and mark it as used
					auto xconn = pdev->GetCrossConnection(srcmatrix, num_routes_used[srcmatrix]);
					num_routes_used[srcmatrix] ++;
					
					//Insert the cross-connection into the path
					xconn->SetInput(src);
					nodemap[src] = xconn;
					src = xconn;
				}
			}
			
			//Destination is an IOB - configure the signal (TODO: output enable for tristates)
			if(iob)
				iob->SetOutputSignal(src);
				
			//Destination is a LUT - multiple ports, figure out which one
			else if(lut)
			{
				unsigned int nport;
				if(1 != sscanf(edge->m_destport.c_str(), "IN%u", &nport))
				{
					printf("WARNING: Ignoring connection to unknown LUT input %s\n", edge->m_destport.c_str());
					continue;
				}
				
				lut->SetInputSignal(nport, src);
			}
			
			//Destination is a flipflop - multiple ports, figure out which one
			else if(ff)
			{
				if(edge->m_destport == "CLK")
					ff->SetClockSignal(src);
				else if(edge->m_destport == "D")
					ff->SetInputSignal(src);
				
				//multiple set/reset modes possible
				else if(edge->m_destport == "nSR")
					ff->SetNSRSignal(src);
				else if(edge->m_destport == "nSET")
				{
					ff->SetSRMode(true);
					ff->SetNSRSignal(src);
				}
				else if(edge->m_destport == "nRST")
				{
					ff->SetSRMode(false);
					ff->SetNSRSignal(src);
				}
				
				else
				{
					printf("WARNING: Ignoring connection to unknown FF input %s\n", edge->m_destport.c_str());
					continue;
				}
			}
			
			//Destination is the low-frequency oscillator
			else if(osc)
			{
				if(edge->m_destport == "PWRDN")
					osc->SetPowerDown(src);
				
				else
				{
					printf("WARNING: Ignoring connection to unknown oscillator input %s\n", edge->m_destport.c_str());
					continue;
				}
			}
			
			//Destination is the ring oscillator
			else if(rosc)
			{
				if(edge->m_destport == "PWRDN")
					rosc->SetPowerDown(src);
				
				else
				{
					printf("WARNING: Ignoring connection to unknown ring oscillator input %s\n",
						edge->m_destport.c_str());
					continue;
				}
			}
			
			//Destination is the RC oscillator
			else if(rcosc)
			{
				if(edge->m_destport == "PWRDN")
					rcosc->SetPowerDown(src);
				
				else
				{
					printf("WARNING: Ignoring connection to unknown RC oscillator input %s\n",
						edge->m_destport.c_str());
					continue;
				}
			}
			
			//Destination is a counter
			else if(count)
			{
				if(edge->m_destport == "CLK")
					count->SetClock(src);
				else if(edge->m_destport == "RST")
					count->SetReset(src);
				
				else
				{
					printf("WARNING: Ignoring connection to unknown counter input %s\n", edge->m_destport.c_str());
					continue;
				}
			}
			
			//Destination is the system reset
			else if(rst)
			{
				if(edge->m_destport == "RST")
					rst->SetReset(src);
				
				else
				{
					printf("WARNING: Ignoring connection to unknown reset input %s\n", edge->m_destport.c_str());
					continue;
				}
			}
			
			//Destination is an inverter
			else if(inv)
			{
				if(edge->m_destport == "IN")
					inv->SetInput(src);
				
				else
				{
					printf("WARNING: Ignoring connection to unknown inverter input %s\n", edge->m_destport.c_str());
					continue;
				}
			}
			
			else if(pwr)
			{
				printf("WARNING: Power rail should not be driven\n");
			}

			//Don't know what to do
			else
			{
				printf("WARNING: Node %s at config base %d has unrecognized entity type\n",
					dst->GetDescription().c_str(), dst->GetConfigBase());
			}
		}
	}
}
