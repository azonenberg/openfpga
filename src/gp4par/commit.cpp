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
		auto count = dynamic_cast<Greenpak4Counter*>(bnode);
		auto rst = dynamic_cast<Greenpak4SystemReset*>(bnode);
			
		//Configure nodes of known type
		if(iob)
			CommitIOBChanges(static_cast<Greenpak4NetlistPort*>(mate->GetData()), iob);		
		else if(lut)
			CommitLUTChanges(static_cast<Greenpak4NetlistCell*>(mate->GetData()), lut);
		else if(ff)
			CommitFFChanges(static_cast<Greenpak4NetlistCell*>(mate->GetData()), ff);
		else if(lfosc)
			CommitLFOscChanges(static_cast<Greenpak4NetlistCell*>(mate->GetData()), lfosc);
		else if(count)
			CommitCounterChanges(static_cast<Greenpak4NetlistCell*>(mate->GetData()), count);
		else if(rst)
			CommitResetChanges(static_cast<Greenpak4NetlistCell*>(mate->GetData()), rst);
			
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
		ff->SetSRMode(ncell->m_parameters["SRMODE"] == "1");

	if(ncell->HasParameter("INIT"))
		ff->SetInitValue(ncell->m_parameters["INIT"] == "1");
}

/**
	@brief Commit post-PAR results from the netlist to the low-frequency oscillator
 */
void CommitLFOscChanges(Greenpak4NetlistCell* ncell, Greenpak4LFOscillator* osc)
{
	if(ncell->HasParameter("PWRDN_EN"))
		osc->SetPowerDownEn(ncell->m_parameters["PWRDN_EN"] == "1");
		
	if(ncell->HasParameter("AUTO_PWRDN"))
		osc->SetAutoPowerDown(ncell->m_parameters["AUTO_PWRDN"] == "1");
		
	if(ncell->HasParameter("OUT_DIV"))
		osc->SetOutputDivider(atoi(ncell->m_parameters["OUT_DIV"].c_str()));
}

/**
	@brief Commit post-PAR results from the netlist to a counter
 */
void CommitCounterChanges(Greenpak4NetlistCell* ncell, Greenpak4Counter* count)
{
	if(ncell->HasParameter("RESET_MODE"))
	{
		Greenpak4Counter::ResetMode mode = Greenpak4Counter::RISING_EDGE;
		string p = ncell->m_parameters["RESET_MODE"];
		if(p == "RISING")
			mode = Greenpak4Counter::RISING_EDGE;
		else if(p == "FALLING")
			mode = Greenpak4Counter::FALLING_EDGE;
		else if(p == "BOTH")
			mode = Greenpak4Counter::BOTH_EDGE;
		else if(p == "LEVEL")
			mode = Greenpak4Counter::HIGH_LEVEL;
		else
		{
			fprintf(
				stderr,
				"ERROR: Counter \"%s\" has illegal reset mode \"%s\" (must be RISING, FALLING, BOTH, or LEVEL)\n",
				ncell->m_name.c_str(),
				p.c_str());
			exit(-1);
		}
		count->SetResetMode(mode);
	}
	
	if(ncell->HasParameter("COUNT_TO"))
		count->SetCounterValue(atoi(ncell->m_parameters["COUNT_TO"].c_str()));
	
	if(ncell->HasParameter("CLKIN_DIVIDE"))
		count->SetPreDivide(atoi(ncell->m_parameters["CLKIN_DIVIDE"].c_str()));
}

/**
	@brief Commit post-PAR results from the netlist to a counter
 */
void CommitResetChanges(Greenpak4NetlistCell* ncell, Greenpak4SystemReset* rst)
{
	if(ncell->HasParameter("RESET_MODE"))
	{
		Greenpak4SystemReset::ResetMode mode = Greenpak4SystemReset::RISING_EDGE;
		string p = ncell->m_parameters["RESET_MODE"];
		if(p == "RISING")
			mode = Greenpak4SystemReset::RISING_EDGE;
		else if(p == "FALLING")
			mode = Greenpak4SystemReset::FALLING_EDGE;
		else if(p == "LEVEL")
			mode = Greenpak4SystemReset::HIGH_LEVEL;
		else
		{
			fprintf(
				stderr,
				"ERROR: Reset \"%s\" has illegal reset mode \"%s\" (must be RISING, FALLING, or LEVEL)\n",
				ncell->m_name.c_str(),
				p.c_str());
			exit(-1);
		}
		rst->SetResetMode(mode);
	}
}

/**
	@brief Commit post-PAR results from the netlist to the routing matrix
	
	TODO: refactor a lot of this stuff to be in the BitstreamEntity derived class
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
			
			//Try casting to every primitive type known to mankind!
			auto iob = dynamic_cast<Greenpak4IOB*>(dst);
			auto lut = dynamic_cast<Greenpak4LUT*>(dst);			
			auto ff = dynamic_cast<Greenpak4Flipflop*>(dst);
			auto pwr = dynamic_cast<Greenpak4PowerRail*>(dst);
			auto spwr = dynamic_cast<Greenpak4PowerRail*>(src);
			auto dosc = dynamic_cast<Greenpak4LFOscillator*>(dst);
			auto sosc = dynamic_cast<Greenpak4LFOscillator*>(src);
			auto count = dynamic_cast<Greenpak4Counter*>(dst);
			auto rst = dynamic_cast<Greenpak4SystemReset*>(dst);
			
			//If the source node is power, patch the topology so that everything comes from the right matrix.
			//We don't want to waste cross-connections on power nets
			if(spwr)
				src = pdev->GetPowerRail(dst->GetMatrix(), spwr->GetDigitalValue());
			
			//If the source node is an oscillator, use the secondary output if needed
			else if(sosc)
			{
				if(dst->GetMatrix() != sosc->GetMatrix())
					src = sosc->GetDual();
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
				
				else
				{
					printf("WARNING: Ignoring connection to unknown FF input %s\n", edge->m_destport.c_str());
					continue;
				}
			}
			
			//Destination is the low-frequency oscillator
			else if(dosc)
			{
				if(edge->m_destport == "PWRDN")
					dosc->SetPowerDown(src);
				
				else
				{
					printf("WARNING: Ignoring connection to unknown oscillator input %s\n", edge->m_destport.c_str());
					continue;
				}
			}
			
			//Destination is a counter
			else if(count)
			{
				if(edge->m_destport == "CLK")
					count->SetClock(src);
				else if(edge->m_destport == "RST")
					count->SetReset(src);
				
				else
				{
					printf("WARNING: Ignoring connection to unknown counter input %s\n", edge->m_destport.c_str());
					continue;
				}
			}
			
			//Destination is the system reset
			else if(rst)
			{
				if(edge->m_destport == "RST")
					rst->SetReset(src);
				
				else
				{
					printf("WARNING: Ignoring connection to unknown reset input %s\n", edge->m_destport.c_str());
					continue;
				}
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
