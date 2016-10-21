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

#include <log.h>
#include "gpdevboard.h"
#include <unistd.h>

using namespace std;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Status check

bool CheckStatus(hdevice hdev)
{
	LogDebug("Requesting board status\n");

	BoardStatus status;
	if(!GetStatus(hdev, status))
		return false;
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
// Initialization

/**
	@brief Connect to the board, but don't change anything
 */
hdevice OpenBoard()
{
	//Set up libusb
	if(!USBSetup())
		return NULL;

	//Try opening the board in "orange" mode
	LogNotice("\nSearching for developer board\n");
	hdevice hdev = OpenDevice(0x0f0f, 0x0006);
	if(!hdev)
	{
		//Try opening the board in "white" mode
 		hdev = OpenDevice(0x0f0f, 0x8006);
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
		hdev = OpenDevice(0x0f0f, 0x0006);
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

	SilegoPart parts[] = { SLG46140V, SLG46620V };
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
				LogNotice("Detected programmed %s (pattern ID %d)\n", PartName(part), patternId);
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

	if(detectedPart == SilegoPart::UNRECOGNIZED)
	{
		LogError("Could not detect a supported part\n");
		return false;
	}

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
