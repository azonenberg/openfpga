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
bool CommitChanges(PARGraph* device, Greenpak4Device* pdev, unsigned int* num_routes_used)
{
	LogNotice("\nBuilding post-route netlist...\n");

	//Go over all of the nodes in the graph and configure the nodes themselves
	//Net routing will come later!
	for(uint32_t i=0; i<device->GetNumNodes(); i++)
	{
		//If no node in the netlist is assigned to us, leave us at default configuration
		PARGraphNode* node = device->GetNodeByIndex(i);
		PARGraphNode* mate = node->GetMate();
		if(mate == NULL)
			continue;

		//TODO: make this return a bool so we can detect problems?
		static_cast<Greenpak4BitstreamEntity*>(node->GetData())->CommitChanges();
	}

	//Done configuring all of the nodes!
	//Configure routes between them
	if(!CommitRouting(device, pdev, num_routes_used))
		return false;

	return true;
}

/**
	@brief Commit post-PAR results from the netlist to the routing matrix
 */
bool CommitRouting(PARGraph* device, Greenpak4Device* pdev, unsigned int* num_routes_used)
{
	num_routes_used[0] = 0;
	num_routes_used[1] = 0;

	//Map of source net to cross-connection output
	map<Greenpak4EntityOutput, Greenpak4EntityOutput> nodemap;

	bool ran_out = false;

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

			//If the source node has a dual, use the secondary output if needed
			//so we don't waste cross connections
			if(src->GetDual())
			{
				if(dst->GetMatrix() != src->GetMatrix())
					src = src->GetDual();
			}

			//Look up the actual NET (not just the entity) for the source.
			//If we don't do this we risk merging cross-connections that should not be (see github issue #13)
			Greenpak4EntityOutput srcnet = src->GetOutput(edge->m_sourceport);

			//Cross connections
			//Only use these if destination node is general fabric routing; dedicated routing can cross between
			//the matrices freely
			unsigned int srcmatrix = src->GetMatrix();
			if( (srcmatrix != dst->GetMatrix()) && dst->IsGeneralFabricInput(edge->m_destport) )
			{
				//Reuse existing connections, if any
				if(nodemap.find(srcnet) != nodemap.end())
					srcnet = nodemap[srcnet];

				//Allocate a new cross-connection
				else
				{
					//We need to jump from one matrix to another!
					//Make sure we have a free cross-connection to use
					if(num_routes_used[srcmatrix] >= 10)
						ran_out = true;

					else
					{
						//Save our cross-connection and mark it as used
						auto xconn = pdev->GetCrossConnection(srcmatrix, num_routes_used[srcmatrix]);


						//Insert the cross-connection into the path
						xconn->SetInput("I", srcnet);
						Greenpak4EntityOutput newsrc = xconn->GetOutput("O");
						nodemap[srcnet] = newsrc;
						srcnet = newsrc;
					}

					num_routes_used[srcmatrix] ++;
				}
			}

			//Yay virtual functions - we can set the input without caring about the node type
			if(!ran_out)
				dst->SetInput(edge->m_destport, srcnet);
		}
	}

	//Don't fail until we attempt full routing.
	//This lets us see how much over the limit we went
	if(ran_out)
	{
		LogError(
			"More than 100%% of device resources are used "
			"(cross connections)\n");
		return false;
	}

	return true;
}
