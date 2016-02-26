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
 
#include "../../../src/greenpak4/Greenpak4.h"
#include <stdio.h>

int main(int /*argc*/, char* /*argv*/[])
{
	//Create the device
	Greenpak4Device device(Greenpak4Device::GREENPAK4_SLG46620);
	
	//To start, set all pins as inputs with 10k pulldowns
	unsigned int pins[] = {2, 3, 4, 5, 6, 7, 8, 9, 10, 12, 13, 14, 15, 16, 17, 18, 19, 20};
	for(auto pin : pins)
	{
		Greenpak4IOB* iob = device.GetIOB(pin);
		iob->SetPullStrength(Greenpak4IOB::PULL_10K);
		iob->SetPullDirection(Greenpak4IOB::PULL_DOWN);
		iob->SetInputThreshold(Greenpak4IOB::THRESHOLD_NORMAL);
		iob->SetSchmittTrigger(false);
	}
		
	//Set up the first LUT as an OR gate between pins 2 and 3
	Greenpak4LUT* lut = device.GetLUT2(0);
	lut->MakeOR();
	lut->SetInputSignal(0, device.GetIOB(2));
	lut->SetInputSignal(1, device.GetIOB(3));
	
	//Set pin 5 to be an output, driven by the LUT
	Greenpak4IOB* iob = device.GetIOB(5);
	iob->SetOutputEnable(true);
	iob->SetOutputSignal(lut);
	
	//Make an AND3 between 2-3-4
	lut = device.GetLUT3(0);
	lut->MakeAND();
	lut->SetInputSignal(0, device.GetIOB(2));
	lut->SetInputSignal(1, device.GetIOB(3));
	lut->SetInputSignal(2, device.GetIOB(4));
	
	//Drive it to pin 6
	iob = device.GetIOB(6);
	iob->SetOutputEnable(true);
	iob->SetOutputSignal(lut);
	
	//Write the bitstream
	device.WriteToFile("/tmp/Blinky-bits.txt");
	
	return 0;
}
