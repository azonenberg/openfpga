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

#include "xbpar_ffi.h"

#include <cstring>

#define PTR_LEN_TO_STRING(x) \
	std::string x = std::string(x##_ptr, x##_len);
#define PTR_LEN_INTO_VECTOR(x) \
	for (size_t i = 0; i < x##_len; i++) \
		x.push_back(x##_ptr[i]);

void xbpar_ffi_free_object(void *obj)
{
	free(obj);
}


//PARGraphEdge
PARGraphNode* xbpar_PARGraphEdge_get_sourcenode(const PARGraphEdge* edge)
{
	return edge->m_sourcenode;
}

PARGraphNode* xbpar_PARGraphEdge_get_destnode(const PARGraphEdge* edge)
{
	return edge->m_destnode;
}

const char* xbpar_PARGraphEdge_get_sourceport(const PARGraphEdge* edge, size_t* len)
{
	*len = edge->m_sourceport.length();
	return edge->m_sourceport.c_str();
}

const char* xbpar_PARGraphEdge_get_destoport(const PARGraphEdge* edge, size_t* len)
{
	*len = edge->m_destport.length();
	return edge->m_destport.c_str();
}


//PARGraphNode
PARGraphNode* xbpar_PARGraphNode_Create(uint32_t label, void* pData)
{
	return new PARGraphNode(label, pData);
}

void xbpar_PARGraphNode_Destroy(PARGraphNode* node)
{
	delete node;
}

void xbpar_PARGraphNode_MateWith(PARGraphNode* node, PARGraphNode* mate)
{
	node->MateWith(mate);
}

PARGraphNode* xbpar_PARGraphNode_GetMate(const PARGraphNode* node)
{
	return node->GetMate();
}

void xbpar_PARGraphNode_Relabel(PARGraphNode* node, uint32_t label)
{
	node->Relabel(label);
}

uint32_t xbpar_PARGraphNode_GetLabel(const PARGraphNode* node)
{
	return node->GetLabel();
}

void* xbpar_PARGraphNode_GetData(const PARGraphNode* node)
{
	return node->GetData();
}

void xbpar_PARGraphNode_AddAlternateLabel(PARGraphNode* node, uint32_t alt)
{
	node->AddAlternateLabel(alt);
}

uint32_t xbpar_PARGraphNode_GetAlternateLabelCount(const PARGraphNode* node)
{
	return node->GetAlternateLabelCount();
}

uint32_t xbpar_PARGraphNode_GetAlternateLabel(const PARGraphNode* node, uint32_t i)
{
	return node->GetAlternateLabel(i);
}

bool xbpar_PARGraphNode_MatchesLabel(const PARGraphNode* node, uint32_t target)
{
	return node->MatchesLabel(target);
}

uint32_t xbpar_PARGraphNode_GetEdgeCount(PARGraphNode* node)
{
	return node->GetEdgeCount();
}

const PARGraphEdge* xbpar_PARGraphNode_GetEdgeByIndex(const PARGraphNode* node, uint32_t index)
{
	return node->GetEdgeByIndex(index);
}

void xbpar_PARGraphNode_AddEdge(
	PARGraphNode* source, const char* srcport_ptr, size_t srcport_len,
	PARGraphNode* sink, const char* dstport_ptr, size_t dstport_len)
{
	PTR_LEN_TO_STRING(srcport);
	PTR_LEN_TO_STRING(dstport);

	source->AddEdge(srcport, sink, dstport);
}
void xbpar_PARGraphNode_RemoveEdge(
	PARGraphNode* source, const char* srcport_ptr, size_t srcport_len,
	PARGraphNode* sink, const char* dstport_ptr, size_t dstport_len)
{
	PTR_LEN_TO_STRING(srcport);
	PTR_LEN_TO_STRING(dstport);

	source->RemoveEdge(srcport, sink, dstport);
}

//PARGraph
PARGraph* xbpar_PARGraph_Create()
{
	return new PARGraph();
}

void xbpar_PARGraph_Destroy(PARGraph* graph)
{
	delete graph;
}

uint32_t xbpar_PARGraph_AllocateLabel(PARGraph* graph)
{
	return graph->AllocateLabel();
}

uint32_t xbpar_PARGraph_GetMaxLabel(const PARGraph* graph)
{
	return graph->GetMaxLabel();
}

uint32_t xbpar_PARGraph_GetNumNodesWithLabel(const PARGraph* graph, uint32_t label)
{
	return graph->GetNumNodesWithLabel(label);
}

void xbpar_PARGraph_IndexNodesByLabel(PARGraph* graph)
{
	graph->IndexNodesByLabel();
}

PARGraphNode* xbpar_PARGraph_GetNodeByLabelAndIndex(const PARGraph* graph, uint32_t label, uint32_t index)
{
	return graph->GetNodeByLabelAndIndex(label, index);
}

uint32_t xbpar_PARGraph_GetNumNodes(const PARGraph* graph)
{
	return graph->GetNumNodes();
}

PARGraphNode* xbpar_PARGraph_GetNodeByIndex(const PARGraph* graph, uint32_t index)
{
	return graph->GetNodeByIndex(index);
}

uint32_t xbpar_PARGraph_GetNumEdges(const PARGraph* graph)
{
	return graph->GetNumEdges();
}

void xbpar_PARGraph_AddNode(PARGraph* graph, PARGraphNode* node)
{
	graph->AddNode(node);
}

//PAREngine
class FFIPAREngine : public PAREngine {
public:
	FFIPAREngine(void* ffiengine, PARGraph* netlist, PARGraph* device,
		t_GetNewPlacementForNode f_GetNewPlacementForNode,
		t_FindSubOptimalPlacements f_FindSubOptimalPlacements,
		t_InitialPlacement_core f_InitialPlacement_core,
		t_GetLabelName f_GetLabelName)
		: PAREngine(netlist, device)
	{
		this->ffiengine = ffiengine;
		this->f_GetNewPlacementForNode = f_GetNewPlacementForNode;
		this->f_FindSubOptimalPlacements = f_FindSubOptimalPlacements;
		this->f_InitialPlacement_core = f_InitialPlacement_core;
		this->f_GetLabelName = f_GetLabelName;
	}

	~FFIPAREngine() {}

	//Pure virtual
	PARGraphNode* GetNewPlacementForNode(const PARGraphNode* pivot)
	{
		return this->f_GetNewPlacementForNode(ffiengine, pivot);
	}

	void FindSubOptimalPlacements(std::vector<PARGraphNode*>& bad_nodes)
	{
		PARGraphNode*const* bad_nodes_ptr;
		size_t bad_nodes_len;
		this->f_FindSubOptimalPlacements(ffiengine, &bad_nodes_ptr, &bad_nodes_len);
		PTR_LEN_INTO_VECTOR(bad_nodes);
	}

	bool InitialPlacement_core()
	{
		return this->f_InitialPlacement_core(ffiengine);
	}

	const char* GetLabelName(uint32_t label) const
	{
		return this->f_GetLabelName(ffiengine, label);
	}

	//Exposing protected
	void base_MoveNode(PARGraphNode* node, PARGraphNode* newpos)
	{
		MoveNode(node, newpos);
	}

	std::string base_GetNodeTypes(const PARGraphNode* node) const
	{
		return GetNodeTypes(node);
	}

	void base_SaveNewBestPlacement()
	{
		SaveNewBestPlacement();
	}

	void base_RestorePreviousBestPlacement()
	{
		RestorePreviousBestPlacement();
	}

	uint32_t base_RandomNumber()
	{
		return RandomNumber();
	}

private:
	void *ffiengine;
	t_GetNewPlacementForNode f_GetNewPlacementForNode;
	t_FindSubOptimalPlacements f_FindSubOptimalPlacements;
	t_InitialPlacement_core f_InitialPlacement_core;
	t_GetLabelName f_GetLabelName;
};

PAREngine* xbpar_PAREngine_Create(void* ffiengine, PARGraph* netlist, PARGraph* device,
	t_GetNewPlacementForNode f_GetNewPlacementForNode,
	t_FindSubOptimalPlacements f_FindSubOptimalPlacements,
	t_InitialPlacement_core f_InitialPlacement_core,
	t_GetLabelName f_GetLabelName)
{
	return new FFIPAREngine(ffiengine, netlist, device, f_GetNewPlacementForNode, f_FindSubOptimalPlacements,
		f_InitialPlacement_core, f_GetLabelName);
}

void xbpar_PAREngine_Destroy(PAREngine* engine)
{
	delete engine;
}

bool xbpar_PAREngine_PlaceAndRoute(PAREngine* engine, uint32_t seed)
{
	return ((FFIPAREngine*)engine)->PlaceAndRoute(seed);
}

uint32_t xbpar_PAREngine_ComputeCost(const PAREngine* engine)
{
	return ((FFIPAREngine*)engine)->ComputeCost();
}

void xbpar_PAREngine_MoveNode(PAREngine* engine, PARGraphNode* node, PARGraphNode* newpos)
{
	((FFIPAREngine*)engine)->base_MoveNode(node, newpos);
}

char* xbpar_PAREngine_GetNodeTypes(const PAREngine* engine, const PARGraphNode* node, size_t* len)
{
	auto ret_str = ((FFIPAREngine*)engine)->base_GetNodeTypes(node);
	*len = ret_str.length();
	auto ret_ptr = (char*)malloc(ret_str.length());
	memcpy(ret_ptr, ret_str.c_str(), ret_str.length());
	return ret_ptr;
}

void xbpar_PAREngine_SaveNewBestPlacement(PAREngine* engine)
{
	((FFIPAREngine*)engine)->base_SaveNewBestPlacement();
}

void xbpar_PAREngine_RestorePreviousBestPlacement(PAREngine* engine)
{
	((FFIPAREngine*)engine)->base_RestorePreviousBestPlacement();
}

uint32_t xbpar_PAREngine_RandomNumber(PAREngine* engine)
{
	return ((FFIPAREngine*)engine)->base_RandomNumber();
}
