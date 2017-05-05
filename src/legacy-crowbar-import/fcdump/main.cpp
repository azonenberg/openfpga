/***********************************************************************************************************************
*                                                                                                                      *
* ANTIKERNEL v0.1                                                                                                      *
*                                                                                                                      *
* Copyright (c) 2012-2016 Andrew D. Zonenberg                                                                          *
* All rights reserved.                                                                                                 *
*                                                                                                                      *
* Redistribution and use in source and binary forms, with or without modification, are permitted provided that the     *
* following conditions are met:                                                                                        *
*                                                                                                                      *
*    * Redistributions of source code must retain the above copyright notice, this list of conditions, and the         *
*      following disclaimer.                                                                                           *
*                                                                                                                      *
*    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the       *
*      following disclaimer in the documentation and/or other materials provided with the distribution.                *
*                                                                                                                      *
*    * Neither the name of the author nor the names of any contributors may be used to endorse or promote products     *
*      derived from this software without specific prior written permission.                                           *
*                                                                                                                      *
* THIS SOFTWARE IS PROVIDED BY THE AUTHORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED   *
* TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL *
* THE AUTHORS BE HELD LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES        *
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR       *
* BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT *
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE       *
* POSSIBILITY OF SUCH DAMAGE.                                                                                          *
*                                                                                                                      *
***********************************************************************************************************************/

/**
	@file
	@author Andrew D. Zonenberg
	@brief Entry point for fcdump
 */

#include "../crowbar/crowbar.h"
#include <svnversion.h>

using namespace std;

void ShowUsage();
void ShowVersion();

int main(int argc, char* argv[])
{
	try
	{
		bool nobanner = false;
		
		//Parse command-line arguments
		std::string devname;
		std::string bitstream;
		std::string output;
		bool show_dump = true;
		for(int i=1; i<argc; i++)
		{
			string s(argv[i]);
			
			if(s == "--help")
			{
				ShowUsage();
				return 0;
			}
			else if(s == "--nobanner")
				nobanner = true;
			else if(s == "--device")
				devname = argv[++i];
			else if(s == "--bitstream")
				bitstream = argv[++i];
			else if(s == "--output")
				output = argv[++i];
			else if(s == "--nodump")
				show_dump = false;
			else if(s == "--version")
			{
				ShowVersion();
				return 0;
			}
			else
			{
				printf("Unrecognized command-line argument \"%s\", use --help\n", s.c_str());
				return 1;
			}
		}
		
		//Complain if no device name specified
		if(devname == "")
		{
			ShowUsage();
			return 0;
		}
		
		//Print version number by default
		if(!nobanner)
			ShowVersion();
			
		//Create the device
		FCDevice* dev = FCDevice::CreateDevice(devname);
		
		//If bitstream specified, load it
		if(!bitstream.empty())
			dev->LoadFromBitstream(bitstream);
		
		//Print the output
		if(show_dump)
		{
			dev->Dump();
			dev->DumpRTL();
		}
			
		//If bitstream specified, save it
		if(!output.empty())
			dev->SaveToBitstream(output);
		
		//Clean up
		delete dev;
	}
	catch(const JtagException& ex)
	{
		printf("%s\n", ex.GetDescription().c_str());
		return 1;
	}

	//Done
	return 0;
}


/**
	@brief Prints program usage
	
	\ingroup jtagclient
 */
void ShowUsage()
{
	printf(
		"Usage: fcdump --device (device name) --bitstream file\n"
		);
}

/**
	@brief Prints program version number
	
	\ingroup jtagclient
 */
void ShowVersion()
{
	printf(
		"Flying Crowbar Dump Utility [SVN rev %s] by Andrew D. Zonenberg.\n"
		"\n"
		"License: 3-clause (\"new\" or \"modified\") BSD.\n"
		"This is free software: you are free to change and redistribute it.\n"
		"There is NO WARRANTY, to the extent permitted by law.\n"
		"\n"
		, SVNVERSION);
}
