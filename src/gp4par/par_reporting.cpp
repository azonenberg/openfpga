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

#include "gp4par.h"

using namespace std;

typedef vector<const PARGraphEdge*> CombinatorialPath;

void FindCombinatorialPaths(
	const set<Greenpak4NetlistCell*>& sources,
	const set<Greenpak4NetlistCell*>& sinks,
	set<CombinatorialPath>& paths);

void FindCombinatorialPaths(
	const set<Greenpak4NetlistCell*>& sinks,
	CombinatorialPath basepath,
	const PARGraphEdge* tip,
	set<CombinatorialPath>& paths);

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
	unsigned int pwrdet_used = 0;
	unsigned int dcmp_used = 0;
	unsigned int dcmpref_used = 0;
	unsigned int dcmpmux_used = 0;
	unsigned int clkbuf_used = 0;
	unsigned int spi_used = 0;
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
		else if(dynamic_cast<Greenpak4DigitalComparator*>(entity))
			dcmp_used ++;
		else if(dynamic_cast<Greenpak4DCMPRef*>(entity))
			dcmpref_used ++;
		else if(dynamic_cast<Greenpak4PGA*>(entity))
			pga_used ++;
		else if(dynamic_cast<Greenpak4Delay*>(entity))
			delay_used ++;
		else if(dynamic_cast<Greenpak4ClockBuffer*>(entity))
			clkbuf_used ++;
	}
	if(device->GetLFOscillator() && device->GetLFOscillator()->GetPARNode()->GetMate() != NULL)
		lfosc_used = 1;
	if(device->GetRingOscillator() && device->GetRingOscillator()->GetPARNode()->GetMate() != NULL)
		ringosc_used = 1;
	if(device->GetRCOscillator() && device->GetRCOscillator()->GetPARNode()->GetMate() != NULL)
		rcosc_used = 1;
	if(device->GetSystemReset() && device->GetSystemReset()->GetPARNode()->GetMate() != NULL)
		sysrst_used = 1;
	if(device->GetPowerDetector() && device->GetPowerDetector()->GetPARNode()->GetMate() != NULL)
		pwrdet_used = 1;
	if(device->GetAbuf() && device->GetAbuf()->GetPARNode()->GetMate() != NULL)
		abuf_used = 1;
	if(device->GetDCMPMux() && device->GetDCMPMux()->GetPARNode()->GetMate() != NULL)
		dcmpmux_used = 1;
	if(device->GetSPI() && device->GetSPI()->GetPARNode()->GetMate() != NULL)
		spi_used = 1;

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
	PrintRow("CLKBUF:",			clkbuf_used,			device->GetClockBufferCount());
	PrintRow("COUNT:",			total_counters_used,	device->GetCounterCount());
	PrintRow("  COUNT8:",		counters_8_used,		device->Get8BitCounterCount(false));
	PrintRow("  COUNT8_ADV:",	counters_8_adv_used,	device->Get8BitCounterCount(true));
	PrintRow("  COUNT14:",		counters_14_used,		device->Get14BitCounterCount(false));
	PrintRow("  COUNT14_ADV:",	counters_14_adv_used,	device->Get14BitCounterCount(true));
	PrintRow("DCMP:",			dcmp_used,				device->GetDcmpCount());
	PrintRow("DCMPREF:",		dcmpref_used,			device->GetDcmpRefCount());
	PrintRow("DCMPMUX:",		dcmpmux_used,			1);
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
	PrintRow("PWRDET:",			pwrdet_used,			1);
	PrintRow("RCOSC:",			rcosc_used,				1);
	PrintRow("RINGOSC:",		ringosc_used,			1);
	PrintRow("SHREG:",			shreg_used,				device->GetShiftRegisterCount());
	PrintRow("SPI:",			spi_used,				1);
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

/**
	@brief Prints the post-PAR static timing analysis

	TODO: Refactor this into Greenpak4PAREngine?
 */
void PrintTimingReport(Greenpak4Netlist* netlist, Greenpak4Device* device)
{
	if(!device->HasTimingData())
	{
		LogWarning("No timing data for target device, not running timing analysis\n");
		return;
	}

	LogNotice("\nTiming report:\n");
	LogIndenter li;

	//Find all nodes in the design that can begin a combinatorial path
	//This is only input buffers for now, but eventually will include DFFs and other stateful blocks
	set<Greenpak4NetlistCell*> sources;
	auto module = netlist->GetTopModule();
	for(auto it = module->cell_begin(); it != module->cell_end(); it++)
	{
		auto cell = it->second;

		//TODO: check stateful internal blocks too
		if(!cell->IsIbuf())
			continue;
		sources.emplace(cell);
	}

	//Find all nodes in the design that can end a combinatorial path
	//This is only output buffers for now, but eventually will include DFFs and other stateful blocks
	set<Greenpak4NetlistCell*> sinks;
	for(auto it = module->cell_begin(); it != module->cell_end(); it++)
	{
		auto cell = it->second;

		//TODO: check stateful internal blocks too
		if(!cell->IsObuf())
			continue;
		sinks.emplace(cell);
	}

	//Find all combinatorial paths in the design.
	set<CombinatorialPath> paths;
	FindCombinatorialPaths(sources, sinks, paths);

	//The corner we're testing at (TODO multi-corner)
	PTVCorner corner(PTVCorner::SPEED_TYPICAL, 25, 3300);
	LogVerbose("Running static timing for %s (%zu paths)\n", corner.toString().c_str(), paths.size());

	//DEBUG: print all paths
	int i=0;
	bool incomplete = false;
	for(auto path : paths)
	{
		LogVerbose("Path %d\n", i++);
		LogIndenter li;
		LogVerbose(
			"+--------------------------------------------------------------+------------+-----------"
			"+------------+------------+------------+\n");
		LogVerbose("| %60s | %10s | %10s| %10s | %10s | %10s |\n",
			"Instance",
			"Site",
			"SrcPort",
			"DstPort",
			"Delay",
			"Cumulative"
			);
		LogVerbose(
			"+--------------------------------------------------------------+------------+-----------"
			"+------------+------------+------------+\n");

		float cdelay = 0;

		//TODO: figure this out for DFFs etc
		string previous_port = "IO";

		for(auto edge : path)
		{
			auto src = static_cast<Greenpak4NetlistEntity*>(edge->m_sourcenode->GetData());
			auto srccell = static_cast<Greenpak4BitstreamEntity*>(edge->m_sourcenode->GetMate()->GetData());
			auto dst = static_cast<Greenpak4NetlistEntity*>(edge->m_destnode->GetData());
			auto dstcell = static_cast<Greenpak4BitstreamEntity*>(edge->m_destnode->GetMate()->GetData());

			//If the source is an input buffer, add a delay for that
			auto sncell = dynamic_cast<Greenpak4NetlistCell*>(src);
			if(sncell->IsIbuf())
			{
				//Look up the delay
				CombinatorialDelay delay;
				if(!srccell->GetCombinatorialDelay(previous_port, edge->m_sourceport, corner, delay))
				{
					//DEBUG: use data from other pins if we haven't characterized this one yet!!!
					auto rscell = device->GetIOB(3);
					if(!rscell->GetCombinatorialDelay(previous_port, edge->m_sourceport, corner, delay))
					{
						//LogWarning("Couldn't get timing data even from fallback pin\n");
						incomplete = true;
						continue;
					}
				}

				float worst = delay.GetWorst();
				cdelay += worst;

				LogVerbose("| %60s | %10s | %10s| %10s | %10.3f | %10.3f |\n",
					src->m_name.c_str(),
					srccell->GetDescription().c_str(),
					previous_port.c_str(),
					edge->m_sourceport.c_str(),
					worst,
					cdelay
					);

				//Done, save the old source
				previous_port = edge->m_destport;
				continue;
			}

			//Cell delay
			CombinatorialDelay delay;
			if(!srccell->GetCombinatorialDelay(previous_port, edge->m_sourceport, corner, delay))
			{
				//LogWarning("Couldn't get timing data\n");
				incomplete = true;
				continue;
			}
			float worst = delay.GetWorst();
			cdelay += worst;

			LogVerbose("| %60s | %10s | %10s| %10s | %10.3f | %10.3f |\n",
				src->m_name.c_str(),
				srccell->GetDescription().c_str(),
				previous_port.c_str(),
				edge->m_sourceport.c_str(),
				worst,
				cdelay
				);

			//Done, save the old source
			previous_port = edge->m_destport;

			//See if we used a cross-connection on the next hop
			auto dsrc = dstcell->GetInput(edge->m_destport);
			auto xc = dynamic_cast<Greenpak4CrossConnection*>(dsrc.GetRealEntity());
			if(xc)
			{
				if(!xc->GetCombinatorialDelay("I", "O", corner, delay))
				{
					//LogWarning("Couldn't get timing data\n");
					incomplete = true;
					continue;
				}
				float worst = delay.GetWorst();
				cdelay += worst;

				LogVerbose("| %60s | %10s | %10s| %10s | %10.3f | %10.3f |\n",
					"__routing__",
					xc->GetDescription().c_str(),
					"I",
					"O",
					worst,
					cdelay
					);
			}

			//If the destination is an output buffer, add that path
			auto dncell = dynamic_cast<Greenpak4NetlistCell*>(dst);
			if(dncell->IsObuf())
			{
				//Look up the delay
				CombinatorialDelay delay;

				if(!dstcell->GetCombinatorialDelay(edge->m_destport, "IO", corner, delay))
				{
					//DEBUG: use data from other pins if we haven't characterized this one yet!!!
					auto rscell = device->GetIOB(3);
					if(!rscell->GetCombinatorialDelay(edge->m_destport, "IO", corner, delay))
					{
						//LogWarning("Couldn't get timing data even from fallback pin\n");
						incomplete = true;
						continue;
					}
				}

				float worst = delay.GetWorst();
				cdelay += worst;

				LogVerbose("| %60s | %10s | %10s| %10s | %10.3f | %10.3f |\n",
					dst->m_name.c_str(),
					dstcell->GetDescription().c_str(),
					edge->m_destport.c_str(),
					"IO",
					worst,
					cdelay
					);
			}
		}
		LogVerbose(
			"+--------------------------------------------------------------+------------+-----------"
			"+------------+------------+------------+\n");
	}
	if(incomplete)
		LogWarning("Timing data doesn't have info for all primitives, report is incomplete\n");
}

/**
	@brief Go over all cells in the netlist and DFS until we find all paths from source to sink nodes

	Source: input buffer or stateful element
	Sink: output buffer or stateful element

	We don't care about the timing of these paths yet, these are just all of the paths to consider
 */
void FindCombinatorialPaths(
	const set<Greenpak4NetlistCell*>& sources,
	const set<Greenpak4NetlistCell*>& sinks,
	set<CombinatorialPath>& paths)
{
	//Find all edges leaving all source nodes
	for(auto cell : sources)
	{
		auto srcnode = cell->m_parnode;
		for(uint32_t i=0; i<srcnode->GetEdgeCount(); i++)
		{
			auto edge = srcnode->GetEdgeByIndex(i);
			//LogDebug("Edge: %s - %s\n", edge->m_sourceport.c_str(), edge->m_destport.c_str());

			//Find all paths that begin with this edge
			CombinatorialPath path;
			path.push_back(edge);
			FindCombinatorialPaths(sinks, path, edge, paths);
		}
	}
}

/**
	@brief Recursively search out from a given path
 */
void FindCombinatorialPaths(
	const set<Greenpak4NetlistCell*>& sinks,
	CombinatorialPath basepath,
	const PARGraphEdge* tip,
	set<CombinatorialPath>& paths)
{
	//Jump through some hoops to see where this edge ends
	auto destnode = tip->m_destnode;
	auto destent = static_cast<Greenpak4NetlistEntity*>(destnode->GetData());
	auto destcell = dynamic_cast<Greenpak4NetlistCell*>(destent);
	if(!destcell)
	{
		LogWarning("Dest node is not cell\n");
		return;
	}

	//If it ends at a sink node, that's the end of this path
	if(sinks.find(destcell) != sinks.end())
	{
		paths.emplace(basepath);
		return;
	}

	//Nope, this node is not the end. Search all outbound edges from it
	for(uint32_t i=0; i<destnode->GetEdgeCount(); i++)
	{
		auto edge = destnode->GetEdgeByIndex(i);

		//Verify that this edge is not already in the netlist. If it is, complain and stop
		bool loop = false;
		for(auto tedge : basepath)
		{
			if(edge == tedge)
			{
				loop = true;

				LogWarning("Found combinatorial loop, not following it for static timing\n");
				LogIndenter li;

				for(auto eedge : basepath)
				{
					auto snode = static_cast<Greenpak4NetlistEntity*>(eedge->m_sourcenode->GetData());
					auto dnode = static_cast<Greenpak4NetlistEntity*>(eedge->m_destnode->GetData());
					LogWarning("From cell %50s port %15s to %50s port %15s\n",
						snode->m_name.c_str(),
						eedge->m_sourceport.c_str(),
						dnode->m_name.c_str(),
						eedge->m_destport.c_str());
				}

				break;
			}
		}
		if(loop)
			continue;

		//Find all paths that begin with the new, longer path
		CombinatorialPath npath = basepath;
		npath.push_back(edge);
		FindCombinatorialPaths(sinks, npath, edge, paths);
	}
}
