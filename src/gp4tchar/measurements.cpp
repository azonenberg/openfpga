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

#include "gp4tchar.h"
#include "solver.h"

using namespace std;

//Device configuration
Greenpak4Device::GREENPAK4_PART part = Greenpak4Device::GREENPAK4_SLG46620;
Greenpak4IOB::PullDirection unused_pull = Greenpak4IOB::PULL_DOWN;
Greenpak4IOB::PullStrength  unused_drive = Greenpak4IOB::PULL_1M;

/**
	@brief The "calibration device"

	We never actually modify the netlist or create a bitstream from this. It's just a convenient repository to put
	timing data in before we serialize to disk.
 */
Greenpak4Device g_calDevice(part, unused_pull, unused_drive);

bool PromptAndMeasureDelay(Socket& sock, int src, int dst, float& value);

bool MeasureCrossConnectionDelay(
	Socket& sock,
	hdevice hdev,
	unsigned int matrix,
	unsigned int index,
	unsigned int src,
	unsigned int dst,
	PTVCorner corner,
	float& delay);

bool MeasurePinToPinDelay(
	Socket& sock,
	hdevice hdev,
	int src,
	int dst,
	Greenpak4IOB::DriveStrength drive,
	bool schmitt,
	int voltage_mv,
	float& delay);

bool MeasureLUTDelay(
	Socket& sock,
	hdevice hdev,
	int nlut,
	int ninput,
	PTVCorner corner,
	float& delay);

bool ProgramAndMeasureDelay(
	Socket& sock,
	hdevice hdev,
	vector<uint8_t>& bitstream,
	int src,
	int dst,
	int voltage_mv,
	float& delay);

float GetRoundTripDelayWith2x(
	int src,
	int dst,
	PTVCorner corner);

Greenpak4LUT* GetRealLUT(Greenpak4BitstreamEntity* lut);

//Voltages to test at
//For now, 3.3 +/- 150 mV
const int g_testVoltages[] = {3150, 3300, 3450};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Initial measurement calibration

bool CalibrateTraceDelays(Socket& sock, hdevice hdev)
{
	LogNotice("Calibrate FPGA-to-DUT trace delays\n");
	LogIndenter li;

	if(!IOSetup(hdev))
		return false;

	//Set up the variables
	EquationVariable p3("FPGA pin 3 to DUT rising");
	EquationVariable p4("FPGA pin 4 to DUT rising");
	EquationVariable p5("FPGA pin 5 to DUT rising");
	EquationVariable p13("FPGA pin 13 to DUT rising");
	EquationVariable p14("FPGA pin 14 to DUT rising");
	EquationVariable p15("FPGA pin 15 to DUT rising");

	//Measure each pair of pins individually
	float delay;
	if(!PromptAndMeasureDelay(sock, 3, 4, delay))
		return false;
	Equation e1(delay);
	e1.AddVariable(p3);
	e1.AddVariable(p4);

	if(!PromptAndMeasureDelay(sock, 3, 5, delay))
		return false;
	Equation e2(delay);
	e2.AddVariable(p3);
	e2.AddVariable(p5);

	if(!PromptAndMeasureDelay(sock, 4, 5, delay))
		return false;
	Equation e3(delay);
	e3.AddVariable(p4);
	e3.AddVariable(p5);

	if(!PromptAndMeasureDelay(sock, 13, 14, delay))
		return false;
	Equation e4(delay);
	e4.AddVariable(p13);
	e4.AddVariable(p14);

	if(!PromptAndMeasureDelay(sock, 13, 15, delay))
		return false;
	Equation e5(delay);
	e5.AddVariable(p13);
	e5.AddVariable(p15);

	if(!PromptAndMeasureDelay(sock, 14, 15, delay))
		return false;
	Equation e6(delay);
	e6.AddVariable(p14);
	e6.AddVariable(p15);

	//Solve the system
	EquationSystem p;
	p.AddEquation(e1);
	p.AddEquation(e2);
	p.AddEquation(e3);
	p.AddEquation(e4);
	p.AddEquation(e5);
	p.AddEquation(e6);
	if(!p.Solve())
		return false;

	//Extract the values
	g_devkitCal.pinDelays[3] = p3.m_value;
	g_devkitCal.pinDelays[4] = p4.m_value;
	g_devkitCal.pinDelays[5] = p5.m_value;
	g_devkitCal.pinDelays[13] = p13.m_value;
	g_devkitCal.pinDelays[14] = p14.m_value;
	g_devkitCal.pinDelays[15] = p15.m_value;

	//Print results
	{
		LogNotice("Calculated trace delays:\n");
		LogIndenter li2;
		for(int i=3; i<=5; i++)
			LogNotice("FPGA pin %2d to DUT rising: %.3f ns\n", i, g_devkitCal.pinDelays[i].m_rising);
		for(int i=13; i<=15; i++)
			LogNotice("FPGA pin %2d to DUT rising: %.3f ns\n", i, g_devkitCal.pinDelays[i].m_rising);
	}

	//Write to file
	LogNotice("Writing calibration to file pincal.csv\n");
	FILE* fp = fopen("pincal.csv", "w");
	for(int i=3; i<=5; i++)
		fprintf(fp, "%d,%.3f\n", i, g_devkitCal.pinDelays[i].m_rising);
	for(int i=13; i<=15; i++)
		fprintf(fp, "%d,%.3f\n", i, g_devkitCal.pinDelays[i].m_rising);
	fclose(fp);

	//Prompt to put the actual DUT in
	LogNotice("Insert a SLG46620 into the socket\n");
	WaitForKeyPress();

	return true;
}

bool ReadTraceDelays()
{
	FILE* fp = fopen("pincal.csv", "r");
	if(!fp)
		return false;

	int i;
	float f;
	LogNotice("Reading devkit calibration from pincal.csv (delete this file to force re-calibration)...\n");
	LogIndenter li;
	while(2 == fscanf(fp, "%d, %f", &i, &f))
	{
		if( (i > 20) || (i < 1) )
			continue;

		g_devkitCal.pinDelays[i] = CombinatorialDelay(f, -1);

		LogNotice("FPGA pin %2d to DUT rising: %.3f ns\n", i, f);
	}

	return true;
}

bool PromptAndMeasureDelay(Socket& sock, int src, int dst, float& value)
{
	LogNotice("Use a jumper to short pins %d and %d on the ZIF header\n", src, dst);

	LogIndenter li;

	WaitForKeyPress();
	if(!MeasureDelay(sock, src, dst, value))
		return false;
	LogNotice("Measured pin-to-pin: %.3f ns\n", value);
	return true;
}

bool MeasureDelay(Socket& sock, int src, int dst, float& delay)
{
	//Send test parameters
	uint8_t		drive = src;
	uint8_t		sample = dst;
	if(!sock.SendLooped(&drive, 1))
		return false;
	if(!sock.SendLooped(&sample, 1))
		return false;

	//Read the results back
	uint8_t ok;
	if(!sock.RecvLooped(&ok, 1))
		return false;
	if(!sock.RecvLooped((uint8_t*)&delay, sizeof(delay)))
		return false;

	if(!ok)
	{
		LogError("Couldn't measure delay (open circuit?)\n");
		return false;
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Load the bitstream onto the device and measure a pin-to-pin delay

bool ProgramAndMeasureDelay(
	Socket& sock,
	hdevice hdev,
	vector<uint8_t>& bitstream,
	int src,
	int dst,
	int voltage_mv,
	float& delay)
{
	//Emulate the device
	LogVerbose("Loading new bitstream\n");
	if(!DownloadBitstream(hdev, bitstream, DownloadMode::EMULATION))
		return false;
	if(!PostProgramSetup(hdev, voltage_mv))
		return false;

	usleep (1000 * 10);

	//Measure delay between the affected pins
	return MeasureDelay(sock, src, dst, delay);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Characterize I/O buffers

bool MeasurePinToPinDelays(Socket& sock, hdevice hdev)
{
	LogNotice("Measuring pin-to-pin delays (through same crossbar)...\n");
	LogIndenter li;

	/*
		ASSUMPTIONS
			x2 I/O buffer is exactly twice the speed of x1
			Schmitt trigger vs input buffer is same speed ratio as CoolRunner-II (also 180nm but UMC vs TSMC)

		XC2C32A data for reference (-4 speed)
			Input buffer delay: 1.3 ns, plus 0.5 ns for LVCMOS33 = 1.8 ns
			Schmitt trigger: 3 ns, plus input buffer = 4.8 ns
			Output buffer delay: 1.8 ns, plus 1 ns for LVCMOS33 = 2.8 ns
			Slow-slew output: 4 ns

			Extrapolation: input buffer delay = 1.55x output buffer delay
	 */

	int pins[] = {3, 4, 5, 13, 14, 15};

	//TODO: ↓ edges
	//TODO: x4 drive (if supported on this pin)
	//TODO: Tri-states and open-drain/open-source outputs
	float delay;
	for(auto src : pins)
	{
		for(auto dst : pins)
		{
			//Can't do loopback
			if(src == dst)
				continue;

			//If the pins are on opposite sides of the device, skip it
			//(no cross connections, we only want matrix + buffer delay)
			if(src < 10 && dst > 10)
				continue;
			if(src > 10 && dst < 10)
				continue;

			//Try various voltages
			for(auto voltage : g_testVoltages)
			{
				//Test conditions (TODO: pass this in from somewhere?)
				PTVCorner corner(PTVCorner::SPEED_TYPICAL, 25, voltage);

				//Create some variables
				EquationVariable ibuf("Input buffer delay");
				EquationVariable obuf_x1("x1 output buffer delay");
				EquationVariable obuf_x2("x2 output buffer delay");
				EquationVariable schmitt("Schmitt trigger delay");

				//Set up relationships between the various signals (see assumptions above)
				//TODO: FIB probing to get more accurate parameters here
				Equation e1(0);
				e1.AddVariable(ibuf, 1);
				e1.AddVariable(obuf_x2, -1.55);

				//Gather data from the DUT
				if(!MeasurePinToPinDelay(sock, hdev, src, dst,
					Greenpak4IOB::DRIVE_1X, false, corner.GetVoltage(), delay))
				{
					return false;
				}
				Equation e2(delay);
				e2.AddVariable(ibuf);
				e2.AddVariable(obuf_x1);

				if(!MeasurePinToPinDelay(sock, hdev, src, dst,
					Greenpak4IOB::DRIVE_2X, false, corner.GetVoltage(), delay))
				{
					return false;
				}
				Equation e3(delay);
				e3.AddVariable(ibuf);
				e3.AddVariable(obuf_x2);

				if(!MeasurePinToPinDelay(sock, hdev, src, dst,
					Greenpak4IOB::DRIVE_2X, true, corner.GetVoltage(), delay))
				{
					return false;
				}
				Equation e4(delay);
				e4.AddVariable(ibuf);
				e4.AddVariable(schmitt);
				e4.AddVariable(obuf_x2);

				//Create and solve the equation system
				EquationSystem sys;
				sys.AddEquation(e1);
				sys.AddEquation(e2);
				sys.AddEquation(e3);
				sys.AddEquation(e4);
				if(!sys.Solve())
					return false;

				//Save combinatorial delays for these pins
				auto iob = g_calDevice.GetIOB(src);
				iob->AddCombinatorialDelay("IO", "OUT", corner, CombinatorialDelay(ibuf.m_value, -1));
				iob->SetSchmittTriggerDelay(corner, CombinatorialDelay(schmitt.m_value, -1));
				iob->SetOutputDelay(Greenpak4IOB::DRIVE_1X, corner, CombinatorialDelay(obuf_x1.m_value, -1));
				iob->SetOutputDelay(Greenpak4IOB::DRIVE_2X, corner, CombinatorialDelay(obuf_x2.m_value, -1));
			}
		}
	}

	return true;
}

/**
	@brief Measure the delay for a single (src, dst) pin tuple
 */
bool MeasurePinToPinDelay(
	Socket& sock,
	hdevice hdev,
	int src,
	int dst,
	Greenpak4IOB::DriveStrength drive,
	bool schmitt,
	int voltage_mv,
	float& delay)
{
	delay = -1;

	//Create the device object
	Greenpak4Device device(part, unused_pull, unused_drive);
	device.SetIOPrecharge(false);
	device.SetDisableChargePump(false);
	device.SetLDOBypass(false);
	device.SetNVMRetryCount(1);

	//Configure the input pin
	auto vss = device.GetGround();
	auto srciob = device.GetIOB(src);
	srciob->SetInput("OE", vss);
	srciob->SetSchmittTrigger(schmitt);
	auto din = srciob->GetOutput("OUT");

	//Configure the output pin
	auto vdd = device.GetPower();
	auto dstiob = device.GetIOB(dst);
	dstiob->SetInput("IN", din);
	dstiob->SetInput("OE", vdd);
	dstiob->SetDriveType(Greenpak4IOB::DRIVE_PUSHPULL);
	dstiob->SetDriveStrength(drive);

	//Generate a bitstream
	vector<uint8_t> bitstream;
	device.WriteToBuffer(bitstream, 0, false);
	//device.WriteToFile("/tmp/test.txt", 0, false);			//for debug in case of failure

	//Get the delay
	if(!ProgramAndMeasureDelay(sock, hdev, bitstream, src, dst, voltage_mv, delay))
		return false;

	//Subtract the PCB trace delay at each end of the line
	delay -= g_devkitCal.pinDelays[src].m_rising;
	delay -= g_devkitCal.pinDelays[dst].m_rising;

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Characterize cross-connections

bool MeasureCrossConnectionDelays(Socket& sock, hdevice hdev)
{
	LogNotice("Measuring cross-connection delays...\n");
	LogIndenter li;

	//Try various voltages
	for(auto voltage : g_testVoltages)
	{
		//Test conditions (TODO: pass this in from somewhere?)
		PTVCorner corner(PTVCorner::SPEED_TYPICAL, 25, voltage);

		//East
		float d;
		for(int i=0; i<10; i++)
		{
			if(!MeasureCrossConnectionDelay(sock, hdev, 0, i, 3, 13, corner, d))
				return false;
			g_calDevice.GetCrossConnection(0, i)->AddCombinatorialDelay("I", "O", corner, CombinatorialDelay(d, -1));
		}

		for(int i=0; i<10; i++)
		{
			if(!MeasureCrossConnectionDelay(sock, hdev, 1, i, 13, 3, corner, d))
				return false;
			g_calDevice.GetCrossConnection(1, i)->AddCombinatorialDelay("I", "O", corner, CombinatorialDelay(d, -1));
		}
	}

	return true;
}

bool MeasureCrossConnectionDelay(
	Socket& sock,
	hdevice hdev,
	unsigned int matrix,
	unsigned int index,
	unsigned int src,
	unsigned int dst,
	PTVCorner corner,
	float& delay)
{
	delay = -1;

	//Create the device object
	Greenpak4Device device(part, unused_pull, unused_drive);
	device.SetIOPrecharge(false);
	device.SetDisableChargePump(false);
	device.SetLDOBypass(false);
	device.SetNVMRetryCount(1);

	//Configure the input pin
	auto vss = device.GetGround();
	auto srciob = device.GetIOB(src);
	srciob->SetInput("OE", vss);
	auto din = srciob->GetOutput("OUT");

	//Configure the cross-connection
	auto xc = device.GetCrossConnection(matrix, index);
	xc->SetInput("I", din);

	//Configure the output pin
	auto vdd = device.GetPower();
	auto dstiob = device.GetIOB(dst);
	dstiob->SetInput("IN", xc->GetOutput("O"));
	dstiob->SetInput("OE", vdd);
	dstiob->SetDriveType(Greenpak4IOB::DRIVE_PUSHPULL);
	dstiob->SetDriveStrength(Greenpak4IOB::DRIVE_2X);

	//Generate a bitstream
	vector<uint8_t> bitstream;
	device.WriteToBuffer(bitstream, 0, false);
	//device.WriteToFile("/tmp/test.txt", 0, false);			//for debug in case of failure

	//Get the delay
	if(!ProgramAndMeasureDelay(sock, hdev, bitstream, src, dst, corner.GetVoltage(), delay))
		return false;

	//Subtract PCB and I/O buffer delays
	delay -= GetRoundTripDelayWith2x(src, dst, corner);

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Helper for subtracting I/O pad and test fixture delays from a measurement

float GetRoundTripDelayWith2x(
	int src,
	int dst,
	PTVCorner corner)
{
	//Subtract the PCB trace delay at each end of the line
	float delay = g_devkitCal.pinDelays[src].m_rising;
	delay += g_devkitCal.pinDelays[dst].m_rising;

	//Subtract the I/O buffer delay at each end of the line
	//TODO: import calibration from the Greenpak4Device and/or reset it?
	CombinatorialDelay d;
	auto srciob = g_calDevice.GetIOB(src);
	srciob->SetSchmittTrigger(false);
	if(!srciob->GetCombinatorialDelay("IO", "OUT", corner, d))
		return false;
	delay += d.m_rising;

	auto dstiob = g_calDevice.GetIOB(dst);
	dstiob->SetDriveType(Greenpak4IOB::DRIVE_PUSHPULL);
	dstiob->SetDriveStrength(Greenpak4IOB::DRIVE_2X);
	if(!dstiob->GetCombinatorialDelay("IN", "IO", corner, d))
		return false;
	delay += d.m_rising;

	return delay;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Characterize LUTs

/**
	@brief Gets the actual LUT object out of an entity that either is, or contains, a LUT
 */
Greenpak4LUT* GetRealLUT(Greenpak4BitstreamEntity* lut)
{
	//First, just try casting.
	auto reallut = dynamic_cast<Greenpak4LUT*>(lut);
	if(reallut)
		return reallut;

	//It would be too easy if that just worked, though... what if it's a muxed cell?
	auto pair = dynamic_cast<Greenpak4PairedEntity*>(lut);
	if(pair)
	{
		reallut = dynamic_cast<Greenpak4LUT*>(pair->GetEntity("GP_2LUT"));
		if(reallut)
			return reallut;
	}

	//Something is wrong
	return NULL;
}

bool MeasureLUTDelays(Socket& sock, hdevice hdev)
{
	LogNotice("Measuring LUT delays...\n");
	LogIndenter li;

	//Test conditions (TODO: pass this in from somewhere?)
	int voltage = 3300;
	PTVCorner corner(PTVCorner::SPEED_TYPICAL, 25, voltage);

	//Characterize each LUT
	//Don't forget to only measure pins it actually has!
	float delay;
	for(unsigned int nlut = 0; nlut < g_calDevice.GetLUTCount(); nlut++)
	{
		auto baselut = g_calDevice.GetLUT(nlut);
		auto lut = GetRealLUT(baselut);
		for(unsigned int npin = 0; npin < lut->GetOrder(); npin ++)
		{
			if(!MeasureLUTDelay(sock, hdev, nlut, npin, corner, delay))
				return false;

			//For now, the parent (in case of a muxed lut etc) stores all timing data
			//TODO: does this make the most sense?
			char portname[] = "IN0";
			portname[2] += npin;
			baselut->AddCombinatorialDelay(portname, "OUT", corner, CombinatorialDelay(delay, -1));
		}
	}

	return true;
}

bool MeasureLUTDelay(
	Socket& sock,
	hdevice hdev,
	int nlut,
	int ninput,
	PTVCorner corner,
	float& delay)
{
	delay = -1;

	//Create the device object
	Greenpak4Device device(part, unused_pull, unused_drive);
	device.SetIOPrecharge(false);
	device.SetDisableChargePump(false);
	device.SetLDOBypass(false);
	device.SetNVMRetryCount(1);

	//Look up the LUT
	auto lut = GetRealLUT(device.GetLUT(nlut));

	//See which half of the device it's in. Use pins 3/4 or 13/14 as appropriate
	int src = 3;
	int dst = 4;
	if(lut->GetMatrix() == 1)
	{
		src = 13;
		dst = 14;
	}

	//Configure the input pin
	auto vss = device.GetGround();
	auto srciob = device.GetIOB(src);
	srciob->SetInput("OE", vss);
	auto din = srciob->GetOutput("OUT");

	//Configure the LUT
	lut->MakeXOR();
	lut->SetInput("IN0", vss);
	lut->SetInput("IN1", vss);
	lut->SetInput("IN2", vss);
	lut->SetInput("IN3", vss);
	char portname[] = "IN0";
	portname[2] += ninput;
	lut->SetInput(portname, din);

	//Configure the output pin
	auto vdd = device.GetPower();
	auto dstiob = device.GetIOB(dst);
	dstiob->SetInput("IN", lut->GetOutput("OUT"));
	dstiob->SetInput("OE", vdd);
	dstiob->SetDriveType(Greenpak4IOB::DRIVE_PUSHPULL);
	dstiob->SetDriveStrength(Greenpak4IOB::DRIVE_2X);

	//Generate a bitstream
	vector<uint8_t> bitstream;
	device.WriteToBuffer(bitstream, 0, false);
	//device.WriteToFile("/tmp/test.txt", 0, false);			//for debug in case of failure

	//Get the delay
	if(!ProgramAndMeasureDelay(sock, hdev, bitstream, src, dst, corner.GetVoltage(), delay))
		return false;

	//Subtract PCB trace and IO buffer delays
	delay -= GetRoundTripDelayWith2x(src, dst, corner);

	return true;
}
