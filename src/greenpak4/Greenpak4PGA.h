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

#ifndef Greenpak4PGA_h
#define Greenpak4PGA_h

#include "Greenpak4BitstreamEntity.h"

class Greenpak4PGA : public Greenpak4BitstreamEntity
{
public:

	//Construction / destruction
	Greenpak4PGA(
		Greenpak4Device* device,
		unsigned int cbase);

	//Serialization
	virtual bool Load(bool* bitstream);
	virtual bool Save(bool* bitstream);

	virtual ~Greenpak4PGA();

	virtual std::string GetDescription();

	virtual void SetInput(std::string port, Greenpak4EntityOutput src);
	virtual unsigned int GetOutputNetNumber(std::string port);

	virtual std::vector<std::string> GetInputPorts() const;
	virtual std::vector<std::string> GetOutputPorts() const;

	virtual void CommitChanges();

	Greenpak4EntityOutput GetInputP()
	{ return m_vinp; }

	Greenpak4EntityOutput GetInputN()
	{ return m_vinn; }

	bool IsUsed();

	enum InputModes
	{
		MODE_SINGLE,
		MODE_DIFF,
		MODE_PDIFF
	};

	InputModes GetInputMode()
	{ return m_inputMode; }

protected:
	Greenpak4EntityOutput m_vinp;
	Greenpak4EntityOutput m_vinn;
	Greenpak4EntityOutput m_vinsel;

	//decimal fixed point: legal values 25, 50, 100, 200, 400, 800, 1600, 3200
	unsigned int m_gain;

	InputModes m_inputMode;

	///indicates if we have any loads other than the ADC
	bool m_hasNonADCLoads;
};

#endif	//Greenpak4PGA_h
