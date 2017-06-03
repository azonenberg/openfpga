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

#ifndef xbpar_ffi_h
#define xbpar_ffi_h

#ifdef NOT_BINDGEN
#include "xbpar.h"
#else
#include <stddef.h>
#include <stdint.h>
#define PARGraphNode void
#define PARGraphEdge void
#define PARGraph void
#define PAREngine void
#endif

#ifdef __cplusplus
extern "C" {
#endif

void xbpar_ffi_free_object(void *obj);

//PARGraphEdge
PARGraphNode* xbpar_PARGraphEdge_get_sourcenode(const PARGraphEdge* edge);
PARGraphNode* xbpar_PARGraphEdge_get_destnode(const PARGraphEdge* edge);
//Does NOT give out ownership! Borrowed string output is valid until the user does "anything else" with the object.
const char* xbpar_PARGraphEdge_get_sourceport(const PARGraphEdge* edge, size_t* len);
const char* xbpar_PARGraphEdge_get_destport(const PARGraphEdge* edge, size_t* len);

//PARGraphNode
PARGraphNode* xbpar_PARGraphNode_Create(uint32_t label, void* pData);
void xbpar_PARGraphNode_Destroy(PARGraphNode* node);
void xbpar_PARGraphNode_MateWith(PARGraphNode* node, PARGraphNode* mate);
PARGraphNode* xbpar_PARGraphNode_GetMate(const PARGraphNode* node);
void xbpar_PARGraphNode_Relabel(PARGraphNode* node, uint32_t label);
uint32_t xbpar_PARGraphNode_GetLabel(const PARGraphNode* node);
void* xbpar_PARGraphNode_GetData(const PARGraphNode* node);
void xbpar_PARGraphNode_AddAlternateLabel(PARGraphNode* node, uint32_t alt);
uint32_t xbpar_PARGraphNode_GetAlternateLabelCount(const PARGraphNode* node);
uint32_t xbpar_PARGraphNode_GetAlternateLabel(const PARGraphNode* node, uint32_t i);
int xbpar_PARGraphNode_MatchesLabel(const PARGraphNode* node, uint32_t target);
uint32_t xbpar_PARGraphNode_GetEdgeCount(const PARGraphNode* node);
//Return values only lives as long as node
const PARGraphEdge* xbpar_PARGraphNode_GetEdgeByIndex(const PARGraphNode* node, uint32_t index);
//Borrows the given string and makes a copy; does not require ownership
void xbpar_PARGraphNode_AddEdge(
	PARGraphNode* source, const char* srcport_ptr, size_t srcport_len,
	PARGraphNode* sink, const char* dstport_ptr, size_t dstport_len);
void xbpar_PARGraphNode_RemoveEdge(
	PARGraphNode* source, const char* srcport_ptr, size_t srcport_len,
	PARGraphNode* sink, const char* dstport_ptr, size_t dstport_len);

//PARGraph
PARGraph* xbpar_PARGraph_Create();
void xbpar_PARGraph_Destroy(PARGraph* graph);
uint32_t xbpar_PARGraph_AllocateLabel(PARGraph* graph);
uint32_t xbpar_PARGraph_GetMaxLabel(const PARGraph* graph);
uint32_t xbpar_PARGraph_GetNumNodesWithLabel(const PARGraph* graph, uint32_t label);
void xbpar_PARGraph_IndexNodesByLabel(PARGraph* graph);
//Return value only lives as long as graph
PARGraphNode* xbpar_PARGraph_GetNodeByLabelAndIndex(const PARGraph* graph, uint32_t label, uint32_t index);
uint32_t xbpar_PARGraph_GetNumNodes(const PARGraph* graph);
//Return value only lives as long as graph
PARGraphNode* xbpar_PARGraph_GetNodeByIndex(const PARGraph* graph, uint32_t index);
uint32_t xbpar_PARGraph_GetNumEdges(const PARGraph* graph);
//Takes ownership
void xbpar_PARGraph_AddNode(PARGraph* graph, PARGraphNode* node);

//PAREngine
typedef PARGraphNode* (*t_GetNewPlacementForNode)(void* ffiengine, const PARGraphNode* pivot);
//Gives out ownership; will be freed
typedef void (*t_FindSubOptimalPlacements)(void* ffiengine,
	PARGraphNode*const** bad_nodes_ptr, size_t* bad_nodes_len, size_t* bad_nodes_extra);
typedef int (*t_InitialPlacement_core)(void* ffiengine);
//Return value is borrowed and must live for some amount of time until the next operation
typedef const char* (*t_GetLabelName)(void* ffiengine, uint32_t label);
typedef int (*t_CanMoveNode)(void* ffiengine, const PARGraphNode* node,
	const PARGraphNode* old_mate, const PARGraphNode* new_mate);
//Gives out ownership; will be freed
typedef uint32_t (*t_ComputeAndPrintScore)(void* ffiengine,
	const PARGraphEdge*const** unroutes_ptr, size_t* unroutes_len, size_t* unroutes_extra, uint32_t iteration);
//Borrows unroutes, does not take ownership
typedef void (*t_PrintUnroutes)(void* ffiengine,
	const PARGraphEdge*const* unroutes_ptr, size_t unroutes_len);
typedef uint32_t (*t_ComputeCongestionCost)(void* ffiengine);
typedef uint32_t (*t_ComputeTimingCost)(void* ffiengine);
//Gives out ownership; will be freed
typedef uint32_t (*t_ComputeUnroutableCost)(void* ffiengine,
	const PARGraphEdge*const** unroutes_ptr, size_t* unroutes_len, size_t* unroutes_extra);
typedef int (*t_SanityCheck)(void* ffiengine);
typedef int (*t_InitialPlacement)(void* ffiengine);
//Borrows badnodes, does not take ownership
typedef int (*t_OptimizePlacement)(void* ffiengine,
	PARGraphNode*const* badnodes_ptr, size_t badnodes_len);
typedef uint32_t (*t_ComputeNodeUnroutableCost)(void* ffiengine,
	const PARGraphNode* pivot, const PARGraphNode* candidate);
typedef void (*t_free_edgevec)(const PARGraphEdge*const* v, size_t len, size_t extra);
typedef void (*t_free_nodevec)(const PARGraphNode*const* v, size_t len, size_t extra);


PAREngine* xbpar_PAREngine_Create(void* ffiengine, PARGraph* netlist, PARGraph* device,
	t_GetNewPlacementForNode f_GetNewPlacementForNode,
	t_FindSubOptimalPlacements f_FindSubOptimalPlacements,
	t_InitialPlacement_core f_InitialPlacement_core,
	t_GetLabelName f_GetLabelName,
	t_CanMoveNode f_CanMoveNode,
	t_ComputeAndPrintScore f_ComputeAndPrintScore,
	t_PrintUnroutes f_PrintUnroutes,
	t_ComputeCongestionCost f_ComputeCongestionCost,
	t_ComputeTimingCost f_ComputeTimingCost,
	t_ComputeUnroutableCost f_ComputeUnroutableCost,
	t_SanityCheck f_SanityCheck,
	t_InitialPlacement f_InitialPlacement,
	t_OptimizePlacement f_OptimizePlacement,
	t_ComputeNodeUnroutableCost f_ComputeNodeUnroutableCost,
	t_free_edgevec f_free_edgevec,
	t_free_nodevec f_free_nodevec);
void xbpar_PAREngine_Destroy(PAREngine* engine);
//Public methods
int xbpar_PAREngine_PlaceAndRoute(PAREngine* engine, uint32_t seed);
uint32_t xbpar_PAREngine_ComputeCost(const PAREngine* engine);
//Non-overridable protected methods
void xbpar_PAREngine_MoveNode(PAREngine* engine, PARGraphNode* node, PARGraphNode* newpos);
//DOES give ownership; need to free
char* xbpar_PAREngine_GetNodeTypes(const PAREngine* engine, const PARGraphNode* node, size_t* len);
void xbpar_PAREngine_SaveNewBestPlacement(PAREngine* engine);
void xbpar_PAREngine_RestorePreviousBestPlacement(PAREngine* engine);
uint32_t xbpar_PAREngine_RandomNumber(PAREngine* engine);
//Other protected methods
int xbpar_PAREngine_base_CanMoveNode(const PAREngine* engine, const PARGraphNode* node,
	const PARGraphNode* old_mate, const PARGraphNode* new_mate);
//DOES give ownership; need to free
uint32_t xbpar_PAREngine_base_ComputeAndPrintScore(const PAREngine* engine,
	const PARGraphEdge*const** unroutes_ptr, size_t* unroutes_len, uint32_t iteration);
//Borrows unroutes, does not take ownership
void xbpar_PAREngine_base_PrintUnroutes(const PAREngine* engine,
	const PARGraphEdge*const* unroutes_ptr, size_t unroutes_len);
uint32_t xbpar_PAREngine_base_ComputeCongestionCost(const PAREngine* engine);
uint32_t xbpar_PAREngine_base_ComputeTimingCost(const PAREngine* engine);
//DOES give ownership; need to free
uint32_t xbpar_PAREngine_base_ComputeUnroutableCost(const PAREngine* engine,
	const PARGraphEdge*const** unroutes_ptr, size_t* unroutes_len);
int xbpar_PAREngine_base_SanityCheck(const PAREngine* engine);
int xbpar_PAREngine_base_InitialPlacement(PAREngine* engine);
//Borrows badnodes, does not take ownership
int xbpar_PAREngine_base_OptimizePlacement(PAREngine* engine,
	PARGraphNode*const* badnodes_ptr, size_t badnodes_len);
uint32_t xbpar_PAREngine_base_ComputeNodeUnroutableCost(const PAREngine* engine,
	const PARGraphNode* pivot, const PARGraphNode* candidate);
// Hack to get the graphs back from a PAREngine
PARGraph* xbpar_PAREngine_base_get_m_netlist(const PAREngine* engine);
PARGraph* xbpar_PAREngine_base_get_m_device(const PAREngine* engine);


#ifdef __cplusplus
}
#endif

#endif
