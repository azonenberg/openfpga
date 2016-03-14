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
 
#include "../greenpak4/Greenpak4.h"
#include "../xbpar/xbpar.h"
#include "Greenpak4PAREngine.h"
#include <stdio.h>
#include <string>

using namespace std;

void ShowUsage();
void ShowVersion();

void BuildGraphs(Greenpak4Netlist* netlist, Greenpak4Device* device, PARGraph*& ngraph, PARGraph*& dgraph);
void ApplyLocConstraints(Greenpak4Netlist* netlist, PARGraph* ngraph, PARGraph* dgraph);
bool DoPAR(Greenpak4Netlist* netlist, Greenpak4Device* device);
void PostPARDRC(PARGraph* netlist, PARGraph* device);
void CommitChanges(PARGraph* netlist, PARGraph* device);
void CommitIOBChanges(Greenpak4NetlistPort* niob, Greenpak4IOB* iob);
void CommitLUTChanges(Greenpak4NetlistCell* ncell, Greenpak4LUT* lut);

int main(int argc, char* argv[])
{
	//Netlist file
	string fname = "";
	
	//Output file
	string ofname = "";
	
	//Top-level module name
	string top = "";
	
	//Action to take with unused pins;
	Greenpak4IOB::PullDirection unused_pull = Greenpak4IOB::PULL_NONE;
	Greenpak4IOB::PullStrength  unused_drive = Greenpak4IOB::PULL_1M;
	
	//TODO: make this switchable via command line args
	Greenpak4Device::GREENPAK4_PART part = Greenpak4Device::GREENPAK4_SLG46620;
	
	//Parse command-line arguments
	for(int i=1; i<argc; i++)
	{
		string s(argv[i]);
		
		if(s == "--help")
		{
			ShowUsage();
			return 0;
		}
		else if(s == "--version")
		{
			ShowVersion();
			return 0;
		}
		else if(s == "--top")
		{
			if(i+1 < argc)
				top = argv[++i];
			else
			{
				printf("--top requires an argument\n");
				return 1;
			}
		}		
		else if(s == "--unused-pull")
		{
			if(i+1 < argc)
			{
				string pull = argv[++i];
				if(pull == "down")
					unused_pull = Greenpak4IOB::PULL_DOWN;
				else if(pull == "up")
					unused_pull = Greenpak4IOB::PULL_UP;
				else if( (pull == "none") || (pull == "float") )
					unused_pull = Greenpak4IOB::PULL_NONE;
				else
				{
					printf("--unused-pull must be one of up, down, float, none\n");
					return 1;
				}
			}
			else
			{
				printf("--unused-pull requires an argument\n");
				return 1;
			}
		}
		else if(s == "--unused-drive")
		{
			if(i+1 < argc)
			{
				string drive = argv[++i];
				if(drive == "10k")
					unused_drive = Greenpak4IOB::PULL_10K;
				else if(drive == "100k")
					unused_drive = Greenpak4IOB::PULL_100K;
				else if(drive == "1M")
					unused_drive = Greenpak4IOB::PULL_1M;
				else
				{
					printf("--unused-drive must be one of 10k, 100k, 1M\n");
					return 1;
				}
			}
			else
			{
				printf("--unused-drive requires an argument\n");
				return 1;
			}
		}
		else if(s == "--output")
		{
			if(i+1 < argc)
				ofname = argv[++i];
			else
			{
				printf("--output requires an argument\n");
				return 1;
			}
		}
		
		//assume it's the netlist file if it'[s the first non-switch argument
		else if( (s[0] != '-') && (fname == "") )
			fname = s;
			
		else
		{
			printf("Unrecognized command-line argument \"%s\", use --help\n", s.c_str());
			return 1;
		}
	}
	
	//Netlist filenames must be specified
	if( (fname == "") || (ofname == "") )
	{
		ShowUsage();
		return 1;
	}
	
	//Print header
	ShowVersion();
	
	//Print configuration
	printf("\nDevice configuration:\n");
	printf("Target device: SLG46620V\n");
	printf("VCC range: not yet implemented\n");
	printf("Unused pins: ");
	switch(unused_pull)
	{
		case Greenpak4IOB::PULL_NONE:
			printf("float\n");
			break;
			
		case Greenpak4IOB::PULL_DOWN:
			printf("pull down with ");
			break;
			
		case Greenpak4IOB::PULL_UP:
			printf("pull up with ");
			break;
			
		default:
			printf("invalid\n");
			return 1;
	}
	if(unused_pull != Greenpak4IOB::PULL_NONE)
	{
		switch(unused_drive)
		{
			case Greenpak4IOB::PULL_10K:
				printf("10K\n");
				break;
				
			case Greenpak4IOB::PULL_100K:
				printf("100K\n");
				break;
				
			case Greenpak4IOB::PULL_1M:
				printf("1M\n");
				break;
				
			default:
				printf("invalid\n");
				return 1;
		}
	}
	
	
	//Parse the unplaced netlist
	printf("\nLoading Yosys JSON file \"%s\", expecting top-level module \"%s\"\n",
		fname.c_str(), top.c_str());
	Greenpak4Netlist netlist(fname, top);
	
	//Create the device and initialize all IO pins
	Greenpak4Device device(part, unused_pull, unused_drive);
	
	//Do the actual P&R
	if(!DoPAR(&netlist, &device))
		return 2;
	
	//Write the final bitstream
	printf("\nWriting final bitstream to output file \"%s\"\n", ofname.c_str());
	device.WriteToFile(ofname);	
	
	//TODO: Static timing analysis
	printf("\nStatic timing analysis: not yet implemented\n");
	
	return 0;
}

/**
	@brief The main place-and-route logic
 */
bool DoPAR(Greenpak4Netlist* netlist, Greenpak4Device* device)
{
	//Create the graphs
	printf("\nCreating netlist graphs...\n");
	PARGraph* ngraph = NULL;
	PARGraph* dgraph = NULL;
	BuildGraphs(netlist, device, ngraph, dgraph);

	//Create and run the PAR engine
	Greenpak4PAREngine engine(ngraph, dgraph);
	if(!engine.PlaceAndRoute(true))
	{
		printf("PAR failed\n");
		return false;
	}
	
	//Final DRC to make sure the placement is sane
	PostPARDRC(ngraph, dgraph);
	
	//Copy the netlist over, then clean up
	CommitChanges(ngraph, dgraph);
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
}

/**
	@brief Build the graphs
 */
void BuildGraphs(Greenpak4Netlist* netlist, Greenpak4Device* device, PARGraph*& ngraph, PARGraph*& dgraph)
{
	//Create the graphs
	ngraph = new PARGraph;
	dgraph = new PARGraph;
	
	//This is the module being PAR'd
	Greenpak4NetlistModule* module = netlist->GetTopModule();
	
	//Create device entries for the IOBs
	uint32_t iob_label = ngraph->AllocateLabel();
	dgraph->AllocateLabel();
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
		//Must always allocate from both graphs at the same time (TODO: paired allocation somehow?)
		uint32_t label = ngraph->AllocateLabel();
		dgraph->AllocateLabel();
				
		//Create the node
		//TODO: Support buses here (for now, point to the entire port)
		PARGraphNode* netnode = new PARGraphNode(label, port);
		port->m_parnode = netnode;
		ngraph->AddNode(netnode);
		
		//Re-label the assigned IOB so we get a proper match to it
		iob->GetPARNode()->Relabel(label);
	}
	
	//Make device nodes for each type of LUT
	uint32_t lut2_label = ngraph->AllocateLabel();
	dgraph->AllocateLabel();
	uint32_t lut3_label = ngraph->AllocateLabel();
	dgraph->AllocateLabel();
	uint32_t lut4_label = ngraph->AllocateLabel();
	dgraph->AllocateLabel();
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
	//TODO: LUT4s
	
	//TODO: make nodes for all of the other hard IP
	
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
			printf("        cell %s port %s\n", c.m_cell->m_name.c_str(), c.m_portname.c_str());
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
	//TODO: LUT4s
	for(auto it = device->iobbegin(); it != device->iobend(); it ++)
		device_nodes.push_back(it->second->GetPARNode());
	//TODO: hard IP
	
	//Add the O(n^2) edges between the main fabric nodes
	for(auto x : device_nodes)
	{
		for(auto y : device_nodes)
		{
			if(x != y)
			{
				//If the destination is a LUT, add paths to every input
				auto entity = static_cast<Greenpak4BitstreamEntity*>(y->GetData());
				auto lut = dynamic_cast<Greenpak4LUT*>(entity);
				if(lut)
				{
					x->AddEdge(y, "IN0");
					x->AddEdge(y, "IN1");
					if(lut->GetOrder() > 2)
						x->AddEdge(y, "IN2");
					if(lut->GetOrder() > 3)
						x->AddEdge(y, "IN3");
				}
				
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
void CommitChanges(PARGraph* /*netlist*/, PARGraph* device)
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
			
		//Configure nodes of known type
		if(iob)
			CommitIOBChanges(static_cast<Greenpak4NetlistPort*>(mate->GetData()), iob);		
		else if(lut)
			CommitLUTChanges(static_cast<Greenpak4NetlistCell*>(mate->GetData()), lut);
		
		//No idea what it is
		else
		{
			printf("WARNING: Node at config base %d has unrecognized entity type\n", bnode->GetConfigBase());
		}
	}
	
	//Done configuring all of the nodes!
	//Configure routes between them
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
			
			//TODO: Cross connections
			if(src->GetMatrix() != dst->GetMatrix())
			{
				printf("WARNING: Ignoring cross-matrix connection (not yet implemented)\n");
				continue;
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

			//Don't know what to do
			else
				printf("WARNING: Node at config base %d has unrecognized entity type\n", dst->GetConfigBase());
		}
	}
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

void ShowUsage()
{
	printf("Usage: gp4par --top TopModule --output foo.txt foo.json\n");
	printf("    --unused-pull        [down|up|float]      Specifies direction to pull unused pins\n");
	printf("    --unused-drive       [10k|100k|1m]        Specifies strength of pullup/down resistor on unused pins\n");
}

void ShowVersion()
{
	printf(
		"Greenpak4 place-and-route by Andrew D. Zonenberg.\n"
		"\n"
		"License: LGPL v2.1+\n"
		"This is free software: you are free to change and redistribute it.\n"
		"There is NO WARRANTY, to the extent permitted by law.\n");
}
