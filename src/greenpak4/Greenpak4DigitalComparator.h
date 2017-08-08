/***********************************************************************************************************************
 * Copyright (C) 2016-2017 Andrew Zonenberg and contributors                                                           *
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

#ifndef Greenpak4DigitalComparator_h
#define Greenpak4DigitalComparator_h

/**
	@brief The DCMP/PWM block
 */
class Greenpak4DigitalComparator : public Greenpak4BitstreamEntity
{
public:

	//Construction / destruction
	Greenpak4DigitalComparator(
		Greenpak4Device* device,
		unsigned int cmpnum,
		unsigned int matrix,
		unsigned int ibase,
		unsigned int oword,
		unsigned int cbase);
	virtual ~Greenpak4DigitalComparator();

	//Serialization
	virtual bool Load(bool* bitstream);
	virtual bool Save(bool* bitstream);

	virtual std::string GetDescription() const;

	virtual void SetInput(std::string port, Greenpak4EntityOutput src);
	virtual Greenpak4EntityOutput GetInput(std::string port) const;
	virtual unsigned int GetOutputNetNumber(std::string port);

	virtual std::vector<std::string> GetInputPorts() const;
	virtual std::vector<std::string> GetOutputPorts() const;
	virtual std::map<std::string, std::string> GetParameters() const;

	virtual bool CommitChanges();

	void AddInputPMuxEntry(Greenpak4EntityOutput net, unsigned int sel)
	{ m_inpsels[net] = sel; }

	void AddInputNMuxEntry(Greenpak4EntityOutput net, unsigned int sel)
	{ m_innsels[net] = sel; }

	bool IsUsed();

	Greenpak4EntityOutput GetPowerDown()
	{ return m_powerDown; }

	virtual std::string GetPrimitiveName() const;

protected:
	Greenpak4EntityOutput m_powerDown;
	Greenpak4EntityOutput m_inp[8];
	Greenpak4EntityOutput m_inn[8];
	Greenpak4EntityOutput m_clock;

	unsigned int m_cmpNum;

	bool m_dcmpMode;	//true for dmcp, false for pwm

	unsigned int m_pwmDeadband;	//dead time for differential PWM outputs, in ns

	bool m_compareGreaterEqual;	//select >= if true, > if false

	bool m_clockInvert;			//invert input clock

	bool m_pdSync;				//Power-down synchronization

	//Map of <signal, muxsel> tuples for input mux
	std::map<Greenpak4EntityOutput, unsigned int> m_inpsels;
	std::map<Greenpak4EntityOutput, unsigned int> m_innsels;
};

#endif
