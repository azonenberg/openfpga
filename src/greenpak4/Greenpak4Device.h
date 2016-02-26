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

	Greenpak4Device(Greenpak4Device::GREENPAK4_PART part);
	
	virtual ~Greenpak4Device();
	
	//Write to a bitfile
	bool WriteToFile(const char* fname);
	
	//Get the power rail for a binary constant
	Greenpak4BitstreamEntity* GetPowerRail(unsigned int matrix, bool rail);
	
	//Get the IO buffer for a given pin number
	Greenpak4IOB* GetIOB(unsigned int pin);
	
	//Accessors
	
	unsigned int GetMatrixBits()
	{ return m_matrixBits; }
	
	Greenpak4LUT* GetLUT2(unsigned int i);
	Greenpak4LUT* GetLUT3(unsigned int i);
	
	unsigned int GetMatrixBase(unsigned int matrix);
	
protected:

	void CreateDevice_SLG46620();

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
	std::map<unsigned int, Greenpak4IOB*> m_iobs;
	
	///Constant digital 1 for each matrix
	Greenpak4PowerRail* m_constantOne[2];
	
	///Constant digital 0
	Greenpak4PowerRail* m_constantZero[2];
	
	//Total bitfile length
	unsigned int m_bitlen;
	
	//Base address of each routing matrix
	unsigned int m_matrixBase[2];
};

#endif
