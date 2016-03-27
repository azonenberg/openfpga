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
		GREENPAK4_SLG46620
	};

	Greenpak4Device(
		Greenpak4Device::GREENPAK4_PART part,
		Greenpak4IOB::PullDirection default_pull = Greenpak4IOB::PULL_NONE,
		Greenpak4IOB::PullStrength default_drive = Greenpak4IOB::PULL_1M);
	
	virtual ~Greenpak4Device();
	
	//Write to a bitfile
	bool WriteToFile(std::string fname);
	
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// POWER RAILS
	
	//Get the power rail for a binary constant
	Greenpak4BitstreamEntity* GetPowerRail(unsigned int matrix, bool rail);
	
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
	
	Greenpak4LUT* GetLUT(unsigned int i);
	Greenpak4LUT* GetLUT2(unsigned int i);
	Greenpak4LUT* GetLUT3(unsigned int i);
	Greenpak4LUT* GetLUT4(unsigned int i);
	
	unsigned int GetLUTCount()
	{ return m_luts.size(); }
	
	unsigned int GetLUT2Count()
	{ return m_lut2s.size(); }
	
	unsigned int GetLUT3Count()
	{ return m_lut3s.size(); }
	
	unsigned int GetLUT4Count()
	{ return m_lut4s.size(); }
	
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

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// COUNTERS
	
	unsigned int Get8BitCounterCount()
	{ return m_counters8bit.size(); }
	
	unsigned int Get14BitCounterCount()
	{ return m_counters14bit.size(); }
	
	Greenpak4Counter* Get8BitCounter(unsigned int i)
	{ return m_counters8bit[i]; }
	
	Greenpak4Counter* Get14BitCounter(unsigned int i)
	{ return m_counters14bit[i]; }
	
	unsigned int GetCounterCount()
	{ return m_counters.size(); }
	
	Greenpak4Counter* GetCounter(unsigned int i)
	{ return m_counters[i]; }

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// OTHER

protected:

	void CreateDevice_SLG46620();
	void CreateDevice_common();

	///The part number
	GREENPAK4_PART m_part;
	
	///The number of bits in a routing matrix selector
	unsigned int m_matrixBits;

	///One vector with everything in the bistream
	std::vector<Greenpak4BitstreamEntity*> m_bitstuff;
	
	/**
		@brief Just the LUTs (all sizes)
		
		Sorted with LUT2s, LUT3s, LUT4s in that order
	 */
	std::vector<Greenpak4LUT*> m_luts;
	
	///Just the LUT2s
	std::vector<Greenpak4LUT*> m_lut2s;
	
	///Just the LUT3s
	std::vector<Greenpak4LUT*> m_lut3s;
	
	///Just the LUT4s
	std::vector<Greenpak4LUT*> m_lut4s;
	
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
	
	///Constant digital 1 for each matrix
	Greenpak4PowerRail* m_constantOne[2];
	
	///Constant digital 0
	Greenpak4PowerRail* m_constantZero[2];
	
	//Low-frequency oscillator
	Greenpak4LFOscillator* m_lfosc;
	
	/**
		@brief Cross-connections between our matrices
		
		connections[i][j] is the j'th connection from matrix i to (i+1) mod 2
	 */
	Greenpak4CrossConnection* m_crossConnections[2][10];
	
	//Total bitfile length
	unsigned int m_bitlen;
	
	//Base address of each routing matrix
	unsigned int m_matrixBase[2];
};

#endif
