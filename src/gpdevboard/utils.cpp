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

#include <unistd.h>
#include <cmath>
#include <cstring>

#include <log.h>
#include "gpdevboard.h"

using namespace std;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Status check

bool CheckStatus(hdevice hdev)
{
	LogDebug("Requesting board status\n");

	BoardStatus status;
	if(!GetStatus(hdev, status))
		return false;
	LogDebug("Board voltages: A = %.3f V, B = %.3f V\n", status.voltageA, status.voltageB);

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
// Initialization

/**
	@brief Connect to the board, but don't change anything
 */
hdevice OpenBoard(int nboard)
{
	//Set up libusb
	if(!USBSetup())
		return NULL;

	//Try opening the board in "orange" mode
	LogNotice("\nSearching for developer board\n");
	hdevice hdev = OpenDevice(0x0f0f, 0x0006, nboard);
	if(!hdev)
	{
		//Try opening the board in "white" mode
		hdev = OpenDevice(0x0f0f, 0x8006, nboard);
		if(!hdev)
		{
			LogError("No device found, giving up\n");
			return NULL;
		}

		//Change the board into "orange" mode
		LogVerbose("Switching developer board from bootloader mode\n");
		if(!SwitchMode(hdev))
			return NULL;

		//Takes a while to switch and re-enumerate
		usleep(1200 * 1000);

		//Try opening the board in "orange" mode again
		hdev = OpenDevice(0x0f0f, 0x0006, nboard);
		if(!hdev)
		{
			LogError("Could not switch mode, giving up\n");
			return NULL;
		}
	}

	//Get string descriptors
	string name, vendor;
	if(!GetStringDescriptor(hdev, 1, name) || //board name
	   !GetStringDescriptor(hdev, 2, vendor)) //manufacturer
	{
		return NULL;
	}
	LogNotice("Found: %s %s\n", vendor.c_str(), name.c_str());
	//string 0x80 is 02 03 for this board... what does that mean? firmware rev or something?
	//it's read by emulator during startup but no "2" and "3" are printed anywhere...

	//Check that we're in good standing
	if(!CheckStatus(hdev))
	{
		LogError("Fault condition detected during initial check, exiting\n");
		return NULL;
	}

	//Done
	return hdev;
}

/**
	@brief Detect the part that's currently installed on the board

	(this requires dumping the current firmware)
 */
bool DetectPart(
	hdevice hdev,
	SilegoPart& detectedPart,
	vector<uint8_t>& programmedBitstream,
	BitstreamKind& bitstreamKind)
{
	//Detect the part that's plugged in.
	LogNotice("Detecting part\n");
	LogIndenter li;

	SilegoPart parts[] = { SLG46140V, SLG4662XV };
	for(SilegoPart part : parts)
	{
		LogVerbose("Trying part %s\n", PartName(part));
		if(!SetPart(hdev, part))
			return false;

		//Read extant bitstream and determine part status.
		LogVerbose("Reading bitstream from part\n");
		if(!UploadBitstream(hdev, BitstreamLength(part) / 8, programmedBitstream))
			return false;

		uint8_t patternId = 0;
		bitstreamKind = ClassifyBitstream(part, programmedBitstream, patternId);
		switch(bitstreamKind)
		{
			case BitstreamKind::EMPTY:
				LogNotice("Detected empty %s\n", PartName(part));
				break;

			case BitstreamKind::PROGRAMMED:
				LogNotice("Detected programmed %s (pattern ID 0x%d)\n", PartName(part), patternId);
				break;

			case BitstreamKind::UNRECOGNIZED:
				LogVerbose("Unrecognized bitstream\n");
				continue;
		}

		if(bitstreamKind != BitstreamKind::UNRECOGNIZED)
		{
			detectedPart = part;
			break;
		}
	}

	//If we detected a 4662x, try to tell them apart
	if(detectedPart == SLG4662XV)
	{
		LogVerbose("We detected a 4662x, but not sure which one it is yet\n");
		LogIndenter li;

		//To reproduce: build PowerRailDetector_STQFN20 for SLG46621V
		LogVerbose("Loading identification bitstream\n");
		vector<uint8_t> idBitstream = BitstreamFromHex(
			"0000000000000000000000000000000000000000000000000000000000000000"
			"000000000000000000000000000000000000fc3f0000000000000000c00f0000"
			"0000000000000000000000000000000000000000000000000000000000000000"
			"0000000000000800000000000000000900000008000400000000000000000000"
			"0000000000000000000000000000000000000000000000000000000000000000"
			"00000000000000000000000000000000000000c00f0000000000000000000000"
			"0000000000000000030600002004040000000000000000000000000000000000"
			"0000000000000000000000000000000000000000000004000000000208802000");
		if(!DownloadBitstream(hdev, idBitstream, DownloadMode::EMULATION))
			return false;

		//Developer board I/O pins become stuck after both SRAM and NVM programming;
		//resetting them explicitly makes LEDs and outputs work again.
		LogDebug("Unstucking I/O pins after programming\n");
		IOConfig ioConfig;
		for(size_t i = 2; i <= 20; i++)
			ioConfig.driverConfigs[i] = TP_RESET;
		if(!SetIOConfig(hdev, ioConfig))
			return 1;

		//Set Vdd to 3.3V and Vdd2 to something much lower
		//Note that we have to go below the usual minimum b/c
		//we're sampling with an ADC that tops out at 1.0V!
		double vdd = 3.3;
		double vdd2 = 0.5;
		LogVerbose("Setting voltages for ID bitstream (vccint/vcco_1=%.3f, vcco_2 = %.3f)\n", vdd, vdd2);
		if(!ConfigureSiggen(hdev, 1, vdd))
			return false;
		if(!ConfigureSiggen(hdev, 14, vdd2))
			return false;

		//Read the ADC on pin 10 (sanity check) and 20 (device ID)
		double pin10_value;
		double pin20_value;
		if(!SingleReadADC(hdev, 10, pin10_value))
			return false;
		if(!SingleReadADC(hdev, 20, pin20_value))
			return false;

		//If pin 10 is not saturated, something is wrong
		if(pin10_value < 0.95)
		{
			LogError("Device didn't pull pin 10 high during device ID test\n");
			return false;
		}

		//If pin 20 is too low, something is wrong
		if(pin20_value < 0.3)
		{
			LogError("Device didn't pull pin 20 high during device ID test\n");
			return false;
		}

		//If pin 20 is >> 0.5V, it's a 46620 (since pin 20 is on vcore instead of vccio)
		LogVerbose("Pin 20 value: %.3f V\n", pin20_value);
		if(pin20_value > 0.6)
		{
			LogVerbose("Pin 20 is on vccint/vcco_1 rail, it's a 46620\n");
			detectedPart = SilegoPart::SLG46620V;
		}

		else
		{
			LogVerbose("Pin 20 is on vcco_2 rail, it's a 46621\n");
			detectedPart = SilegoPart::SLG46621V;
		}

		//Done, wipe our test bitstream
		LogVerbose("Resetting board after detecting part\n");
		if(!Reset(hdev))
			return false;
	}

	if(detectedPart == SilegoPart::UNRECOGNIZED)
	{
		LogError("Could not detect a supported part\n");
		return false;
	}

	return true;
}

/**
	@brief Check if a particular device is present and connected
 */
bool VerifyDevicePresent(hdevice hdev, SilegoPart expectedPart)
{
	SilegoPart detectedPart = SilegoPart::UNRECOGNIZED;
	vector<uint8_t> programmedBitstream;
	BitstreamKind bitstreamKind;

	if(!DetectPart(hdev, detectedPart, programmedBitstream, bitstreamKind))
		return false;

	if(expectedPart != detectedPart)
		return false;

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Bitstream classification

BitstreamKind ClassifyBitstream(SilegoPart part, vector<uint8_t> bitstream, uint8_t &patternId)
{
	vector<uint8_t> emptyBitstream(BitstreamLength(part) / 8);
	vector<uint8_t> factoryMask(BitstreamLength(part) / 8);

	switch(part)
	{
		case SLG46620V:
		case SLG46621V:
		case SLG4662XV:
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
	for(size_t i = bitstream.size() - 1; i > 0; i--)
	{
		uint8_t maskedOctet = bitstream[i] & ~factoryMask[i];
		if(maskedOctet != 0x00)
			LogDebug("mask(bitstream[0x%02zx]) = 0x%02hhx\n", i, maskedOctet);

		if(emptyBitstream[i] != 0x00 && maskedOctet != emptyBitstream[i] && isPresent)
		{
			LogDebug("Bitstream does not match empty bitstream signature, part not present.\n");
			isPresent = false;
			break;
		}
		else if(emptyBitstream[i] == 0x00 && maskedOctet != 0x00 && !isProgrammed)
		{
			LogDebug("Bitstream nonzero where empty bitstream isn't, part programmed.\n");
			isProgrammed = true;
		}
	}

	switch(part)
	{
		case SLG46620V:
		case SLG46621V:
		case SLG4662XV:
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
// Part database

const char *PartName(SilegoPart part)
{
	switch(part)
	{
		case SLG46620V: return "SLG46620V";
		case SLG46621V: return "SLG46621V";
		case SLG4662XV: return "SLG46620V/SLG46621V";
		case SLG46140V: return "SLG46140V";

		default: LogFatal("Unknown part\n");
	}
}

size_t BitstreamLength(SilegoPart part)
{
	switch(part)
	{
		case SLG46620V:
		case SLG46621V:
		case SLG4662XV: return 2048;
		case SLG46140V: return 1024;

		default: LogFatal("Unknown part\n");
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Socket test

bool SocketTest(hdevice hdev, SilegoPart part)
{
	LogVerbose("Running electrical loopback test on socket\n");
	LogIndenter li;

	vector<uint8_t> loopbackBitstream;

	int avail_gpios = 18;

	switch(part)
	{
		case SLG46620V:
			//To reproduce: build SocketTestLoopback_STQFN20 for SLG46620V
			loopbackBitstream = BitstreamFromHex(
				"0000000000000000000000000000000000000000000000000000000000000000"
				"00000000000000000000d88f613f86fd18f6633f000000000000000000000000"
				"0600000000000000000000000000000000000000000000000000000000000000"
				"0000000000000800000000000000000900000008000400080002800000000000"
				"0000000000000000000000000000000000000000000000000000000000000000"
				"0000000000000000000034fdd33f4dff34fdd33f0d0000000000000000000000"
				"0000000000000000000000002004000000000000000000000000000000000000"
				"0000000000000000000000000000000200800020000004000000000200000000");

			avail_gpios = 18;	//single rail STQFN-20
			break;

		case SLG46621V:
			//To reproduce: build SocketTestLoopback_STQFN20D for SLG46621V
			loopbackBitstream = BitstreamFromHex(
				"0000000000000000000000000000000000000000000000000000000000000000"
				"00000000000000000000d88f613f86fd18f6633f0000000000000000c00f0000"
				"0600000000000000000000000000000000000000000000000000000000000000"
				"000000000000080000000000000000090000000800040008000280000000005a"
				"0000000000000000000000000000000000000000000000000000000000000000"
				"0000000000000000000034fd03004dff34fdd33f0d0000000000000000000000"
				"0000000000000000030600002004040000000000000000000000000000000000"
				"00000000000000000000000000000002008000200000040000000002088020a5");

			avail_gpios = 17;	//dual rail STQFN-20
			break;

		case SLG46140V:
			//To reproduce: build SocketTestLoopback_STQFN14 for SLG46140V
			loopbackBitstream = BitstreamFromHex(
				"0000000000000000000000000000000000000000000000000000000000000000"
				"00d66ffdd66f59bff55b96f55bbff5bf00000000000000000000000000900000"
				"0000000000000000000000000000000000000000000000000000000000000000"
				"00004000002040000000000000000000000000000000000000000000008020a5");

			avail_gpios = 12;	//single rail STQFN-14
			break;

		default:
			LogFatal("Unknown part\n");
	}

	LogVerbose("Downloading test bitstream\n");
	if(!DownloadBitstream(hdev, loopbackBitstream, DownloadMode::EMULATION))
		return false;

	LogVerbose("Initializing test I/O\n");
	double supplyVoltage = 5.0;
	if(!ConfigureSiggen(hdev, 1, supplyVoltage))
		return false;

	IOConfig ioConfig;
	for(size_t i = 2; i <= 20; i++)
		ioConfig.driverConfigs[i] = TP_RESET;
	if(!SetIOConfig(hdev, ioConfig))
		return false;

	bool ok = true;
	tuple<TPConfig, TPConfig, double> sequence[] =
	{
		make_tuple(TP_GND, TP_FLIMSY_PULLUP,   0.0),
		make_tuple(TP_VDD, TP_FLIMSY_PULLDOWN, 1.0),
	};

	for(auto config : sequence)
	{
		if(get<0>(config) == TP_GND)
			LogVerbose("Testing logical low output\n");
		else
			LogVerbose("Testing logical high output\n");

		ioConfig.driverConfigs[2] = get<0>(config);
		for(size_t i = 3; i <= 20; i++)
		{
			//Don't mess with the VCCIO2 driver in dual-rail parts
			if( (avail_gpios == 17) && (i == 14) )
				continue;

			ioConfig.driverConfigs[i] = get<1>(config);
		}
		if(!SetIOConfig(hdev, ioConfig))
			return false;

		for(int i = 2; i <= 20; i++)
		{
			if(i == 11) continue;

			//Skip un-bonded pins for lower pin count packages
			if(avail_gpios == 12)
			{
				if( (i == 8) || (i == 9) || (i == 10) || (i == 18) || (i == 19) || (i == 20) )
					continue;
			}
			else if( (avail_gpios == 17) && (i == 14) )
				continue;

			if(!SelectADCChannel(hdev, i))
				return false;
			double value;
			if(!ReadADC(hdev, value))
				return false;
			LogDebug("P%d = %.3f V\n", i, supplyVoltage * value);

			if(fabs(value - get<2>(config)) > 0.01)
			{
				LogError("Socket functional test (%s level) test failed on pin P%d\n",
				         (get<0>(config) == TP_GND) ? "low" : "high", i);
				ok = false;
			}
		}
	}

	LogVerbose("Resetting board after socket test\n");
	if(!Reset(hdev))
		return false;
	return ok;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Oscillator trimming

bool TrimOscillator(hdevice hdev, SilegoPart part, double voltage, unsigned freq, uint8_t &ftw)
{
	LogVerbose("Trimming RC oscillator for %.3f kHz\n", freq/1000.0f);
	LogIndenter li;

	vector<uint8_t> trimBitstream;

	switch(part)
	{
		case SLG46620V:
		case SLG46621V:
		case SLG4662XV:
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

		case SLG46140V:
			LogWarning("FIXME: not doing anything in SocketTest\n");
			return true;

		default: LogFatal("Unknown part\n");
	}

	LogVerbose("Resetting board before oscillator trimming\n");
	if(!Reset(hdev))
		return false;

	LogVerbose("Downloading oscillator trimming bitstream\n");
	if(!DownloadBitstream(hdev, trimBitstream, DownloadMode::TRIMMING))
		return false;

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
	if(!SetIOConfig(hdev, config))
		return false;

	LogVerbose("Setting voltage for oscillator trimming\n");
	if(!ConfigureSiggen(hdev, 1, voltage))
		return false;

	//The frequency tuning word is 7-bit
	uint8_t low = 0, high = 0x7f, mid;
	unsigned actualFreq;
	while(low < high)
	{
		mid = low + (high - low) / 2;
		LogDebug("Trimming with FTW %d\n", mid);
		if(!TrimOscillator(hdev, mid))
			return false;
		if(!MeasureOscillatorFrequency(hdev, actualFreq))
			return false;
		LogDebug("Oscillator frequency is %.3f kHz\n", actualFreq/1000.0f);
		if(actualFreq > freq)
		{
			high = mid;
		}
		else if(actualFreq < freq)
		{
			low = mid + 1;
		}
		else
			break;
	}
	LogNotice("Trimmed RC oscillator to %.3f kHz\n", actualFreq/1000.0f);
	ftw = mid;

	LogVerbose("Resetting board after oscillator trimming\n");
	if(!Reset(hdev))
		return false;

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Bitstream input/output

vector<uint8_t> BitstreamFromHex(string hex)
{
	std::vector<uint8_t> bitstream;
	for(size_t i = 0; i < hex.size(); i += 2)
	{
		uint8_t octet;
		sscanf(&hex[i], "%02hhx", &octet);
		bitstream.push_back(octet);
	}
	return bitstream;
}

bool ReadBitstream(string fname, vector<uint8_t>& bitstream, SilegoPart part)
{
	FILE* fp = fopen(fname.c_str(), "rt");
	if(!fp)
	{
		LogError("Couldn't open %s for reading\n", fname.c_str());
		return false;
	}

	char signature[64];
	fgets(signature, sizeof(signature), fp);
	if(strcmp(signature, "index\t\tvalue\t\tcomment\n") &&
	   //GP4 on Linux still outputs a \r
	   strcmp(signature, "index\t\tvalue\t\tcomment\r\n"))
	{
		LogError("%s is not a GreenPAK bitstream\n", fname.c_str());
		fclose(fp);
		return false;
	}

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
			return false;
		}

		if(byteindex >= (int)bitstream.size())
			bitstream.resize(byteindex + 1, 0);
		bitstream[byteindex] |= (value << (index % 8));
	}

	fclose(fp);

	//TODO: check ID words?

	//Verify that the bitstream is a sane length
	if(bitstream.size() != BitstreamLength(part) / 8)
	{
		LogError("Provided bitstream has incorrect length for selected part\n");
		return false;
	}

	return true;
}

/**
	@brief Take a bitstream and modify some of the configuration

	OR in the read protect and pattern ID... only meaningful if the old values were unprotected and 0 resp
 */
bool TweakBitstream(vector<uint8_t>& bitstream, SilegoPart part, uint8_t oscTrim, uint8_t patternID, bool readProtect)
{
	LogNotice("Applying requested configuration to bitstream\n");
	LogIndenter li;

	if( (part == SLG46621V) || (part == SLG46620V) || (part == SLG4662XV) )
	{
		//Set trim value reg<1981:1975>
		bitstream[246] &= 0xFE;
		bitstream[247] &= 0x01;
		bitstream[246] |= oscTrim << 7;
		bitstream[247] |= oscTrim >> 1;
		LogVerbose("Oscillator trim value: %d\n", oscTrim);

		//Set pattern ID reg<2031:2038>
		if(patternID != 0)
		{
			bitstream[253] &= 0xFE;
			bitstream[254] &= 0x01;
		}
		bitstream[253] |= patternID << 7;
		bitstream[254] |= patternID >> 1;
		LogNotice("Bitstream ID code: 0x%02x\n", patternID);

		//Set read protection reg<2039>
		//OR with the existing value: we can set the read protect bit here, but not overwrite the bit if
		//it was set by gp4par. If you REALLY need to unprotect a bitstream, do it by hand in a text editor.
		bitstream[254] |= ((uint8_t)readProtect) << 7;
		if(bitstream[254] & 0x80)
			LogNotice("Read protection: enabled\n");
		else
			LogNotice("Read protection: disabled\n");
	}

	else if(part == SLG46140V)
	{
		LogWarning("Oscillator trim value: NOT IMPLEMENTED\n");

		//Set pattern ID reg<1014:1007>
		if(patternID != 0)
		{
			bitstream[125] &= 0xFE;
			bitstream[126] &= 0x01;
		}
		bitstream[125] |= patternID << 7;
		bitstream[126] |= patternID >> 1;
		LogNotice("Bitstream ID code: 0x%02x\n", patternID);

		//Set read protection... 1015 probably, but not sure?
		LogWarning("Read protection: NOT IMPLEMENTED\n");

		/*
		//Set read protection reg<2039>
		//OR with the existing value: we can set the read protect bit here, but not overwrite the bit if
		//it was set by gp4par. If you REALLY need to unprotect a bitstream, do it by hand in a text editor.
		bitstream[254] |= ((uint8_t)readProtect) << 7;
		if(bitstream[254] & 0x80)
			LogNotice("Read protection: enabled\n");
		else
			LogNotice("Read protection: disabled\n");
		*/
	}

	else
	{
		LogError("TweakBitstream: unrecognized part\n");
		return false;
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Helper for HiL tests

/**
	@brief Selects an ADC channel and reads a single value
 */
bool SingleReadADC(hdevice hdev, unsigned int chan, double &value)
{
	if(!SelectADCChannel(hdev, chan))
		return false;
	if(!ReadADC(hdev, value))
		return false;

	return true;
}

/**
	@brief Wrapper around the test to do some board setup etc
 */
bool TestSetup(hdevice hdev, string fname, int rcOscFreq, double voltage, SilegoPart targetPart)
{
	//Make sure we have the right part
	if(!VerifyDevicePresent(hdev, targetPart))
	{
		LogNotice("Couldn't find the expected part, giving up\n");
		return false;
	}

	//Make sure the board is electrically functional
	if(!SocketTest(hdev, targetPart))
	{
		LogError("Target board self-test failed\n");
		return false;
	}

	//Trim the oscillator
	uint8_t rcFtw = 0;
	if(!TrimOscillator(hdev, targetPart, voltage, rcOscFreq, rcFtw))
		return false;

	//No need to reset board, the trim does that for us

	//Read the bitstream
	vector<uint8_t> bitstream;
	if(!ReadBitstream(fname, bitstream, targetPart))
		return false;

	//Apply the oscillator trim
	if(!TweakBitstream(bitstream, targetPart, rcFtw, 0xCC, false))
		return false;

	//Program the device
	LogNotice("Downloading bitstream to board\n");
	if(!DownloadBitstream(hdev, bitstream, DownloadMode::EMULATION))
		return false;

	//Developer board I/O pins become stuck after both SRAM and NVM programming;
	//resetting them explicitly makes LEDs and outputs work again.
	LogDebug("Resetting board I/O pins after programming\n");
	IOConfig ioConfig;
	for(size_t i = 2; i <= 20; i++)
		ioConfig.driverConfigs[i] = TP_RESET;
	if(!SetIOConfig(hdev, ioConfig))
		return false;

	//Configure the signal generator for Vdd
	LogNotice("Setting Vdd to %.3g V\n", voltage);
	if(!ConfigureSiggen(hdev, 1, voltage))
		return false;

	//Final fault check after programming
	if(!CheckStatus(hdev))
	{
		LogError("Fault condition detected during final check\n");
		return false;
	}

	LogNotice("Test setup complete\n");
	return true;
}
