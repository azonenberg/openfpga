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

int main(int argc, char* argv[])
{
	//Netlist file
	string fname = "";
	
	//Top-level module name
	string top = "";
	
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
		
		//assume it's the netlist file if it'[s the first non-switch argument
		else if( (s[0] != '-') && (fname == "") )
			fname = s;
			
		else
		{
			printf("Unrecognized command-line argument \"%s\", use --help\n", s.c_str());
			return 1;
		}
	}
	
	//Netlist filename must be specified
	if(fname == "")
	{
		ShowUsage();
		return 1;
	}
	
	//Print header
	ShowVersion();
	printf("Loading Yosys JSON file \"%s\", expecting top-level module \"%s\"\n",
		fname.c_str(), top.c_str());
	
	//Parse the unplaced netlist
	Greenpak4Netlist netlist(fname, top);
	
	//Create the device
	Greenpak4Device device(part);
	
	return 0;
}

void ShowUsage()
{
	
}

void ShowVersion()
{
	printf(
		"Greenpak4 place-and-route by Andrew D. Zonenberg.\n"
		"\n"
		"License: LGPL v2.1+\n"
		"This is free software: you are free to change and redistribute it.\n"
		"There is NO WARRANTY, to the extent permitted by law.\n"
		"\n");
}
