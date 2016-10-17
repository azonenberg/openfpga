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

#ifndef Greenpak4DAC_h
#define Greenpak4DAC_h

#include "Greenpak4BitstreamEntity.h"

class Greenpak4DAC : public Greenpak4BitstreamEntity
{
public:

	//Construction / destruction
	Greenpak4DAC(
		Greenpak4Device* device,
		unsigned int cbase_reg,
		unsigned int cbase_pwr,
		unsigned int cbase_insel,
		unsigned int cbase_aon,
		unsigned int dacnum);

	//Serialization
	virtual bool Load(bool* bitstream);
	virtual bool Save(bool* bitstream);

	virtual ~Greenpak4DAC();

	virtual std::string GetDescription();

	virtual void SetInput(std::string port, Greenpak4EntityOutput src);
	virtual unsigned int GetOutputNetNumber(std::string port);

	virtual std::vector<std::string> GetInputPorts() const;
	virtual std::vector<std::string> GetOutputPorts() const;

	virtual bool CommitChanges();

	unsigned int GetDACNum()
	{ return m_dacnum; }

	bool IsUsed();

protected:
	Greenpak4EntityOutput m_vref;
	Greenpak4EntityOutput m_din[8];

	unsigned int m_dacnum;

	unsigned int m_cbaseReg;
	unsigned int m_cbasePwr;
	unsigned int m_cbaseInsel;
	unsigned int m_cbaseAon;
};

#endif	//Greenpak4DAC_h
