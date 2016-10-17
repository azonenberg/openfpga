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

#ifndef Greenpak4LUT_h
#define Greenpak4LUT_h

/**
	@brief A single LUT
 */
class Greenpak4LUT : public Greenpak4BitstreamEntity
{
public:

	//Construction / destruction
	Greenpak4LUT(
		Greenpak4Device* device,
		unsigned int lutnum,
		unsigned int matrix,
		unsigned int ibase,
		unsigned int oword,
		unsigned int cbase,
		unsigned int order);
	virtual ~Greenpak4LUT();

	//Serialization
	virtual bool Load(bool* bitstream);
	virtual bool Save(bool* bitstream);

	unsigned int GetOrder()
	{ return m_order; }

	unsigned int GetLutIndex()
	{ return m_lutnum; }

	virtual std::string GetDescription();

	virtual void SetInput(std::string port, Greenpak4EntityOutput src);
	virtual unsigned int GetOutputNetNumber(std::string port);

	virtual std::vector<std::string> GetInputPorts() const;
	virtual std::vector<std::string> GetOutputPorts() const;

	virtual bool CommitChanges();

protected:

	///Index of our LUT
	unsigned int m_lutnum;

	///Number of inputs to the LUT
	unsigned int m_order;

	/**
		@brief The actual LUT data

		Statically allocate 16 bits per LUT to avoid the overhead of dynamic allocation.
		Only bits 0... (2^order - 1) are valid.
	 */
	bool m_truthtable[16];

	/**
		@brief Array of LUT inputs.

		Statically allocate 4 per LUT to avoid the overhead of dynamic allocation.
		Only inputs 0... (order-1) are valid.
	 */
	Greenpak4EntityOutput m_inputs[4];
};

#endif
