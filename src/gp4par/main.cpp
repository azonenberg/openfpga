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
bool DoPAR(Greenpak4Netlist* netlist, Greenpak4Device* device);
void BuildGraphs(Greenpak4Netlist* netlist, Greenpak4Device* device, PARGraph*& ngraph, PARGraph*& dgraph);
void CommitChanges(PARGraph* netlist, PARGraph* device);
void ApplyLocConstraints(Greenpak4Netlist* netlist, PARGraph* ngraph, PARGraph* dgraph);
void PostPARDRC(PARGraph* netlist, PARGraph* device);

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
	
	//TODO: check 
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
		Greenpak4IOB* iob =it->second;
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
		//TODO: Support buses here (for now, assume the first one)
		PARGraphNode* netnode = new PARGraphNode(label, net->m_nodes[0]);
		net->m_nodes[0]->m_parnode = netnode;
		ngraph->AddNode(netnode);
		
		//Re-label the assigned IOB so we get a proper match to it
		iob->GetPARNode()->Relabel(label);
	}
	
	//Make device nodes for each type of LUT
	uint32_t lut2_label = ngraph->AllocateLabel();
	dgraph->AllocateLabel();
	uint32_t lut3_label = ngraph->AllocateLabel();
	dgraph->AllocateLabel();
	//uint32_t lut4_label = ngraph->AllocateLabel();
	//dgraph->AllocateLabel();
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
		
	}
	
	//Once all of the nodes are created, make all of the edges between them!
}

/**
	@brief After a successful PAR, copy all of the data from the unplaced to placed nodes
 */
void CommitChanges(PARGraph* /*netlist*/, PARGraph* /*device*/)
{
	printf("\nBuilding final post-route netlist...\n");
	
	/*
	//If we get here, everything looks good! Pair the port wth the IOB
	//TODO: support vectors
	port->m_iob = iob;
	
	//Iterate over attributes and figure out what to do
	for(auto jt : net->m_attributes)
	{
		//do nothing, only for debugging
		if(jt.first == "src")
		{}
		
		//already handled elsewhere
		else if(jt.first == "LOC")
		{}
		
		//IO schmitt trigger
		else if(jt.first == "SCHMITT_TRIGGER")
		{
			if(jt.second == "0")
				iob->SetSchmittTrigger(false);
			else
				iob->SetSchmittTrigger(true);
		}
		
		else if(jt.first == "PULLUP")
		{
			iob->SetPullDirection(Greenpak4IOB::PULL_UP);
			if(jt.second == "10k")
				iob->SetPullStrength(Greenpak4IOB::PULL_10K);
			else if(jt.second == "100k")
				iob->SetPullStrength(Greenpak4IOB::PULL_100K);
			else if(jt.second == "1M")
				iob->SetPullStrength(Greenpak4IOB::PULL_1M);
		}
		
		else if(jt.first == "PULLDOWN")
		{
			iob->SetPullDirection(Greenpak4IOB::PULL_DOWN);
			if(jt.second == "10k")
				iob->SetPullStrength(Greenpak4IOB::PULL_10K);
			else if(jt.second == "100k")
				iob->SetPullStrength(Greenpak4IOB::PULL_100K);
			else if(jt.second == "1M")
				iob->SetPullStrength(Greenpak4IOB::PULL_1M);
		}

		else
		{
			printf("WARNING: Top-level port \"%s\" has unrecognized attribute %s, ignoring\n",
				it->first.c_str(), jt.first.c_str());
		}
	}
	*/
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
