/***********************************************************************************************************************
 * Copyright (C) 2016-2017 Andrew Zonenberg and contributors                                                           *
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

#include "gpkjson.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

using namespace std;

int main(int argc, char* argv[])
{
#ifdef __EMSCRIPTEN__
	EM_ASM(
		if (ENVIRONMENT_IS_NODE) {
			FS.mkdir('/x');
			FS.mount(NODEFS, { root: '.' }, '/x');
		}
	);
#endif
	
	Severity console_verbosity = Severity::NOTICE;

	//Netlist file
	string fname = "";

	//Output file
	string ofname = "";

	//Disables colored output
	bool noColors = false;

	//Target chip
	Greenpak4Device::GREENPAK4_PART part = Greenpak4Device::GREENPAK4_SLG46620;

	//Parse command-line arguments
	for(int i=1; i<argc; i++)
	{
		string s(argv[i]);

		//Let the logger eat its args first
		if(ParseLoggerArguments(i, argc, argv, console_verbosity))
			continue;

		else if(s == "--help")
		{
			ShowUsage();
			return 0;
		}
		else if(s == "--version")
		{
			ShowVersion();
			return 0;
		}
		else if( (s == "--part") || (s == "-p") )
		{
			if(i+1 < argc)
			{
				int p;
				sscanf(argv[++i], "SLG%d", &p);

				switch(p)
				{
					case 46620:
						part = Greenpak4Device::GREENPAK4_SLG46620;
						break;

					case 46621:
						part = Greenpak4Device::GREENPAK4_SLG46621;
						break;

					case 46140:
						part = Greenpak4Device::GREENPAK4_SLG46140;
						break;

					default:
						printf("ERROR: Invalid part (supported: SLG46620, SLG46621, SLG46140)\n");
						return 1;
				}
			}
			else
			{
				printf("ERROR: --part requires an argument\n");
				return 1;
			}
		}
		else if(s == "--nocolors")
			noColors = true;
		else if(s == "-o" || s == "--output")
		{
			if(i+1 < argc)
				ofname = argv[++i];
			else
			{
				printf("ERROR: --output requires an argument\n");
				return 1;
			}
		}

		//assume it's the netlist file if it's the first non-switch argument
		else if( (s[0] != '-') && (fname == "") )
			fname = s;

		else
		{
			printf("ERROR: Unrecognized command-line argument \"%s\", use --help\n", s.c_str());
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
	if(noColors)
		g_log_sinks.emplace(g_log_sinks.begin(), new STDLogSink(console_verbosity));
	else
		g_log_sinks.emplace(g_log_sinks.begin(), new ColoredSTDLogSink(console_verbosity));

	//Print header
	if(console_verbosity >= Severity::NOTICE)
		ShowVersion();

	//Initialize the device
	Greenpak4Device device(part);
	if(!device.ReadFromFile(fname))
		return 1;

	//Print configuration
	LogNotice("\nDevice configuration:\n");
	{
		LogIndenter li;

		string dev = "<invalid>";
		switch(device.GetPart())
		{
			case Greenpak4Device::GREENPAK4_SLG46620:
				dev = "SLG46620V";
				break;

			case Greenpak4Device::GREENPAK4_SLG46621:
				dev = "SLG46621V";
				break;

			case Greenpak4Device::GREENPAK4_SLG46140:
				dev = "SLG46140V";
				break;
		}

		LogNotice("Target device:   %s\n", dev.c_str());
		LogNotice("VCC range:       not yet implemented\n");

		/*
		LogNotice("User ID code:    %02x\n", userid);
		LogNotice("Read protection: %s\n", readProtect ? "enabled" : "disabled");
		LogNotice("I/O precharge:   %s\n", ioPrecharge ? "enabled" : "disabled");
		LogNotice("Charge pump:     %s\n", disableChargePump ? "off" : "auto");
		LogNotice("LDO:             %s\n", ldoBypass ? "bypassed" : "enabled");
		LogNotice("Boot retry:      %d times\n", bootRetry);
		*/
	}

	//Write the final bitstream
	LogNotice("\nWriting final bitstream to output file \"%s\"\n", ofname.c_str());
	{
		LogIndenter li;
		if(!device.WriteToJSON(ofname, "Bitstream"))
			return 1;
	}
	return 0;
}

void ShowUsage()
{
	/*
	printf(//                                                                               v 80th column
		"Usage: gp4par [options] -p part -o bitstream.txt netlist.json\n"
		"    -c, --constraints <file>\n"
		"        Reads placement constraints from <file>\n"
		"    --debug\n"
		"        Prints lots of internal debugging information.\n"
		"    --disable-charge-pump\n"
		"        Disables the on-die charge pump which powers the analog hard IP when the\n"
		"        supply voltage drops below 2.7V. Provided for completeness since the\n"
		"        Silego GUI lets you specify it; there's no obvious reason to use it.\n"
		"    --io-precharge\n"
		"        Hooks a 2K resistor in parallel with pullup/down resistors during POR.\n"
		"        This can help external capacitive loads to reach a stable voltage faster.\n"
		"    -l, --logfile        <file>\n"
		"        Causes verbose log messages to be written to <file>.\n"
		"    -L, --logfile-lines  <file>\n"
		"        Causes verbose log messages to be written to <file>, flushing after\n"
		"        each line.\n"
		"    --ldo-bypass\n"
		"        Disable the on-die LDO and use an external 1.8V Vdd as Vcore.\n"
		"        May cause device damage if set with higher Vdd supply.\n"
		"    -o, --output         <bitstream>\n"
		"        Writes bitstream into the specified file.\n"
		"    -p, --part\n"
		"        Specifies the part to target (SLG46620V, SLG46621V, or SLG46140V)\n"
		"    -q, --quiet\n"
		"        Causes only warnings and errors to be written to the console.\n"
		"        Specify twice to also silence warnings.\n"
		"    --unused-pull        [down|up|float]\n"
		"        Specifies direction to pull unused pins.\n"
		"    --unused-drive       [10k|100k|1m]\n"
		"        Specifies strength of pullup/down resistor on unused pins.\n"
		"    --verbose\n"
		"        Prints additional information about the design.\n");
	*/
}

void ShowVersion()
{
	printf(
		"GreenPAK JSON netlist exporter by Andrew D. Zonenberg.\n"
		"\n"
		"License: LGPL v2.1+\n"
		"This is free software: you are free to change and redistribute it.\n"
		"There is NO WARRANTY, to the extent permitted by law.\n");
}
