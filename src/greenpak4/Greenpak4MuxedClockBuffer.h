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

#ifndef Greenpak4MuxedClockBuffer_h
#define Greenpak4MuxedClockBuffer_h

#include "Greenpak4BitstreamEntity.h"

class Greenpak4MuxedClockBuffer : public Greenpak4ClockBuffer
{
public:

	//Construction / destruction
	Greenpak4MuxedClockBuffer(Greenpak4Device* device, unsigned int bufnum, unsigned int matrix, unsigned int cbase);

	//Serialization
	virtual bool Load(bool* bitstream);
	virtual bool Save(bool* bitstream);

	virtual ~Greenpak4MuxedClockBuffer();

	void AddInputMuxEntry(Greenpak4EntityOutput node, unsigned int select)
	{ m_inputs[node] = select; }

protected:
	std::map<Greenpak4EntityOutput, unsigned int> m_inputs;
};

#endif	//Greenpak4MuxedClockBuffer_h
