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
	Severity console_verbosity = Severity::NOTICE;

	//Netlist file
	string fname = "";

	//Output file
	string ofname = "";

	//Action to take with unused pins;
	Greenpak4IOB::PullDirection unused_pull = Greenpak4IOB::PULL_NONE;
	Greenpak4IOB::PullStrength  unused_drive = Greenpak4IOB::PULL_1M;

	//Specifies whether we should increase drive current of pullups/downs during boot to reach a stable state faster
	bool ioPrecharge = false;

	//Specifies that we should disable the on-die charge pump for the analog IP
	bool disableChargePump = false;

	//Turns off the internal LDO and connects Vdd directly to Vcore
	bool ldoBypass = false;

	//Number of times to re-try the boot process
	int bootRetry = 1;

	//Target chip
	Greenpak4Device::GREENPAK4_PART part = Greenpak4Device::GREENPAK4_SLG46620;

	//Bitstream metadata
	unsigned int userid = 0;
	bool readProtect = false;

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
		else if(s == "--usercode")
		{
			if(i+1 < argc)
				sscanf(argv[++i], "%x", &userid);
			else
			{
				printf("--usercode requires an argument\n");
				return 1;
			}
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
						printf("invalid part (supported: 46620, 46621, 46140)\n");
						return 1;
				}
			}
			else
			{
				printf("--part requires an argument\n");
				return 1;
			}
		}
		else if(s == "--read-protect")
			readProtect = true;
		else if(s == "--io-precharge")
			ioPrecharge = true;
		else if(s == "--disable-charge-pump")
			disableChargePump = true;
		else if(s == "--ldo-bypass")
			ldoBypass = true;
		else if(s == "--boot-retry")
		{
			if(i+1 < argc)
				bootRetry = atoi(argv[++i]);
			else
			{
				printf("--boot-retry requires an argument\n");
				return 1;
			}
		}
		else if(s == "-o" || s == "--output")
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
	if(console_verbosity >= Severity::NOTICE)
		ShowVersion();

	//Print configuration
	LogNotice("\nDevice configuration:\n");
	{
		LogIndenter li;

		string dev = "<invalid>";
		switch(part)
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

		string pull;
		string drive;

		switch(unused_pull)
		{
			case Greenpak4IOB::PULL_NONE:
				pull = "float";
				break;

			case Greenpak4IOB::PULL_DOWN:
				pull = "pull down with ";
				break;

			case Greenpak4IOB::PULL_UP:
				pull = "pull up with ";
				break;

			default:
				LogError("Invalid pull direction\n");
				return 1;
		}

		if(unused_pull != Greenpak4IOB::PULL_NONE)
		{
			switch(unused_drive)
			{
				case Greenpak4IOB::PULL_10K:
					drive = "10K";
					break;

				case Greenpak4IOB::PULL_100K:
					drive = "100K";
					break;

				case Greenpak4IOB::PULL_1M:
					drive = "1M";
					break;

				default:
					LogError("Invalid pull strength\n");
					return 1;
			}
		}

		LogNotice("Unused pins:     %s %s\n", pull.c_str(), drive.c_str());

		LogNotice("User ID code:    %02x\n", userid);
		LogNotice("Read protection: %s\n", readProtect ? "enabled" : "disabled");
		LogNotice("I/O precharge:   %s\n", ioPrecharge ? "enabled" : "disabled");
		LogNotice("Charge pump:     %s\n", disableChargePump ? "off" : "auto");
		LogNotice("LDO:             %s\n", ldoBypass ? "bypassed" : "enabled");
		LogNotice("Boot retry:      %d times\n", bootRetry);
	}

	//Parse the unplaced netlist
	LogNotice("\nLoading Yosys JSON file \"%s\".\n", fname.c_str());
	Greenpak4Netlist netlist(fname);
	if(!netlist.Validate())
		return 1;

	//Create the device and initialize all IO pins
	Greenpak4Device device(part, unused_pull, unused_drive);
	device.SetIOPrecharge(ioPrecharge);
	device.SetDisableChargePump(disableChargePump);
	device.SetLDOBypass(ldoBypass);
	device.SetNVMRetryCount(bootRetry);

	//Do the actual P&R
	LogNotice("\nSynthesizing top-level module \"%s\".\n", netlist.GetTopModule()->GetName().c_str());
	if(!DoPAR(&netlist, &device))
		return 1;

	//Write the final bitstream
	LogNotice("\nWriting final bitstream to output file \"%s\", using ID code 0x%x.\n",
		ofname.c_str(), (int)userid);
	{
		LogIndenter li;
		if(!device.WriteToFile(ofname, userid, readProtect))
			return 1;
	}

	//TODO: Static timing analysis
	LogNotice("\nStatic timing analysis: not yet implemented\n");

	return 0;
}

void ShowUsage()
{
	printf(//                                                                               v 80th column
		"Usage: gp4par -p part -o bitstream.txt netlist.json\n"
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
}

void ShowVersion()
{
	printf(
		"GreenPAK 4 place-and-route by Andrew D. Zonenberg.\n"
		"\n"
		"License: LGPL v2.1+\n"
		"This is free software: you are free to change and redistribute it.\n"
		"There is NO WARRANTY, to the extent permitted by law.\n");
}
