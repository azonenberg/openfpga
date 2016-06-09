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

int main(int argc, char* argv[])
{
	LogSink::Severity console_verbosity = LogSink::VERBOSE;

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
		else if(s == "-q" || s == "--quiet")
		{
			if(console_verbosity == LogSink::VERBOSE)
				console_verbosity = LogSink::WARNING;
			else if(console_verbosity == LogSink::WARNING)
				console_verbosity = LogSink::ERROR;
		}
		else if(s == "-l" || s == "--logfile" ||
		        s == "-L" || s == "--logfile-lines")
		{
			bool line_buffered = (s == "-L" || s == "--logfile-lines");
			if(i+1 < argc) {
				FILE *log = fopen(argv[++i], "wt");
				g_log_sinks.emplace_back(new FILELogSink(log, line_buffered));
			}
			else
			{
				printf("%s requires an argument\n", s.c_str());
				return 1;
			}
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

	//Set up logging
	g_log_sinks.emplace(g_log_sinks.begin(), new STDLogSink(console_verbosity));
	
	//Print header
	ShowVersion();
	
	//Print configuration
	LogNotice("\nDevice configuration:\n");
	LogNotice("    Target device: SLG46620V\n");
	LogNotice("    VCC range: not yet implemented\n");
	LogNotice("    Unused pins: ");
	switch(unused_pull)
	{
		case Greenpak4IOB::PULL_NONE:
			LogNotice("float\n");
			break;
			
		case Greenpak4IOB::PULL_DOWN:
			LogNotice("pull down with ");
			break;
			
		case Greenpak4IOB::PULL_UP:
			LogNotice("pull up with ");
			break;
			
		default:
			LogNotice("invalid\n");
			return 1;
	}
	if(unused_pull != Greenpak4IOB::PULL_NONE)
	{
		switch(unused_drive)
		{
			case Greenpak4IOB::PULL_10K:
				LogNotice("10K\n");
				break;
				
			case Greenpak4IOB::PULL_100K:
				LogNotice("100K\n");
				break;
				
			case Greenpak4IOB::PULL_1M:
				LogNotice("1M\n");
				break;
				
			default:
				LogNotice("invalid\n");
				return 1;
		}
	}
	
	
	//Parse the unplaced netlist
	LogNotice("\nLoading Yosys JSON file \"%s\", expecting top-level module \"%s\"\n",
		fname.c_str(), top.c_str());
	Greenpak4Netlist netlist(fname, top);
	
	//Create the device and initialize all IO pins
	Greenpak4Device device(part, unused_pull, unused_drive);
	
	//Do the actual P&R
	if(!DoPAR(&netlist, &device))
		return 2;
	
	//Write the final bitstream
	LogNotice("\nWriting final bitstream to output file \"%s\"\n", ofname.c_str());
	device.WriteToFile(ofname);	
	
	//TODO: Static timing analysis
	LogNotice("\nStatic timing analysis: not yet implemented\n");
	
	return 0;
}

void ShowUsage()
{
	printf(//                                                                               v 80th column
		"Usage: gp4par --top TopModule --output foo.txt foo.json\n"
		"    -q, --quiet\n"
		"        Causes only warnings and errors to be written to the console.\n"
		"        Specify twice to also silence warnings.\n"
		"    -l, --logfile        <file>\n"
		"        Causes log messages to be written to <file>."
		"    -L, --logfile-lines  <file>\n"
		"        Causes log messages to be written to <file>, flushing after each line.\n"
		"    --unused-pull        [down|up|float]\n"
		"        Specifies direction to pull unused pins.\n"
		"    --unused-drive       [10k|100k|1m]\n"
		"        Specifies strength of pullup/down resistor on unused pins.\n");
}

void ShowVersion()
{
	LogNotice(
		"Greenpak4 place-and-route by Andrew D. Zonenberg.\n"
		"\n"
		"License: LGPL v2.1+\n"
		"This is free software: you are free to change and redistribute it.\n"
		"There is NO WARRANTY, to the extent permitted by law.\n");
}
