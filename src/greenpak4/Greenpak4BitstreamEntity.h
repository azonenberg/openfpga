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
 
#ifndef Greenpak4BitstreamEntity_h
#define Greenpak4BitstreamEntity_h

class Greenpak4Device;
class PARGraphNode;
class Greenpak4DualEntity;

#include <string>
#include <vector>

/**
	@brief An entity which is serialized to/from the bitstream
 */ 
class Greenpak4BitstreamEntity
{
public:
	Greenpak4BitstreamEntity(
		Greenpak4Device* device,
		unsigned int matrix,
		unsigned int ibase,
		unsigned int obase,
		unsigned int cbase
		);
	virtual ~Greenpak4BitstreamEntity();
	
	//TODO: Print for debugging

	///Deserialize from an external bitstream
	virtual bool Load(bool* bitstream) =0;
	
	///Serialize to an external bitstream
	virtual bool Save(bool* bitstream) =0;
	
	/**
		@brief Returns the length, in bits, of our configuration data
	 */
	virtual unsigned int GetConfigLen() =0;
	
	/**
		@brief Returns the index of the routing matrix our OUTPUT is attached to
	 */
	unsigned int GetMatrix()
	{ return m_matrix; }
	
	/**
		@brief Gets the index of the first (only, in most cases) output word
	 */
	unsigned int GetOutputBase()
	{ return m_outputBaseWord; }
	
	PARGraphNode* GetPARNode()
	{ return m_parnode; }
	
	void SetPARNode(PARGraphNode* node)
	{ m_parnode = node; }
	
	unsigned int GetConfigBase()
	{ return m_configBase; }
	
	Greenpak4Device* GetDevice()
	{ return m_device; }
	
	/**
		@brief Returns a human-readable description of this node (like LUT3_1)
	 */
	virtual std::string GetDescription() =0;
	
	/**
		@brief Returns the real entity if we are a dual, or us if we're not
	 */
	virtual Greenpak4BitstreamEntity* GetRealEntity();
	
	//Return our dual, or NULL if we don't have one
	Greenpak4DualEntity* GetDual()
	{ return m_dual; }
	
	//Get a list of input ports on this node that connect to general fabric routing (may be empty)
	virtual std::vector<std::string> GetInputPorts() =0;
	
	//Get a list of output ports on this node that connect to general fabric routing (may be empty)
	virtual std::vector<std::string> GetOutputPorts() =0;
	
protected:

	/**
		@brief Writes a matrix select value to the bitstream
		
		Set cross_matrix for cross connections only
	 */
	bool WriteMatrixSelector(
		bool* bitstream,
		unsigned int wordpos,
		Greenpak4BitstreamEntity* signal,
		bool cross_matrix = false);

	///The device we're attached to
	Greenpak4Device* m_device;

	///Number of the routing matrix we're attached to (currently 0 or 1 for all GP4 devices)
	unsigned int m_matrix;
	
	/**
		@brief Base address of our input bus.
		
		This is measured in *words* from the start of the routing matrix.
		
		For example, in the SLG46620 matrix 0 uses 6 bit words, so if m_inputBaseWord is 2 and m_inputMatrix is 0,
		then our first input is bits 12-17 from the start of matrix 0 (offset 0 in the bitstream).
	 */
	unsigned int m_inputBaseWord;
	
	///Base address of the output bus
	unsigned int m_outputBaseWord;
	
	///Base address of our configuration data, in bits
	unsigned int m_configBase;
	
	///The graph node used for place-and-route
	PARGraphNode* m_parnode;
	
	///Our dual entity (if we have one)
	Greenpak4DualEntity* m_dual;
};

#endif
