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

bool SocketTest(hdevice hdev, SilegoPart part);
bool CheckStatus(hdevice hdev);

enum class BitstreamKind {
	UNRECOGNIZED,
	EMPTY,
	PROGRAMMED,
	PROTECTED // implies PROGRAMMED
};
BitstreamKind ClassifyBitstream(SilegoPart part, vector<uint8_t> bitstream);

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
	string emulateFilename, readFilename;
	double voltage = 0.0;
	vector<int> nets;

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
			if(console_verbosity == LogSink::NOTICE)
				console_verbosity = LogSink::WARNING;
			else if(console_verbosity == LogSink::WARNING)
				console_verbosity = LogSink::ERROR;
		}
		else if(s == "--verbose")
			console_verbosity = LogSink::VERBOSE;
		else if(s == "--debug")
			console_verbosity = LogSink::DEBUG;
		else if(s == "-r" || s == "--reset")
		{
			reset = true;
		}
		else if(s == "-R" || s == "--read")
		{
			if(i+1 < argc)
			{
				readFilename = argv[++i];
			}
			else
			{
				printf("--read requires an argument\n");
				return 1;
			}
		}
		else if(s == "-t" || s == "--test-socket")
			test = true;
		else if(s == "-e" || s == "--emulate")
		{
			if(i+1 < argc)
			{
				emulateFilename = argv[++i];
			}
			else
			{
				printf("--emulate requires an argument\n");
				return 1;
			}
		}
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
		else if( (s[0] != '-') && (emulateFilename == "") )
		{
			emulateFilename = s;
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
	if(emulateFilename.empty() && readFilename.empty() && voltage == 0.0 && nets.empty() && !test && !reset)
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

	//Detect the part that's plugged in.
	LogNotice("Detecting part\n");
	SilegoPart parts[] = { SLG46140V, SLG46620V };
	SilegoPart detectedPart;
	vector<uint8_t> programmedBitstream;
	BitstreamKind bitstreamKind;
	for(SilegoPart part : parts) {
		LogVerbose("Selecting part %s\n", PartName(part));
		SetPart(hdev, part);

		//Read extant bitstream and determine part status.
		LogVerbose("Reading bitstream from part\n");
		programmedBitstream = UploadBitstream(hdev, BitstreamLength(part) / 8);
		
		bitstreamKind = ClassifyBitstream(part, programmedBitstream);
		switch(bitstreamKind)
		{
			case BitstreamKind::EMPTY:
				LogNotice("Detected empty %s\n", PartName(part));
				break;

			case BitstreamKind::PROGRAMMED:
				LogNotice("Detected pre-programmed %s\n", PartName(part));
				break;


			case BitstreamKind::PROTECTED:
				LogNotice("Detected pre-programmed and read-protected %s\n", PartName(part));
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

	if(bitstreamKind == BitstreamKind::UNRECOGNIZED)
	{
		LogError("Could not detect a supported part\n");
		SetStatusLED(hdev, 0);
		return 1;
	}

	//We already have the programmed bitstream, so simply write it to a file
	if(!readFilename.empty())
	{
		WriteBitstream(readFilename, programmedBitstream);
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
		IOConfig config;
		SetIOConfig(hdev, config);
		ResetAllSiggens(hdev);
	}

	//If we're programming, do that first
	if(!emulateFilename.empty())
	{
		vector<uint8_t> newBitstream = ReadBitstream(emulateFilename);
		if(newBitstream.empty())
			return 1;
		if(newBitstream.size() != BitstreamLength(detectedPart) / 8) {
			LogError("Provided bitstream has incorrect length for selected part\n");
			SetStatusLED(hdev, 0);
			return 1;
		}

		//Load bitstream
		LogNotice("Loading bitstream into SRAM\n");
		DownloadBitstream(hdev, newBitstream);
	}

	if(voltage != 0.0)
	{
		//Configure the signal generator for Vdd
		LogNotice("Setting Vdd=%.3gV\n", voltage);
		ConfigureSiggen(hdev, 1, voltage);
		SetSiggenStatus(hdev, 1, SIGGEN_START);
	}

	if(!nets.empty())
	{
		//Set the I/O configuration on the test points
		LogNotice("Setting I/O configuration\n");

		IOConfig config;
		for(int net : nets)
			config.driverConfigs[net] = TP_RESET;
		SetIOConfig(hdev, config);

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
		"    -e, --emulate        <bitstream filename>\n"
		"        Downloads the specified bitstream into volatile memory.\n"
		"        Implies --reset --voltage 3.3.\n"
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
	}

	LogFatal("Unknown part\n");
}

size_t BitstreamLength(SilegoPart part)
{
	switch(part)
	{
		case SLG46620V: return 2048;
		case SLG46140V: return 1024;
	}

	LogFatal("Unknown part\n");
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Status check

bool CheckStatus(hdevice hdev)
{
	BoardStatus status = GetStatus(hdev);
	LogVerbose("Board voltages: A = %.3f V, B = %.3f B\n", status.voltageA, status.voltageB);

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
			// module top(
			// 		(* LOC="P2" *) input i,
			// 		output o1, o2, o3, o4, o5, o6, o7, o8, o9, o10, o11, o12, o13, o14, o15, o16, o17
			// 	);
			// 	assign {o1, o2, o3, o4, o5, o6, o7, o8, o9, o10, o11, o12, o13, o14, o15, o16, o17} = {17{i}};
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
	DownloadBitstream(hdev, loopbackBitstream);

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

	return ok;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Bitstream classification

BitstreamKind ClassifyBitstream(SilegoPart part, vector<uint8_t> bitstream)
{
	vector<uint8_t> emptyBitstream(BitstreamLength(part) / 8);
	vector<uint8_t> preprogrammedMask(BitstreamLength(part) / 8);
	pair<size_t, uint8_t> protectedMask;

	switch(part) {
		case SLG46620V: 
			emptyBitstream[0x7f] = 0x5a;
			emptyBitstream[0xff] = 0xa5;
			//TODO: Some trim value? Figure out exactly what this does
			preprogrammedMask[0xf7] = 0xff; 
			preprogrammedMask[0xf8] = 0xff;
			protectedMask = make_pair(0xfe, 0x80);
			break;

		case SLG46140V:
			emptyBitstream[0x7b] = 0x5a;
			emptyBitstream[0x7f] = 0xa5;
			//TODO: Some trim value? Figure out exactly what this does
			preprogrammedMask[0x77] = 0xff;
			preprogrammedMask[0x78] = 0xff;
			protectedMask = make_pair(0x7e, 0x80);
			break;

		default:
			LogFatal("Unknown part\n");
	}

	if(bitstream.size() != emptyBitstream.size())
		return BitstreamKind::UNRECOGNIZED;

	//Iterate over the bitstream, and print our decisions after every octet, in --debug mode.
	bool isPresent = true;
	bool isProgrammed = false;
	bool isProtected = false;
	for(size_t i = bitstream.size() - 1; i > 0; i--) {
		uint8_t maskedOctet = bitstream[i] & ~preprogrammedMask[i];
		if(maskedOctet != 0x00)
			LogDebug("mask(bitstream[0x%02zx]) = 0x%02hhx\n", i, maskedOctet);

		if(emptyBitstream[i] != 0x00 && maskedOctet != emptyBitstream[i]) {
			LogDebug("Bitstream does not match empty bitstream signature, part not present.\n");
			isPresent = false;
			break;
		} else if(emptyBitstream[i] == 0x00 && maskedOctet != 0x00) {
			LogDebug("Bitstream nonzero where empty bitstream isn't, part programmed.\n");
			isProgrammed = true;
		}

		// TODO: verify that this works
		if(protectedMask.first == i && protectedMask.second != 0 && 
		   (bitstream[i] & protectedMask.second) == protectedMask.second) {
			LogDebug("Bitstream matches protected mask, part protected.\n");
			isProtected = true;
		}
	}

	if(isPresent && isProgrammed)
		return isProtected ? BitstreamKind::PROTECTED : BitstreamKind::PROGRAMMED;
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
