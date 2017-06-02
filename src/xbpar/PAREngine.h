/***********************************************************************************************************************
 * Copyright (C) 2017 Andrew Zonenberg and contributors                                                                *
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
#include <map>

/**
	@brief The core place-and-route engine
 */
class PAREngine
{
public:
	PAREngine(PARGraph* netlist, PARGraph* device);
	virtual ~PAREngine();

	virtual bool PlaceAndRoute(std::map<uint32_t, std::string> label_names, uint32_t seed = 0);

	virtual uint32_t ComputeCost() const;

protected:

	virtual bool CanMoveNode(
		const PARGraphNode* node,
		const PARGraphNode* old_mate,
		const PARGraphNode* new_mate) const;

	void MoveNode(PARGraphNode* node, PARGraphNode* newpos, std::map<uint32_t, std::string>& label_names);

	virtual PARGraphNode* GetNewPlacementForNode(const PARGraphNode* pivot) =0;
	virtual void FindSubOptimalPlacements(std::vector<PARGraphNode*>& bad_nodes) =0;

	virtual uint32_t ComputeAndPrintScore(std::vector<const PARGraphEdge*>& unroutes, uint32_t iteration) const;

	virtual void PrintUnroutes(const std::vector<const PARGraphEdge*>& unroutes) const;

	virtual uint32_t ComputeCongestionCost() const;
	virtual uint32_t ComputeTimingCost() const;
	virtual uint32_t ComputeUnroutableCost(std::vector<const PARGraphEdge*>& unroutes) const;

	virtual bool SanityCheck(std::map<uint32_t, std::string> label_names) const;
	virtual bool InitialPlacement(std::map<uint32_t, std::string>& label_names);
	virtual bool InitialPlacement_core() =0;
	virtual bool OptimizePlacement(
		const std::vector<PARGraphNode*>& badnodes,
		std::map<uint32_t, std::string>& label_names);

	virtual uint32_t ComputeNodeUnroutableCost(const PARGraphNode* pivot, const PARGraphNode* candidate) const;

	std::string GetNodeTypes(const PARGraphNode* node, std::map<uint32_t, std::string>& label_names) const;

	PARGraph*const m_netlist;
	PARGraph*const m_device;

	//Best solution found so far
	std::map<PARGraphNode*, PARGraphNode*> m_bestPlacementFound;

	void SaveNewBestPlacement();
	void RestorePreviousBestPlacement();

	uint32_t m_temperature;
	uint32_t m_maxTemperature;

	//libc-independent RNG
	//A PCG random number generator
	//Does not currently generate a k-dimensional equidistribution, but is
	//much better than an LCG.
	uint32_t RandomNumber();
	uint64_t m_randomState;
};

#endif

