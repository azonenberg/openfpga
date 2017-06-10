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
#include <time.h>

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

bool MeasureDelay(Socket& sock, int src, int dst, CombinatorialDelay& delay, bool invertOutput = false);

bool PromptAndMeasureDelay(Socket& sock, int src, int dst, CombinatorialDelay& value);

bool MeasureCrossConnectionDelay(
	Socket& sock,
	hdevice hdev,
	unsigned int matrix,
	unsigned int index,
	unsigned int src,
	unsigned int dst,
	PTVCorner corner,
	map<PTVCorner, CombinatorialDelay>& delays);

bool MeasurePinToPinDelay(
	Socket& sock,
	hdevice hdev,
	int src,
	int dst,
	Greenpak4IOB::DriveStrength drive,
	bool schmitt,
	int voltage_mv,
	CombinatorialDelay& delay);

bool MeasureLUTDelay(
	Socket& sock,
	hdevice hdev,
	int nlut,
	int ninput,
	PTVCorner corner,
	map<PTVCorner, CombinatorialDelay>& delays);

bool MeasureInverterDelay(
	Socket& sock,
	hdevice hdev,
	int ninv,
	PTVCorner corner,
	map<PTVCorner, CombinatorialDelay>& delays);

bool MeasureDelayLineDelay(
	Socket& sock,
	hdevice hdev,
	int ndel,
	int ntap,
	bool glitchFilter,
	PTVCorner corner,
	map<PTVCorner, CombinatorialDelay>& delays);

bool ProgramAndMeasureDelay(
	Socket& sock,
	hdevice hdev,
	vector<uint8_t>& bitstream,
	int src,
	int dst,
	int voltage_mv,
	CombinatorialDelay& delay);

bool ProgramAndMeasureDelayAcrossVoltageCorners(
	Socket& sock,
	hdevice hdev,
	vector<uint8_t>& bitstream,
	int src,
	int dst,
	PTVCorner corner,
	bool subtractPadDelay,
	map<PTVCorner, CombinatorialDelay>& delays,
	bool invertOutput = false);

CombinatorialDelay GetRoundTripDelayWith2x(
	int src,
	int dst,
	PTVCorner corner,
	bool invertOutput);

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

	//Forcibly reset the whole board to make sure nothing else is using our expansion pins
	if(!ResetAllSiggens(hdev))
		return false;
	if(!IOReset(hdev))
		return false;
	if(!IOSetup(hdev))
		return false;
	if(!PowerSetup(hdev, 3300))
		return false;

	//Set up the variables
	EquationVariable r3("FPGA pin 3 to DUT rising");
	EquationVariable r4("FPGA pin 4 to DUT rising");
	EquationVariable r5("FPGA pin 5 to DUT rising");
	EquationVariable r13("FPGA pin 13 to DUT rising");
	EquationVariable r14("FPGA pin 14 to DUT rising");
	EquationVariable r15("FPGA pin 15 to DUT rising");

	EquationVariable f3("FPGA pin 3 to DUT falling");
	EquationVariable f4("FPGA pin 4 to DUT falling");
	EquationVariable f5("FPGA pin 5 to DUT falling");
	EquationVariable f13("FPGA pin 13 to DUT falling");
	EquationVariable f14("FPGA pin 14 to DUT falling");
	EquationVariable f15("FPGA pin 15 to DUT falling");

	//Measure each pair of pins individually
	CombinatorialDelay delay;
	if(!PromptAndMeasureDelay(sock, 3, 4, delay))
		return false;
	Equation e1(delay.m_rising);
	e1.AddVariable(r3);
	e1.AddVariable(r4);
	Equation e2(delay.m_falling);
	e2.AddVariable(f3);
	e2.AddVariable(f4);

	if(!PromptAndMeasureDelay(sock, 3, 5, delay))
		return false;
	Equation e3(delay.m_rising);
	e3.AddVariable(r3);
	e3.AddVariable(r5);
	Equation e4(delay.m_falling);
	e4.AddVariable(f3);
	e4.AddVariable(f5);

	if(!PromptAndMeasureDelay(sock, 4, 5, delay))
		return false;
	Equation e5(delay.m_rising);
	e5.AddVariable(r4);
	e5.AddVariable(r5);
	Equation e6(delay.m_falling);
	e6.AddVariable(f4);
	e6.AddVariable(f5);

	if(!PromptAndMeasureDelay(sock, 13, 14, delay))
		return false;
	Equation e7(delay.m_rising);
	e7.AddVariable(r13);
	e7.AddVariable(r14);
	Equation e8(delay.m_falling);
	e8.AddVariable(f13);
	e8.AddVariable(f14);

	if(!PromptAndMeasureDelay(sock, 13, 15, delay))
		return false;
	Equation e9(delay.m_rising);
	e9.AddVariable(r13);
	e9.AddVariable(r15);
	Equation e10(delay.m_falling);
	e10.AddVariable(f13);
	e10.AddVariable(f15);

	if(!PromptAndMeasureDelay(sock, 14, 15, delay))
		return false;
	Equation e11(delay.m_rising);
	e11.AddVariable(r14);
	e11.AddVariable(r15);
	Equation e12(delay.m_falling);
	e12.AddVariable(f14);
	e12.AddVariable(f15);

	//Solve the system
	EquationSystem p;
	p.AddEquation(e1);
	p.AddEquation(e2);
	p.AddEquation(e3);
	p.AddEquation(e4);
	p.AddEquation(e5);
	p.AddEquation(e6);
	p.AddEquation(e7);
	p.AddEquation(e8);
	p.AddEquation(e9);
	p.AddEquation(e10);
	p.AddEquation(e11);
	p.AddEquation(e12);
	if(!p.Solve())
		return false;

	//Extract the values
	g_devkitCal.pinDelays[3] = CombinatorialDelay(r3.m_value, f3.m_value);
	g_devkitCal.pinDelays[4] = CombinatorialDelay(r4.m_value, f4.m_value);
	g_devkitCal.pinDelays[5] = CombinatorialDelay(r5.m_value, f5.m_value);
	g_devkitCal.pinDelays[13] = CombinatorialDelay(r13.m_value, f13.m_value);
	g_devkitCal.pinDelays[14] = CombinatorialDelay(r14.m_value, f14.m_value);
	g_devkitCal.pinDelays[15] = CombinatorialDelay(r15.m_value, f15.m_value);

	//Print results
	{
		LogNotice("Calculated trace delays:\n");
		LogIndenter li2;
		for(int i=3; i<=5; i++)
		{
			LogNotice("FPGA pin %2d to DUT: %.3f ns rising, %.3f ns falling\n",
				i,
				g_devkitCal.pinDelays[i].m_rising,
				g_devkitCal.pinDelays[i].m_falling);
		}
		for(int i=13; i<=15; i++)
		{
			LogNotice("FPGA pin %2d to DUT: %.3f ns rising, %.3f ns falling\n",
				i,
				g_devkitCal.pinDelays[i].m_rising,
				g_devkitCal.pinDelays[i].m_falling);
		}
	}

	//Write to file
	LogNotice("Writing calibration to file pincal.csv\n");
	FILE* fp = fopen("pincal.csv", "w");
	for(int i=3; i<=5; i++)
		fprintf(fp, "%d,%.3f,%.3f\n", i, g_devkitCal.pinDelays[i].m_rising, g_devkitCal.pinDelays[i].m_falling);
	for(int i=13; i<=15; i++)
		fprintf(fp, "%d,%.3f,%.3f\n", i, g_devkitCal.pinDelays[i].m_rising, g_devkitCal.pinDelays[i].m_falling);
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
	float r;
	float f;
	LogNotice("Reading devkit calibration from pincal.csv (delete this file to force re-calibration)...\n");
	LogIndenter li;
	while(3 == fscanf(fp, "%d, %f, %f", &i, &r, &f))
	{
		if( (i > 20) || (i < 1) )
			continue;

		g_devkitCal.pinDelays[i] = CombinatorialDelay(r, f);

		LogNotice("FPGA pin %2d to DUT: %.3f ns rising, %.3f ns falling\n", i, r, f);
	}

	return true;
}

bool PromptAndMeasureDelay(Socket& sock, int src, int dst, CombinatorialDelay& value)
{
	LogNotice("Use a jumper to short pins %d and %d on the ZIF header\n", src, dst);

	LogIndenter li;

	WaitForKeyPress();
	if(!MeasureDelay(sock, src, dst, value))
		return false;
	LogNotice("Measured pin-to-pin: %.3f ns rising, %.3f ns falling\n", value.m_rising, value.m_falling);
	return true;
}

bool MeasureDelay(Socket& sock, int src, int dst, CombinatorialDelay& delay, bool invertOutput)
{
	//Send test parameters
	uint8_t sendbuf[3] =
	{
		(uint8_t)src,
		(uint8_t)dst,
		(uint8_t)3
	};
	uint8_t ok;

	if(invertOutput)
		sendbuf[2] = 2;
	if(!sock.SendLooped(sendbuf, sizeof(sendbuf)))
		return false;

	//Read the results back
	if(!sock.RecvLooped(&ok, 1))
		return false;
	if(!sock.RecvLooped((uint8_t*)&delay.m_rising, sizeof(float)))
		return false;
	if(!sock.RecvLooped((uint8_t*)&delay.m_falling, sizeof(float)))
		return false;

	if(!ok)
	{
		LogError("Couldn't measure delays (open circuit?)\n");
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
	CombinatorialDelay& delay)
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

bool ProgramAndMeasureDelayAcrossVoltageCorners(
	Socket& sock,
	hdevice hdev,
	vector<uint8_t>& bitstream,
	int src,
	int dst,
	PTVCorner corner,
	bool subtractPadDelay,
	map<PTVCorner, CombinatorialDelay>& delays,
	bool invertOutput)
{
	//Emulate the device
	LogVerbose("Loading new bitstream\n");
	if(!DownloadBitstream(hdev, bitstream, DownloadMode::EMULATION))
		return false;

	for(auto v : g_testVoltages)
	{
		corner.SetVoltage(v);
		if(!PostProgramSetup(hdev, v))
			return false;

		//wait a bit to let voltages stabilize
		usleep (1000 * 50);

		//TODO: falling edge delays here!
		CombinatorialDelay delay;
		if(!MeasureDelay(sock, src, dst, delay, invertOutput))
			return false;

		//Remove off-die delays if requested
		if(subtractPadDelay)
		{
			//Subtract PCB trace and IO buffer delays
			delay -= GetRoundTripDelayWith2x(src, dst, corner, invertOutput);
		}

		delays[corner] = delay;
	}

	return true;
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
	//TODO: more pins?
	CombinatorialDelay delay;
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
				EquationVariable ribuf("Input buffer delay rising");
				EquationVariable robuf_x1("x1 output buffer delay rising");
				EquationVariable robuf_x2("x2 output buffer delay rising");
				EquationVariable rschmitt("Schmitt trigger delay rising");
				EquationVariable fibuf("Input buffer delay falling");
				EquationVariable fobuf_x1("x1 output buffer delay falling");
				EquationVariable fobuf_x2("x2 output buffer delay falling");
				EquationVariable fschmitt("Schmitt trigger delay falling");

				//Set up relationships between the various signals (see assumptions above)
				//TODO: FIB probing to get more accurate ratio
				float ibuf_to_obuf_ratio = -1.55;
				Equation e1(0);
				e1.AddVariable(ribuf, 1);
				e1.AddVariable(robuf_x2, ibuf_to_obuf_ratio);

				Equation e2(0);
				e2.AddVariable(fibuf, 1);
				e2.AddVariable(fobuf_x2, ibuf_to_obuf_ratio);

				//Gather data from the DUT
				if(!MeasurePinToPinDelay(sock, hdev, src, dst,
					Greenpak4IOB::DRIVE_1X, false, corner.GetVoltage(), delay))
				{
					return false;
				}
				Equation e3(delay.m_rising);
				e3.AddVariable(ribuf);
				e3.AddVariable(robuf_x1);
				Equation e4(delay.m_falling);
				e4.AddVariable(fibuf);
				e4.AddVariable(fobuf_x1);

				if(!MeasurePinToPinDelay(sock, hdev, src, dst,
					Greenpak4IOB::DRIVE_2X, false, corner.GetVoltage(), delay))
				{
					return false;
				}
				Equation e5(delay.m_rising);
				e5.AddVariable(ribuf);
				e5.AddVariable(robuf_x2);
				Equation e6(delay.m_falling);
				e6.AddVariable(fibuf);
				e6.AddVariable(fobuf_x2);

				if(!MeasurePinToPinDelay(sock, hdev, src, dst,
					Greenpak4IOB::DRIVE_2X, true, corner.GetVoltage(), delay))
				{
					return false;
				}
				Equation e7(delay.m_rising);
				e7.AddVariable(ribuf);
				e7.AddVariable(rschmitt);
				e7.AddVariable(robuf_x2);
				Equation e8(delay.m_falling);
				e8.AddVariable(fibuf);
				e8.AddVariable(fschmitt);
				e8.AddVariable(fobuf_x2);

				//Create and solve the equation system
				EquationSystem sys;
				sys.AddEquation(e1);
				sys.AddEquation(e2);
				sys.AddEquation(e3);
				sys.AddEquation(e4);
				sys.AddEquation(e5);
				sys.AddEquation(e6);
				sys.AddEquation(e7);
				sys.AddEquation(e8);
				if(!sys.Solve())
					return false;

				//Save combinatorial delays for these pins
				auto iob = g_calDevice.GetIOB(src);
				iob->AddCombinatorialDelay(
					"IO",
					"OUT",
					corner,
					CombinatorialDelay(ribuf.m_value, fibuf.m_value));
				iob->SetSchmittTriggerDelay(
					corner,
					CombinatorialDelay(rschmitt.m_value, fschmitt.m_value));
				iob->SetOutputDelay(
					Greenpak4IOB::DRIVE_1X,
					corner,
					CombinatorialDelay(robuf_x1.m_value, fobuf_x1.m_value));
				iob->SetOutputDelay(
					Greenpak4IOB::DRIVE_2X,
					corner,
					CombinatorialDelay(robuf_x2.m_value, fobuf_x2.m_value));
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
	CombinatorialDelay& delay)
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
	delay -= g_devkitCal.pinDelays[src];
	delay -= g_devkitCal.pinDelays[dst];

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Characterize cross-connections

bool MeasureCrossConnectionDelays(Socket& sock, hdevice hdev)
{
	LogNotice("Measuring cross-connection delays...\n");
	LogIndenter li;

	//Test conditions (TODO: pass this in from somewhere?)
	PTVCorner corner(PTVCorner::SPEED_TYPICAL, 25, 3300);

	//East
	for(int i=0; i<10; i++)
	{
		map<PTVCorner, CombinatorialDelay> delays;
		if(!MeasureCrossConnectionDelay(sock, hdev, 0, i, 3, 13, corner, delays))
			return false;
		for(auto it : delays)
			g_calDevice.GetCrossConnection(0, i)->AddCombinatorialDelay("I", "O", it.first, it.second);
	}

	for(int i=0; i<10; i++)
	{
		map<PTVCorner, CombinatorialDelay> delays;
		if(!MeasureCrossConnectionDelay(sock, hdev, 1, i, 13, 3, corner, delays))
			return false;
		for(auto it : delays)
			g_calDevice.GetCrossConnection(1, i)->AddCombinatorialDelay("I", "O", it.first, it.second);
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
	map<PTVCorner, CombinatorialDelay>& delays)
{
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

	//Get the delays
	if(!ProgramAndMeasureDelayAcrossVoltageCorners(sock, hdev, bitstream, src, dst, corner, true, delays))
		return false;

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Helper for subtracting I/O pad and test fixture delays from a measurement

CombinatorialDelay GetRoundTripDelayWith2x(
	int src,
	int dst,
	PTVCorner corner,
	bool invertOutput)
{
	//PCB trace delay at each end of the line
	CombinatorialDelay delay = g_devkitCal.pinDelays[dst];
	if(invertOutput)
		delay = CombinatorialDelay(delay.m_falling, delay.m_rising);
	delay += g_devkitCal.pinDelays[src];

	//I/O buffer delay at each end of the line
	//TODO: import calibration from the Greenpak4Device and/or reset it?
	CombinatorialDelay d;
	auto srciob = g_calDevice.GetIOB(src);
	srciob->SetSchmittTrigger(false);
	if(!srciob->GetCombinatorialDelay("IO", "OUT", corner, d))
		return false;
	delay += d;

	auto dstiob = g_calDevice.GetIOB(dst);
	dstiob->SetDriveType(Greenpak4IOB::DRIVE_PUSHPULL);
	dstiob->SetDriveStrength(Greenpak4IOB::DRIVE_2X);
	if(!dstiob->GetCombinatorialDelay("IN", "IO", corner, d))
		return false;

	if(invertOutput)
		delay += CombinatorialDelay(d.m_falling, d.m_rising);
	else
		delay += d;

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

	//Characterize each LUT
	//Don't forget to only measure pins it actually has!
	for(unsigned int nlut = 0; nlut < g_calDevice.GetLUTCount(); nlut++)
	{
		auto baselut = g_calDevice.GetLUT(nlut);
		auto lut = GetRealLUT(baselut);
		for(unsigned int npin = 0; npin < lut->GetOrder(); npin ++)
		{
			//Test conditions (TODO: pass this in from somewhere?)
			PTVCorner corner(PTVCorner::SPEED_TYPICAL, 25, 3300);

			map<PTVCorner, CombinatorialDelay> delays;
			if(!MeasureLUTDelay(sock, hdev, nlut, npin, corner, delays))
				return false;

			//For now, the parent (in case of a muxed lut etc) stores all timing data
			//TODO: does this make the most sense?
			for(auto it : delays)
			{
				char portname[] = "IN0";
				portname[2] += npin;
				baselut->AddCombinatorialDelay(portname, "OUT", it.first, it.second);
			}
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
	map<PTVCorner, CombinatorialDelay>& delays)
{
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

	//Get the delays
	if(!ProgramAndMeasureDelayAcrossVoltageCorners(sock, hdev, bitstream, src, dst, corner, true, delays))
		return false;

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Characterize inverters

bool MeasureInverterDelays(Socket& sock, hdevice hdev)
{
	LogNotice("Measuring inverter delays...\n");
	LogIndenter li;

	//Test conditions (TODO: pass this in from somewhere?)
	PTVCorner corner(PTVCorner::SPEED_TYPICAL, 25, 3300);

	for(unsigned int ninv = 0; ninv < g_calDevice.GetInverterCount(); ninv++)
	{
		map<PTVCorner, CombinatorialDelay> delays;
		if(!MeasureInverterDelay(sock, hdev, ninv, corner, delays))
			return false;

		for(auto it : delays)
			g_calDevice.GetInverter(ninv)->AddCombinatorialDelay("IN", "OUT", it.first, it.second);
	}

	return true;
}

bool MeasureInverterDelay(
	Socket& sock,
	hdevice hdev,
	int ninv,
	PTVCorner corner,
	map<PTVCorner, CombinatorialDelay>& delays)
{
	//Create the device object
	Greenpak4Device device(part, unused_pull, unused_drive);
	device.SetIOPrecharge(false);
	device.SetDisableChargePump(false);
	device.SetLDOBypass(false);
	device.SetNVMRetryCount(1);

	//Look up the DUT
	auto inv = device.GetInverter(ninv);

	//See which half of the device it's in. Use pins 3/4 or 13/14 as appropriate
	int src = 3;
	int dst = 4;
	if(inv->GetMatrix() == 1)
	{
		src = 13;
		dst = 14;
	}

	//Configure the input pin
	auto vss = device.GetGround();
	auto vdd = device.GetPower();
	auto srciob = device.GetIOB(src);
	srciob->SetInput("OE", vss);
	auto din = srciob->GetOutput("OUT");

	//Configure the inverter
	inv->SetInput("IN", din);

	//Configure the output pin
	auto dstiob = device.GetIOB(dst);
	dstiob->SetInput("IN", inv->GetOutput("OUT"));
	dstiob->SetInput("OE", vdd);
	dstiob->SetDriveType(Greenpak4IOB::DRIVE_PUSHPULL);
	dstiob->SetDriveStrength(Greenpak4IOB::DRIVE_2X);

	//Generate a bitstream
	vector<uint8_t> bitstream;
	device.WriteToBuffer(bitstream, 0, false);
	device.WriteToFile("/tmp/test.txt", 0, false);			//for debug in case of failure

	//Get the delays
	if(!ProgramAndMeasureDelayAcrossVoltageCorners(sock, hdev, bitstream, src, dst, corner, true, delays, true))
		return false;

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Characterize delay lines

bool MeasureDelayLineDelays(Socket& sock, hdevice hdev)
{
	LogNotice("Measuring delay line delays...\n");
	LogIndenter li;

	//Test conditions (TODO: pass this in from somewhere?)
	PTVCorner corner(PTVCorner::SPEED_TYPICAL, 25, 3300);

	for(unsigned int ndel = 0; ndel < g_calDevice.GetDelayCount(); ndel++)
	{
		auto line = g_calDevice.GetDelay(ndel);

		for(unsigned int ntap=1; ntap<=4; ntap ++)
		{
			//First round: no glitch filter
			map<PTVCorner, CombinatorialDelay> delays;
			if(!MeasureDelayLineDelay(sock, hdev, ndel, ntap, false, corner, delays))
				return false;
			for(auto it : delays)
				line->SetUnfilteredDelay(ntap, it.first, it.second);

			//Do it again with the glitch filter on
			delays.clear();
			if(!MeasureDelayLineDelay(sock, hdev, ndel, ntap, true, corner, delays))
				return false;
			for(auto it : delays)
				line->SetFilteredDelay(ntap, it.first, it.second);
		}
	}

	return true;
}

bool MeasureDelayLineDelay(
	Socket& sock,
	hdevice hdev,
	int ndel,
	int ntap,
	bool glitchFilter,
	PTVCorner corner,
	map<PTVCorner, CombinatorialDelay>& delays)
{
	//Create the device object
	Greenpak4Device device(part, unused_pull, unused_drive);
	device.SetIOPrecharge(false);
	device.SetDisableChargePump(false);
	device.SetLDOBypass(false);
	device.SetNVMRetryCount(1);

	//Look up the DUT
	auto delay = device.GetDelay(ndel);

	//See which half of the device it's in. Use pins 3/4 or 13/14 as appropriate
	int src = 3;
	int dst = 4;
	if(delay->GetMatrix() == 1)
	{
		src = 13;
		dst = 14;
	}

	//Configure the input pin
	auto vss = device.GetGround();
	auto srciob = device.GetIOB(src);
	srciob->SetInput("OE", vss);
	auto din = srciob->GetOutput("OUT");

	//Configure the delay line
	delay->SetInput("IN", din);
	delay->SetTap(ntap);
	delay->SetGlitchFilter(glitchFilter);

	//Configure the output pin
	auto vdd = device.GetPower();
	auto dstiob = device.GetIOB(dst);
	dstiob->SetInput("IN", delay->GetOutput("OUT"));
	dstiob->SetInput("OE", vdd);
	dstiob->SetDriveType(Greenpak4IOB::DRIVE_PUSHPULL);
	dstiob->SetDriveStrength(Greenpak4IOB::DRIVE_2X);

	//Generate a bitstream
	vector<uint8_t> bitstream;
	device.WriteToBuffer(bitstream, 0, false);
	//device.WriteToFile("/tmp/test.txt", 0, false);			//for debug in case of failure

	//Get the delays
	if(!ProgramAndMeasureDelayAcrossVoltageCorners(sock, hdev, bitstream, src, dst, corner, true, delays))
		return false;

	return true;
}
