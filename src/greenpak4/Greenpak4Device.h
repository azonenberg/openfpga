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

#ifndef Greenpak4Device_h
#define Greenpak4Device_h

#include <algorithm>
#include <vector>
#include <map>

/**
	@brief Top level class for an entire Silego Greenpak4 device
 */
class Greenpak4Device
{
public:

	enum GREENPAK4_PART
	{
		GREENPAK4_SLG46140,
		GREENPAK4_SLG46620,
		GREENPAK4_SLG46621
	};

	Greenpak4Device(
		Greenpak4Device::GREENPAK4_PART part,
		Greenpak4IOB::PullDirection default_pull = Greenpak4IOB::PULL_NONE,
		Greenpak4IOB::PullStrength default_drive = Greenpak4IOB::PULL_1M);

	virtual ~Greenpak4Device();

	//Write to a bitfile
	bool WriteToFile(std::string fname, uint8_t userid, bool readProtect);

	//Write to an in-memory array
	bool WriteToBuffer(std::vector<uint8_t>& bitstream, uint8_t userid, bool readProtect);

	GREENPAK4_PART GetPart()
	{ return m_part; }

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// POWER RAILS

	//Get the power rail for a binary constant
	Greenpak4BitstreamEntity* GetPowerRail(bool rail);

	Greenpak4EntityOutput GetGround()
	{ return GetPowerRail(false)->GetOutput("OUT"); }

	Greenpak4EntityOutput GetPower()
	{ return GetPowerRail(true)->GetOutput("OUT"); }

	Greenpak4EntityOutput GetPowerNet(bool b)
	{ return GetPowerRail(b)->GetOutput("OUT"); }

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// IOBS

	//Get the IO buffer for a given pin number
	Greenpak4IOB* GetIOB(unsigned int pin);

	typedef std::map<unsigned int, Greenpak4IOB*> iobmap;

	iobmap::iterator iobbegin()
	{ return m_iobs.begin(); }

	iobmap::iterator iobend()
	{ return m_iobs.end(); }

	unsigned int GetIOBCount()
	{ return m_iobs.size(); }

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// GLOBAL ROUTING

	unsigned int GetMatrixBits()
	{ return m_matrixBits; }

	unsigned int GetMatrixBase(unsigned int matrix);

	Greenpak4CrossConnection* GetCrossConnection(unsigned int src_matrix, unsigned int index)
	{ return m_crossConnections[src_matrix][index]; }

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// LUTS

	Greenpak4BitstreamEntity* GetLUT(unsigned int i);
	Greenpak4BitstreamEntity* GetLUT2(unsigned int i);
	Greenpak4BitstreamEntity* GetLUT3(unsigned int i);
	Greenpak4BitstreamEntity* GetLUT4(unsigned int i);

	unsigned int GetLUTCount()
	{ return m_luts.size(); }

	unsigned int GetLUT2Count()
	{ return m_lut2s.size(); }

	unsigned int GetLUT3Count()
	{ return m_lut3s.size(); }

	unsigned int GetLUT4Count()
	{ return m_lut4s.size(); }

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// INVERTERS

	Greenpak4Inverter* GetInverter(unsigned int i)
	{ return m_inverters[i]; }

	unsigned int GetInverterCount()
	{ return m_inverters.size(); }

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// FLIPFLOPS

	unsigned int GetTotalFFCount()
	{ return m_dffAll.size(); }

	unsigned int GetDFFCount()
	{ return m_dffs.size(); }

	unsigned int GetDFFSRCount()
	{ return m_dffsr.size(); }

	Greenpak4Flipflop* GetFlipflopByIndex(unsigned int i)
	{ return m_dffAll[i]; }

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// OSCILLATORS

	Greenpak4LFOscillator* GetLFOscillator()
	{ return m_lfosc; }

	Greenpak4RingOscillator* GetRingOscillator()
	{ return m_ringosc; }

	Greenpak4RCOscillator* GetRCOscillator()
	{ return m_rcosc; }

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// COUNTERS

	unsigned int Get8BitCounterCount()
	{ return m_counters8bit.size(); }

	unsigned int Get8BitCounterCount(bool withFSM)
	{ return std::count_if(m_counters8bit.begin(), m_counters8bit.end(),
						   [&](Greenpak4Counter *c) { return c->HasFSM() == withFSM; }); }

	unsigned int Get14BitCounterCount()
	{ return m_counters14bit.size(); }

	unsigned int Get14BitCounterCount(bool withFSM)
	{ return std::count_if(m_counters14bit.begin(), m_counters14bit.end(),
						   [&](Greenpak4Counter *c) { return c->HasFSM() == withFSM; }); }

	Greenpak4Counter* Get8BitCounter(unsigned int i)
	{ return m_counters8bit[i]; }

	Greenpak4Counter* Get14BitCounter(unsigned int i)
	{ return m_counters14bit[i]; }

	unsigned int GetCounterCount()
	{ return m_counters.size(); }

	Greenpak4Counter* GetCounter(unsigned int i)
	{ return m_counters[i]; }

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// RESETS

	Greenpak4SystemReset* GetSystemReset()
	{ return m_sysrst; }

	Greenpak4PowerOnReset* GetPowerOnReset()
	{ return m_por; }

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// BANDGAP

	Greenpak4Bandgap* GetBandgap()
	{ return m_bandgap; }

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// PGA

	Greenpak4PGA* GetPGA()
	{ return m_pga; }

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// SHIFT REGISTERS

	unsigned int GetShiftRegisterCount()
	{ return m_shregs.size(); }

	Greenpak4ShiftRegister* GetShiftRegister(unsigned int i)
	{ return m_shregs[i]; }

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// VOLTAGE REFERENCE

	unsigned int GetVrefCount()
	{ return m_vrefs.size(); }

	Greenpak4VoltageReference* GetVref(unsigned int i)
	{ return m_vrefs[i]; }

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// ANALOG COMPARATORS

	unsigned int GetAcmpCount()
	{ return m_acmps.size(); }

	Greenpak4Comparator* GetAcmp(unsigned int i)
	{ return m_acmps[i]; }

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// DIGITAL COMPARATORS

	unsigned int GetDcmpCount()
	{ return m_dcmps.size(); }

	Greenpak4DigitalComparator* GetDcmp(unsigned int i)
	{ return m_dcmps[i]; }

	unsigned int GetDcmpRefCount()
	{ return m_dcmprefs.size(); }

	Greenpak4DCMPRef* GetDcmpRef(unsigned int i)
	{ return m_dcmprefs[i]; }

	Greenpak4DCMPMux* GetDCMPMux()
	{ return m_dcmpmux; }

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// ANALOG BUFFERS

	Greenpak4Abuf* GetAbuf()
	{ return m_abuf; }

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// CLOCK BUFFERS

	unsigned int GetClockBufferCount()
	{ return m_clkbufs.size(); }

	Greenpak4ClockBuffer* GetClockBuffer(unsigned int i)
	{ return m_clkbufs[i]; }

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// POWER DETECTOR

	Greenpak4PowerDetector* GetPowerDetector()
	{ return m_pwrdet; }

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// SPI SLAVE

	Greenpak4SPI* GetSPI()
	{ return m_spi; }

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// DACS

	Greenpak4DAC* GetDAC(unsigned int i)
	{ return m_dacs[i]; }

	unsigned int GetDACCount()
	{ return m_dacs.size(); }

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// DELAYS

	Greenpak4Delay* GetDelay(unsigned int i)
	{ return m_delays[i]; }

	unsigned int GetDelayCount()
	{ return m_delays.size(); }

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Pattern generator

	//LUT4_0 is the PGEN for all GreenPAK4 devices.
	//Note that it's at array position 1 so it's lower priority in initial placement.
	//TODO: better handle conflicts in initial placement
	Greenpak4BitstreamEntity* GetPgen()
	{
		if(m_lut4s.size() < 2)
			return NULL;
		return m_lut4s[1];
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// ALL NODES

	unsigned int GetEntityCount()
	{ return m_bitstuff.size(); }

	Greenpak4BitstreamEntity* GetEntity(unsigned int i)
	{ return m_bitstuff[i]; }

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// CONFIGURATION

	void SetIOPrecharge(bool precharge);
	void SetDisableChargePump(bool disable);
	void SetLDOBypass(bool bypass);

	bool IsChargePumpDisabled()
	{ return m_disableChargePump; }

	void SetNVMRetryCount(int count);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// TIMING

	void PrintTimingData() const;
	void SaveTimingData(std::string fname);

protected:

	bool GenerateBitstream(bool* bitstream, uint8_t userid, bool readProtect);

	void CreateDevice_SLG46140();
	void CreateDevice_SLG4662x(bool dual_rail);
	void CreateDevice_common();

	///The part number
	GREENPAK4_PART m_part;

	///The number of bits in a routing matrix selector
	unsigned int m_matrixBits;

	///One vector with everything in the bistream
	std::vector<Greenpak4BitstreamEntity*> m_bitstuff;

	///The inverters
	std::vector<Greenpak4Inverter*> m_inverters;

	/**
		@brief Just the LUTs (all sizes)

		Sorted with LUT2s, LUT3s, LUT4s in that order
	 */
	std::vector<Greenpak4BitstreamEntity*> m_luts;

	///Just the LUT2s
	std::vector<Greenpak4LUT*> m_lut2s;

	///Just the LUT3s
	std::vector<Greenpak4LUT*> m_lut3s;

	///Just the LUT4s
	std::vector<Greenpak4BitstreamEntity*> m_lut4s;

	///I/O pins (map from pin numbers to IOBs)
	iobmap m_iobs;

	///Flipflops of all types
	std::vector<Greenpak4Flipflop*> m_dffAll;

	///Flipflops WITHOUT set/reset
	std::vector<Greenpak4Flipflop*> m_dffs;

	///Flipflops WITH set/reset
	std::vector<Greenpak4Flipflop*> m_dffsr;

	///8-bit counters
	std::vector<Greenpak4Counter*> m_counters8bit;

	///14-bit counters
	std::vector<Greenpak4Counter*> m_counters14bit;

	///Counters of all types
	std::vector<Greenpak4Counter*> m_counters;

	///Shift registers
	std::vector<Greenpak4ShiftRegister*> m_shregs;

	///Voltage references
	std::vector<Greenpak4VoltageReference*> m_vrefs;

	///Analog comparators
	std::vector<Greenpak4Comparator*> m_acmps;

	///Digital comparators
	std::vector<Greenpak4DigitalComparator*> m_dcmps;

	///Digital comparator references
	std::vector<Greenpak4DCMPRef*> m_dcmprefs;

	///Clock buffers
	std::vector<Greenpak4ClockBuffer*> m_clkbufs;

	///Digital to analog converters
	std::vector<Greenpak4DAC*> m_dacs;

	///Constant digital 1
	Greenpak4PowerRail* m_constantOne;

	///Constant digital 0
	Greenpak4PowerRail* m_constantZero;

	///Low-frequency oscillator
	Greenpak4LFOscillator* m_lfosc;

	///Ring oscillator
	Greenpak4RingOscillator* m_ringosc;

	///RC oscillator
	Greenpak4RCOscillator* m_rcosc;

	///Analog buffer
	Greenpak4Abuf* m_abuf;

	///Digital comparator mux
	Greenpak4DCMPMux* m_dcmpmux;

	///System reset
	Greenpak4SystemReset* m_sysrst;

	///SPI slave
	Greenpak4SPI* m_spi;

	///Bandgap reference
	Greenpak4Bandgap* m_bandgap;

	///Programmable-gain amplifier
	Greenpak4PGA* m_pga;

	///Digital delay lines
	std::vector<Greenpak4Delay*> m_delays;

	///Power-on reset
	Greenpak4PowerOnReset* m_por;

	///Power detector
	Greenpak4PowerDetector* m_pwrdet;

	/**
		@brief Cross-connections between our matrices

		connections[i][j] is the j'th connection from matrix i to (i+1) mod 2
	 */
	Greenpak4CrossConnection* m_crossConnections[2][10];

	//Total bitfile length
	unsigned int m_bitlen;

	//Base address of each routing matrix
	unsigned int m_matrixBase[2];

	/**
		@brief Indicates whether I/O pin precharge should be enabled.

		If enabled, connect 2K ohms nominal across each pullup/down resistor during POR to help
		external signals stabilize faster.
	 */
	bool m_ioPrecharge;

	/**
		@brief Specifies that the charge pump should be disabled

		By default the on-die charge pump automatically powers the analog hard IP when Vdd drops below 2.7V.
		This bit turns it off regardless of supply voltage. Why would you ever want that?
	 */
	bool m_disableChargePump;

	/**
		@brief Bypasses the on-chip LDO and runs Vcore directly from Vdd.

		This requires Vdd = 1.8V.
	 */
	bool m_ldoBypass;

	/**
		@brief Number of times to attempt re-reading NVM in case of boot failure
	 */
	int m_nvmLoadRetryCount;
};

#endif
