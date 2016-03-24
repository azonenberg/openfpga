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
#include <math.h>

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
// Congestion metrics

uint32_t Greenpak4PAREngine::ComputeCongestionCost()
{
	uint32_t costs[2] = {0};
	
	for(uint32_t i=0; i<m_device->GetNumNodes(); i++)
	{
		//If no node in the netlist is assigned to us, nothing to do
		PARGraphNode* node = m_device->GetNodeByIndex(i);
		PARGraphNode* netnode = node->GetMate();
		if(netnode == NULL)
			continue;
			
		//Find all edges crossing the central routing matrix
		for(uint32_t i=0; i<netnode->GetEdgeCount(); i++)
		{
			auto edge = netnode->GetEdgeByIndex(i);
			auto src = static_cast<Greenpak4BitstreamEntity*>(edge->m_sourcenode->GetMate()->GetData());
			uint32_t sm = src->GetMatrix();
			uint32_t dm = static_cast<Greenpak4BitstreamEntity*>(edge->m_destnode->GetMate()->GetData())->GetMatrix();
			
			//If the source is a power rail, don't count this in the cost
			if(dynamic_cast<Greenpak4PowerRail*>(src) != NULL)
				continue;
			
			if(sm != dm)
				costs[sm] ++;
		}
	}
	
	//Squaring each half makes minimizing the larger one more important
	//vs if we just summed
	return sqrt(costs[0]*costs[0] + costs[1]*costs[1]);
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

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Optimization helpers

/**
	@brief Find all nodes that are not optimally placed.
	
	This means that the node contributes in some nonzero fashion to the overall score of the system.
	
	Nodes are in the NETLIST graph, not the DEVICE graph.
 */
void Greenpak4PAREngine::FindSubOptimalPlacements(std::vector<PARGraphNode*>& bad_nodes)
{
	std::set<PARGraphNode*> nodes;
	
	//Find all nodes that have at least one cross-spine route
	for(uint32_t i=0; i<m_device->GetNumNodes(); i++)
	{
		//If no node in the netlist is assigned to us, nothing to do
		PARGraphNode* node = m_device->GetNodeByIndex(i);
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
						
			//Cross connections
			unsigned int srcmatrix = src->GetMatrix();
			if(srcmatrix != dst->GetMatrix())
			{
				//If either node is an IOB, do NOT add to the sub-optimal list
				//because the IOB is constrained and can't move
				if(dynamic_cast<Greenpak4IOB*>(src) == NULL)
					nodes.insert(edge->m_sourcenode);
				if(dynamic_cast<Greenpak4IOB*>(dst) == NULL)
					nodes.insert(edge->m_destnode);
					
				//Power rails are always in optimal locations (because they're everywhere)
				if(dynamic_cast<Greenpak4PowerRail*>(src) == NULL)
					nodes.insert(edge->m_sourcenode);
			}
		}
	}
	
	//Push into the final output list
	for(auto x : nodes)
		bad_nodes.push_back(x);
}

/**
	@brief Find a new (hopefully more efficient) placement for a given netlist node
 */
PARGraphNode* Greenpak4PAREngine::GetNewPlacementForNode(PARGraphNode* pivot)
{
	//Find which matrix we were assigned to
	PARGraphNode* current_node = pivot->GetMate();
	auto current_site = static_cast<Greenpak4BitstreamEntity*>(current_node->GetData());
	uint32_t current_matrix = current_site->GetMatrix();
	uint32_t label = current_node->GetLabel();
	uint32_t ncandidates = m_device->GetNumNodesWithLabel(label);
	
	//Pick a random node of the same label that's in the correct half of the device
	PARGraphNode* newmate = NULL;
	Greenpak4BitstreamEntity* newsite = NULL;
	for(int i=0; i<20; i++)
	{
		newmate = m_device->GetNodeByLabelAndIndex(label, rand() % ncandidates);
		newsite = static_cast<Greenpak4BitstreamEntity*>(newmate->GetData());
		if(newsite->GetMatrix() != current_matrix)
			return newmate;
	}
		
	//Nothing found, give up
	return NULL;
}
