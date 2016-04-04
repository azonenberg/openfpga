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
 
#ifndef Greenpak4LFOscillator_h
#define Greenpak4LFOscillator_h

/**
	@brief The low frequency (1.73 kHz) oscillator
 */ 
class Greenpak4LFOscillator : public Greenpak4BitstreamEntity
{
public:

	//Construction / destruction
	Greenpak4LFOscillator(
		Greenpak4Device* device,
		unsigned int matrix,
		unsigned int ibase,
		unsigned int oword,
		unsigned int cbase);
	virtual ~Greenpak4LFOscillator();
		
	//Bitfile metadata
	virtual unsigned int GetConfigLen();
	
	//Serialization
	virtual bool Load(bool* bitstream);
	virtual bool Save(bool* bitstream);
	
	virtual std::string GetDescription();
	
	//Set our power-down input
	void SetPowerDown(Greenpak4BitstreamEntity* pwrdn);
	
	//Get the power-down input (used for DRC)
	Greenpak4BitstreamEntity* GetPowerDown()
	{ return m_powerDown; }
	
	//Enable accessors
	void SetPowerDownEn(bool en)
	{ m_powerDownEn = en; }
	
	void SetAutoPowerDown(bool en)
	{ m_autoPowerDown = en; }
	
	//Divider
	void SetOutputDivider(int div);

	virtual std::vector<std::string> GetInputPorts();
	virtual std::vector<std::string> GetOutputPorts();
	
	virtual void CommitChanges();
	
protected:

	///Power-down input (if implemented)
	Greenpak4BitstreamEntity* m_powerDown;
	
	///Power-down enable
	bool m_powerDownEn;
	
	///Auto power-down
	bool m_autoPowerDown;
	
	///Output divider
	int m_outDiv;
};

#endif
