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
			
			//If the source has a dual, don't count this in the cost since it can route anywhere
			if(src->GetDual() != NULL)
				continue;
			
			//If matrices don't match, bump cost
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
		{
			auto entity = static_cast<Greenpak4BitstreamEntity*>(edge->m_sourcenode->GetMate()->GetData());
			printf("    from cell %s (mapped to %s) ", scell->m_name.c_str(), entity->GetDescription().c_str());
		}
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
			auto entity = static_cast<Greenpak4BitstreamEntity*>(edge->m_destnode->GetMate()->GetData());
			printf(
				"cell %s (mapped to %s) pin %s\n",
				dcell->m_name.c_str(),
				entity->GetDescription().c_str(),
				edge->m_destport.c_str()
				);
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
		
		for(uint32_t i=0; i<netnode->GetEdgeCount(); i++)
		{
			auto edge = netnode->GetEdgeByIndex(i);
			auto src = static_cast<Greenpak4BitstreamEntity*>(edge->m_sourcenode->GetMate()->GetData());
			auto dst = static_cast<Greenpak4BitstreamEntity*>(edge->m_destnode->GetMate()->GetData());
						
			//Cross connections
			unsigned int srcmatrix = src->GetMatrix();
			if(srcmatrix != dst->GetMatrix())
			{			
				//If there's nothing we can do about it, skip
				if(CantMoveSrc(src))
					continue;
				if(CantMoveDst(dst))
					continue;
					
				//Add the node
				nodes.insert(edge->m_sourcenode);
			}
		}
	}
	
	//Find all nodes that are on one end of an unroutable edge
	std::vector<PARGraphEdge*> unroutes;
	for(auto edge : unroutes)
	{
		if(!CantMoveSrc(static_cast<Greenpak4BitstreamEntity*>(edge->m_sourcenode->GetData())))
			nodes.insert(edge->m_sourcenode);
		if(!CantMoveDst(static_cast<Greenpak4BitstreamEntity*>(edge->m_sourcenode->GetData())))
			nodes.insert(edge->m_destnode);
	}	
	
	//Push into the final output list
	for(auto x : nodes)
		bad_nodes.push_back(x);
}

/**
	@brief Returns true if the given source node cannot be moved
 */
bool Greenpak4PAREngine::CantMoveSrc(Greenpak4BitstreamEntity* src)
{
	//If source node is an IOB, do NOT add it to the sub-optimal list
	//because the IOB is constrained and can't move.
	//It's OK if the destination node is an IOB, moving its source is OK
	if(dynamic_cast<Greenpak4IOB*>(src) != NULL)
		return true;

	//TODO: if it has a LOC constraint, don't add it
		
	//Anything with a dual is always in optimal locations (because they're everywhere)
	if(src->GetDual() != NULL)
		return true;
		
	//nope, it's movable
	return false;
}

/**
	@brief Returns true if the given destination node cannot be moved
 */
bool Greenpak4PAREngine::CantMoveDst(Greenpak4BitstreamEntity* /*dst*/)
{
	//Oscillator as destination?
	//if(dynamic_cast<Greenpak4LFOscillator*>(dst) != NULL)
	//	return true;
	
	//nope, it's movable	
	return false;
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
	
	//TODO: Decide value for this based on whether routing pressure was the reason for the move
	uint32_t target_matrix = 1 - current_matrix;
	
	//Make the list of candidate placements
	std::set<PARGraphNode*> temp_candidates;
	for(uint32_t i=0; i<m_device->GetNumNodesWithLabel(label); i++)
	{
		PARGraphNode* node = m_device->GetNodeByLabelAndIndex(label, i);
		Greenpak4BitstreamEntity* entity = static_cast<Greenpak4BitstreamEntity*>(node->GetData());
		
		//TODO: Check if candidate placement is routable
		
		if(entity->GetMatrix() == target_matrix)
			temp_candidates.insert(node);
	}
		
	//If no routable candidates found in the optimal matrix, check the other matrix too
	if(temp_candidates.empty())
	{
		for(uint32_t i=0; i<m_device->GetNumNodesWithLabel(label); i++)
		{
			PARGraphNode* node = m_device->GetNodeByLabelAndIndex(label, i);
			//TODO: check if candidate placement is routable
			temp_candidates.insert(node);
		}
	}
	
	//Move to a vector for random access
	std::vector<PARGraphNode*> candidates;
	for(auto x : temp_candidates)
		candidates.push_back(x);
	uint32_t ncandidates = candidates.size();
	if(ncandidates == 0)
		return NULL;
	
	//Search for an unused node, return the first one we find (since they're indistinguishable, right?)
	//TODO: how do we handle non-indistinguishable nodes (hard IP with dedicated connections)
	for(auto x : candidates)
	{
		if(x->GetMate() == NULL)
			return x;
	}
	
	//All nodes are used, we have to swap anyway. Pick one at random
	return candidates[rand() % ncandidates];
}
