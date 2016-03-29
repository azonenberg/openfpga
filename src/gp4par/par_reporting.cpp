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

#include "gp4par.h"

using namespace std;

/**
	@brief Print the report showing how many resources were used
 */
void PrintUtilizationReport(PARGraph* netlist, Greenpak4Device* device, unsigned int* num_routes_used)
{
	//Get resource counts from the whole device
	unsigned int lut_counts[5] =
	{
		0,	//no LUT0
		0,	//no LUT1
		device->GetLUT2Count(),
		device->GetLUT3Count(),
		device->GetLUT4Count()
	};
	unsigned int iob_count = device->GetIOBCount();
	
	//Loop over nodes, find how many of each type were used
	unsigned int luts_used[5] = {0};
	unsigned int iobs_used = 0;
	unsigned int dff_used = 0;
	unsigned int dffsr_used = 0;
	unsigned int counters_8_used = 0;
	unsigned int counters_14_used = 0;
	unsigned int lfosc_used = 0;
	unsigned int sysrst_used = 0;
	for(uint32_t i=0; i<netlist->GetNumNodes(); i++)
	{
		auto entity = static_cast<Greenpak4BitstreamEntity*>(netlist->GetNodeByIndex(i)->GetMate()->GetData());
		auto iob = dynamic_cast<Greenpak4IOB*>(entity);
		auto lut = dynamic_cast<Greenpak4LUT*>(entity);
		auto ff = dynamic_cast<Greenpak4Flipflop*>(entity);
		auto count = dynamic_cast<Greenpak4Counter*>(entity);
		if(lut)
			luts_used[lut->GetOrder()] ++;
		else if(iob)
			iobs_used ++;
		else if(ff)
		{
			if(ff->HasSetReset())
				dffsr_used ++;
			else
				dff_used ++;
		}
		else if(count)
		{
			if(count->GetDepth() == 8)
				counters_8_used ++;
			else
				counters_14_used ++;
		}
	}
	if(device->GetLFOscillator()->GetPARNode()->GetMate() != NULL)
		lfosc_used = 1;
	if(device->GetSystemReset()->GetPARNode()->GetMate() != NULL)
		sysrst_used = 1;
	
	//Print the actual report
	printf("\nDevice utilization:\n");
	unsigned int total_luts_used = luts_used[2] + luts_used[3] + luts_used[4];
	unsigned int total_lut_count = lut_counts[2] + lut_counts[3] + lut_counts[4];
	unsigned int total_ff = device->GetTotalFFCount();
	unsigned int total_dff = device->GetDFFCount();
	unsigned int total_dffsr = device->GetDFFSRCount();
	unsigned int total_dff_used = dff_used + dffsr_used;
	unsigned int total_counters_used = counters_8_used + counters_14_used;
	unsigned int total_counters = device->GetCounterCount();
	unsigned int total_counters_8 = device->Get8BitCounterCount();
	unsigned int total_counters_14 = device->Get14BitCounterCount();
	printf("    COUNT:     %2d/%2d (%d %%)\n",
		total_counters_used, total_counters, total_counters_used*100 / total_counters);
	printf("      COUNT8:  %2d/%2d (%d %%)\n",
		counters_8_used, total_counters_8, counters_8_used*100 / total_counters_8);
	printf("      COUNT14: %2d/%2d (%d %%)\n",
		counters_14_used, total_counters_14, counters_14_used*100 / total_counters_14);
	printf("    FF:        %2d/%2d (%d %%)\n", total_dff_used, total_ff, total_dff_used*100 / total_ff);
	printf("      DFF:     %2d/%2d (%d %%)\n", dff_used, total_dff, dff_used*100 / total_dff);
	printf("      DFFSR:   %2d/%2d (%d %%)\n", dffsr_used, total_dffsr, dffsr_used*100 / total_dffsr);
	printf("    IOB:       %2d/%2d (%d %%)\n", iobs_used, iob_count, iobs_used*100 / iob_count);
	printf("    LFOSC:     %2d/%2d (%d %%)\n", lfosc_used, 1, lfosc_used*100);
	printf("    LUT:       %2d/%2d (%d %%)\n", total_luts_used, total_lut_count, total_luts_used*100 / total_lut_count);
	for(unsigned int i=2; i<=4; i++)
	{
		unsigned int used = luts_used[i];
		unsigned int count = lut_counts[i];
		unsigned int percent = 0;
		if(count)
			percent = 100*used / count;
		printf("      LUT%d:    %2d/%2d (%d %%)\n", i, used, count, percent);
	}
	printf("    SYSRST:    %2d/%2d (%d %%)\n", sysrst_used, 1, sysrst_used*100);
	unsigned int total_routes_used = num_routes_used[0] + num_routes_used[1];
	printf("    X-conn:    %2d/20 (%d %%)\n", total_routes_used, total_routes_used*100 / 20);
	printf("      East:    %2d/10 (%d %%)\n", num_routes_used[0], num_routes_used[0]*100 / 10);
	printf("      West:    %2d/10 (%d %%)\n", num_routes_used[1], num_routes_used[1]*100 / 10);
}

/**
	@brief Print the report showing which cells in the netlist got placed where
 */
void PrintPlacementReport(PARGraph* netlist, Greenpak4Device* /*device*/)
{
	printf("\nPlacement report:\n");
	printf("    +----------------------------------------------------+-----------------+\n");
	printf("    | %-50s | %-15s |\n", "Node", "Site");
	
	for(uint32_t i=0; i<netlist->GetNumNodes(); i++)
	{
		auto nnode = netlist->GetNodeByIndex(i);
		auto src = static_cast<Greenpak4NetlistEntity*>(nnode->GetData());
		auto dnode = nnode->GetMate();
		
		//unplaced, ignore it for now
		if(dnode == NULL)
			continue;
		auto dst = static_cast<Greenpak4BitstreamEntity*>(dnode->GetData());
			
		printf("    | %-50s | %-15s |\n", src->m_name.c_str(), dst->GetDescription().c_str());
		
	}
	
	printf("    +----------------------------------------------------+-----------------+\n");
}
