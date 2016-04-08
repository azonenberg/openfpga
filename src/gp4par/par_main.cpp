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
	if(!engine.PlaceAndRoute(lmap, true))
	{
		//Print the placement we have so far
		PrintPlacementReport(ngraph, device);
		
		printf("PAR failed\n");
		return false;
	}
		
	//Copy the netlist over
	unsigned int num_routes_used[2];
	CommitChanges(dgraph, device, num_routes_used);
	
	//Final DRC to make sure the placement is sane
	PostPARDRC(ngraph, device);
	
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
void PostPARDRC(PARGraph* netlist, Greenpak4Device* device)
{
	printf("\nPost-PAR design rule checks\n");
		
	//Check for nodes in the netlist that have no load
	for(uint32_t i=0; i<netlist->GetNumNodes(); i++)
	{
		auto node = netlist->GetNodeByIndex(i);
		auto src = static_cast<Greenpak4NetlistEntity*>(node->GetData());
		auto mate = node->GetMate();
		auto dst = static_cast<Greenpak4BitstreamEntity*>(mate->GetData());
		
		//Sanity check - must be fully PAR'd
		if(mate == NULL)
		{
			fprintf(
				stderr,
				"    ERROR: Node \"%s\" is not mapped to any site in the device\n",
				src->m_name.c_str());
			exit(-1);
		}
		
		//Do not warn if power rails have no load, that's perfectly normal
		if(dynamic_cast<Greenpak4PowerRail*>(dst) != NULL)
			continue;
			
		//If the node has no output ports, of course it won't have any loads
		if(dst->GetOutputPorts().size() == 0)
			continue;
			
		//If the node is an IOB configured as an output, there's no internal load for its output.
		//This is perfectly normal, obviously.
		Greenpak4NetlistPort* port = dynamic_cast<Greenpak4NetlistPort*>(src);
		if( (port != NULL) && (port->m_direction == Greenpak4NetlistPort::DIR_OUTPUT) )
			continue;
		
		//If we have no loads, warn
		if(node->GetEdgeCount() == 0)
		{
			printf(
				"    WARNING: Node \"%s\" has no load\n",
				src->m_name.c_str());
		}
		
	}
	
	//TODO: check floating inputs etc
	
	//TODO: check invalid IOB configuration (driving an input-only pin etc) - is this possible?
	
	//Check for multiple oscillators with power-down enabled but not the same source
	typedef pair<string, Greenpak4BitstreamEntity*> spair;
	Greenpak4LFOscillator* lfosc = device->GetLFOscillator();
	Greenpak4RingOscillator* rosc = device->GetRingOscillator();
	vector<spair> powerdowns;
	if(lfosc->IsUsed() && lfosc->GetPowerDownEn() && !lfosc->IsConstantPowerDown())
		powerdowns.push_back(spair(lfosc->GetDescription(), lfosc->GetPowerDown()));
	if(rosc->IsUsed() && rosc->GetPowerDownEn() && !rosc->IsConstantPowerDown())
		powerdowns.push_back(spair(rosc->GetDescription(), rosc->GetPowerDown()));
	//TODO: RC oscillator
	if(!powerdowns.empty())
	{
		Greenpak4BitstreamEntity* src = NULL;
		bool ok = true;
		for(auto p : powerdowns)
		{
			if(src == NULL)
				src = p.second;
			if(src != p.second)
				ok = false;
		}
		
		if(!ok)
		{
			fprintf(stderr,
				"    FAIL: Multiple oscillators have power-down enabled, but do not share the same power-down signal\n");
			for(auto p : powerdowns)
				printf("    Oscillator %10s powerdown is %s\n", p.first.c_str(), p.second->GetOutputName().c_str());
			exit(-1);
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
