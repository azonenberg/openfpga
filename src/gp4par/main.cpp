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
#include <stdio.h>
#include <string>

using namespace std;

void ShowUsage();
void ShowVersion();
void DoPAR(Greenpak4Netlist* netlist, Greenpak4Device* device);
PARGraph* BuildNetlistGraph(Greenpak4Netlist* netlist);
PARGraph* BuildDeviceGraph(Greenpak4Device* device);

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
	DoPAR(netlist, device);
	
	//Write the final bitstream
	printf("\nWriting final bitstream to output file \"%s\"\n", ofname.c_str());
	device.WriteToFile(ofname);	
	
	//TODO: Static timing analysis
	printf("\nStatic timing analysis: not yet implemented\n");
	
	return 0;
}

void DoPAR(Greenpak4Netlist* netlist, Greenpak4Device* device)
{
	
}

/**
	@brief
 */
PARGraph* BuildNetlistGraph(Greenpak4Netlist* netlist)
{
}

/**
	@brief Make the graph for the target device
 */
PARGraph* BuildDeviceGraph(Greenpak4Device* device)
{
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
