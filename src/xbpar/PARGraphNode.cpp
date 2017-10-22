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

#include <xbpar.h>

using namespace std;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Construction / destruction

PARGraphNode::PARGraphNode(uint32_t label, void* pData)
	: m_label(label)
	, m_pData(pData)
	, m_mate(NULL)
{
}

PARGraphNode::~PARGraphNode()
{
	for(auto x : m_edges)
		delete x;
	m_edges.clear();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Accessors

void PARGraphNode::MateWith(PARGraphNode* mate)
{
	//Clear our prior mate, if any
	//need to set m_mate to null FIRST to avoid infinite recursion
	if(m_mate != NULL)
	{
		PARGraphNode* oldmate = m_mate;
		m_mate = NULL;
		oldmate->MateWith(NULL);
	}

	//Valid mate, set up back pointer
	//Clear out old partner of the new mating node, of any
	if(mate != NULL)
	{
		mate->MateWith(NULL);
		mate->m_mate = this;
	}

	//and set the forward pointer regardless
	m_mate = mate;
}

uint32_t PARGraphNode::GetEdgeCount() const
{
	return m_edges.size();
}

const PARGraphEdge* PARGraphNode::GetEdgeByIndex(uint32_t index) const
{
	return m_edges[index];
}

bool PARGraphNode::MatchesLabel(uint32_t target) const
{
	if(m_label == target)
		return true;
	for(auto x : m_alternateLabels)
	{
		if(x == target)
			return true;
	}
	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Topology modification

/**
	@brief Remove the given edge, if found
 */
void PARGraphNode::RemoveEdge(string srcport, PARGraphNode* sink, string dstport)
{
	for(ssize_t i=m_edges.size()-1; i>=0; i--)
	{
		//skip if not a match
		auto edge = m_edges[i];
		if( (edge->m_sourceport != srcport) || (edge->m_destport != dstport) )
			continue;
		if(edge->m_destnode != sink)
			continue;

		//Match, remove it
		delete edge;
		m_edges.erase(m_edges.begin() + i);
	}
}
