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

	virtual uint32_t ComputeCost();

protected:

	virtual bool CanMoveNode(PARGraphNode* node, PARGraphNode* old_mate, PARGraphNode* new_mate);

	void MoveNode(PARGraphNode* node, PARGraphNode* newpos, std::map<uint32_t, std::string>& label_names);

	virtual PARGraphNode* GetNewPlacementForNode(PARGraphNode* pivot) =0;
	virtual void FindSubOptimalPlacements(std::vector<PARGraphNode*>& bad_nodes) =0;

	virtual uint32_t ComputeAndPrintScore(std::vector<PARGraphEdge*>& unroutes, uint32_t iteration);

	virtual void PrintUnroutes(std::vector<PARGraphEdge*>& unroutes);

	virtual uint32_t ComputeCongestionCost();
	virtual uint32_t ComputeTimingCost();
	virtual uint32_t ComputeUnroutableCost(std::vector<PARGraphEdge*>& unroutes);

	virtual bool SanityCheck(std::map<uint32_t, std::string> label_names);
	virtual bool InitialPlacement(std::map<uint32_t, std::string>& label_names);
	virtual bool InitialPlacement_core() =0;
	virtual bool OptimizePlacement(
		std::vector<PARGraphNode*>& badnodes,
		std::map<uint32_t, std::string>& label_names);

	virtual uint32_t ComputeNodeUnroutableCost(PARGraphNode* pivot, PARGraphNode* candidate);

	std::string GetNodeTypes(PARGraphNode* node, std::map<uint32_t, std::string>& label_names);

	PARGraph* m_netlist;
	PARGraph* m_device;

	//Best solution found so far
	std::map<PARGraphNode*, PARGraphNode*> m_bestPlacementFound;

	void SaveNewBestPlacement();
	void RestorePreviousBestPlacement();

	uint32_t m_temperature;
};

#endif

