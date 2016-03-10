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
bool PAREngine::Route(bool verbose)
{
	if(verbose)
		printf("XBPAR initializing...\n");
		
	//Detect obviously impossible-to-route designs
	if(!SanityCheck(verbose))
		return false;
		
	//Do an initial valid, but not necessarily routable, placement
	InitialPlacement(verbose);
		
	//Converge until we get a passing placement
	if(!OptimizePlacement(verbose))
		return false;
		
	return true;
}

/**
	@brief Quickly find obviously unroutable designs.
	
	As of now, we only check for the condition where the netlist has more nodes with a given label than the device.
 */
bool PAREngine::SanityCheck(bool verbose)
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
	m_netlist->CountLabels();
	m_device->CountLabels();
	
	//For each legal label, verify we have enough nodes to map to
	for(uint32_t label = 0; label <= nmax_net; label ++)
	{
		uint32_t nnet = m_netlist->GetNumNodesWithLabel(label);
		uint32_t ndev = m_device->GetNumNodesWithLabel(label);
		
		//TODO: error reporting by device type, not just node IDs
		if(nnet > ndev)
		{
			printf("ERROR: Design is not too big (netlist has %d nodes with label %d, device only has %d)\n",
				nnet, label, ndev);
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
	//Cache the indexes
	m_netlist->IndexNodesByLabel();
	m_device->IndexNodesByLabel();
	
	//For each label, mate each node in the netlist with the first legal mate in the device.
	//Simple and deterministic.
	uint32_t nmax_net = m_netlist->GetMaxLabel();
	for(uint32_t label = 0; label <= nmax_net; label ++)
	{
		uint32_t nnet = m_netlist->GetNumNodesWithLabel(label);
		for(uint32_t net = 0; net<nnet; net++)
		{
			PARGraphNode* netnode = m_netlist->GetNodeByLabelAndIndex(label, net);
			PARGraphNode* devnode = m_device->GetNodeByLabelAndIndex(label, net);
			netnode->MateWith(devnode);
		}
	}
}

/**
	@brief Iteratively refine the placement
	
	Calculate a cost function for the current placement, then optimize
 */
bool PAREngine::OptimizePlacement(bool verbose)
{
	return false;	
}
