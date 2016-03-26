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

void MakeIOBNodes(
	Greenpak4NetlistModule* module,
	Greenpak4Device* device,
	PARGraph*& ngraph,
	PARGraph*& dgraph,
	labelmap& lmap,
	uint32_t iob_label
	);

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
	MakeIOBNodes(module, device, ngraph, dgraph, lmap, iob_label);
	
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
	@brief Make netlist nodes for the IOBs
 */
void MakeIOBNodes(
	Greenpak4NetlistModule* module,
	Greenpak4Device* device,
	PARGraph*& ngraph,
	PARGraph*& dgraph,
	labelmap& lmap,
	uint32_t iob_label
	)
{
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
}
