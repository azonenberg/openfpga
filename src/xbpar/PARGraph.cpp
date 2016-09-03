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

#include "xbpar.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Construction / destruction

PARGraph::PARGraph()
	: m_nextLabel(0)
{

}

PARGraph::~PARGraph()
{
	for(auto x : m_nodes)
		delete x;
	m_nodes.clear();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Accessors

/**
	@brief Allocate a new unique label ID
 */
uint32_t PARGraph::AllocateLabel()
{
	return (m_nextLabel ++);
}

/**
	@brief Get the maximum allocated label value
 */
uint32_t PARGraph::GetMaxLabel()
{
	return m_nextLabel - 1;
}

uint32_t PARGraph::GetNumNodes()
{
	return m_nodes.size();
}

PARGraphNode* PARGraph::GetNodeByIndex(uint32_t index)
{
	return m_nodes[index];
}

uint32_t PARGraph::GetNumEdges()
{
	uint32_t netcount = 0;

	for(auto x : m_nodes)
		netcount += x->GetEdgeCount();

	return netcount;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Insertion

void PARGraph::AddNode(PARGraphNode* node)
{
	m_nodes.push_back(node);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Label counting helpers

/**
	@brief Look up how many nodes have a given label.

	Value is cached by IndexNodesByLabel();
 */
uint32_t PARGraph::GetNumNodesWithLabel(uint32_t label)
{
	return m_labeledNodes[label].size();
}

/**
	@brief Generate an index (in m_labeledNodes) of nodes sorted by label
 */
void PARGraph::IndexNodesByLabel()
{
	//Rebuild the label table
	m_labeledNodes.clear();
	for(uint32_t i=0; i<m_nextLabel; i++)
		m_labeledNodes.push_back(NodeVector());

	//Do the indexing for primary labels first
	for(auto x : m_nodes)
		m_labeledNodes[x->GetLabel()].push_back(x);

	//Add secondary labels last (so lower priority
	for(auto x : m_nodes)
	{
		for(uint32_t i = 0; i<x->GetAlternateLabelCount(); i++)
			m_labeledNodes[x->GetAlternateLabel(i)].push_back(x);
	}
}

/**
	@brief Get the Nth node with a given index
 */
PARGraphNode* PARGraph::GetNodeByLabelAndIndex(uint32_t label, uint32_t index)
{
	return m_labeledNodes[label][index];
}
