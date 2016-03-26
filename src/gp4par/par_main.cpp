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
	@brief The main place-and-route logic
 */
bool DoPAR(Greenpak4Netlist* netlist, Greenpak4Device* device)
{
	labelmap lmap;
	
	//Create the graphs
	printf("\nCreating netlist graphs...\n");
	PARGraph* ngraph = NULL;
	PARGraph* dgraph = NULL;
	BuildGraphs(netlist, device, ngraph, dgraph, lmap);

	//Create and run the PAR engine
	Greenpak4PAREngine engine(ngraph, dgraph);
	if(!engine.PlaceAndRoute(true))
	{
		printf("PAR failed\n");
		return false;
	}
	
	//Final DRC to make sure the placement is sane
	PostPARDRC(ngraph, dgraph);
		
	//Copy the netlist over
	unsigned int num_routes_used[2];
	CommitChanges(dgraph, device, num_routes_used);
	
	//Print reports
	PrintUtilizationReport(ngraph, device, num_routes_used);
	PrintPlacementReport(ngraph, device);
	
	//Final cleanup
	delete ngraph;
	delete dgraph;
	return true;
}

/**
	@brief Do various sanity checks after the design is routed
 */
void PostPARDRC(PARGraph* /*netlist*/, PARGraph* /*device*/)
{
	printf("\nPost-PAR design rule checks\n");
	
	//TODO: check floating inputs etc
	
	//TODO: check invalid IOB configuration (driving an input-only pin etc)
	
	//TODO: Check for multiple oscillators with power-down enabled but not the same source
}

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
		
		//See what kind of device we are
		auto bnode = static_cast<Greenpak4BitstreamEntity*>(node->GetData());
		auto iob = dynamic_cast<Greenpak4IOB*>(bnode);
		auto lut = dynamic_cast<Greenpak4LUT*>(bnode);
		auto ff = dynamic_cast<Greenpak4Flipflop*>(bnode);
		auto lfosc = dynamic_cast<Greenpak4LFOscillator*>(bnode);
		auto pwr = dynamic_cast<Greenpak4PowerRail*>(bnode);
			
		//Configure nodes of known type
		if(iob)
			CommitIOBChanges(static_cast<Greenpak4NetlistPort*>(mate->GetData()), iob);		
		else if(lut)
			CommitLUTChanges(static_cast<Greenpak4NetlistCell*>(mate->GetData()), lut);
		else if(ff)
			CommitFFChanges(static_cast<Greenpak4NetlistCell*>(mate->GetData()), ff);
		else if(lfosc)
		{
			printf("LFOsc commit changes not implemented\n");
		}
			
		//Ignore power rails, they have no configuration
		else if(pwr)
		{}
		
		//No idea what it is
		else
		{
			fprintf(stderr, "WARNING: Node at config base %d has unrecognized entity type\n", bnode->GetConfigBase());
		}
	}
	
	//Done configuring all of the nodes!
	//Configure routes between them
	CommitRouting(device, pdev, num_routes_used);
}

/**
	@brief Commit post-PAR results from the netlist to a single IOB
 */
void CommitIOBChanges(Greenpak4NetlistPort* niob, Greenpak4IOB* iob)
{
	//TODO: support array nets
	auto net = niob->m_net;
			
	//printf("    Configuring IOB %d\n", iob->GetPinNumber());

	//Apply attributes to configure the net
	for(auto x : net->m_attributes)
	{
		//do nothing, only for debugging
		if(x.first == "src")
		{}
		
		//already PAR'd, useless now
		else if(x.first == "LOC")
		{}
		
		//IO schmitt trigger
		else if(x.first == "SCHMITT_TRIGGER")
		{
			if(x.second == "0")
				iob->SetSchmittTrigger(false);
			else
				iob->SetSchmittTrigger(true);
		}
		
		//Pullup strength/direction
		else if(x.first == "PULLUP")
		{
			iob->SetPullDirection(Greenpak4IOB::PULL_UP);
			if(x.second == "10k")
				iob->SetPullStrength(Greenpak4IOB::PULL_10K);
			else if(x.second == "100k")
				iob->SetPullStrength(Greenpak4IOB::PULL_100K);
			else if(x.second == "1M")
				iob->SetPullStrength(Greenpak4IOB::PULL_1M);
		}
		
		//Pulldown strength/direction
		else if(x.first == "PULLDOWN")
		{
			iob->SetPullDirection(Greenpak4IOB::PULL_DOWN);
			if(x.second == "10k")
				iob->SetPullStrength(Greenpak4IOB::PULL_10K);
			else if(x.second == "100k")
				iob->SetPullStrength(Greenpak4IOB::PULL_100K);
			else if(x.second == "1M")
				iob->SetPullStrength(Greenpak4IOB::PULL_1M);
		}
		
		//Ignore flipflop initialization, that's handled elsewhere
		else if(x.first == "init")
		{
		}
		
		//TODO: 
		
		else
		{
			printf("WARNING: Top-level port \"%s\" has unrecognized attribute %s, ignoring\n",
				niob->m_name.c_str(), x.first.c_str());
		}
		
		//printf("        %s = %s\n", x.first.c_str(), x.second.c_str());
	}
	
	//Configure output enable
	switch(niob->m_direction)
	{
		case Greenpak4NetlistPort::DIR_OUTPUT:
			iob->SetOutputEnable(true);
			break;
			
		case Greenpak4NetlistPort::DIR_INPUT:
			iob->SetOutputEnable(false);
			break;
			
		case Greenpak4NetlistPort::DIR_INOUT:
		default:
			printf("ERROR: Requested invalid output configuration (or inout, which isn't implemented)\n");
			break;
	}
}

/**
	@brief Commit post-PAR results from the netlist to a single LUT
 */
void CommitLUTChanges(Greenpak4NetlistCell* ncell, Greenpak4LUT* lut)
{
	//printf("    Configuring LUT %s\n", ncell->m_name.c_str() );
	
	for(auto x : ncell->m_parameters)
	{
		//LUT initialization value, as decimal
		if(x.first == "INIT")
		{
			//convert to bit array format for the bitstream library
			uint32_t truth_table = atoi(x.second.c_str());
			unsigned int nbits = 1 << lut->GetOrder();
			for(unsigned int i=0; i<nbits; i++)
			{
				bool a3 = (i & 8) ? true : false;
				bool a2 = (i & 4) ? true : false;
				bool a1 = (i & 2) ? true : false;
				bool a0 = (i & 1) ? true : false;
				bool nbit = (truth_table & (1 << i)) ? true : false;
				//printf("        inputs %d %d %d %d: %d\n", a3, a2, a1, a0, nbit);
				lut->SetBit(nbit, a0, a1, a2, a3);
			}
		}
		
		else
		{
			printf("WARNING: Cell\"%s\" has unrecognized parameter %s, ignoring\n",
				ncell->m_name.c_str(), x.first.c_str());
		}
	}
}

/**
	@brief Commit post-PAR results from the netlist to a single flipflop
 */
void CommitFFChanges(Greenpak4NetlistCell* ncell, Greenpak4Flipflop* ff)
{
	if(ncell->HasParameter("SRMODE"))
	{
		if(ncell->m_parameters["SRMODE"] == "1")
			ff->SetSRMode(true);
		else
			ff->SetSRMode(false);
	}
	
	if(ncell->HasParameter("INIT"))
	{
		if(ncell->m_parameters["INIT"] == "1")
			ff->SetInitValue(true);
		else
			ff->SetInitValue(false);
	}
}

/**
	@brief Commit post-PAR results from the netlist to the routing matrix
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
			auto iob = dynamic_cast<Greenpak4IOB*>(dst);
			auto lut = dynamic_cast<Greenpak4LUT*>(dst);
			auto ff = dynamic_cast<Greenpak4Flipflop*>(dst);
			auto pwr = dynamic_cast<Greenpak4PowerRail*>(dst);
			auto spwr = dynamic_cast<Greenpak4PowerRail*>(src);
			auto dosc = dynamic_cast<Greenpak4LFOscillator*>(dst);
			auto sosc = dynamic_cast<Greenpak4LFOscillator*>(src);
			
			//If the source node is power, patch the topology so that everything comes from the right matrix.
			//We don't want to waste cross-connections on power nets
			if(spwr)
				src = pdev->GetPowerRail(dst->GetMatrix(), spwr->GetDigitalValue());
				
			else if(sosc)
			{
				printf("TODO: how to handle lfosc src\n");
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
			}
			
			//Destination is the low-frequency oscillator
			else if(dosc)
			{
				printf("Don't know how to configure LFOsc inputs\n");
			}
			
			else if(pwr)
			{
				printf("WARNING: Power rail should not be driven\n");
			}

			//Don't know what to do
			else
				printf("WARNING: Node at config base %d has unrecognized entity type\n", dst->GetConfigBase());
		}
	}
}

/**
	@brief Allocate and name a graph label
 */
uint32_t AllocateLabel(PARGraph*& ngraph, PARGraph*& dgraph, labelmap& lmap, std::string description)
{
	uint32_t nlabel = ngraph->AllocateLabel();
	uint32_t dlabel = dgraph->AllocateLabel();
	if(nlabel != dlabel)
	{
		fprintf(stderr, "INTERNAL ERROR: labels were allocated at the same time but don't match up\n");
		exit(-1);
	}
	
	lmap[nlabel] = description;
	
	return nlabel;
}
