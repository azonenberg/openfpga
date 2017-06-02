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

#ifndef Greenpak4PAREngine_h
#define Greenpak4PAREngine_h

/**
	@brief The place-and-route engine for Greenpak4
 */
class Greenpak4PAREngine : public PAREngine
{
public:
	Greenpak4PAREngine(PARGraph* netlist, PARGraph* device, labelmap& lmap);
	virtual ~Greenpak4PAREngine();

protected:
	virtual void PrintUnroutes(const std::vector<const PARGraphEdge*>& unroutes) const override;

	virtual void FindSubOptimalPlacements(std::vector<PARGraphNode*>& bad_nodes) override;
	virtual PARGraphNode* GetNewPlacementForNode(const PARGraphNode* pivot) override;

	virtual uint32_t ComputeCongestionCost() const override;
	virtual bool InitialPlacement_core() override;

	virtual bool CanMoveNode(const PARGraphNode* node,
		const PARGraphNode* old_mate, const PARGraphNode* new_mate) const override;

	virtual const char* GetLabelName(uint32_t label) const override;

	bool CantMoveSrc(Greenpak4BitstreamEntity* src);
	bool CantMoveDst(Greenpak4BitstreamEntity* dst);

	//Cached list of unroutable nodes for the current iteration
	std::set<const PARGraphNode*> m_unroutableNodes;

	//used for error messages only
	labelmap m_lmap;
};

#endif
