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

bool CheckAnalogIbuf(Greenpak4BitstreamEntity* load, Greenpak4IOB* iob);

/**
	@brief The main place-and-route logic
 */
bool DoPAR(Greenpak4Netlist* netlist, Greenpak4Device* device)
{
	labelmap lmap;

	//Create the graphs
	LogNotice("\nCreating netlist graphs...\n");
	PARGraph* ngraph = NULL;
	PARGraph* dgraph = NULL;
	if(!BuildGraphs(netlist, device, ngraph, dgraph, lmap))
		return false;

	//Create and run the PAR engine
	Greenpak4PAREngine engine(ngraph, dgraph, lmap);
	uint32_t seed = 0;
	if(!engine.PlaceAndRoute(lmap, seed))
	{
		//Print the placement we have so far
		PrintPlacementReport(ngraph, device);

		LogNotice("PAR failed\n");
		return false;
	}

	//Copy the netlist over
	unsigned int num_routes_used[2];
	if(!CommitChanges(dgraph, device, num_routes_used))
	{
		LogNotice("Final routing failed\n");

		//Placement is done, so print the placement report before we die
		PrintUtilizationReport(ngraph, device, num_routes_used);
		PrintPlacementReport(ngraph, device);
		return false;
	}

	//Final DRC to make sure the placement is sane
	if(!PostPARDRC(ngraph, device))
		return false;

	//Print reports
	PrintUtilizationReport(ngraph, device, num_routes_used);
	PrintPlacementReport(ngraph, device);
	PrintTimingReport(netlist, device);

	//Final cleanup
	delete ngraph;
	delete dgraph;
	return true;
}

/**
	@brief Do various sanity checks after the design is routed

	Also do a couple of tweaks to the bitstream that we can't find any other good spot for
	TODO: this really should be done somewhere else?
 */
bool PostPARDRC(PARGraph* netlist, Greenpak4Device* device)
{
	LogNotice("\nChecking post-route design rules...\n");
	LogIndenter li;

	bool ok = true;

	//Check for nodes in the netlist that have no load
	for(uint32_t i=0; i<netlist->GetNumNodes(); i++)
	{
		auto node = netlist->GetNodeByIndex(i);
		auto src = static_cast<Greenpak4NetlistEntity*>(node->GetData());
		auto mate = node->GetMate();
		auto dst = static_cast<Greenpak4BitstreamEntity*>(mate->GetData());

		//Sanity check - must be fully PAR'd
		if(mate == NULL)
		{
			LogError(
				"Node \"%s\" is not mapped to any site in the device\n",
				src->m_name.c_str());
			ok = false;
		}

		//Do not warn if power rails have no load, that's perfectly normal
		if(dynamic_cast<Greenpak4PowerRail*>(dst) != NULL)
			continue;

		//If the node has no output ports, of course it won't have any loads
		if(dst->GetOutputPorts().size() == 0)
			continue;

		//If the node is an IOB configured as an output, there's no internal load for its output.
		//This is perfectly normal, obviously.
		Greenpak4NetlistCell* cell = dynamic_cast<Greenpak4NetlistCell*>(src);
		if( (cell != NULL) &&  ( (cell->m_type == "GP_IOBUF") || (cell->m_type == "GP_OBUF") ) )
			continue;

		//If we have a magic attribute set, it's OK
		//(for example, inferred ACMP for a VREF we used for another purpose)
		if(cell->HasAttribute("__IGNORE__NOLOAD__"))
			continue;

		//If we have no loads, warn
		if(node->GetEdgeCount() == 0)
		{
			LogWarning(
				"Node \"%s\" has no load\n",
				src->m_name.c_str());
		}

	}

	//Check for IOBs that are not LOC'd
	for(auto it = device->iobbegin(); it != device->iobend(); it ++)
	{
		Greenpak4IOB* iob = it->second;

		//Get the netlist cell for this IOB
		//If no cell found we're unused so no point in looking
		auto npnode = iob->GetPARNode()->GetMate();
		if(npnode == NULL)
			continue;
		Greenpak4NetlistCell* ncell = static_cast<Greenpak4NetlistCell*>(npnode->GetData());
		if(ncell == NULL)
			continue;

		//If we are not LOC'd, warn.
		//This is not an error because sometimes a user may want to let PAR find a placement for a complex
		//netlist before laying out the board, to improve routability. We warn because it's easy to forget
		//to add LOC constraints afterwards, causing a future ECO to break the pinout.
		if(!ncell->HasLOC())
		{
			LogWarning(
				"IOB cell %s was placed at location %s, but was not locked with a LOC constraint.\n"
				"This can lead to unexpected pinout changes if the netlist is modified.\n",
				ncell->m_name.c_str(), iob->GetDescription().c_str()
				);
		}
	}

	//TODO: check floating inputs etc?

	//Check invalid IOB configuration
	//TODO: driving an input-only pin etc - is this possible?
	for(auto it = device->iobbegin(); it != device->iobend(); it ++)
	{
		Greenpak4IOB* iob = it->second;

		auto signal = iob->GetOutputSignal();
		auto src = signal.GetRealEntity();

		if(!iob->IsAnalogIbuf())
		{
			//Check for analog output driving a pin not configured as analog for the input
			if(	(dynamic_cast<Greenpak4VoltageReference*>(src) != NULL) ||
				(dynamic_cast<Greenpak4PGA*>(src) != NULL) )
			{
				LogError("Pin %d is driven by an analog source (%s) but does not have IBUF_TYPE = ANALOG\n",
					it->first,
					signal.GetOutputName().c_str()
					);
				ok = false;
			}
		}
	}

	//Check for ACMPs with inputs driven from non-analog IOs
	for(unsigned int i=0; i<device->GetAcmpCount(); i++)
	{
		auto acmp = device->GetAcmp(i);
		if(!CheckAnalogIbuf(acmp, dynamic_cast<Greenpak4IOB*>(acmp->GetInput().GetRealEntity())))
			ok = false;
	}

	//Check for ABUF with inputs driven from non-analog IOs
	auto abuf = device->GetAbuf();
	if(abuf && !CheckAnalogIbuf(abuf, dynamic_cast<Greenpak4IOB*>(abuf->GetInput().GetRealEntity())))
		return false;

	//Check for PGA with inputs driven from non-analog IOs
	auto pga = device->GetPGA();
	if(pga)
	{
		if(!CheckAnalogIbuf(abuf, dynamic_cast<Greenpak4IOB*>(pga->GetInputP().GetRealEntity())))
			ok = false;
		if(!CheckAnalogIbuf(abuf, dynamic_cast<Greenpak4IOB*>(pga->GetInputN().GetRealEntity())))
			ok = false;
	}

	//TODO: Check for VREF with inputs driven from non-analog IOs

	//TODO: Check for PGA driving an IOB when ADC is enabled
	//auto adc = device->GetADC();
	if(pga /*&& adc*/)
	{
		//We do not yet implement the ADC
		//so nothing to do here
	}

	//TODO: Check for DACs not sharing input with DCMP1 negative, but also not a constant

	//TODO: check for DCMPs with wrong input bit ordering or not same source for all bits

	//Check for multiple ACMPs using different settings of ACMP0's output mux
	typedef pair<string, Greenpak4EntityOutput> spair;
	switch(device->GetPart())
	{
		case Greenpak4Device::GREENPAK4_SLG46620:
			{
				auto pin6 = device->GetIOB(6)->GetOutput("");
				auto vdd = device->GetPower();
				auto gnd = device->GetGround();

				vector<spair> inputs;

				//Loop over each ACMP that could possibly use the ACMP0 (shared) mux
				for(unsigned int i=0; i<device->GetAcmpCount(); i++)
				{
					auto acmp = device->GetAcmp(i);
					auto input = acmp->GetInput();

					//If this comparator is not using one of ACMP0's inputs, we don't care
					//TODO: buffered pin 6 is a candidate too
					if((input != pin6) && (input != vdd) )
						continue;

					//Look up the instance name of the comparator. Sanity check that it's used.
					auto mate = acmp->GetPARNode()->GetMate();
					if(mate == NULL)
						continue;
					auto node = static_cast<Greenpak4NetlistEntity*>(mate->GetData());
					inputs.push_back(spair(node->m_name, input));
				}

				//Check the active inputs and make sure they're the same
				Greenpak4EntityOutput shared_input = gnd;
				for(auto s : inputs)
				{
					//If the shared input isn't used, this is the new value for the mux
					if(shared_input == gnd)
						shared_input = s.second;

					//If the shared input is used, but has the same value, we're good - the sharing did its job
					if(shared_input == s.second)
						continue;

					//Problem! Incompatible mux settings
					LogError(
						"Multiple comparators tried to simultaneously use different outputs from "
						"the ACMP0 input mux\n");
					LogIndenter li;
					for(auto p : inputs)
					{
						LogNotice("Comparator %10s requested %s\n",
							p.first.c_str(), p.second.GetOutputName().c_str());
					}
					ok = false;
				}

				//If ACMP0 is not used, but we use its output, configure it
				//TODO: for better power efficiency, turn on only when a downstream comparator is on?
				//TODO: This really should not be done in the DRC
				if( (device->GetAcmp(0)->GetInput() == gnd) && (inputs.size() > 0) )
				{
					LogNotice(
						"Enabling ACMP0 and configuring input mux, since output of mux "
						"is used but ACMP0 is not instantiated\n");

					auto acmp0 = device->GetAcmp(0);
					acmp0->SetInput(shared_input);
					acmp0->SetPowerEn(device->GetPowerOnReset()->GetOutput("RST_DONE"));
				}
			}
			break;

		default:
			break;
	}

	//Check for multiple oscillators with power-down enabled but not the same source
	Greenpak4LFOscillator* lfosc = device->GetLFOscillator();
	Greenpak4RingOscillator* rosc = device->GetRingOscillator();
	Greenpak4RCOscillator* rcosc = device->GetRCOscillator();
	vector<spair> powerdowns;
	if(lfosc && lfosc->IsUsed() && lfosc->GetPowerDownEn() && !lfosc->IsConstantPowerDown())
		powerdowns.push_back(spair(lfosc->GetDescription(), lfosc->GetPowerDown()));
	if(rosc && rosc->IsUsed() && rosc->GetPowerDownEn() && !rosc->IsConstantPowerDown())
		powerdowns.push_back(spair(rosc->GetDescription(), rosc->GetPowerDown()));
	if(rcosc && rcosc->IsUsed() && rcosc->GetPowerDownEn() && !rcosc->IsConstantPowerDown())
		powerdowns.push_back(spair(rcosc->GetDescription(), rcosc->GetPowerDown()));
	if(!powerdowns.empty())
	{
		Greenpak4EntityOutput src = device->GetGround();
		bool xok = true;
		for(auto p : powerdowns)
		{
			if(src.IsPowerRail())
				src = p.second;
			if(src != p.second)
				xok = false;
		}

		if(!xok)
		{
			LogError(
				"Multiple oscillators have power-down enabled, but do not share the same power-down signal\n");
			for(auto p : powerdowns)
				LogNotice("    Oscillator %10s powerdown is %s\n", p.first.c_str(), p.second.GetOutputName().c_str());

			ok = false;
		}
	}

	//Check for multiple oscillators with power-down enabled but not the same source
	powerdowns.clear();
	for(size_t i=0; i<device->GetDcmpCount(); i++)
	{
		auto d = device->GetDcmp(i);
		if(d->IsUsed())
			powerdowns.push_back(spair(d->GetDescription(), d->GetPowerDown()));
	}
	if(!powerdowns.empty())
	{
		Greenpak4EntityOutput src = powerdowns[0].second;
		bool xok = true;
		for(auto p : powerdowns)
		{
			if(src != p.second)
				xok = false;
		}

		if(!xok)
		{
			LogError(
				"Multiple DCMPs have power-down enabled, but do not share the same power-down signal\n");
			for(auto p : powerdowns)
				LogNotice("    DCMPs %10s powerdown is %s\n", p.first.c_str(), p.second.GetOutputName().c_str());

			ok = false;
		}
	}

	//ADC/DAC conflicts
	if( (device->GetPart() == Greenpak4Device::GREENPAK4_SLG46620) ||
		(device->GetPart() == Greenpak4Device::GREENPAK4_SLG46621))
	{
		//null check on pga is unnecessary for the 4662x, but necessary to avoid false positive from static analysis
		auto dac1 = device->GetDAC(1);
		if(pga && pga->IsUsed() && dac1->IsUsed())
		{
			LogError(
				"Both DAC1 and the PGA are used. This is illegal due to a poorly documented control hazard.\n"
				"Enabling the PGA turns on the SAR ADC, forcing DAC1 to emit a sawtooth waveform instead of the "
				"desired signal.\n");
			ok = false;
		}

		//null check to avoid false positive on static analysis
		auto dac0 = device->GetDAC(0);
		if(pga && pga->IsUsed() && dac0->IsUsed() && (pga->GetInputMode() == Greenpak4PGA::MODE_PDIFF) )
		{
			LogError(
				"DAC0 is used while the PGA is in pseudo-differential mode. This is illegal since the "
				"PGA uses DAC0 to provide the pseudo-differential offset voltage.\n");
		}

		//TODO: Cannot use DAC1 when ADC is used
	}

	//If POR is driven to an IOB, it should use dedicated routing
	//If not, warn about timing
	auto por = device->GetPowerOnReset()->GetPARNode()->GetMate();
	if(por)
	{
		Greenpak4IOB* reset_iob = NULL;

		//Find the dedicated reset pin
		auto part = device->GetPart();
		switch(part)
		{
			case Greenpak4Device::GREENPAK4_SLG46620:
			case Greenpak4Device::GREENPAK4_SLG46621:
				reset_iob = device->GetIOB(8);
				break;

			case Greenpak4Device::GREENPAK4_SLG46140:
				reset_iob = device->GetIOB(13);
				break;

			default:
				LogError("unrecognized device, cannot DRC POR routing\n");
		}

		//Look for IOBs driven by the PAR
		for(uint32_t i=0; i<por->GetEdgeCount(); i++)
		{
			auto dest = por->GetEdgeByIndex(i)->m_destnode->GetMate();
			auto n = static_cast<Greenpak4BitstreamEntity*>(dest->GetData())->GetRealEntity();

			auto ioba = dynamic_cast<Greenpak4IOBTypeA*>(n);
			auto iobb = dynamic_cast<Greenpak4IOBTypeB*>(n);

			if( (ioba == NULL) && (iobb == NULL) )
				continue;

			if(n != reset_iob)
			{
				LogWarning(
					"Pin %s is driven by the power-on reset, but does not have dedicated reset routing available.\n"
					"This may lead to synchronization issues or glitches if this pin is used to drive resets on "
					"external logic.\n",
					n->GetDescription().c_str());
			}
		}
	}

	//Charge pump must be ENABLED for brownout detector to work
	auto pwrdet = device->GetPowerDetector();
	if(pwrdet && (pwrdet->GetPARNode()->GetMate() != NULL) & device->IsChargePumpDisabled())
	{
		LogError(
			"The power detector is being used, but the charge pump is disabled.\n"
			"Enable the charge pump to use the power detector.\n"
			);
		ok = false;
	}

	//Done
	return ok;
}

bool CheckAnalogIbuf(Greenpak4BitstreamEntity* load, Greenpak4IOB* iob)
{
	if(!iob)
		return true;
	if(iob->IsAnalogIbuf())
		return true;

	LogError("%s is driven by IOB %s, which does not have IBUF_TYPE = ANALOG\n",
		load->GetDescription().c_str(),
		iob->GetDescription().c_str()
		);

	return false;
}

/**
	@brief Allocate and name a graph label
 */
uint32_t AllocateLabel(PARGraph*& ngraph, PARGraph*& dgraph, labelmap& lmap, std::string description)
{
	uint32_t nlabel = ngraph->AllocateLabel();
	uint32_t dlabel = dgraph->AllocateLabel();
	if(nlabel != dlabel)
	{
		LogFatal("Labels were allocated at the same time but don't match up\n");
	}

	lmap[nlabel] = description;

	return nlabel;
}
