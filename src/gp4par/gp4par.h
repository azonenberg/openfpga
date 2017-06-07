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

#ifndef gp4par_h
#define gp4par_h

#include <cstdio>
#include <string>
#include <map>
#include <log.h>
#include <xbpar.h>
#include <Greenpak4.h>

typedef std::map<uint32_t, std::string> labelmap;
typedef std::map<std::string, uint32_t> ilabelmap;

#include "Greenpak4PAREngine.h"

//Console help
void ShowUsage();
void ShowVersion();

//Setup
uint32_t AllocateLabel(
	PARGraph*& ngraph,
	PARGraph*& dgraph,
	labelmap& lmap,
	std::string description);
bool BuildGraphs(
	Greenpak4Netlist* netlist,
	Greenpak4Device* device,
	PARGraph*& ngraph,
	PARGraph*& dgraph,
	labelmap& lmap);
void ApplyLocConstraints(Greenpak4Netlist* netlist, PARGraph* ngraph, PARGraph* dgraph);

//PAR core
bool DoPAR(Greenpak4Netlist* netlist, Greenpak4Device* device);

//DRC
bool PostPARDRC(PARGraph* netlist, Greenpak4Device* device);

//Committing
bool CommitChanges(PARGraph* device, Greenpak4Device* pdev, unsigned int* num_routes_used);
bool CommitRouting(PARGraph* device, Greenpak4Device* pdev, unsigned int* num_routes_used);

//Reporting
void PrintUtilizationReport(PARGraph* netlist, Greenpak4Device* device, unsigned int* num_routes_used);
void PrintPlacementReport(PARGraph* netlist, Greenpak4Device* device);
void PrintTimingReport(Greenpak4Netlist* netlist, Greenpak4Device* device);

#endif
