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

#include <cstring>
#include <cmath>
#include "gp4prog.h"

using namespace std;

void ShowUsage();
void ShowVersion();

const char *PartName(SilegoPart part);
size_t BitstreamLength(SilegoPart part);
const char *BitFunction(SilegoPart part, size_t bitno);

bool SocketTest(hdevice hdev, SilegoPart part);
uint8_t TrimOscillator(hdevice hdev, SilegoPart part, double voltage, unsigned freq);
bool CheckStatus(hdevice hdev);

enum class BitstreamKind {
	UNRECOGNIZED,
	EMPTY,
	PROGRAMMED
};
BitstreamKind ClassifyBitstream(SilegoPart part, vector<uint8_t> bitstream, uint8_t &patternId);

vector<uint8_t> BitstreamFromHex(string hex);
vector<uint8_t> ReadBitstream(string fname);
void WriteBitstream(string fname, vector<uint8_t> bitstream);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Entry point

int main(int argc, char* argv[])
{
	LogSink::Severity console_verbosity = LogSink::NOTICE;

	bool reset = false;
	bool test = false;
	unsigned rcOscFreq = 0;
	string downloadFilename, uploadFilename;
	bool programNvram = false;
	bool force = false;
	uint8_t patternId = 0;
	bool readProtect = false;	
	double voltage = 0.0;
	vector<int> nets;

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
		else if(s == "-r" || s == "--reset")
		{
			reset = true;
		}
		else if(s == "-R" || s == "--read")
		{
			if(i+1 < argc)
			{
				uploadFilename = argv[++i];
			}
			else
			{
				printf("--read requires an argument\n");
				return 1;
			}
		}
		else if(s == "-t" || s == "--test-socket")
			test = true;
		else if(s == "-T" || s == "--trim")
		{
			if(i+1 < argc)
			{
				const char *value = argv[++i];
				if(!strcmp(value, "25k"))
					rcOscFreq = 25000;
				else if(!strcmp(value, "2M"))
					rcOscFreq = 2000000;
				else
				{
					printf("--trim argument must be 25k or 2M\n");
					return 1;
				}
			}
			else
			{
				printf("--trim requires an argument\n");
				return 1;
			}
		}
		else if(s == "-e" || s == "--emulate")
		{
			if(!downloadFilename.empty())
			{
				printf("only one --emulate or --program option can be specified\n");
				return 1;
			}
			if(i+1 < argc)
			{
				downloadFilename = argv[++i];
			}
			else
			{
				printf("--emulate requires an argument\n");
				return 1;
			}
		}
		else if(s == "--program")
		{
			if(!downloadFilename.empty())
			{
				printf("only one --emulate or --program option can be specified\n");
				return 1;
			}
			if(i+1 < argc)
			{
				downloadFilename = argv[++i];
				programNvram = true;
			}
			else
			{
				printf("--program requires an argument\n");
				return 1;
			}
		}
		else if(s == "--force")
			force = true;
		else if(s == "--pattern-id")
		{
			if(i+1 < argc)
			{
				char *arg = argv[++i];
				long id = strtol(arg, &arg, 10);
				if(*arg == '\0' && id >= 0 && id <= 255)
					patternId = id;
				else
				{
					printf("--pattern-id argument must be a number between 0 and 255\n");
					return 1;
				}
			}
			else
			{
				printf("--pattern-id requires an argument\n");
				return 1;
			}
		}
		else if(s == "--read-protect")
			readProtect = true;
		else if(s == "-v" || s == "--voltage")
		{
			if(i+1 < argc)
			{
				char *endptr;
				voltage = strtod(argv[++i], &endptr);
				if(*endptr)
				{
					printf("--voltage must be a decimal value\n");
					return 1;
				}
				if(!(voltage == 0.0 || (voltage >= 1.71 && voltage <= 5.5)))
				{
					printf("--voltage %.3g outside of valid range\n", voltage);
					return 1;
				}
			}
			else
			{
				printf("--voltage requires an argument\n");
				return 1;
			}
		}
		else if(s == "-n" || s == "--nets")
		{
			if(i+1 < argc)
			{
				char *arg = argv[++i];
				do {
					long net = strtol(arg, &arg, 10);
					if(*arg && *arg != ',')
					{
						printf("--nets must be a comma-separate list of net numbers\n");
						return 1;
					}
					if(net < 1 || net > 20 || net == 11)
					{
						printf("--nets used with an invalid net %ld\n", net);
						return 1;
					}
					nets.push_back(net);
				} while(*arg++);
			}
			else
			{
				printf("--nets requires an argument\n");
				return 1;
			}
		}

		//assume it's the bitstream file if it's the first non-switch argument
		else if( (s[0] != '-') && (downloadFilename == "") )
		{
			downloadFilename = s;
		}

		else
		{
			printf("Unrecognized command-line argument \"%s\", use --help\n", s.c_str());
			return 1;
		}
	}

	//Set up logging
	g_log_sinks.emplace(g_log_sinks.begin(), new STDLogSink(console_verbosity));

	//Print header
	if(console_verbosity >= LogSink::NOTICE)
		ShowVersion();

	//Set up libusb
	USBSetup();

	// Try opening the board in "orange" mode
	LogNotice("\nSearching for developer board\n");
	hdevice hdev = OpenDevice(0x0f0f, 0x0006);
	if(!hdev) {
		// Try opening the board in "white" mode
 		hdev = OpenDevice(0x0f0f, 0x8006);
		if(!hdev) {
			LogError("No device found, giving up\n");
			return 1;
		}

		// Change the board into "orange" mode
		LogVerbose("Switching developer board from bootloader mode\n");
		SwitchMode(hdev);

		// Takes a while to switch and re-enumerate
		usleep(1200 * 1000);

		// Try opening the board in "orange" mode again
		hdev = OpenDevice(0x0f0f, 0x0006);
		if(!hdev) {
			LogError("Could not switch mode, giving up\n");
			return 1;
		}
	}

	//Get string descriptors
	string name = GetStringDescriptor(hdev, 1);			//board name
	string vendor = GetStringDescriptor(hdev, 2);		//manufacturer
	LogNotice("Found: %s %s\n", vendor.c_str(), name.c_str());
	//string 0x80 is 02 03 for this board... what does that mean? firmware rev or something?
	//it's read by emulator during startup but no "2" and "3" are printed anywhere...

	//If we're run with no bitstream and no reset flag, stop now without changing board configuration
	if(downloadFilename.empty() && uploadFilename.empty() && voltage == 0.0 && nets.empty() && 
	   rcOscFreq == 0 && !test && !reset)
	{
		LogNotice("No actions requested, exiting\n");
		return 0;
	}

	//Check that we're in good standing
	if(!CheckStatus(hdev)) {
		LogError("Fault condition detected during initial check, exiting\n");
		return 1;
	}

	//Light up the status LED
	SetStatusLED(hdev, 1);

	//See if any of the options require knowing what part we use.
	SilegoPart detectedPart = SilegoPart::UNRECOGNIZED;
	vector<uint8_t> programmedBitstream;
	BitstreamKind bitstreamKind;
	if(!(uploadFilename.empty() && downloadFilename.empty() && rcOscFreq == 0 && !test && !programNvram)) {
		//Detect the part that's plugged in.
		LogNotice("Detecting part\n");
		SilegoPart parts[] = { SLG46140V, SLG46620V };
		for(SilegoPart part : parts) {
			LogVerbose("Selecting part %s\n", PartName(part));
			SetPart(hdev, part);

			//Read extant bitstream and determine part status.
			LogVerbose("Reading bitstream from part\n");
			programmedBitstream = UploadBitstream(hdev, BitstreamLength(part) / 8);
			
			uint8_t patternId = 0;
			bitstreamKind = ClassifyBitstream(part, programmedBitstream, patternId);
			switch(bitstreamKind)
			{
				case BitstreamKind::EMPTY:
					LogNotice("Detected empty %s\n", PartName(part));
					break;

				case BitstreamKind::PROGRAMMED:
					LogNotice("Detected programmed %s (pattern ID %d)\n", PartName(part), patternId);
					break;

				case BitstreamKind::UNRECOGNIZED:
					LogVerbose("Unrecognized bitstream\n");
					continue;
			}

			if(bitstreamKind != BitstreamKind::UNRECOGNIZED) {
				detectedPart = part;
				break;
			}
		}

		if(detectedPart == SilegoPart::UNRECOGNIZED)
		{
			LogError("Could not detect a supported part\n");
			SetStatusLED(hdev, 0);
			return 1;
		}
	}

	if(programNvram && bitstreamKind != BitstreamKind::EMPTY) {
		if(!force) {
			LogError("Non-empty part detected; refusing to program without --force\n");
			SetStatusLED(hdev, 0);
			return 1;
		} else {
			LogNotice("Non-empty part detected and --force is specified; proceeding\n");
		}
	}

	//We already have the programmed bitstream, so simply write it to a file
	if(!uploadFilename.empty()) {
		LogNotice("Writing programmed bitstream to %s\n", uploadFilename.c_str());
		WriteBitstream(uploadFilename, programmedBitstream);
	}

	//Do a socket test before doing anything else, to catch failures early
	if(test)
	{
		if(!SocketTest(hdev, detectedPart)) {
			LogError("Socket test has failed\n");
			SetStatusLED(hdev, 0);
			return 1;
		} else {
			LogNotice("Socket test has passed\n");
		}
	}

	//If we're resetting, do that
	if(reset)
	{
		LogNotice("Resetting board I/O and signal generators\n");
		Reset(hdev);
	}

	//If we need to trim oscillator, do that before programming
	uint8_t rcFtw = 0;
	if(rcOscFreq != 0)
	{
		if(voltage == 0.0) {
			LogError("Trimming oscillator requires specifying target voltage\n");
			return 1;
		}

		LogNotice("Trimming oscillator for %d Hz at %.3g V\n", rcOscFreq, voltage);
		rcFtw = TrimOscillator(hdev, detectedPart, voltage, rcOscFreq);
	}

	//If we're programming, do that first
	if(!downloadFilename.empty())
	{
		vector<uint8_t> newBitstream = ReadBitstream(downloadFilename);
		if(newBitstream.empty())
			return 1;
		if(newBitstream.size() != BitstreamLength(detectedPart) / 8) {
			LogError("Provided bitstream has incorrect length for selected part\n");
			SetStatusLED(hdev, 0);
			return 1;
		}

		//Set trim value reg<1981:1975>
		newBitstream[246] |= rcFtw << 7;
		newBitstream[247] |= rcFtw >> 1;

		//Set pattern ID reg<2031:2038>
		newBitstream[253] |= patternId << 7;
		newBitstream[254] |= patternId >> 1;

		//Set read protection reg<2039>
		newBitstream[254] |= ((uint8_t)readProtect) << 7;

		if(!programNvram) {
			//Load bitstream into SRAM
			LogNotice("Downloading bitstream into SRAM\n");
			DownloadBitstream(hdev, newBitstream, DownloadMode::EMULATION);
		} else {
			//Program bitstream into NVM
			LogNotice("Programming bitstream into NVM\n");
			DownloadBitstream(hdev, newBitstream, DownloadMode::PROGRAMMING);

			LogNotice("Verifying programmed bitstream\n");
			size_t bitstreamLength = BitstreamLength(detectedPart) / 8;
			vector<uint8_t> bitstreamToVerify = UploadBitstream(hdev, bitstreamLength);
			bool failed = false;
			for(size_t i = 0; i < bitstreamLength * 8; i++) {
				bool expectedBit = ((newBitstream     [i/8] >> (i%8)) & 1) == 1;
				bool actualBit   = ((bitstreamToVerify[i/8] >> (i%8)) & 1) == 1;
				if(expectedBit != actualBit) {
					LogNotice("Bit %4zd differs: expected %d, actual %d",
					          i, (int)expectedBit, (int)actualBit);
					failed = true;

					//Explain what undocumented bits do; most of these are also trimming values, and so
					//it is normal for them to vary even if flashing the exact same bitstream many times.
					const char *bitFunction = BitFunction(detectedPart, i);
					if(bitFunction)
						LogNotice(" (bit meaning: %s)\n", bitFunction);
					else
						LogNotice("\n");
				}
			}

			if(failed)
				LogError("Verification failed\n");
			else
				LogNotice("Verification passed\n");
		}

		//Developer board I/O pins become stuck after both SRAM and NVM programming;
		//resetting them explicitly makes LEDs and outputs work again.
		LogDebug("Unstucking I/O pins after programming\n");
		IOConfig ioConfig;
		for(size_t i = 2; i <= 20; i++)
			ioConfig.driverConfigs[i] = TP_RESET;
		SetIOConfig(hdev, ioConfig);
	}

	if(voltage != 0.0)
	{
		//Configure the signal generator for Vdd
		LogNotice("Setting Vdd to %.3g V\n", voltage);
		ConfigureSiggen(hdev, 1, voltage);
	}

	if(!nets.empty())
	{
		//Set the I/O configuration on the test points
		LogNotice("Setting I/O configuration\n");

		IOConfig config;
		for(int net : nets)
		{
			config.driverConfigs[net] = TP_FLOAT;
			config.ledEnabled[net] = true;
			config.expansionEnabled[net] = true;
		}
		SetIOConfig(hdev, config);
	}

	//Check that we didn't break anything
	if(!CheckStatus(hdev))
	{
		LogError("Fault condition detected during final check, exiting\n");
		SetStatusLED(hdev, 0);
		return 1;
	}

	//Done
	LogNotice("Done\n");
	SetStatusLED(hdev, 0);

	USBCleanup(hdev);
	return 0;
}

void ShowUsage()
{
	printf(//                                                                               v 80th column
		"Usage: gp4prog bitstream.txt\n"
		"    When run with no arguments, scans for the board but makes no config changes.\n"
		"    -q, --quiet\n"
		"        Causes only warnings and errors to be written to the console.\n"
		"        Specify twice to also silence warnings.\n"
		"    --verbose\n"
		"        Prints additional information about the design.\n"
		"    --debug\n"
		"        Prints lots of internal debugging information.\n"
		"    --force\n"
		"        Perform actions that may be potentially inadvisable.\n"
		"\n"
		"    The following options are instructions for the developer board. They are\n"
		"    executed in the order listed here, regardless of their order on command line.\n"
		"    -r, --reset\n"
		"        Resets the board:\n"
		"          * disables every LED;\n"
		"          * disables every expansion connector passthrough;\n"
		"          * disables Vdd supply.\n"
		"    -R, --read           <bitstream filename>\n"
		"        Uploads the bitstream stored in non-volatile memory.\n"
		"    -t, --test-socket\n"
		"        Verifies that every connection between socket and device is intact.\n"
		"    -T, --trim           [25k|2M]\n"
		"        Trims the RC oscillator to achieve the specified frequency.\n"
		"    -e, --emulate        <bitstream filename>\n"
		"        Downloads the specified bitstream into volatile memory.\n"
		"        Implies --reset --voltage 3.3.\n"
		"    --program            <bitstream filename>\n"
		"        Programs the specified bitstream into non-volatile memory.\n"
		"        THIS CAN BE DONE ONLY ONCE FOR EVERY INTEGRATED CIRCUIT.\n"
		"        Attempts to program non-empty parts will be rejected unless --force\n"
		"        is specified.\n"
		"    -v, --voltage        <voltage>\n"
		"        Adjusts Vdd to the specified value in volts (0V to 5.5V), ±70mV.\n"
		"    -n, --nets           <net list>\n"
		"        For every test point in the specified comma-separated list:\n"
		"          * enables a non-inverted LED, if any;\n"
		"          * enables expansion connector passthrough.\n");
}

void ShowVersion()
{
	printf(
		"GreenPAK 4 programmer by Andrew D. Zonenberg and whitequark.\n"
		"\n"
		"License: LGPL v2.1+\n"
		"This is free software: you are free to change and redistribute it.\n"
		"There is NO WARRANTY, to the extent permitted by law.\n");
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Part database

const char *PartName(SilegoPart part)
{
	switch(part)
	{
		case SLG46620V: return "SLG46620V";
		case SLG46140V: return "SLG46140V";

		default: LogFatal("Unknown part\n");
	}
}

size_t BitstreamLength(SilegoPart part)
{
	switch(part)
	{
		case SLG46620V: return 2048;
		case SLG46140V: return 1024;

		default: LogFatal("Unknown part\n");
	}
}

const char *BitFunction(SilegoPart part, size_t bitno)
{
	//The conditionals in this function are structured to resemble the structure of the datasheet.
	//This is because the datasheet accurately *groups* reserved bits according to function;
	//they simply black out the parts that aren't meant to be public, but do not mash them together.

	const char *bitFunction = NULL;
	
	switch(part)
	{
		case SLG46620V:
			if((bitno >= 570 && bitno <= 575) ||
			   bitno == 833 ||
			   bitno == 835 ||
			   bitno == 881)
				bitFunction = NULL;
			else if(bitno >= 887 && bitno <= 891)
				bitFunction = "Vref fine tune trimming value";
			else if(bitno == 922 ||
			        bitno == 937 ||
			        bitno == 938 ||
			        bitno == 939 ||
			        (bitno >= 1003 && bitno <= 1015) ||
			        (bitno >= 1594 && bitno <= 1599))
				bitFunction = NULL;
			else if(bitno >= 1975 && bitno <= 1981)
				bitFunction = "RC oscillator trimming value";
			else if((bitno >= 1982 && bitno <= 1987) ||
			        (bitno >= 1988 && bitno <= 1995) ||
			        (bitno >= 1996 && bitno <= 2001) ||
			        (bitno >= 2002 && bitno <= 2007) ||
			        (bitno >= 2013 && bitno <= 2014) ||
			        (bitno >= 2021 && bitno <= 2027) ||
			        (bitno >= 2028 && bitno <= 2029) ||
			        bitno == 2030)
				bitFunction = NULL;
			else if(bitno >= 2031 && bitno <= 2038)
				bitFunction = "pattern ID";
			else if(bitno == 2039)
				bitFunction = "read protection";
			else
				bitFunction = "see datasheet";
			break;

		default: LogFatal("Unknown part\n");
	}

	if(bitFunction == NULL)
		bitFunction = "unknown--reserved";

	return bitFunction;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Status check

bool CheckStatus(hdevice hdev)
{	
	LogDebug("Requesting board status\n");

	BoardStatus status = GetStatus(hdev);
	LogVerbose("Board voltages: A = %.3f V, B = %.3f V\n", status.voltageA, status.voltageB);

	if(status.externalOverCurrent)
		LogError("Overcurrent condition detected on external supply\n");
	if(status.internalOverCurrent)
		LogError("Overcurrent condition detected on internal supply\n");
	if(status.internalUnderVoltage)
		LogError("Undervoltage condition detected on internal supply\n");

	return !(status.externalOverCurrent && 
			 status.internalOverCurrent &&
			 status.internalUnderVoltage);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Socket test

bool SocketTest(hdevice hdev, SilegoPart part)
{
	vector<uint8_t> loopbackBitstream;

	switch(part)
	{
		case SLG46620V:
			// To reproduce:
			// module top((* LOC="P2" *) input i, output [16:0] o));
			// 	assign o = {17{i}};
			// endmodule
			loopbackBitstream = BitstreamFromHex(
				"0000000000000000000000000000000000000000000000000000000000000000"
				"00000000000000000000d88f613f86fd18f6633f000000000000000000000000"
				"0600000000000000000000000000000000000000000000000000000000000000"
				"0000000000000800000000000000000900000008000400080002800000000000"
				"0000000000000000000000000000000000000000000000000000000000000000"
				"0000000000000000000034fdd33f4dff34fdd33f0d0000000000000000000000"
				"0000000000000000000000002004000000000000000000000000000000000000"
				"0000000000000000000000000000000200800020000004000000000200000000");
			break;

		default: LogFatal("Unknown part\n");
	}

	LogVerbose("Downloading test bitstream\n");
	DownloadBitstream(hdev, loopbackBitstream, DownloadMode::EMULATION);

	LogVerbose("Initializing test I/O\n");
	double supplyVoltage = 5.0;
	ConfigureSiggen(hdev, 1, supplyVoltage);

	IOConfig ioConfig;
	for(size_t i = 2; i <= 20; i++)
		ioConfig.driverConfigs[i] = TP_RESET;
	SetIOConfig(hdev, ioConfig);

	bool ok = true;
	tuple<TPConfig, TPConfig, double> sequence[] = {
		make_tuple(TP_GND, TP_FLIMSY_PULLUP,   0.0),
		make_tuple(TP_VDD, TP_FLIMSY_PULLDOWN, 1.0),
	};
	for(auto config : sequence) {
		if(get<0>(config) == TP_GND)
			LogVerbose("Testing logical low output\n");
		else
			LogVerbose("Testing logical high output\n");

		ioConfig.driverConfigs[2] = get<0>(config);
		for(size_t i = 3; i <= 20; i++)
			ioConfig.driverConfigs[i] = get<1>(config);
		SetIOConfig(hdev, ioConfig);

		for(int i = 2; i <= 20; i++) {
			if(i == 11) continue;

			SelectADCChannel(hdev, i);
			double value = ReadADC(hdev);
			LogDebug("P%d = %.3f V\n", i, supplyVoltage * value);

			if(fabs(value - get<2>(config)) > 0.01) {
				LogError("Socket functional test (%s level) test failed on pin P%d\n",
				         (get<0>(config) == TP_GND) ? "low" : "high", i);
				ok = false;
			}
		}
	}

	LogVerbose("Resetting board after socket test\n");
	Reset(hdev);
	return ok;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Oscillator trimming

uint8_t TrimOscillator(hdevice hdev, SilegoPart part, double voltage, unsigned freq)
{
	vector<uint8_t> trimBitstream;

	switch(part)
	{
		case SLG46620V:
			// To reproduce:
			// module top((* LOC="P13" *) output q);
			// 	GP_RCOSC rcosc (.CLKOUT_FABRIC(q));
			// endmodule
			trimBitstream = BitstreamFromHex(
				"0000000000000000000000000000000000000000000000000000000000000000"
				"00000000000000000000000000000000000000000000000000000000c0bf6900"
				"0000000000000000000000000000000000000000000000000000000000000000"
				"0000000000000800300000000000200600000004000c000000c060300000005a"
				"0000000000000000000000000000000000000000000000000000000000000000"
				"0000000000000000000040fc0300000000000000000000000000000000000000"
				"0000000000000000000000308004000000000000000000000000000000000000"
				"0000000000000000000000000000000000000000000000e006088200000000a5");
			if(freq == 2000000) // set the .OSC_FREQ("2M") bit at reg<1650>
				trimBitstream[1650/8] |= 1 << (1650%8);
			break;

		default: LogFatal("Unknown part\n");
	}

	LogVerbose("Resetting board before oscillator trimming\n");
	Reset(hdev);

	LogVerbose("Downloading oscillator trimming bitstream\n");
	DownloadBitstream(hdev, trimBitstream, DownloadMode::TRIMMING);

	LogVerbose("Configuring I/O for oscillator trimming\n");
	IOConfig config;
	for(size_t i = 2; i <= 20; i++)
		config.driverConfigs[i] = TP_PULLDOWN;
	config.driverConfigs[2] = TP_FLOAT;
	config.driverConfigs[3] = TP_VDD;
	config.driverConfigs[4] = TP_GND;
	config.driverConfigs[5] = TP_GND;
	config.driverConfigs[6] = TP_GND;
	config.driverConfigs[7] = TP_GND;
	config.driverConfigs[10] = TP_VDD;
	config.driverConfigs[12] = TP_VDD;
	config.driverConfigs[15] = TP_VDD;
	SetIOConfig(hdev, config);

	LogVerbose("Setting voltage for oscillator trimming\n");
	ConfigureSiggen(hdev, 1, voltage);

	//The frequency tuning word is 7-bit
	uint8_t low = 0, high = 0x7f, mid;
	unsigned actualFreq;
	while(low < high) {
		mid = low + (high - low) / 2;
		LogDebug("Trimming with FTW %d\n", mid);
		TrimOscillator(hdev, mid);
		actualFreq = MeasureOscillatorFrequency(hdev);
		LogDebug("Oscillator frequency is %d Hz\n", actualFreq);
		if(actualFreq > freq) {
			high = mid;
		} else if(actualFreq < freq) {
			low = mid + 1;
		} else break;
	}
	LogNotice("Trimmed RC oscillator to %d Hz\n", actualFreq);
		
	LogVerbose("Resetting board after oscillator trimming\n");
	Reset(hdev);
	return mid;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Bitstream classification

BitstreamKind ClassifyBitstream(SilegoPart part, vector<uint8_t> bitstream, uint8_t &patternId)
{
	vector<uint8_t> emptyBitstream(BitstreamLength(part) / 8);
	vector<uint8_t> factoryMask(BitstreamLength(part) / 8);

	switch(part) {
		case SLG46620V: 
			emptyBitstream[0x7f] = 0x5a;
			emptyBitstream[0xff] = 0xa5;
			//TODO: RC oscillator trim value, why is it factory programmed?
			factoryMask[0xf7] = 0xff; 
			factoryMask[0xf8] = 0xff;
			break;

		case SLG46140V:
			emptyBitstream[0x7b] = 0x5a;
			emptyBitstream[0x7f] = 0xa5;
			//TODO: RC oscillator trim value, why is it factory programmed?
			factoryMask[0x77] = 0xff;
			factoryMask[0x78] = 0xff;
			break;

		default:
			LogFatal("Unknown part\n");
	}

	if(bitstream.size() != emptyBitstream.size())
		return BitstreamKind::UNRECOGNIZED;

	//Iterate over the bitstream, and print our decisions after every octet, in --debug mode.
	bool isPresent = true;
	bool isProgrammed = false;
	for(size_t i = bitstream.size() - 1; i > 0; i--) {
		uint8_t maskedOctet = bitstream[i] & ~factoryMask[i];
		if(maskedOctet != 0x00)
			LogDebug("mask(bitstream[0x%02zx]) = 0x%02hhx\n", i, maskedOctet);

		if(emptyBitstream[i] != 0x00 && maskedOctet != emptyBitstream[i] && isPresent) {
			LogDebug("Bitstream does not match empty bitstream signature, part not present.\n");
			isPresent = false;
			break;
		} else if(emptyBitstream[i] == 0x00 && maskedOctet != 0x00 && !isProgrammed) {
			LogDebug("Bitstream nonzero where empty bitstream isn't, part programmed.\n");
			isProgrammed = true;
		}
	}

	switch(part) {
		case SLG46620V: 
			patternId = (bitstream[0xfd] >> 7) | (bitstream[0xfe] << 1);
			break;
	
		case SLG46140V:
			patternId = (bitstream[0x7d] >> 7) | (bitstream[0x7e] << 1);
			break;

		default:
			LogFatal("Unknown part\n");
	}

	if(isPresent && isProgrammed)
		return BitstreamKind::PROGRAMMED;
	else if(isPresent)
		return BitstreamKind::EMPTY;
	else
		return BitstreamKind::UNRECOGNIZED;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Bitstream input/output

vector<uint8_t> BitstreamFromHex(string hex)
{
	std::vector<uint8_t> bitstream;
	for(size_t i = 0; i < hex.size(); i += 2) {
		uint8_t octet;
		sscanf(&hex[i], "%02hhx", &octet);
		bitstream.push_back(octet);
	}
	return bitstream;
}

vector<uint8_t> ReadBitstream(string fname)
{
	FILE* fp = fopen(fname.c_str(), "rt");
	if(!fp)
	{
		LogError("Couldn't open %s for reading\n", fname.c_str());
		return {};
	}

	char signature[64];
	fgets(signature, sizeof(signature), fp);
	if(strcmp(signature, "index\t\tvalue\t\tcomment\n") &&
	   //GP4 on Linux still outputs a \r
	   strcmp(signature, "index\t\tvalue\t\tcomment\r\n"))
	{
		LogError("%s is not a GreenPAK bitstream\n", fname.c_str());
		fclose(fp);
		return {};
	}

	vector<uint8_t> bitstream;
	while(!feof(fp))
	{
		int index;
		int value;
		fscanf(fp, "%d\t\t%d\t\t//\n", &index, &value);

		int byteindex = index / 8;
		if(byteindex < 0 || (value != 0 && value != 1))
		{
			LogError("%s contains a malformed GreenPAK bitstream\n", fname.c_str());
			fclose(fp);
			return {};
		}

		if(byteindex >= (int)bitstream.size())
			bitstream.resize(byteindex + 1, 0);
		bitstream[byteindex] |= (value << (index % 8));
	}

	fclose(fp);

	return bitstream;
}

void WriteBitstream(string fname, vector<uint8_t> bitstream)
{
	FILE* fp = fopen(fname.c_str(), "wt");
	if(!fp)
	{
		LogError("Couldn't open %s for reading\n", fname.c_str());
		return;
	}

	fputs("index\t\tvalue\t\tcomment\n", fp);
	for(size_t i = 0; i < bitstream.size() * 8; i++) {
		int value = (bitstream[i / 8] >> (i % 8)) & 1;
		fprintf(fp, "%d\t\t%d\t\t//\n", (int)i, value);
	}

	fclose(fp);
}
