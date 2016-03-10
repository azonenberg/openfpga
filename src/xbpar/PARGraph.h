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
 
#ifndef PARGraph_h
#define PARGraph_h

#include <vector>

class PARGraphNode;

/**
	@brief A place-and-route graph (may be either a netlist or a device)
 */
class PARGraph
{
public:
	PARGraph();
	virtual ~PARGraph();
	
	//Label stuff
	uint32_t AllocateLabel();
	uint32_t GetMaxLabel();
	
	//Label indexing helpers
	void CountLabels();
	uint32_t GetNumNodesWithLabel(uint32_t label);
	void IndexNodesByLabel();
	PARGraphNode* GetNodeByLabelAndIndex(uint32_t label, uint32_t index);

protected:

	typedef std::vector<PARGraphNode*> NodeVector;

	/**
		@brief The set of all nodes in the graph. No implied order.
	 */
	NodeVector m_nodes;
	
	/**
		@brief The highest label allocated to date.
	 */
	uint32_t m_nextLabel;
	
	/**
		@brief Count of how many nodes have each label

		Updated only when CountLabels() is called
	 */
	std::vector<uint32_t> m_labelCount;
	
	/**
		@brief Set of nodes sorted by label
	 */
	std::vector< NodeVector > m_labeledNodes;
};

#endif
