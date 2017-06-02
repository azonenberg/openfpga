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

#define PTR_LEN_TO_STRING(x) \
	std::string x = std::string(x##_ptr, x##_len);


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
