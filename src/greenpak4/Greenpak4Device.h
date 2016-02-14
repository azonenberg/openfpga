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
};

#endif
