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

void MakeNetlistEdges(Greenpak4Netlist* netlist);
void MakeDeviceEdges(Greenpak4Device* device);

void MakeNetlistNodes(
	Greenpak4Netlist* netlist,
	PARGraph*& ngraph,
	ilabelmap& ilmap);

void MakeDeviceNodes(
	Greenpak4Device* device,
	PARGraph*& ngraph,
	PARGraph*& dgraph,
	labelmap& lmap);

void MakeSingleNode(
	string type,
	Greenpak4BitstreamEntity* entity,
	PARGraph* ngraph,
	PARGraph* dgraph,
	labelmap& lmap);

PARGraphNode* MakeNode(
	uint32_t label,
	Greenpak4BitstreamEntity* entity,
	PARGraph* dgraph);

void InferExtraNodes(
	Greenpak4Netlist* netlist,
	Greenpak4Device* device,
	PARGraph*& ngraph,
	ilabelmap& ilap);

void ReplicateVREF(
	Greenpak4NetlistModule* module,
	Greenpak4NetlistCell* cell,
	Greenpak4NetlistNode* net,
	Greenpak4NetlistCell* load,
	PARGraph*& ngraph,
	ilabelmap& ilmap);

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

	//Create the device graph.
	//This is independent of the final netlist and has to be done first to assign graph labels
	MakeDeviceNodes(device, ngraph, dgraph, lmap);
	MakeDeviceEdges(device);

	//Build inverse label map
	ilabelmap ilmap;
	for(auto it : lmap)
		ilmap[it.second] = it.first;

	//Create all of the nodes for the netlist, then connect with edges.
	//This requires breaking point-to-multipoint nets into multiple point-to-point links.
	MakeNetlistNodes(netlist, ngraph, ilmap);
	MakeNetlistEdges(netlist);

	//Infer extra support nodes for things that use hidden functions of others
	InferExtraNodes(netlist, device, ngraph, ilmap);
}

/**
	@brief Replicate a voltage reference
 */
void ReplicateVREF(
	Greenpak4NetlistModule* module,
	Greenpak4NetlistCell* cell,
	Greenpak4NetlistNode* net,
	Greenpak4NetlistCell* load,
	PARGraph*& ngraph,
	ilabelmap& ilmap)
{


	//Monotonically increasing counter used to ensure unique node IDs
	static unsigned int vref_id = 1;

	//Create a new VREF and copy the input config
	Greenpak4NetlistCell* vref = new Greenpak4NetlistCell(module);
	vref->m_type = "GP_VREF";
	vref->m_connections["VIN"].push_back(cell->m_connections["VIN"][0]);
	vref->m_parameters = cell->m_parameters;
	vref->m_attributes = cell->m_attributes;

	//Give it a name
	char tmp[128];
	snprintf(tmp, sizeof(tmp), "$auto$make_graphs.cpp:%d:vref$%u",
		__LINE__,
		vref_id ++);
	vref->m_name = tmp;

	//and add it to the module
	module->AddCell(vref);

	//Create a net for the output
	snprintf(tmp, sizeof(tmp), "$auto$make_graphs.cpp:%d:vref$%u",
		__LINE__,
		vref_id ++);
	Greenpak4NetlistNode* vout = new Greenpak4NetlistNode;
	vout->m_name = tmp;
	vout->m_src_locations = net->m_src_locations;

	//and add it to the module
	module->AddNet(vout);

	//Hook up the output
	vref->m_connections["VOUT"].push_back(vout);

	//IOBs have different port naming from anything else we can drive
	if(load->IsIOB())
	{
		load->m_connections["OUT"].clear();
		load->m_connections["OUT"].push_back(vout);
	}
	else
	{
		load->m_connections["VREF"].clear();
		load->m_connections["VREF"].push_back(vout);
	}

	//Remove stale edge in the PAR graph
	cell->m_parnode->RemoveEdge("VOUT", load->m_parnode, "VREF");

	//Create the PAR node for it
	PARGraphNode* nnode = new PARGraphNode(ilmap[vref->m_type], vref);
	vref->m_parnode = nnode;
	ngraph->AddNode(nnode);

	//Copy the netlist edges to the PAR graph
	//TODO: automate this somehow? Seems error-prone to do it twice
	if(load->IsIOB())
		nnode->AddEdge("VOUT", load->m_parnode, "OUT");
	else
		nnode->AddEdge("VOUT", load->m_parnode, "VREF");
}


/**
	@brief Add extra nodes to handle dependencies between nodes that share hard IP under the hood
 */
void InferExtraNodes(
	Greenpak4Netlist* netlist,
	Greenpak4Device* device,
	PARGraph*& ngraph,
	ilabelmap& ilmap)
{
	LogVerbose("Replicating nodes to control hard IP dependencies...\n");
	LogIndenter li;

	bool madeChanges = false;

	//Cache power rails, as they're frequently used
	auto top = netlist->GetTopModule();
	auto vdd = top->GetNet("GP_VDD");
	auto vddn = device->GetPowerRail(true)->GetPARNode();

	//Look for IOBs driven by GP_VREF cells
	Greenpak4NetlistModule* module = netlist->GetTopModule();
	for(auto it = module->cell_begin(); it != module->cell_end(); it ++)
	{
		//See if we're an IOB
		Greenpak4NetlistCell* cell = it->second;
		if(!cell->IsIOB())
			continue;

		//See if we're driven by a GP_VREF
		auto net = cell->m_connections["IN"][0];
		auto driver = net->m_driver;
		if(driver.IsNull())
			continue;
		if( (driver.m_cell->m_type != "GP_VREF") || (driver.m_portname != "VOUT") )
			continue;
		Greenpak4NetlistCell* vref = driver.m_cell;

		//We have an IOB driven by a VREF.
		//Check if we have a comparator driven by that VREF too.
		LogDebug("IOB \"%s\" is driven by VREF \"%s\"\n", cell->m_name.c_str(), vref->m_name.c_str());
		LogIndenter li;

		vector<Greenpak4NetlistCell*> acmps;
		for(auto jt : net->m_nodeports)
		{
			if(jt.IsNull())
				continue;
			if(jt.m_cell->m_type == "GP_ACMP")
				acmps.push_back(jt.m_cell);
		}

		//No comparator driven by this voltage.
		//We need to *create* a GP_ACMP cell that we can use to reserve our vref
		if(acmps.empty())
		{
			LogDebug("No comparator driven by this VREF, creating a dummy\n");
			madeChanges = true;

			//Monotonically increasing counter used to ensure unique node IDs
			static unsigned int acmp_id = 1;

			//Create the cell and tie its VREF to our input
			Greenpak4NetlistCell* acmp = new Greenpak4NetlistCell(module);
			acmp->m_type = "GP_ACMP";
			acmp->m_connections["VREF"].push_back(net);

			//TODO: Determine whether we actually *need* to power on the comparator
			//in order to turn on the voltage reference block.
			//GreenPAK Designer seems to require this
			acmp->m_connections["PWREN"].push_back(vdd);

			//Give it a name
			char tmp[128];
			snprintf(tmp, sizeof(tmp), "$auto$make_graphs.cpp:%d:acmp$%u",
				__LINE__,
				acmp_id ++);
			acmp->m_name = tmp;

			//Add the cell to the module
			module->AddCell(acmp);

			//Create the PAR node for it
			PARGraphNode* nnode = new PARGraphNode(ilmap[acmp->m_type], acmp);
			acmp->m_parnode = nnode;
			ngraph->AddNode(nnode);

			//Copy the netlist edges to the PAR graph
			//TODO: automate this somehow? Seems error-prone to do it twice
			vref->m_parnode->AddEdge("VOUT", nnode, "VREF");
			vddn->AddEdge("OUT", nnode, "PWREN");

			acmps.push_back(acmp);
		}

		//Error out if we have multiple ACMPs (we should have split by this point)
		//At this point, routing *should* force us to pick the correct placement for the ACMP
		if(acmps.size() > 1)
			LogFatal("Multiple ACMPs driven by one GP_VREF (should have been split by InferExtraNodes pass)\n");
	}

	//Re-index the graph if we changed it
	if(madeChanges)
	{
		LogNotice("Re-indexing graph because we inferred additional nodes..\n");
		netlist->Reindex(true);
		ngraph->IndexNodesByLabel();
		madeChanges = false;
	}

	//If one GP_VREF drives multiple GP_ACMP/GP_DAC blocks, split it
	//This must come after the IOB pass since that might infer GP_ACMPs we need to contend with
	for(auto it = module->cell_begin(); it != module->cell_end(); it ++)
	{
		//See if we're a VREF
		Greenpak4NetlistCell* cell = it->second;
		if(cell->m_type != "GP_VREF")
			continue;
		//LogDebug("vref %s\n", cell->m_name.c_str());

		//See what we drive
		auto net = cell->m_connections["VOUT"][0];
		bool found_target = false;
		for(int i=net->m_nodeports.size()-1; i>=0; i--)
		{
			//Skip anything not a comparator or DAC
			auto load = net->m_nodeports[i].m_cell;
			//LogDebug("    load %s type %s\n", load->m_name.c_str(), load->m_type.c_str());
			if( (load->m_type != "GP_ACMP") && (load->m_type != "GP_DAC") )
				continue;

			//If this is the first one, flag it but don't do anything
			if(!found_target)
			{
				found_target = true;
				continue;
			}

			//We have a second ACMP.
			//Need to create a new net and VREF in both netlists to fix it
			LogDebug("VREF %s drives multiple ACMP/DAC cells, replicating to drive %s\n",
				cell->m_name.c_str(),
				load->m_name.c_str());
			madeChanges = true;

			//Replicate it
			ReplicateVREF(module, cell, net, load, ngraph, ilmap);
		}
	}

	//Re-index the graph if we changed it
	if(madeChanges)
	{
		LogVerbose("Re-indexing graph because we inferred additional nodes..\n");
		netlist->Reindex(true);
		ngraph->IndexNodesByLabel();
		//madeChanges = false;
	}
}

/**
	@brief Make all of the nodes for the netlist graph
 */
void MakeNetlistNodes(
	Greenpak4Netlist* netlist,
	PARGraph*& ngraph,
	ilabelmap& ilmap)
{
	//Add aliases for different primitive names that map to the same node type
	ilmap["GP_DFFR"] = ilmap["GP_DFFSR"];
	ilmap["GP_DFFS"] = ilmap["GP_DFFSR"];

	ilmap["GP_DFFI"] = ilmap["GP_DFF"];
	ilmap["GP_DFFSI"] = ilmap["GP_DFFS"];
	ilmap["GP_DFFRI"] = ilmap["GP_DFFR"];
	ilmap["GP_DFFSRI"] = ilmap["GP_DFFSR"];

	//Create the actual nodes in the netlist
	Greenpak4NetlistModule* module = netlist->GetTopModule();
	for(auto it = module->cell_begin(); it != module->cell_end(); it ++)
	{
		//Figure out the type of node
		Greenpak4NetlistCell* cell = it->second;
		uint32_t label = 0;
		if(ilmap.find(cell->m_type) != ilmap.end())
			label = ilmap[cell->m_type];
		else
		{
			LogError(
				"Cell \"%s\" is of type \"%s\" which is not a valid GreenPak4 primitive\n",
				cell->m_name.c_str(), cell->m_type.c_str());
			exit(-1);
		}

		//Create a node for the cell
		PARGraphNode* nnode = new PARGraphNode(label, cell);
		cell->m_parnode = nnode;
		ngraph->AddNode(nnode);
	}
}

/**
	@brief Make all of the nodes for the device graph
 */
void MakeDeviceNodes(
	Greenpak4Device* device,
	PARGraph*& ngraph,
	PARGraph*& dgraph,
	labelmap& lmap)
{
	//Create device entries for the IOBs
	uint32_t ibuf_label = AllocateLabel(ngraph, dgraph, lmap, "GP_IBUF");
	uint32_t obuf_label = AllocateLabel(ngraph, dgraph, lmap, "GP_OBUF");
	uint32_t iobuf_label = AllocateLabel(ngraph, dgraph, lmap, "GP_IOBUF");
	for(auto it = device->iobbegin(); it != device->iobend(); it ++)
	{
		auto iob = it->second;

		//Type A (and not input-only)? Can be anything
		if( (dynamic_cast<Greenpak4IOBTypeA*>(iob) != NULL) && !iob->IsInputOnly() )
		{
			auto node = MakeNode(iobuf_label, iob, dgraph);
			node->AddAlternateLabel(obuf_label);
			node->AddAlternateLabel(ibuf_label);
		}

		//Not input only, but type B? OBUF or IBUF but can't be IOBUF
		else if(!iob->IsInputOnly())
		{
			auto node = MakeNode(obuf_label, iob, dgraph);
			node->AddAlternateLabel(ibuf_label);
		}

		//Nope, just an input
		else
			MakeNode(ibuf_label, iob, dgraph);
	}

	//Make device nodes for the inverters
	uint32_t inv_label  = AllocateLabel(ngraph, dgraph, lmap, "GP_INV");
	for(unsigned int i=0; i<device->GetInverterCount(); i++)
		MakeNode(inv_label, device->GetInverter(i), dgraph);

	//Make device nodes for each type of LUT
	uint32_t lut2_label = AllocateLabel(ngraph, dgraph, lmap, "GP_2LUT");
	uint32_t lut3_label = AllocateLabel(ngraph, dgraph, lmap, "GP_3LUT");
	uint32_t lut4_label = AllocateLabel(ngraph, dgraph, lmap, "GP_4LUT");
	for(unsigned int i=0; i<device->GetLUT2Count(); i++)
	{
		auto node = MakeNode(lut2_label, device->GetLUT2(i), dgraph);
		node->AddAlternateLabel(inv_label);
	}
	for(unsigned int i=0; i<device->GetLUT3Count(); i++)
	{
		auto node = MakeNode(lut3_label, device->GetLUT3(i), dgraph);
		node->AddAlternateLabel(lut2_label);
		node->AddAlternateLabel(inv_label);
	}
	for(unsigned int i=0; i<device->GetLUT4Count(); i++)
	{
		auto node = MakeNode(lut4_label, device->GetLUT4(i), dgraph);
		node->AddAlternateLabel(lut2_label);
		node->AddAlternateLabel(lut3_label);
		node->AddAlternateLabel(inv_label);
	}

	//Make device nodes for the shift registers
	uint32_t shreg_label  = AllocateLabel(ngraph, dgraph, lmap, "GP_SHREG");
	for(unsigned int i=0; i<device->GetShiftRegisterCount(); i++)
		MakeNode(shreg_label, device->GetShiftRegister(i), dgraph);

	//Make device nodes for the voltage references
	uint32_t vref_label  = AllocateLabel(ngraph, dgraph, lmap, "GP_VREF");
	for(unsigned int i=0; i<device->GetVrefCount(); i++)
		MakeNode(vref_label, device->GetVref(i), dgraph);

	//Make device nodes for the comparators
	uint32_t acmp_label  = AllocateLabel(ngraph, dgraph, lmap, "GP_ACMP");
	for(unsigned int i=0; i<device->GetAcmpCount(); i++)
		MakeNode(acmp_label, device->GetAcmp(i), dgraph);

	//Make device nodes for the DACs
	uint32_t dac_label  = AllocateLabel(ngraph, dgraph, lmap, "GP_DAC");
	for(unsigned int i=0; i<device->GetDACCount(); i++)
		MakeNode(dac_label, device->GetDAC(i), dgraph);

	//Make device nodes for each type of flipflop
	uint32_t dff_label = AllocateLabel(ngraph, dgraph, lmap, "GP_DFF");
	uint32_t dffsr_label = AllocateLabel(ngraph, dgraph, lmap, "GP_DFFSR");
	for(unsigned int i=0; i<device->GetTotalFFCount(); i++)
	{
		Greenpak4Flipflop* flop = device->GetFlipflopByIndex(i);
		if(flop->HasSetReset())
		{
			auto node = MakeNode(dffsr_label, flop, dgraph);

			//It's legal to map a DFF to a DFFSR site, so add that as an alternate
			node->AddAlternateLabel(dff_label);
		}
		else
			MakeNode(dff_label, flop, dgraph);
	}

	//Make device nodes for all of the single-instance cells
	MakeSingleNode("GP_ABUF",		device->GetAbuf(), ngraph, dgraph, lmap);
	MakeSingleNode("GP_BANDGAP",	device->GetBandgap(), ngraph, dgraph, lmap);
	MakeSingleNode("GP_LFOSC",		device->GetLFOscillator(), ngraph, dgraph, lmap);
	MakeSingleNode("GP_PGA",		device->GetPGA(), ngraph, dgraph, lmap);
	MakeSingleNode("GP_POR",		device->GetPowerOnReset(), ngraph, dgraph, lmap);
	MakeSingleNode("GP_RCOSC",		device->GetRCOscillator(), ngraph, dgraph, lmap);
	MakeSingleNode("GP_RINGOSC",	device->GetRingOscillator(), ngraph, dgraph, lmap);
	MakeSingleNode("GP_SYSRESET",	device->GetSystemReset(), ngraph, dgraph, lmap);

	//Make device nodes for the power rails
	MakeSingleNode("GP_VDD",	device->GetPowerRail(true), ngraph, dgraph, lmap);
	MakeSingleNode("GP_VSS",	device->GetPowerRail(false), ngraph, dgraph, lmap);

	//Make device nodes for the counters
	uint32_t count8_label = AllocateLabel(ngraph, dgraph, lmap, "GP_COUNT8");
	uint32_t count8_adv_label = AllocateLabel(ngraph, dgraph, lmap, "GP_COUNT8_ADV");
	uint32_t count14_label = AllocateLabel(ngraph, dgraph, lmap, "GP_COUNT14");
	uint32_t count14_adv_label = AllocateLabel(ngraph, dgraph, lmap, "GP_COUNT14_ADV");
	for(unsigned int i=0; i<device->GetCounterCount(); i++)
	{
		auto counter = device->GetCounter(i);

		//Decide on primary label
		if(counter->GetDepth() == 14)
		{
			if(counter->HasFSM()) {
				auto node = MakeNode(count14_adv_label, counter, dgraph);

				//It's legal to map a COUNT8 or a COUNT14 to a COUNT14_ADV site, so add that as an alternate.
				//It's not legal to map a COUNT8_ADV to a COUNT14_ADV site because they have different behavior
				//when counting up.
				node->AddAlternateLabel(count8_label);
				node->AddAlternateLabel(count14_label);
			} else {
				auto node = MakeNode(count14_label, counter, dgraph);

				//It's legal to map a COUNT8 to a COUNT14 site, so add that as an alternate
				node->AddAlternateLabel(count8_label);
			}
		}
		else {
			if(counter->HasFSM()) {
				auto node = MakeNode(count8_adv_label, counter, dgraph);

				//It's legal to map a COUNT8 to a COUNT8_ADV site, so add that as an alternate.
				node->AddAlternateLabel(count8_label);
			} else {
				MakeNode(count8_label, counter, dgraph);
			}
		}
	}

	//TODO: make nodes for all of the other hard IP
}

/**
	@brief Make all of the edges in the netlist
 */
void MakeNetlistEdges(Greenpak4Netlist* netlist)
{
	LogDebug("Creating PAR netlist...\n");
	LogIndenter li;

	for(auto it = netlist->nodebegin(); it != netlist->nodeend(); it ++)
	{
		Greenpak4NetlistNode* node = *it;

		LogDebug("Node %s is sourced by:\n", node->m_name.c_str());
		LogIndenter li;

		PARGraphNode* source = NULL;
		string sourceport = "";

		//Nets sourced by port are special - no edges
		bool sourced_by_port = false;
		for(auto p : node->m_ports)
		{
			if(p->m_direction != Greenpak4NetlistPort::DIR_OUTPUT)
			{
				//Greenpak4NetlistNet* net = netlist->GetTopModule()->GetNet(p->m_name);
				//LogVerbose("        port %s (loc %s)\n", p->m_name.c_str(), net->m_attributes["LOC"].c_str());
				sourced_by_port = true;
			}
		}

		//See if it was sourced by a node
		for(auto c : node->m_nodeports)
		{
			Greenpak4NetlistModule* module = netlist->GetModule(c.m_cell->m_type);
			Greenpak4NetlistPort* port = module->GetPort(c.m_portname);

			if(port->m_direction == Greenpak4NetlistPort::DIR_INPUT)
				continue;

			source = c.m_cell->m_parnode;
			sourceport = c.m_portname;
			LogDebug("cell %s port %s\n", c.m_cell->m_name.c_str(), c.m_portname.c_str());

			node->m_driver = c;

			//TODO: detect multiple drivers and complain
			break;
		}

		if((source == NULL) && !sourced_by_port)
			LogDebug("[NULL]\n");
		LogDebug("and drives\n");

		//If node is sourced by a port, special processing needed.
		//We can only drive IBUF/IOBUF cells
		bool has_loads = false;
		if(sourced_by_port)
		{
			if(node->m_ports.size() != 1)
			{
				LogError(
					"Net \"%s\" is connected directly to multiple top-level ports (need an IOB)\n",
					node->m_name.c_str());
				exit(-1);
			}

			for(auto c : node->m_nodeports)
			{
				//Don't add edges to ourself (happens with inouts etc)
				if( (source == c.m_cell->m_parnode) && (sourceport == c.m_portname) )
					continue;

				has_loads = true;
				LogDebug("cell %s port %s\n", c.m_cell->m_name.c_str(), c.m_portname.c_str());

				//Verify the type is IBUF/IOBUF
				if( (c.m_cell->m_type == "GP_IBUF") || (c.m_cell->m_type == "GP_IOBUF") )
					continue;

				LogError(
					"Net \"%s\" directly drives cell %s port %s (type %s, should be IOB)\n",
					node->m_name.c_str(),
					c.m_cell->m_name.c_str(),
					c.m_portname.c_str(),
					c.m_cell->m_type.c_str()
					);
				exit(-1);
			}
		}

		//Create edges from this source node to all sink nodes
		else
		{
			for(auto c : node->m_nodeports)
			{
				Greenpak4NetlistModule* module = netlist->GetModule(c.m_cell->m_type);
				Greenpak4NetlistPort* port = module->GetPort(c.m_portname);

				//Don't add edges to ourself (happens with inouts etc)
				if( (source == c.m_cell->m_parnode) && (sourceport == c.m_portname) )
					continue;

				if(port->m_direction == Greenpak4NetlistPort::DIR_OUTPUT)
					continue;

				//Name the net
				string nname = c.m_portname;
				if(c.m_vector)
				{
					char tmp[256];
					snprintf(tmp, sizeof(tmp), "%s[%u]", c.m_portname.c_str(), c.m_nbit);
					nname = tmp;
				}

				//Use the new name
				has_loads = true;
				LogDebug("cell %s port %s\n", c.m_cell->m_name.c_str(), nname.c_str());
				if(source)
					source->AddEdge(sourceport, c.m_cell->m_parnode, nname);
			}
		}

		//DRC fail if undriven net.
		//BUGFIX: undriven nets are legal if they also have no loads.
		//This is possible if, for example, some bits of a vector net were absorbed into hard IP.
		if( (source == NULL) && !sourced_by_port && has_loads)
		{
			LogError(
				"Net \"%s\" has loads, but no driver\n",
				node->m_name.c_str());
			exit(-1);
		}
		else if(!has_loads)
			LogDebug("[NULL]\n");
	}
}

/**
	@brief Make a PAR graph node and type for for a given bitstream entity, given that the device only has one of them
 */
void MakeSingleNode(
	string type,
	Greenpak4BitstreamEntity* entity,
	PARGraph* ngraph,
	PARGraph* dgraph,
	labelmap& lmap)
{
	uint32_t label = AllocateLabel(ngraph, dgraph, lmap, type);
	MakeNode(label, entity, dgraph);
}

/**
	@brief Make a PAR graph node and type for for a given bitstream entity
 */
PARGraphNode* MakeNode(
	uint32_t label,
	Greenpak4BitstreamEntity* entity,
	PARGraph* dgraph)
{
	PARGraphNode* node = new PARGraphNode(label, entity);
	entity->SetPARNode(node);
	dgraph->AddNode(node);
	return node;
}

/**
	@brief Make all of the edges for the device graph (list of all possible connections)

	TODO: Should this be in the Greenpak4Device class?
 */
void MakeDeviceEdges(Greenpak4Device* device)
{
	//Get all of the nodes in the device
	vector<PARGraphNode*> device_nodes;
	for(unsigned int i=0; i<device->GetEntityCount(); i++)
	{
		PARGraphNode* pnode = device->GetEntity(i)->GetPARNode();
		if(pnode)
			device_nodes.push_back(pnode);
	}

	//Add the O(n^2) edges between the main fabric nodes
	for(auto x : device_nodes)
	{
		auto oports = static_cast<Greenpak4BitstreamEntity*>(x->GetData())->GetOutputPorts();
		for(auto srcport : oports)
		{
			for(auto y : device_nodes)
			{
				//Add paths to each cell input
				auto iports = static_cast<Greenpak4BitstreamEntity*>(y->GetData())->GetInputPorts();
				for(auto ip : iports)
					x->AddEdge(srcport, y, ip);
			}
		}
	}

	//Add dedicated routing between hard IP
	if(device->GetPart() == Greenpak4Device::GREENPAK4_SLG46620)
	{
		auto lfosc = device->GetLFOscillator()->GetPARNode();
		auto rosc = device->GetRingOscillator()->GetPARNode();
		auto rcosc = device->GetRCOscillator()->GetPARNode();

		//TODO: Disable clock outputs to dedicated routing in matrix 1 if SPI slave is enabled?

		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Cache some commonly used stuff

		auto pin2 = device->GetIOB(2)->GetPARNode();
		auto pin3 = device->GetIOB(3)->GetPARNode();
		auto pin4 = device->GetIOB(4)->GetPARNode();
		auto pin6 = device->GetIOB(6)->GetPARNode();
		auto pin7 = device->GetIOB(7)->GetPARNode();
		auto pin8 = device->GetIOB(8)->GetPARNode();
		auto pin9 = device->GetIOB(9)->GetPARNode();
		auto pin12 = device->GetIOB(12)->GetPARNode();
		auto pin13 = device->GetIOB(13)->GetPARNode();
		auto pin15 = device->GetIOB(15)->GetPARNode();
		auto pin16 = device->GetIOB(16)->GetPARNode();
		auto pin18 = device->GetIOB(18)->GetPARNode();
		auto pin19 = device->GetIOB(19)->GetPARNode();

		auto vdd = device->GetPowerRail(true)->GetPARNode();
		auto gnd = device->GetPowerRail(false)->GetPARNode();

		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// CLOCK INPUTS TO COUNTERS

		PARGraphNode* cnodes[] =
		{
			device->GetCounter(0)->GetPARNode(),
			device->GetCounter(1)->GetPARNode(),
			device->GetCounter(2)->GetPARNode(),
			device->GetCounter(3)->GetPARNode(),
			device->GetCounter(4)->GetPARNode(),
			device->GetCounter(5)->GetPARNode(),
			device->GetCounter(6)->GetPARNode(),
			device->GetCounter(7)->GetPARNode(),
			device->GetCounter(8)->GetPARNode(),
			device->GetCounter(9)->GetPARNode()
		};

		//TODO: other clock sources
		lfosc->AddEdge("CLKOUT", cnodes[0], "CLK");
		rosc->AddEdge("CLKOUT_HARDIP", cnodes[0], "CLK");
		rcosc->AddEdge("CLKOUT_HARDIP", cnodes[0], "CLK");

		//TODO: other clock sources
		lfosc->AddEdge("CLKOUT", cnodes[1], "CLK");
		rosc->AddEdge("CLKOUT_HARDIP", cnodes[1], "CLK");
		rcosc->AddEdge("CLKOUT_HARDIP", cnodes[1], "CLK");

		//TODO: other clock sources
		lfosc->AddEdge("CLKOUT", cnodes[2], "CLK");
		rosc->AddEdge("CLKOUT_HARDIP", cnodes[2], "CLK");
		rcosc->AddEdge("CLKOUT_HARDIP", cnodes[2], "CLK");

		//TODO: other clock sources
		lfosc->AddEdge("CLKOUT", cnodes[3], "CLK");
		rosc->AddEdge("CLKOUT_HARDIP", cnodes[3], "CLK");
		rcosc->AddEdge("CLKOUT_HARDIP", cnodes[3], "CLK");

		//TODO: other clock sources
		lfosc->AddEdge("CLKOUT", cnodes[4], "CLK");
		rosc->AddEdge("CLKOUT_HARDIP", cnodes[4], "CLK");
		rcosc->AddEdge("CLKOUT_HARDIP", cnodes[4], "CLK");

		//TODO: other clock sources
		lfosc->AddEdge("CLKOUT", cnodes[5], "CLK");
		rosc->AddEdge("CLKOUT_HARDIP", cnodes[5], "CLK");
		rcosc->AddEdge("CLKOUT_HARDIP", cnodes[5], "CLK");

		//TODO: other clock sources
		lfosc->AddEdge("CLKOUT", cnodes[6], "CLK");
		rosc->AddEdge("CLKOUT_HARDIP", cnodes[6], "CLK");
		rcosc->AddEdge("CLKOUT_HARDIP", cnodes[6], "CLK");

		//TODO: other clock sources
		lfosc->AddEdge("CLKOUT", cnodes[7], "CLK");
		rosc->AddEdge("CLKOUT_HARDIP", cnodes[7], "CLK");
		rcosc->AddEdge("CLKOUT_HARDIP", cnodes[7], "CLK");

		//TODO: other clock sources
		lfosc->AddEdge("CLKOUT", cnodes[8], "CLK");
		rosc->AddEdge("CLKOUT_HARDIP", cnodes[8], "CLK");
		rcosc->AddEdge("CLKOUT_HARDIP", cnodes[8], "CLK");

		//TODO: other clock sources
		lfosc->AddEdge("CLKOUT", cnodes[9], "CLK");
		rosc->AddEdge("CLKOUT_HARDIP", cnodes[9], "CLK");
		rcosc->AddEdge("CLKOUT_HARDIP", cnodes[9], "CLK");

		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// SYSTEM RESET

		//Can drive reset with ground or pin 2 only
		auto sysrst = device->GetSystemReset()->GetPARNode();
		pin2->AddEdge("OUT", sysrst, "RST");
		gnd->AddEdge("OUT", sysrst, "RST");

		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// REFERENCE OUT

		PARGraphNode* vrefs[] =
		{
			device->GetVref(0)->GetPARNode(),
			device->GetVref(1)->GetPARNode(),
			device->GetVref(2)->GetPARNode(),
			device->GetVref(3)->GetPARNode(),
			device->GetVref(4)->GetPARNode(),
			device->GetVref(5)->GetPARNode(),

			//DAC references
			device->GetVref(6)->GetPARNode(),
			device->GetVref(7)->GetPARNode()
		};

		//VREF0/1 can drive pin 19
		vrefs[0]->AddEdge("VOUT", pin19, "IN");
		vrefs[1]->AddEdge("VOUT", pin19, "IN");

		//VREF2/3 can drive pin 18
		vrefs[2]->AddEdge("VOUT", pin18, "IN");
		vrefs[3]->AddEdge("VOUT", pin18, "IN");

		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// REFERENCE TO COMPARATORS

		PARGraphNode* acmps[] =
		{
			device->GetAcmp(0)->GetPARNode(),
			device->GetAcmp(1)->GetPARNode(),
			device->GetAcmp(2)->GetPARNode(),
			device->GetAcmp(3)->GetPARNode(),
			device->GetAcmp(4)->GetPARNode(),
			device->GetAcmp(5)->GetPARNode()
		};

		/*
		//Any vref can drive any comparator, we hide the complexity of the actual routing structure
		//TODO: add a 6th vref for the DACs?
		for(auto acmp : acmps)
		{
			for(auto vref : vrefs)
				vref->AddEdge("VOUT", acmp, "VREF");
		}
		*/

		//Only allow one VREF to drive its attached comparator.
		//TODO: Add a 6th vref for DAC reference
		for(unsigned int i=0; i<6; i++)
			vrefs[i]->AddEdge("VOUT", acmps[i], "VREF");

		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// INPUTS TO COMPARATORS

		auto pga = device->GetPGA()->GetPARNode();
		auto abuf = device->GetAbuf()->GetPARNode();

		//Input to buffer
		pin6->AddEdge("OUT", abuf, "IN");

		//Dedicated inputs for ACMP0 (none)

		//Dedicated inputs for acmps[1]
		pin12->AddEdge("OUT", acmps[1], "VIN");
		pga->AddEdge("VOUT", acmps[1], "VIN");

		//Dedicated inputs for acmps[2]
		pin13->AddEdge("OUT", acmps[2], "VIN");

		//Dedicated inputs for acmps[3]
		pin15->AddEdge("OUT", acmps[3], "VIN");
		pin13->AddEdge("OUT", acmps[3], "VIN");

		//Dedicated inputs for acmps[4]
		pin3->AddEdge("OUT", acmps[4], "VIN");
		pin15->AddEdge("OUT", acmps[4], "VIN");

		//Dedicated inputs for acmps[5]
		pin4->AddEdge("OUT", acmps[5], "VIN");

		//acmps[0] input before gain stage is fed to everything but acmps[5]
		pin6->AddEdge("OUT", acmps[0], "VIN");
		vdd->AddEdge("OUT", acmps[0], "VIN");
		abuf->AddEdge("OUT", acmps[0], "VIN");

		pin6->AddEdge("OUT", acmps[1], "VIN");
		vdd->AddEdge("OUT", acmps[1], "VIN");
		abuf->AddEdge("OUT", acmps[1], "VIN");

		pin6->AddEdge("OUT", acmps[2], "VIN");
		vdd->AddEdge("OUT", acmps[2], "VIN");
		abuf->AddEdge("OUT", acmps[2], "VIN");

		pin6->AddEdge("OUT", acmps[3], "VIN");
		vdd->AddEdge("OUT", acmps[3], "VIN");
		abuf->AddEdge("OUT", acmps[3], "VIN");

		pin6->AddEdge("OUT", acmps[4], "VIN");
		vdd->AddEdge("OUT", acmps[4], "VIN");
		abuf->AddEdge("OUT", acmps[4], "VIN");

		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// INPUTS TO PGA

		vdd->AddEdge("OUT", pga, "VIN_P");
		pin8->AddEdge("OUT", pga, "VIN_P");

		pin9->AddEdge("OUT", pga, "VIN_N");
		gnd->AddEdge("OUT", pga, "VIN_N");
		//TODO: DAC output

		pin16->AddEdge("OUT", pga, "VIN_SEL");
		vdd->AddEdge("OUT", pga, "VIN_SEL");

		//TODO: Output to ADC

		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// PGA to IOB

		pga->AddEdge("VOUT", pin7, "IN");

		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// INPUTS TO DAC

		PARGraphNode* dacs[] =
		{
			device->GetDAC(0)->GetPARNode(),
			device->GetDAC(1)->GetPARNode()
		};

		//DAC voltage references driving DAC inputs
		vrefs[6]->AddEdge("VOUT", dacs[0], "VREF");
		vrefs[7]->AddEdge("VOUT", dacs[1], "VREF");

		//Static 1/0 for register configuration
		for(size_t i=0; i<device->GetDACCount(); i++)
		{
			auto dac = device->GetDAC(i)->GetPARNode();

			vdd->AddEdge("OUT", dac, "DIN[0]");
			vdd->AddEdge("OUT", dac, "DIN[1]");
			vdd->AddEdge("OUT", dac, "DIN[2]");
			vdd->AddEdge("OUT", dac, "DIN[3]");
			vdd->AddEdge("OUT", dac, "DIN[4]");
			vdd->AddEdge("OUT", dac, "DIN[5]");
			vdd->AddEdge("OUT", dac, "DIN[6]");
			vdd->AddEdge("OUT", dac, "DIN[7]");

			gnd->AddEdge("OUT", dac, "DIN[0]");
			gnd->AddEdge("OUT", dac, "DIN[1]");
			gnd->AddEdge("OUT", dac, "DIN[2]");
			gnd->AddEdge("OUT", dac, "DIN[3]");
			gnd->AddEdge("OUT", dac, "DIN[4]");
			gnd->AddEdge("OUT", dac, "DIN[5]");
			gnd->AddEdge("OUT", dac, "DIN[6]");
			gnd->AddEdge("OUT", dac, "DIN[7]");
		}

		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// OUTPUTS FROM DAC

		// Both DACs can drive to every comparator vref
		for(int i=0; i<2; i++)
		{
			for(int j=0; j<6; j++)
				dacs[i]->AddEdge("VOUT", vrefs[j], "VIN");
		}

		//DACs can drive I/O pins directly without going through a GP_VREF
		dacs[0]->AddEdge("VOUT", pin19, "IN");
		dacs[1]->AddEdge("VOUT", pin18, "IN");
	}
}
