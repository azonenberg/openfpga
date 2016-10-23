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

static void PrintRow(string kind, int used, int total)
{
	if(total == 0)
		return;

	Severity severity = (used > 0) ? Severity::NOTICE : Severity::VERBOSE;
	string padded_kind = kind + std::string(14 - kind.size(), ' ');
	Log(severity, "%s%2d/%2d (%d %%)\n", padded_kind.c_str(), used, total, used*100/total);
}

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

	//Loop over nodes, find how many of each type were used
	//TODO: use PAR labels for this?
	unsigned int luts_used[5] = {0};
	unsigned int iobs_used = 0;
	unsigned int dff_used = 0;
	unsigned int dffsr_used = 0;
	unsigned int counters_8_used = 0;
	unsigned int counters_8_adv_used = 0;
	unsigned int counters_14_used = 0;
	unsigned int counters_14_adv_used = 0;
	unsigned int lfosc_used = 0;
	unsigned int ringosc_used = 0;
	unsigned int rcosc_used = 0;
	unsigned int sysrst_used = 0;
	unsigned int inv_used = 0;
	unsigned int bandgap_used = 0;
	unsigned int por_used = 0;
	unsigned int shreg_used = 0;
	unsigned int vref_used = 0;
	unsigned int acmp_used = 0;
	unsigned int pga_used = 0;
	unsigned int abuf_used = 0;
	unsigned int delay_used = 0;
	for(uint32_t i=0; i<netlist->GetNumNodes(); i++)
	{
		auto entity = static_cast<Greenpak4BitstreamEntity*>(netlist->GetNodeByIndex(i)->GetMate()->GetData());
		auto lut = dynamic_cast<Greenpak4LUT*>(entity);
		auto ff = dynamic_cast<Greenpak4Flipflop*>(entity);
		auto count = dynamic_cast<Greenpak4Counter*>(entity);
		if(lut)
			luts_used[lut->GetOrder()] ++;
		else if(dynamic_cast<Greenpak4IOB*>(entity) != NULL)
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
			if(count->HasFSM())
			{
				if(count->GetDepth() == 8)
					counters_8_adv_used ++;
				else
					counters_14_adv_used ++;
			}
			else
			{
				if(count->GetDepth() == 8)
					counters_8_used ++;
				else
					counters_14_used ++;
			}
		}
		else if(dynamic_cast<Greenpak4Inverter*>(entity))
			inv_used ++;
		else if(dynamic_cast<Greenpak4Bandgap*>(entity))
			bandgap_used ++;
		else if(dynamic_cast<Greenpak4PowerOnReset*>(entity))
			por_used ++;
		else if(dynamic_cast<Greenpak4ShiftRegister*>(entity))
			shreg_used ++;
		else if(dynamic_cast<Greenpak4VoltageReference*>(entity))
			vref_used ++;
		else if(dynamic_cast<Greenpak4Comparator*>(entity))
			acmp_used ++;
		else if(dynamic_cast<Greenpak4PGA*>(entity))
			pga_used ++;
		else if(dynamic_cast<Greenpak4Abuf*>(entity))
			abuf_used ++;
		else if(dynamic_cast<Greenpak4Delay*>(entity))
			delay_used ++;
	}
	if(device->GetLFOscillator() && device->GetLFOscillator()->GetPARNode()->GetMate() != NULL)
		lfosc_used = 1;
	if(device->GetRingOscillator() && device->GetRingOscillator()->GetPARNode()->GetMate() != NULL)
		ringosc_used = 1;
	if(device->GetRCOscillator() && device->GetRCOscillator()->GetPARNode()->GetMate() != NULL)
		rcosc_used = 1;
	if(device->GetSystemReset() && device->GetSystemReset()->GetPARNode()->GetMate() != NULL)
		sysrst_used = 1;

	//Print the actual report
	//TODO: Figure out how to use indentation framework here for better columnar indents?

	LogNotice("\nDevice utilization:\n");
	LogIndenter li;

	unsigned int total_dff_used = dff_used + dffsr_used;
	unsigned int total_luts_used = luts_used[2] + luts_used[3] + luts_used[4];
	unsigned int total_counters_used = counters_8_used + counters_8_adv_used +
									    counters_14_used + counters_14_adv_used;
	unsigned int total_luts_count = lut_counts[2] + lut_counts[3] + lut_counts[4];
	unsigned int total_routes_used = num_routes_used[0] + num_routes_used[1];
	PrintRow("ABUF:",			abuf_used,				1);
	PrintRow("ACMP:",			acmp_used,				device->GetAcmpCount());
	PrintRow("BANDGAP:",		bandgap_used,			1);
	PrintRow("COUNT:",			total_counters_used,	device->GetCounterCount());
	PrintRow("  COUNT8:",		counters_8_used,		device->Get8BitCounterCount(false));
	PrintRow("  COUNT8_ADV:",	counters_8_adv_used,	device->Get8BitCounterCount(true));
	PrintRow("  COUNT14:",		counters_14_used,		device->Get14BitCounterCount(false));
	PrintRow("  COUNT14_ADV:",	counters_14_adv_used,	device->Get14BitCounterCount(true));
	PrintRow("DELAY:",			delay_used,				device->GetDelayCount());
	//TODO: print {as DELAY / as EDGEDET}
	PrintRow("FF:",				total_dff_used,			device->GetTotalFFCount());
	PrintRow("  DFF:",			dff_used,				device->GetDFFCount());
	PrintRow("  DFFSR:",		dffsr_used,				device->GetDFFSRCount());
	PrintRow("IOB:",			iobs_used,				device->GetIOBCount());
	PrintRow("INV:",			inv_used,				device->GetInverterCount());
	PrintRow("LFOSC:",			lfosc_used,				1);
	PrintRow("LUT:",			total_luts_used,		total_luts_count);
	for(unsigned int i=2; i<=4; i++)
		PrintRow(string("  LUT") + std::to_string(i),
		         				luts_used[i],			lut_counts[i]);
	PrintRow("PGA:",			pga_used,				1);
	PrintRow("POR:",			por_used,				1);
	PrintRow("RCOSC:",			rcosc_used,				1);
	PrintRow("RINGOSC:",		ringosc_used,			1);
	PrintRow("SHREG:",			shreg_used,				device->GetShiftRegisterCount());
	PrintRow("SYSRST:",			sysrst_used,			1);
	PrintRow("VREF:",			vref_used,				device->GetVrefCount());
	PrintRow("X-conn:",			total_routes_used,		20);
	PrintRow("  East:",			num_routes_used[0],		10);
	PrintRow("  West:",			num_routes_used[1],		10);
}

/**
	@brief Print the report showing which cells in the netlist got placed where
 */
void PrintPlacementReport(PARGraph* netlist, Greenpak4Device* /*device*/)
{
	LogVerbose("\nPlacement report:\n");
	LogIndenter li;

	LogVerbose("+----------------------------------------------------+-----------------+\n");
	LogVerbose("| %-50s | %-15s |\n", "Node", "Site");

	for(uint32_t i=0; i<netlist->GetNumNodes(); i++)
	{
		auto nnode = netlist->GetNodeByIndex(i);
		auto src = static_cast<Greenpak4NetlistEntity*>(nnode->GetData());
		auto dnode = nnode->GetMate();

		//unplaced, ignore it for now
		if(dnode == NULL)
			continue;
		auto dst = static_cast<Greenpak4BitstreamEntity*>(dnode->GetData());

		LogVerbose("| %-50s | %-15s |\n", src->m_name.c_str(), dst->GetDescription().c_str());

	}

	LogVerbose("+----------------------------------------------------+-----------------+\n");
}
