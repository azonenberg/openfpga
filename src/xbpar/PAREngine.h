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
 
#ifndef PAREngine_h
#define PAREngine_h

#include <vector>

/**
	@brief The core place-and-route engine
 */
class PAREngine
{
public:
	PAREngine(PARGraph* netlist, PARGraph* device);
	virtual ~PAREngine();
	
	virtual bool Route(bool verbose = true);
	
protected:

	virtual bool SanityCheck(bool verbose);
	virtual void InitialPlacement(bool verbose);
	virtual bool OptimizePlacement(bool verbose);

	PARGraph* m_netlist;
	PARGraph* m_device;
};

#endif

