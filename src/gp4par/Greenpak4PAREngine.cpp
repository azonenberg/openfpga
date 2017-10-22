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

#include <cmath>
#include "gp4par.h"

using namespace std;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Construction / destruction

Greenpak4PAREngine::Greenpak4PAREngine(PARGraph* netlist, PARGraph* device, labelmap& lmap)
	: PAREngine(netlist, device)
	, m_lmap(lmap)
{

}

Greenpak4PAREngine::~Greenpak4PAREngine()
{

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Initial placement

bool Greenpak4PAREngine::InitialPlacement_core()
{
	//Make a map of all nodes to their names
	map<string, Greenpak4BitstreamEntity*> nmap;
	for(size_t i=0; i<m_device->GetNumNodes(); i++)
	{
		auto entity = static_cast<Greenpak4BitstreamEntity*>(m_device->GetNodeByIndex(i)->GetData());
		nmap[entity->GetDescription()] = entity;
	}

	//Go over the netlist nodes, see if any have LOC constraints.
	//If so, place those at the constrained locations
	for(size_t i=0; i<m_netlist->GetNumNodes(); i++)
	{
		auto node = m_netlist->GetNodeByIndex(i);
		auto entity = static_cast<Greenpak4NetlistEntity*>(node->GetData());
		auto cell = dynamic_cast<Greenpak4NetlistCell*>(entity);
		if(cell == NULL)
		{
			LogError("Cell in netlist is not a Greenpak4NetlistCell\n");
			return false;
		}

		//Search for a LOC constraint, if there is one
		if(!cell->HasLOC())
			continue;
		string loc = cell->GetLOC();

		//Verify that the constrained location exists
		if(nmap.find(loc) == nmap.end())
		{
			LogError("Cell %s has invalid LOC constraint %s (no matching site in device)\n",
				cell->m_name.c_str(), loc.c_str());
			return false;
		}

		//If it exists, is it a legal site?
		auto site = nmap[loc];
		auto spnode = site->GetPARNode();
		if(!spnode->MatchesLabel(node->GetLabel()))
		{
			LogError(
				"Cell %s has invalid LOC constraint %s (site is of type %s, instance is of type %s)\n",
				cell->m_name.c_str(),
				loc.c_str(),
				m_lmap[site->GetPARNode()->GetLabel()].c_str(),
				m_lmap[node->GetLabel()].c_str()
				);
			return false;
		}

		//The site exists, is valid, and we're constrained to it.
		//Verify that there's not already something constrained to that site
		if(spnode->GetMate() != NULL)
		{
			LogError(
				"Cell %s has invalid LOC constraint %s (another instance is already constrained there)\n",
				cell->m_name.c_str(), loc.c_str());
			return false;
		}

		//Everything is good, apply the constraint
		LogVerbose("Applying LOC constraint %s to cell %s\n", loc.c_str(), cell->m_name.c_str());
		node->MateWith(spnode);
	}

	//For each label, mate each node in the netlist with the first legal mate in the device.
	//Simple and deterministic.
	uint32_t nmax_net = m_netlist->GetMaxLabel();
	for(uint32_t label = 0; label <= nmax_net; label ++)
	{
		uint32_t nnet = m_netlist->GetNumNodesWithLabel(label);
		uint32_t nsites = m_device->GetNumNodesWithLabel(label);

		uint32_t nsite = 0;
		for(uint32_t net = 0; net<nnet; net++)
		{
			PARGraphNode* netnode = m_netlist->GetNodeByLabelAndIndex(label, net);

			//If the netlist node is already constrained, don't auto-place it
			if(netnode->GetMate() != NULL)
				continue;

			//Try to find a legal site
			bool found = false;
			while(nsite < nsites)
			{
				PARGraphNode* devnode = m_device->GetNodeByLabelAndIndex(label, nsite);
				nsite ++;

				//If the site is used, we don't want to disturb what's already there
				//because it was probably LOC'd
				if(devnode->GetMate() != NULL)
					continue;

				//Site is unused, mate with it
				netnode->MateWith(devnode);
				found = true;
				break;
			}

			//This can happen in rare cases
			//(for example, we constrained all of the 8-bit counters to COUNT14 sites and now have a COUNT14).
			if(!found)
			{
				auto cell = static_cast<Greenpak4NetlistEntity*>(netnode->GetData());

				LogError(
					"Could not place netlist cell \"%s\" because we ran out of sites with type \"%s\"\n"
					"       This can happen if you have overly restrictive LOC constraints.\n",
					cell->m_name.c_str(),
					m_lmap[label].c_str()
					);
				return false;
			}
		}
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Congestion metrics

uint32_t Greenpak4PAREngine::ComputeCongestionCost() const
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
			auto dst = static_cast<Greenpak4BitstreamEntity*>(edge->m_destnode->GetMate()->GetData());
			uint32_t sm = src->GetMatrix();
			uint32_t dm = dst->GetMatrix();

			//If we're driving a port that isn't general fabric routing, then it doesn't compete for cross connections
			if(!dst->IsGeneralFabricInput(edge->m_destport))
				continue;

			//If the source has a dual, don't count this in the cost since it can route anywhere
			if(src->GetDual() != NULL)
				continue;

			//If matrices don't match, bump cost
			if(sm != dm)
				costs[sm] ++;
		}
	}

	//Squaring each half makes minimizing the larger one more important
	//vs if we just summed. Also add in a fixed penalty if we are using >100% resources
	uint32_t cost = costs[0]*costs[0] + costs[1]*costs[1];
	if( (costs[0] > 10) || (costs[1] > 10) )
		cost += 20;
	return cost;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Print logic

void Greenpak4PAREngine::PrintUnroutes(const vector<const PARGraphEdge*>& unroutes) const
{
	Log(Severity::ERROR, "\nUnroutable nets (%zu):\n", unroutes.size());
	LogIndenter li;
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
			Log(Severity::ERROR, "from cell %s (mapped to %s) port %s ",
				scell->m_name.c_str(),
				entity->GetDescription().c_str(),
				edge->m_sourceport.c_str()
				);
		}
		else if(sport != NULL)
		{
			auto iob = static_cast<Greenpak4IOB*>(edge->m_sourcenode->GetMate()->GetData());
			Log(Severity::ERROR, "from port %s (mapped to IOB_%d)", sport->m_name.c_str(), iob->GetPinNumber());
		}
		else
			Log(Severity::ERROR, "from [invalid] ");

		Log(Severity::ERROR, " to ");
		if(dcell != NULL)
		{
			auto entity = static_cast<Greenpak4BitstreamEntity*>(edge->m_destnode->GetMate()->GetData());
			Log(Severity::ERROR,
				"cell %s (mapped to %s) pin %s\n",
				dcell->m_name.c_str(),
				entity->GetDescription().c_str(),
				edge->m_destport.c_str()
				);
		}
		else if(dport != NULL)
			Log(Severity::ERROR, "port %s\n", dport->m_name.c_str());
		else
			Log(Severity::ERROR, "[invalid]\n");
	}

	Log(Severity::ERROR, "\n");
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
				if(!dst->IsGeneralFabricInput(edge->m_destport))
					continue;

				//Anything with a dual is always in an optimal location as far as congestion goes
				if(src->GetDual() != NULL)
					continue;

				//Add the node
				nodes.insert(edge->m_sourcenode);
			}
		}
	}

	//Find all nodes that are on one end of an unroutable edge
	m_unroutableNodes.clear();
	std::vector<const PARGraphEdge*> unroutes;
	ComputeUnroutableCost(unroutes);
	for(auto edge : unroutes)
	{
		if(!CantMoveSrc(static_cast<Greenpak4BitstreamEntity*>(edge->m_sourcenode->GetMate()->GetData())))
		{
			m_unroutableNodes.insert(edge->m_sourcenode);
			nodes.insert(edge->m_sourcenode);
		}
		if(!CantMoveDst(static_cast<Greenpak4BitstreamEntity*>(edge->m_destnode->GetMate()->GetData())))
		{
			m_unroutableNodes.insert(edge->m_destnode);
			nodes.insert(edge->m_destnode);
		}
	}

	//Push into the final output list
	for(auto x : nodes)
		bad_nodes.push_back(x);

	//DEBUG
	/*
	LogVerbose("Optimizing (%d bad nodes, %d unroutes)\n", bad_nodes.size(), unroutes.size());
	LogIndenter li;
	for(auto x : bad_nodes)
		LogVerbose("* %s\n",
			static_cast<Greenpak4BitstreamEntity*>(x->GetMate()->GetData())->GetDescription().c_str());
	*/
}

/**
	@brief Returns true if the given source node cannot be moved
 */
bool Greenpak4PAREngine::CantMoveSrc(Greenpak4BitstreamEntity* src)
{
	//If we have only one node of this type, we can't move it because there's nowhere to go
	auto pn = src->GetPARNode();
	if(pn == NULL)
		return true;
	if(m_device->GetNumNodesWithLabel(pn->GetLabel()) == 1)
		return true;

	//If it has a LOC constraint, don't move it
	auto se = static_cast<Greenpak4NetlistEntity*>(pn->GetMate()->GetData());
	auto netnode = dynamic_cast<Greenpak4NetlistCell*>(se);
	if( (netnode != NULL) && netnode->HasLOC() )
		return true;

	//nope, it's movable
	return false;
}

bool Greenpak4PAREngine::CanMoveNode(const PARGraphNode* node,
	const PARGraphNode* old_mate, const PARGraphNode* new_mate) const
{
	//Let base class filter stuff
	if(!PAREngine::CanMoveNode(node, old_mate, new_mate))
		return false;

	//Remaining checks only make sense if we are displacing another node
	auto displaced = new_mate->GetMate();
	if(displaced == NULL)
		return true;

	//If the displaced node has a LOC constraint, don't use that site
	auto netnode = dynamic_cast<Greenpak4NetlistCell*>(static_cast<Greenpak4NetlistEntity*>(displaced->GetData()));
	if( (netnode != NULL) && netnode->HasLOC() )
	{
		//LogDebug("Tried to move cell %s\n", netnode->m_name.c_str());
		return false;
	}

	return true;
}

/**
	@brief Returns true if the given destination node cannot be moved
 */
bool Greenpak4PAREngine::CantMoveDst(Greenpak4BitstreamEntity* dst)
{
	//If we have only one node of this type, we can't move it because there's nowhere to go
	auto pn = dst->GetPARNode();
	if(pn == NULL)
		return true;
	if(m_device->GetNumNodesWithLabel(pn->GetLabel()) == 1)
		return true;

	//If it has a LOC constraint, don't move it
	auto se = static_cast<Greenpak4NetlistEntity*>(pn->GetMate()->GetData());
	auto netnode = dynamic_cast<Greenpak4NetlistCell*>(se);
	if( (netnode != NULL) && netnode->HasLOC() )
		return true;

	//nope, it's movable
	return false;
}

/**
	@brief Find a new (hopefully more efficient) placement for a given netlist node
 */
PARGraphNode* Greenpak4PAREngine::GetNewPlacementForNode(const PARGraphNode* pivot)
{
	//Find which matrix we were assigned to
	PARGraphNode* current_node = pivot->GetMate();
	auto current_site = static_cast<Greenpak4BitstreamEntity*>(current_node->GetData());
	uint32_t current_matrix = current_site->GetMatrix();

	//BUGFIX: Use the netlist node's label, not the PAR node
	uint32_t label = pivot->GetLabel();

	//Debug log
	bool unroutable = (m_unroutableNodes.find(pivot) != m_unroutableNodes.end());
	Greenpak4NetlistEntity* ne = static_cast<Greenpak4NetlistEntity*>(pivot->GetData());
	LogDebug("Seeking new placement for node %s (at %s, unroutable = %d)\n",
		ne->m_name.c_str(),
		current_site->GetDescription().c_str(), unroutable);

	//Default to trying the opposite matrix
	uint32_t target_matrix = 1 - current_matrix;

	//Make the list of candidate placements
	std::set<PARGraphNode*> temp_candidates;
	for(uint32_t i=0; i<m_device->GetNumNodesWithLabel(label); i++)
	{
		PARGraphNode* node = m_device->GetNodeByLabelAndIndex(label, i);
		Greenpak4BitstreamEntity* entity = static_cast<Greenpak4BitstreamEntity*>(node->GetData());

		//Do not consider unroutable positions at this time
		if(0 != ComputeNodeUnroutableCost(pivot, node))
			continue;

		if(entity->GetMatrix() == target_matrix)
			temp_candidates.insert(node);
	}

	//If no routable candidates found in the opposite matrix, check all matrices
	if(temp_candidates.empty())
	{
		for(uint32_t i=0; i<m_device->GetNumNodesWithLabel(label); i++)
		{
			PARGraphNode* node = m_device->GetNodeByLabelAndIndex(label, i);

			if(0 != ComputeNodeUnroutableCost(pivot, node))
				continue;

			temp_candidates.insert(node);
		}
	}

	//If no routable candidates found anywhere, consider the entire chip and hope we can patch things up later
	if(temp_candidates.empty())
	{
		LogDebug("No routable candidates found\n");
		for(uint32_t i=0; i<m_device->GetNumNodesWithLabel(label); i++)
			temp_candidates.insert(m_device->GetNodeByLabelAndIndex(label, i));
	}

	//Move to a vector for random access
	std::vector<PARGraphNode*> candidates;
	for(auto x : temp_candidates)
		candidates.push_back(x);
	uint32_t ncandidates = candidates.size();
	if(ncandidates == 0)
		return NULL;

	//Pick one at random
	auto c = candidates[RandomNumber() % ncandidates];
	LogDebug("Selected %s\n",
		static_cast<Greenpak4BitstreamEntity*>(c->GetData())->GetDescription().c_str());
	return c;
}

const char* Greenpak4PAREngine::GetLabelName(uint32_t label) const {
	// Need to use find() rather than [] because [] can mutate.
	auto found_label = m_lmap.find(label);
	if (found_label == m_lmap.end())
		return "";
	return found_label->second.c_str();
}
