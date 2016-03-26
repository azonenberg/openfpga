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
	@brief Build the graphs
 */
void BuildGraphs(
	Greenpak4Netlist* netlist,
	Greenpak4Device* device,
	PARGraph*& ngraph,
	PARGraph*& dgraph,
	labelmap& lmap)
{
	//Create the graphs
	ngraph = new PARGraph;
	dgraph = new PARGraph;
	
	//This is the module being PAR'd
	Greenpak4NetlistModule* module = netlist->GetTopModule();
	
	//Create device entries for the IOBs
	uint32_t iob_label = AllocateLabel(ngraph, dgraph, lmap, "Unconstrained IOB");
	for(auto it = device->iobbegin(); it != device->iobend(); it ++)
	{
		Greenpak4IOB* iob = it->second;
		PARGraphNode* inode = new PARGraphNode(iob_label, iob);
		iob->SetPARNode(inode);
		dgraph->AddNode(inode);
	}
	
	//Create netlist nodes for the IOBs
	for(auto it = module->port_begin(); it != module->port_end(); it ++)
	{
		Greenpak4NetlistPort* port = it->second;
		
		if(!module->HasNet(it->first))
		{
			fprintf(stderr, "INTERNAL ERROR: Netlist has a port named \"%s\" but no corresponding net\n",
				it->first.c_str());
			exit(-1);
		}
		
		//Look up the net and make sure there's a LOC
		Greenpak4NetlistNet* net = module->GetNet(it->first);
		if(!net->HasAttribute("LOC"))
		{
			fprintf(
				stderr,
				"ERROR: Top-level port \"%s\" does not have a constrained location (LOC attribute).\n"
				"       In order to ensure proper device functionality all IO pins must be constrained.\n",
				it->first.c_str());
			exit(-1);
		}
		
		//Look up the matching IOB
		int pin_num;
		string sloc = net->GetAttribute("LOC");
		if(1 != sscanf(sloc.c_str(), "P%d", &pin_num))
		{
			fprintf(
				stderr,
				"ERROR: Top-level port \"%s\" has an invalid LOC constraint \"%s\" (expected P3, P5, etc)\n",
				it->first.c_str(),
				sloc.c_str());
			exit(-1);
		}
		Greenpak4IOB* iob = device->GetIOB(pin_num);
		if(iob == NULL)
		{
			fprintf(
				stderr,
				"ERROR: Top-level port \"%s\" has an invalid LOC constraint \"%s\" (no such pin, or not a GPIO)\n",
				it->first.c_str(),
				sloc.c_str());
			exit(-1);
		}
		
		//Type B IOBs cannot be used for inout
		if( (port->m_direction == Greenpak4NetlistPort::DIR_INOUT) &&
			(dynamic_cast<Greenpak4IOBTypeB*>(iob) != NULL) )
		{
			fprintf(
				stderr,
				"ERROR: Top-level inout port \"%s\" is constrained to a pin \"%s\" which does "
					"not support bidirectional IO\n",
				it->first.c_str(),
				sloc.c_str());
			exit(-1);
		}
		
		//Input-only pins cannot be used for IO or output
		if( (port->m_direction != Greenpak4NetlistPort::DIR_INPUT) &&
			iob->IsInputOnly() )
		{
			fprintf(
				stderr,
				"ERROR: Top-level port \"%s\" is constrained to an input-only pin \"%s\" but is not "
					"declared as an input\n",
				it->first.c_str(),
				sloc.c_str());
			exit(-1);
		}
		
		//Allocate a new graph label for these IOBs
		string sname = string("Constrained IOB ") + it->first;
		uint32_t label = AllocateLabel(ngraph, dgraph, lmap, sname);
		
		//If the IOB already has a custom label, we constrained two nets to the same place!
		PARGraphNode* ipnode = iob->GetPARNode();
		if(ipnode->GetLabel() != iob_label)
		{
			fprintf(
				stderr,
				"ERROR: Top-level port \"%s\" is constrained to pin \"%s\" but another port is already constrained "
				"to this pin.\n",
				it->first.c_str(),
				sloc.c_str());
			exit(-1);
		}
		
		//Create the node
		//TODO: Support buses here (for now, point to the entire port)
		PARGraphNode* netnode = new PARGraphNode(label, port);
		port->m_parnode = netnode;
		ngraph->AddNode(netnode);
		
		//Re-label the assigned IOB so we get a proper match to it
		ipnode->Relabel(label);
	}
	
	//Make device nodes for each type of LUT
	uint32_t lut2_label = AllocateLabel(ngraph, dgraph, lmap, "GP_2LUT");
	uint32_t lut3_label = AllocateLabel(ngraph, dgraph, lmap, "GP_3LUT");
	uint32_t lut4_label = AllocateLabel(ngraph, dgraph, lmap, "GP_4LUT");
	for(unsigned int i=0; i<device->GetLUT2Count(); i++)
	{
		Greenpak4LUT* lut = device->GetLUT2(i);
		PARGraphNode* lnode = new PARGraphNode(lut2_label, lut);
		lut->SetPARNode(lnode);
		dgraph->AddNode(lnode);
	}
	for(unsigned int i=0; i<device->GetLUT3Count(); i++)
	{
		Greenpak4LUT* lut = device->GetLUT3(i);
		PARGraphNode* lnode = new PARGraphNode(lut3_label, lut);
		lut->SetPARNode(lnode);
		dgraph->AddNode(lnode);
	}
	for(unsigned int i=0; i<device->GetLUT4Count(); i++)
	{
		Greenpak4LUT* lut = device->GetLUT4(i);
		PARGraphNode* lnode = new PARGraphNode(lut4_label, lut);
		lut->SetPARNode(lnode);
		dgraph->AddNode(lnode);
	}	
	//Make device nodes for each type of flipflop
	uint32_t dff_label = AllocateLabel(ngraph, dgraph, lmap, "GP_DFF");
	uint32_t dffsr_label = AllocateLabel(ngraph, dgraph, lmap, "GP_DFFSR");
	for(unsigned int i=0; i<device->GetTotalFFCount(); i++)
	{
		Greenpak4Flipflop* flop = device->GetFlipflopByIndex(i);
		PARGraphNode* fnode = NULL;
		if(flop->HasSetReset())
		{
			fnode = new PARGraphNode(dffsr_label, flop);
			
			//It's legal to map a DFF to a DFFSR site, so add that as an alternate
			fnode->AddAlternateLabel(dff_label);
		}
		else
			fnode = new PARGraphNode(dff_label, flop);
		flop->SetPARNode(fnode);
		dgraph->AddNode(fnode);
	}
	
	//Make device nodes for the low-frequency oscillator
	uint32_t lfosc_label = AllocateLabel(ngraph, dgraph, lmap, "GP_LFOSC");
	Greenpak4LFOscillator* lfosc = device->GetLFOscillator();
	PARGraphNode* lfnode = new PARGraphNode(lfosc_label, lfosc);
	lfosc->SetPARNode(lfnode);
	dgraph->AddNode(lfnode);
	
	//TODO: make nodes for all of the other hard IP
	
	//Power nets
	uint32_t vdd_label = AllocateLabel(ngraph, dgraph, lmap, "GP_VDD");
	uint32_t vss_label = AllocateLabel(ngraph, dgraph, lmap, "GP_VSS");
	for(unsigned int matrix = 0; matrix<2; matrix++)
	{
		auto vdd = device->GetPowerRail(matrix, true);
		auto vss = device->GetPowerRail(matrix, false);
		PARGraphNode* vnode = new PARGraphNode(vdd_label, vdd);
		PARGraphNode* gnode = new PARGraphNode(vss_label, vss);
		vdd->SetPARNode(vnode);
		vss->SetPARNode(gnode);
		dgraph->AddNode(vnode);
		dgraph->AddNode(gnode);
	}
	
	//Make netlist nodes for cells
	for(auto it = module->cell_begin(); it != module->cell_end(); it ++)
	{
		//TODO: Support LOC constraints on individual cells
		//For now, just label by type
		
		//Figure out the type of node
		Greenpak4NetlistCell* cell = it->second;
		uint32_t label = 0;
		if(cell->m_type == "GP_2LUT")
			label = lut2_label;
		else if(cell->m_type == "GP_3LUT")
			label = lut3_label;
		else if(cell->m_type == "GP_4LUT")
			label = lut4_label;
		else if(cell->m_type == "GP_DFF")
			label = dff_label;
		else if( (cell->m_type == "GP_DFFR") || (cell->m_type == "GP_DFFS") || (cell->m_type == "GP_DFFSR") )
			label = dffsr_label;
		else if(cell->m_type == "GP_LFOSC")
			label = lfosc_label;
		
		//Power nets are virtual cells we use internally
		else if(cell->m_type == "GP_VDD")
			label = vdd_label;
		else if(cell->m_type == "GP_VSS")
			label = vss_label;
		
		else
		{
			fprintf(
				stderr,
				"ERROR: Cell \"%s\" is of type \"%s\" which is not a valid GreenPak4 primitive\n",
				cell->m_name.c_str(), cell->m_type.c_str());
			exit(-1);			
		}
		
		//Create a node for the cell's output
		//TODO: handle cells with multiple outputs
		PARGraphNode* nnode = new PARGraphNode(label, cell);
		cell->m_parnode = nnode;
		ngraph->AddNode(nnode);
	}
	
	//Create edges in the netlist.
	//This requires breaking point-to-multipoint nets into multiple point-to-point links.
	for(auto it = netlist->nodebegin(); it != netlist->nodeend(); it ++)
	{
		Greenpak4NetlistNode* node = *it;
			
		//printf("    Node %s is sourced by:\n", node->m_name.c_str());
		
		PARGraphNode* source = NULL;
		
		//See if it was sourced by a port
		for(auto p : node->m_ports)
		{
			if(p->m_direction == Greenpak4NetlistPort::DIR_INPUT)
			{
				source = p->m_parnode;
				//Greenpak4NetlistNet* net = netlist->GetTopModule()->GetNet(p->m_name);
				//printf("        port %s (loc %s)\n", p->m_name.c_str(), net->m_attributes["LOC"].c_str());
			}
			
			else if(p->m_direction == Greenpak4NetlistPort::DIR_INOUT)
			{
				fprintf(
					stderr,
					"ERROR: Tristates not implemented\n");
				exit(-1);
			}
		}
		
		//See if it was sourced by a node
		for(auto c : node->m_nodeports)
		{
			Greenpak4NetlistModule* module = netlist->GetModule(c.m_cell->m_type);
			Greenpak4NetlistPort* port = module->GetPort(c.m_portname);
			
			if(port->m_direction == Greenpak4NetlistPort::DIR_INPUT)
				continue;
			else if(port->m_direction == Greenpak4NetlistPort::DIR_INOUT)
			{
				fprintf(
					stderr,
					"ERROR: Tristates not implemented\n");
				exit(-1);
			}
			
			//TODO: Get the graph node for this port
			//For now, the entire cell has a single node as its output
			source = c.m_cell->m_parnode;
			//printf("        cell %s port %s\n", c.m_cell->m_name.c_str(), c.m_portname.c_str());
		}
		
		//printf("        and drives\n");
		
		//DRC fail if undriven net
		if(source == NULL)
		{
			fprintf(
				stderr,
				"ERROR: Net \"%s\" has loads, but no driver\n",
				node->m_name.c_str());
			exit(-1);	
		}
		
		//Create edges from this source node to all sink nodes
		for(auto p : node->m_ports)
		{
			if(p->m_parnode != source)
			{
				source->AddEdge(p->m_parnode);
				//Greenpak4NetlistNet* net = netlist->GetTopModule()->GetNet(p->m_name);
				//printf("        port %s (loc %s)\n", p->m_name.c_str(), net->m_attributes["LOC"].c_str());
			}
		}
		for(auto c : node->m_nodeports)
		{
			if(c.m_cell->m_parnode != source)
			{
				source->AddEdge(c.m_cell->m_parnode, c.m_portname);
				//printf("        cell %s port %s\n", c.m_cell->m_name.c_str(), c.m_portname.c_str());
			}
		}
	}
	
	//Create edges in the device. This is easy as we know a priori what connections are legal
	//TODO: do more hard IP
	
	//Make a list of all nodes in each half of the device connected to general fabric routing
	std::vector<PARGraphNode*> device_nodes;
	for(unsigned int i=0; i<device->GetLUT2Count(); i++)
		device_nodes.push_back(device->GetLUT2(i)->GetPARNode());
	for(unsigned int i=0; i<device->GetLUT3Count(); i++)
		device_nodes.push_back(device->GetLUT3(i)->GetPARNode());
	for(unsigned int i=0; i<device->GetLUT4Count(); i++)
		device_nodes.push_back(device->GetLUT4(i)->GetPARNode());
	for(unsigned int i=0; i<device->GetTotalFFCount(); i++)
		device_nodes.push_back(device->GetFlipflopByIndex(i)->GetPARNode());
	for(auto it = device->iobbegin(); it != device->iobend(); it ++)
		device_nodes.push_back(it->second->GetPARNode());
	for(unsigned int i=0; i<2; i++)
	{
		device_nodes.push_back(device->GetPowerRail(i, true)->GetPARNode());
		device_nodes.push_back(device->GetPowerRail(i, false)->GetPARNode());
	}
	//TODO: hard IP
	
	//Add the O(n^2) edges between the main fabric nodes
	for(auto x : device_nodes)
	{
		for(auto y : device_nodes)
		{
			if(x != y)
			{
				//Add paths to individual cell pins
				auto entity = static_cast<Greenpak4BitstreamEntity*>(y->GetData());
				auto lut = dynamic_cast<Greenpak4LUT*>(entity);
				auto ff = dynamic_cast<Greenpak4Flipflop*>(entity);
				if(lut)
				{
					x->AddEdge(y, "IN0");
					x->AddEdge(y, "IN1");
					if(lut->GetOrder() > 2)
						x->AddEdge(y, "IN2");
					if(lut->GetOrder() > 3)
						x->AddEdge(y, "IN3");
				}
				else if(ff)
				{
					x->AddEdge(y, "D");
					x->AddEdge(y, "CLK");
					if(ff->HasSetReset())
					{
						//allow all ports and we figure out which to use later
						x->AddEdge(y, "nSR");
						x->AddEdge(y, "nSET");
						x->AddEdge(y, "nRST");
					}
					x->AddEdge(y, "Q");
				}
				
				//TODO: add paths to oscillator
				
				//no, just add path to the node in general
				else
					x->AddEdge(y);
			}
		}
	}
	
	//TODO: add dedicated routing between hard IP etc
	
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