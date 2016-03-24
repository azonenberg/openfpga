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
 
#ifndef PARGraphNode_h
#define PARGraphNode_h

#include <vector>
#include <string>
#include <set>

class PARGraphEdge
{
public:
	
	PARGraphEdge(PARGraphNode* source, PARGraphNode* dest, std::string port)
		: m_sourcenode(source)
		, m_destnode(dest)
		, m_destport(port)
	{
	}

	//the source node
	PARGraphNode* m_sourcenode;
	
	//todo: port name for source

	//the destination node
	PARGraphNode* m_destnode;
	
	//port name (if multiple ports in target)
	std::string m_destport;
};

/**
	@brief A single node in a place-and-route graph
 */
class PARGraphNode
{
public:
	PARGraphNode(uint32_t label, void* pData);
	virtual ~PARGraphNode();
	
	void MateWith(PARGraphNode* mate);

	//do not call during PAR, only during initialization of constraints
	//or caches will get very confused
	void Relabel(uint32_t label)
	{ m_label = label; }

	uint32_t GetLabel()
	{ return m_label; }
	
	PARGraphNode* GetMate()
	{ return m_mate; }

	uint32_t GetEdgeCount();
	PARGraphEdge* GetEdgeByIndex(uint32_t index);

	void AddEdge(PARGraphNode* sink, std::string port = "")
	{ m_edges.push_back(new PARGraphEdge(this, sink, port)); }

	void* GetData()
	{ return m_pData; }

	void AddAlternateLabel(uint32_t alt)
	{ m_alternateLabels.insert(alt); }

protected:
	
	/**
		@brief Label of this node. All nodes with the same label in a given graph are indistinguishable.
		
		Typically there is one label for each type of resource (LUT2, FDCE, etc) without location constraints.
		Each primitive with an exact location constraint has a unique label (so there is only one legal placement).
		
		Labels range from 0 to a given maximum in a particular graph.
	 */
	uint32_t m_label;
	
	/**
		@brief Set of alternate labels that this node can map to.
		
		Only used in netlist graphs (not device graphs).
	 */
	std::set<uint32_t> m_alternateLabels;
	
	/**
		@brief Pointer to the external node (netlist or device entity) associated with this PAR node
	 */
	void* m_pData;
	
	/**
		@brief Pointer to the mating node of this one.
		
		For the netlist, this points to a device node. For the device, this points to a netlist node.
	 */
	PARGraphNode* m_mate;
	
	/**
		@brief List of all outbound edges from this node (TODO have more metadata on them?)
	 */
	std::vector<PARGraphEdge*> m_edges;
};

#endif
