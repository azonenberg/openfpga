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

#ifndef TimingData_h
#define TimingData_h

#include "../xbpar/CombinatorialDelay.h"

//Devkit calibration
class DevkitCalibration
{
public:
	DevkitCalibration();

	//Delays from FPGA to DUT pins
	CombinatorialDelay	pinDelays[21];		//0 is unused so we can use 1-based pin numbering like the chip does
											//For now, only pins 3-4-5 and 13-14-15 are used
};

//DUT measurements
typedef std::pair<int, int> PinPair;
class DeviceProperties
{
public:

	/*
	//Map from (src, dst) pin to delay tuple
	typedef std::map<PinPair, DelayPair> IODelayMap;

	//Map from drive strength to delay tuples
	std::map<Greenpak4IOB::DriveStrength, IODelayMap> ioDelays;
	*/
};

/*
//Delays from FPGA to DUT, one way, per pin
//TODO: DelayPair for these

//Delays within the device for input/output buffers
//Map from (src, dst) to delay
map<pair<int, int>, CellDelay> g_pinToPinDelaysX1;
map<pair<int, int>, CellDelay> g_pinToPinDelaysX2;
//TODO: x4 drive strength

//Delay through each cross connection
DelayPair g_eastXconnDelays[10];
DelayPair g_westXconnDelays[10];

//Propagation delay through each LUT to the output
map<Greenpak4LUT*, CellDelay> g_lutDelays;
*/

#endif
