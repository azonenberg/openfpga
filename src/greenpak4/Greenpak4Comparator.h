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

#ifndef Greenpak4Comparator_h
#define Greenpak4Comparator_h

#include "Greenpak4BitstreamEntity.h"

class Greenpak4Comparator : public Greenpak4BitstreamEntity
{
public:

	//Construction / destruction
	Greenpak4Comparator(
		Greenpak4Device* device,
		unsigned int cmpnum,
		unsigned int matrix,
		unsigned int ibase,
		unsigned int oword,
		unsigned int cbase_isrc,
		unsigned int cbase_bw,
		unsigned int cbase_gain,
		unsigned int cbase_vin,
		unsigned int cbase_hyst,
		unsigned int cbase_vref
		);
	
	//Serialization
	virtual bool Load(bool* bitstream);
	virtual bool Save(bool* bitstream);
		
	virtual ~Greenpak4Comparator();

	virtual std::string GetDescription();
	
	virtual void SetInput(std::string port, Greenpak4EntityOutput src);
	virtual unsigned int GetOutputNetNumber(std::string port);
	
	virtual std::vector<std::string> GetInputPorts() const;
	virtual std::vector<std::string> GetOutputPorts() const;
	
	virtual void CommitChanges();
	
	//Accessors
	void AddInputMuxEntry(Greenpak4EntityOutput net, unsigned int sel)
	{ m_muxsels[net] = sel; }
	
	Greenpak4EntityOutput GetInput()
	{ return m_vin; }
	
	//Helper used by DRC to poke ACMP0's config if necessary
	void SetInput(Greenpak4EntityOutput input)
	{ m_vin = input; }
	
	void SetPowerEn(Greenpak4EntityOutput pwren)
	{ m_pwren = pwren; }
	
protected:
	Greenpak4EntityOutput m_pwren;
	Greenpak4EntityOutput m_vin;
	Greenpak4EntityOutput m_vref;
	
	unsigned int m_cmpNum;
	
	unsigned int m_cbaseIsrc;
	unsigned int m_cbaseBw;
	unsigned int m_cbaseGain;
	unsigned int m_cbaseVin;
	unsigned int m_cbaseHyst;
	unsigned int m_cbaseVref;
	
	bool m_bandwidthHigh;
	int m_vinAtten;
	bool m_isrcEn;
	int m_hysteresis;
	
	//Map of <signal, muxsel> tuples for input mux
	std::map<Greenpak4EntityOutput, unsigned int> m_muxsels;
};

#endif	//Greenpak4Comparator_h
