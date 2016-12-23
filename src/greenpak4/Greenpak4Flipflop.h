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

#ifndef Greenpak4Flipflop_h
#define Greenpak4Flipflop_h

/**
	@brief A single flipflop/latch (TODO: support latch mode)
 */
class Greenpak4Flipflop : public Greenpak4BitstreamEntity
{
public:

	//Construction / destruction
	Greenpak4Flipflop(
		Greenpak4Device* device,
		unsigned int ffnum,
		bool has_sr,
		unsigned int matrix,
		unsigned int ibase,
		unsigned int oword,
		unsigned int cbase);
	virtual ~Greenpak4Flipflop();

	bool HasSetReset()
	{ return m_hasSR; }

	//Serialization
	virtual bool Load(bool* bitstream);
	virtual bool Save(bool* bitstream);

	//Set inputs

	unsigned int GetFlipflopIndex()
	{ return m_ffnum; }

	virtual std::string GetDescription();

	virtual void SetInput(std::string port, Greenpak4EntityOutput src);
	virtual unsigned int GetOutputNetNumber(std::string port);

	virtual std::vector<std::string> GetInputPorts() const;
	virtual std::vector<std::string> GetOutputPorts() const;

	virtual bool CommitChanges();

protected:

	///Index of our flipflop
	unsigned int m_ffnum;

	///True if we have a set/reset input
	bool m_hasSR;

	///Power-on reset value
	bool m_initValue;

	///Set/reset mode (1=set, 0=reset)
	bool m_srmode;

	///Output inverter flag (1 = QB, 0=Q)
	bool m_outputInvert;

	///Latch mode (1=latch, 0=ff)
	bool m_latchMode;

	///Input signal
	Greenpak4EntityOutput m_input;

	///Clock signal
	Greenpak4EntityOutput m_clock;

	///Negative set/reset signal
	Greenpak4EntityOutput m_nsr;
};

#endif
