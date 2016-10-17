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

#ifndef Greenpak4IOB_h
#define Greenpak4IOB_h

/**
	@brief A single IOB
 */
class Greenpak4IOB : public Greenpak4BitstreamEntity
{
public:

	//Construction / destruction
	Greenpak4IOB(
		Greenpak4Device* device,
		unsigned int pin_num,
		unsigned int matrix,
		unsigned int ibase,
		unsigned int oword,
		unsigned int cbase,
		unsigned int flags = IOB_FLAG_NORMAL);
	virtual ~Greenpak4IOB();

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Accessors for format-dependent bitstream state

	//Drive strength for pullup/down resistor
	enum PullStrength
	{
		PULL_10K,
		PULL_100K,
		PULL_1M
	};

	//Direction for pullup/down resistor
	enum PullDirection
	{
		PULL_NONE,
		PULL_DOWN,
		PULL_UP
	};

	//Drive strength for output
	enum DriveStrength
	{
		DRIVE_1X,
		DRIVE_2X,
		DRIVE_4X
	};

	//Output driver type
	enum DriveType
	{
		//Push-pull
		DRIVE_PUSHPULL,

		///Pull low only
		DRIVE_NMOS_OPENDRAIN,

		///Pull high only
		DRIVE_PMOS_OPENDRAIN
	};

	//Input voltage threshold
	enum InputThreshold
	{
		///Normal digital input
		THRESHOLD_NORMAL,

		///Low-voltage digital input
		THRESHOLD_LOW,

		///Analog input
		THRESHOLD_ANALOG
	};

	//Special configuration flags
	enum IOBFlags
	{
		IOB_FLAG_NORMAL = 0,
		IOB_FLAG_INPUTONLY = 1,
		IOB_FLAG_X4DRIVE = 2
	};

	unsigned int GetFlags()
	{ return m_flags; }

	bool IsInputOnly()
	{ return (m_flags & IOB_FLAG_INPUTONLY) ? true : false; }

	unsigned int GetPinNumber()
	{ return m_pinNumber; }

	virtual std::vector<std::string> GetInputPorts() const;
	virtual std::vector<std::string> GetOutputPorts() const;

	virtual void SetInput(std::string port, Greenpak4EntityOutput src);
	virtual unsigned int GetOutputNetNumber(std::string port);

	virtual bool CommitChanges();

	//Used to set defaults in Greenpak4Device constructor
	void SetPullDirection(PullDirection dir)
	{ m_pullDirection = dir; }

	void SetPullStrength(PullStrength str)
	{ m_pullStrength = str; }

	void SetAnalogConfigBase(unsigned int base)
	{ m_analogConfigBase = base; }

	//Get our source (used for DRC)
	Greenpak4EntityOutput GetOutputSignal()
	{ return m_outputSignal; }

	bool IsAnalogIbuf()
	{ return (m_inputThreshold == THRESHOLD_ANALOG); }

protected:

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Abstracted version of format-dependent bitstream state

	///Pin number in the packaged device
	unsigned int m_pinNumber;

	///Set true to enable Schmitt triggering on the input
	bool m_schmittTrigger;

	///Strength of the pullup/down resistor, if any
	PullStrength m_pullStrength;

	///Direction of the pullup/down resistor, if any
	PullDirection m_pullDirection;

	///Strength of the output driver
	DriveStrength m_driveStrength;

	///Type of the output driver
	DriveType m_driveType;

	///Type of the input threshold
	InputThreshold m_inputThreshold;

	///Signal source for our output enable
	Greenpak4EntityOutput m_outputEnable;

	///Signal source for our output driver
	Greenpak4EntityOutput m_outputSignal;

	///Flags indicating special capabilities of this IOB
	unsigned int m_flags;

	///Second configuration base for analog output
	unsigned int m_analogConfigBase;
};

#endif
