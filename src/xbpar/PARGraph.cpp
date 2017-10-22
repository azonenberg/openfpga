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
#include <unordered_map>
#include <unordered_set>

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
uint32_t PARGraph::GetMaxLabel() const
{
	return m_nextLabel - 1;
}

uint32_t PARGraph::GetNumNodes() const
{
	return m_nodes.size();
}

PARGraphNode* PARGraph::GetNodeByIndex(uint32_t index) const
{
	return m_nodes[index];
}

uint32_t PARGraph::GetNumEdges() const
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
uint32_t PARGraph::GetNumNodesWithLabel(uint32_t label) const
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
PARGraphNode* PARGraph::GetNodeByLabelAndIndex(uint32_t label, uint32_t index) const
{
	return m_labeledNodes[label][index];
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Debugging

#include <cstdio>

std::string PARGraph::DumpAsDot() const
{
	std::string ret;

	// Need to collect all inbound edges because these are harder to obtain
	std::unordered_map<PARGraphNode*, std::unordered_set<std::string>> inbound_edge_map;
	for(auto n : m_nodes)
	{
		for(uint32_t i = 0; i < n->GetEdgeCount(); i++)
		{
			auto e = n->GetEdgeByIndex(i);
			inbound_edge_map[e->m_destnode].insert(e->m_destport);
		}
	}

	ret += "digraph pargraph {\n";
	ret += "node [shape=record];\n";

	// Write out nodes
	for(auto n : m_nodes)
	{
		ret += "n" + std::to_string((uintptr_t)n) + " [label=\"";

		// Inbound
		if (inbound_edge_map[n].size())
		{
			ret += "{";
			size_t count = 0;
			for(auto i = inbound_edge_map[n].begin(); i != inbound_edge_map[n].end(); i++, count++)
			{
				ret += "<" + *i + "> ";
				ret += *i;
				if (count != inbound_edge_map[n].size() - 1)
					ret += "|";
			}
			ret += "}|";
		}

		// Labels
		ret += std::to_string(n->GetLabel());
		if (n->GetAlternateLabelCount())
		{
			ret += " (";
			for(size_t i = 0; i < n->GetAlternateLabelCount(); i++)
			{
				ret += std::to_string(n->GetAlternateLabel(i));
				if (i != n->GetAlternateLabelCount() - 1)
					ret += ", ";
			}
			ret += ")";
		}

		// Outbound
		if (n->GetEdgeCount())
		{
			std::unordered_set<std::string> used_outbound_ports;

			for(size_t i = 0; i < n->GetEdgeCount(); i++)
				used_outbound_ports.insert(n->GetEdgeByIndex(i)->m_sourceport);
			
			ret += "|{";
			size_t count = 0;
			for(auto i = used_outbound_ports.begin(); i != used_outbound_ports.end(); i++, count++)
			{
				ret += "<" + *i + "> ";
				ret += *i;
				if (count != used_outbound_ports.size() - 1)
					ret += "|";
			}
			ret += "}";
		}

		ret += "\"];\n";
	}

	// Write out edges
	for(auto n : m_nodes)
	{
		for(uint32_t i = 0; i < n->GetEdgeCount(); i++)
		{
			auto e = n->GetEdgeByIndex(i);

			ret += "n" + std::to_string((uintptr_t)e->m_sourcenode) + ":\"" + e->m_sourceport;
			ret += "\" -> ";
			ret += "n" + std::to_string((uintptr_t)e->m_destnode) + ":\"" + e->m_destport;
			ret += "\";\n";
		}
	}

	ret += "}\n";

	printf("%s", ret.c_str());
	fflush(stdout);

	return ret;
}
