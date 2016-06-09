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
 
#include "xbpar.h"

using namespace std;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Construction / destruction

PAREngine::PAREngine(PARGraph* netlist, PARGraph* device)
	: m_netlist(netlist)
	, m_device(device)
{
	
}

PAREngine::~PAREngine()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// The core P&R logic

/**
	@brief Place-and-route implementation
	
	@return true on success, fail if design could not be routed
 */
bool PAREngine::PlaceAndRoute(map<uint32_t, string> label_names, bool verbose, uint32_t seed)
{
	if(verbose)
		printf("\nXBPAR initializing...\n");
	m_temperature = 100;
	
	//TODO: glibc rand sucks, replace with something a bit more random
	//(this may not make a difference for a device this tiny though)
	srand(seed);
		
	//Detect obviously impossible-to-route designs
	if(!SanityCheck(label_names, verbose))
		return false;
		
	//Do an initial valid, but not necessarily routable, placement
	InitialPlacement(verbose);
		
	//Converge until we get a passing placement
	printf("\nOptimizing placement...\n");
	uint32_t iteration = 0;
	vector<PARGraphEdge*> unroutes;
	uint32_t best_cost = 1000000;
	uint32_t time_since_best_cost = 0;
	bool made_change = true;
	uint32_t newcost = 0;
	while(m_temperature > 0)
	{
		//Figure out how good we are now.
		//Don't recompute the cost if we didn't accept the last iteration's changes
		if(made_change)
			newcost = ComputeAndPrintScore(unroutes, iteration);
		time_since_best_cost ++;
		iteration ++;
		
		//If cost is zero, stop now - we found a satisfactory placement!
		if(newcost == 0)
			break;
		
		//If the new placement is better than our previous record, make a note of that
		if(newcost < best_cost)
		{
			best_cost = newcost;
			time_since_best_cost = 0;
		}
		
		//If we failed to improve placement after ten iterations it's hopeless, give up
		//if(time_since_best_cost > 10)
		//	break;
		
		//Find the set of nodes in the netlist that we can optimize
		//If none were found, give up
		vector<PARGraphNode*> badnodes;
		FindSubOptimalPlacements(badnodes);
		if(badnodes.empty())
			break;
		
		//Try to optimize the placement more
		made_change = OptimizePlacement(badnodes, label_names, verbose);
			
		//Cool the system down
		//TODO: Decide on a good rate for this?
		m_temperature --;
	}
	
	//Check for any remaining unroutable nets
	unroutes.clear();
	if(0 != ComputeUnroutableCost(unroutes))
	{
		printf("ERROR: Some nets could not be completely routed!\n");
		PrintUnroutes(unroutes);
		return false;
	}
		
	return true;
}

/**
	@brief Update the scores for the current netlist and then print the result
 */
uint32_t PAREngine::ComputeAndPrintScore(vector<PARGraphEdge*>& unroutes, uint32_t iteration)
{
	uint32_t ucost = ComputeUnroutableCost(unroutes);
	uint32_t ccost = ComputeCongestionCost();
	uint32_t tcost = ComputeTimingCost();
	uint32_t cost = ComputeCost();
	
	unroutes.clear();
	printf(
		"    Iteration %d: unroutability %d, congestion %d, timing %d (total cost %d)\n",
		iteration,
		ucost,
		ccost,
		tcost,
		cost
		);
		
	return cost;
}

void PAREngine::PrintUnroutes(vector<PARGraphEdge*>& /*unroutes*/)
{
}

/**
	@brief Quickly find obviously unroutable designs.
	
	As of now, we only check for the condition where the netlist has more nodes with a given label than the device.
 */
bool PAREngine::SanityCheck(map<uint32_t, string> label_names, bool verbose)
{
	if(verbose)
		printf("Initial design feasibility check...\n");
		
	uint32_t nmax_net = m_netlist->GetMaxLabel();
	uint32_t nmax_dev = m_device->GetMaxLabel();
	
	//Make sure we'll detect if the netlist is bigger than the device
	if(nmax_net > nmax_dev)
	{
		printf("ERROR: Netlist contains a node with label %d, largest in device is %d\n",
			nmax_net, nmax_dev);
		return false;
	}
		
	//Cache the node count for both
	m_netlist->IndexNodesByLabel();
	m_device->IndexNodesByLabel();
	
	//For each legal label, verify we have enough nodes to map to
	for(uint32_t label = 0; label <= nmax_net; label ++)
	{
		uint32_t nnet = m_netlist->GetNumNodesWithLabel(label);
		uint32_t ndev = m_device->GetNumNodesWithLabel(label);
		
		//TODO: error reporting by device type, not just node IDs
		if(nnet > ndev)
		{
			printf("ERROR: Design is too big for the device "
				"(netlist has %d nodes of type %s, device only has %d)\n",
				nnet, label_names[label].c_str(), ndev);
			return false;
		}
	}
		
	//OK
	return true;
}

/**
	@brief Generate an initial placement that is legal, but may or may not be routable
 */
void PAREngine::InitialPlacement(bool verbose)
{
	if(verbose)
	{
		printf("Global placement of %d instances into %d sites...\n",
			m_netlist->GetNumNodes(),
			m_device->GetNumNodes());
		printf("    %d nets, %d routing channels available\n",
			m_netlist->GetNumEdges(),
			m_device->GetNumEdges());
	}
	
	//Cache the indexes
	m_netlist->IndexNodesByLabel();
	m_device->IndexNodesByLabel();
	
	//Do the actual placement (technology specific)
	InitialPlacement_core(verbose);
}

/**
	@brief Iteratively refine the placement until we can't get any better.
	
	Calculate a cost function for the current placement, then optimize
	
	@return True if we made changes to the netlist, false if nothing was done
 */
bool PAREngine::OptimizePlacement(
	vector<PARGraphNode*>& badnodes, map<uint32_t, string>& label_names,
	bool /*verbose*/)
{
	//Pick one of the nodes at random as our pivot node
	PARGraphNode* pivot = badnodes[rand() % badnodes.size()];
	
	//Find a new site for the pivot node (but remember the old site)
	//If nothing was found, bail out
	PARGraphNode* old_mate = pivot->GetMate();
	PARGraphNode* new_mate = GetNewPlacementForNode(pivot);
	if(new_mate == NULL)
		return false;
	
	//Do the swap, and measure the old/new scores
	uint32_t original_cost = ComputeCost();
	MoveNode(pivot, new_mate, label_names);
	uint32_t new_cost = ComputeCost();
	
	//TODO: say what we swapped?
	
	//if(verbose)
	//	printf("    Original cost %u, new cost %u\n", original_cost, new_cost);

	//If new cost is less, or greater with probability temperature, accept it
	//TODO: make probability depend on dCost?
	if(new_cost < original_cost)
		return true;
	if( (rand() % 100) < (int)m_temperature )
		return true;
		
	//If we don't like the change, revert
	MoveNode(pivot, old_mate, label_names);
	return false;
}

/**
	@brief Moves a netlist node to a new placement.
	
	If there is already a node at the requested site, the two are swapped.
	
	@param node			Netlist node to be moved
	@param newpos		Device node with the new position
 */
void PAREngine::MoveNode(
	PARGraphNode* node,
	PARGraphNode* newpos,
	map<uint32_t, string>& label_names)
{
	//Verify the labels match
	if(!newpos->MatchesLabel(node->GetLabel()))
	{
		fprintf(
			stderr,
			"INTERNAL ERROR: tried to assign node to illegal site (forward direction)\n"
			"    This indicates a bug in gp4par, please file a report on github.\n"
			"    We attempted to move a node of type \"%s\". The target site is valid for types:\n",
			label_names[node->GetLabel()].c_str()
			);
		PrintNodeTypes(newpos, label_names);
		
		//asm("int3");
		exit(-1);
	}
	
	//If the new position is already used by a netlist node, we have to fix that
	if(newpos->GetMate() != NULL)
	{
		PARGraphNode* other_net = newpos->GetMate();
		PARGraphNode* old_pos = node->GetMate();
		
		//Verify the labels match in the reverse direction of the swap
		if(!old_pos->MatchesLabel(other_net->GetLabel()))
		{
			fprintf(
				stderr,
				"INTERNAL ERROR: tried to assign node to illegal site (reverse direction)\n"
				"    This indicates a bug in gp4par, please file a report on github.\n"
				"    We attempted to move a node of type \"%s\". The target site is valid for types:\n",
				label_names[other_net->GetLabel()].c_str()
				);
			PrintNodeTypes(old_pos, label_names);
		
			//asm("int3");
			exit(-1);
		}
		
		other_net->MateWith(old_pos);
	}
	
	//Now that the new node has no mate, just hook them up
	node->MateWith(newpos);
}

/**
	@brief Prints all of the types of a given node for debugging
 */
void PAREngine::PrintNodeTypes(PARGraphNode* node, std::map<uint32_t, std::string>& label_names)
{
	fprintf(stderr, "    * %s\n", label_names[node->GetLabel()].c_str());
	for(uint32_t i=0; i<node->GetAlternateLabelCount(); i++)
		fprintf(stderr, "    * %s\n", label_names[node->GetAlternateLabel(i)].c_str());
}

/**
	@brief Compute the cost of a given placement.
 */
uint32_t PAREngine::ComputeCost()
{
	vector<PARGraphEdge*> unroutes;
	return
		ComputeUnroutableCost(unroutes)*10 +	//weight unroutability above everything else
		ComputeTimingCost() +
		ComputeCongestionCost();
}

/**
	@brief Compute the unroutability cost (measure of how many requested routes do not exist)
 */
uint32_t PAREngine::ComputeUnroutableCost(vector<PARGraphEdge*>& unroutes)
{
	uint32_t cost = 0;
	
	//Loop over each edge in the source netlist and try to find a matching edge in the destination.
	//No checks for multiple signals in one place for now.
	for(uint32_t i=0; i<m_netlist->GetNumNodes(); i++)
	{
		PARGraphNode* netsrc = m_netlist->GetNodeByIndex(i);
		for(uint32_t j=0; j<netsrc->GetEdgeCount(); j++)
		{
			PARGraphEdge* nedge = netsrc->GetEdgeByIndex(j);
			PARGraphNode* netdst = nedge->m_destnode;
			
			//For now, just bruteforce to find a matching edge (if there is one)
			bool found = false;
			PARGraphNode* devsrc = netsrc->GetMate();
			PARGraphNode* devdst = netdst->GetMate();
			for(uint32_t k=0; k<devsrc->GetEdgeCount(); k++)
			{
				PARGraphEdge* dedge = devsrc->GetEdgeByIndex(k);
				if(
					(dedge->m_destnode == devdst) &&
					(dedge->m_sourceport == nedge->m_sourceport) &&
					(dedge->m_destport == nedge->m_destport)
					)
				{
					
					found = true;
					break;
				}
			}
			
			//If nothing found, add to list
			if(!found)
			{
				unroutes.push_back(nedge);
				cost ++;
			}
		}
	}
	
	return cost;
}

/**
	@brief Compute the unroutability cost for a single node and a candidate placement for it
 */
uint32_t PAREngine::ComputeNodeUnroutableCost(PARGraphNode* pivot, PARGraphNode* candidate)
{
	uint32_t cost = 0;
	
	//Loop over each edge in the source netlist and try to find a matching edge in the destination.
	//No checks for multiple signals in one place for now.
	for(uint32_t i=0; i<m_netlist->GetNumNodes(); i++)
	{
		PARGraphNode* netsrc = m_netlist->GetNodeByIndex(i);
		for(uint32_t j=0; j<netsrc->GetEdgeCount(); j++)
		{
			PARGraphEdge* nedge = netsrc->GetEdgeByIndex(j);
			PARGraphNode* netdst = nedge->m_destnode;
			
			//If either the source or destination is not our pivot node, ignore it
			if( (netsrc != pivot) && (netdst != pivot) )
				continue;
			
			//Find the hypothetical source/dest pair
			PARGraphNode* devsrc = netsrc->GetMate();
			PARGraphNode* devdst = netdst->GetMate();
			if(netsrc == pivot)
				devsrc = candidate;
			else
				devdst = candidate;
			
			//For now, just bruteforce to find a matching edge (if there is one)
			bool found = false;
			for(uint32_t k=0; k<devsrc->GetEdgeCount(); k++)
			{
				PARGraphEdge* dedge = devsrc->GetEdgeByIndex(k);
				if(
					(dedge->m_destnode == devdst) &&
					(dedge->m_sourceport == nedge->m_sourceport) &&
					(dedge->m_destport == nedge->m_destport)
					)
				{
					
					found = true;
					break;
				}
			}
			
			//If nothing found, add to cost
			if(!found)
				cost ++;

		}
	}
	
	return cost;
}

/**
	@brief Computes the timing cost (measure of how much the current placement fails timing constraints).
	
	Default is zero (no timing analysis performed).
 */
uint32_t PAREngine::ComputeTimingCost()
{
	return 0;
}

/**
	@brief Computes the congestion cost (measure of how many routes are simultaneously occupied by multiple signals)
	
	Default is zero (no congestion analysis performed)
 */
uint32_t PAREngine::ComputeCongestionCost()
{
	return 0;	
}
