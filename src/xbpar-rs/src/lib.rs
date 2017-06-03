/***********************************************************************************************************************
 * Copyright (C) 2017 Robert Ou and contributors                                                                       *
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

mod ffi {
#![allow(dead_code)]
#![allow(non_camel_case_types)]

include!(concat!(env!("OUT_DIR"), "/bindings.rs"));
}

use std::cell::UnsafeCell;
use std::hash::{Hash, Hasher};
use std::marker::PhantomData;
use std::mem;
use std::os::raw::*;
use std::ops::{Deref, DerefMut};
use std::ptr;
use std::slice;
use std::str;

pub struct PARGraphEdge(
    // <eddyb> it prevents the compiler from assuming there can only be truly immutable data inside
    UnsafeCell<()>
);

impl PARGraphEdge {
    pub fn sourceport(&self) -> String {
        unsafe {
            let mut str_len: usize = 0;
            let str_ptr = ffi::xbpar_PARGraphEdge_get_sourceport(
                self as *const PARGraphEdge as *const c_void, &mut str_len);
            let str_slice = slice::from_raw_parts(str_ptr as *const u8, str_len);
            let str_str = str::from_utf8(str_slice).unwrap();
            str_str.to_owned()
        }
    }

    pub fn destport(&self) -> String {
        unsafe {
            let mut str_len: usize = 0;
            let str_ptr = ffi::xbpar_PARGraphEdge_get_destport(
                self as *const PARGraphEdge as *const c_void, &mut str_len);
            let str_slice = slice::from_raw_parts(str_ptr as *const u8, str_len);
            let str_str = str::from_utf8(str_slice).unwrap();
            str_str.to_owned()
        }
    }

    pub fn sourcenode(&self) -> &PARGraphNode {
        unsafe {
            &*(ffi::xbpar_PARGraphEdge_get_sourcenode(self as *const PARGraphEdge as *const c_void)
                as *const PARGraphNode)
        }
    }

    pub fn destnode(&self) -> &PARGraphNode {
        unsafe {
            &*(ffi::xbpar_PARGraphEdge_get_destnode(self as *const PARGraphEdge as *const c_void)
                as *const PARGraphNode)
        }
    }

    // We need to take in a &mut in order to hand out a &mut
    pub fn sourcenode_mut(&mut self) -> &mut PARGraphNode {
        unsafe {
            &mut*(ffi::xbpar_PARGraphEdge_get_sourcenode(self as *const PARGraphEdge as *const c_void)
                as *mut PARGraphNode)
        }
    }

    pub fn destnode_mut(&mut self) -> &mut PARGraphNode {
        unsafe {
            &mut*(ffi::xbpar_PARGraphEdge_get_destnode(self as *const PARGraphEdge as *const c_void)
                as *mut PARGraphNode)
        }
    }
}

// We are never actually allowed to own the C++ PARGraphNode object, so we never have to drop it here on the Rust side
pub struct PARGraphNode (
    UnsafeCell<()>
);

impl PartialEq for PARGraphNode {
    fn eq(&self, other: &PARGraphNode) -> bool {
        self as *const PARGraphNode == other as *const PARGraphNode
    }
}

impl Eq for PARGraphNode {}

impl Hash for PARGraphNode {
    fn hash<H: Hasher>(&self, state: &mut H) {
        (self as *const PARGraphNode).hash(state);
    }
}

impl PARGraphNode {
    pub fn mate_with<'a>(&'a mut self, mate: &'a mut PARGraphNode) {
        unsafe {
            ffi::xbpar_PARGraphNode_MateWith(self as *mut PARGraphNode as *mut c_void,
                mate as *mut PARGraphNode as *mut c_void);
        }
    }

    pub fn get_mate(&self) -> Option<&PARGraphNode> {
        unsafe {
            let ret_ptr = ffi::xbpar_PARGraphNode_GetMate(
                self as *const PARGraphNode as *const c_void) as *const PARGraphNode;
            if ret_ptr.is_null() {
                None
            } else {
                Some(&*(ret_ptr))
            }
        }
    }

    pub fn get_mate_mut(&mut self) -> Option<&mut PARGraphNode> {
        unsafe {
            let ret_ptr = ffi::xbpar_PARGraphNode_GetMate(
                self as *const PARGraphNode as *const c_void) as *mut PARGraphNode;
            if ret_ptr.is_null() {
                None
            } else {
                Some(&mut*(ret_ptr))
            }
        }
    }

    pub fn relabel(&mut self, label: u32) {
        unsafe {
            ffi::xbpar_PARGraphNode_Relabel(self as *mut PARGraphNode as *mut c_void, label);
        }
    }

    pub fn get_label(&self) -> u32 {
        unsafe {
            ffi::xbpar_PARGraphNode_GetLabel(self as *const PARGraphNode as *const c_void)
        }
    }

    pub fn get_data_pointer_raw(&self) -> *mut c_void {
        unsafe {
            ffi::xbpar_PARGraphNode_GetData(self as *const PARGraphNode as *const c_void)
        }
    }

    pub fn add_alternate_label(&mut self, alt: u32) {
        unsafe {
            ffi::xbpar_PARGraphNode_AddAlternateLabel(self as *mut PARGraphNode as *mut c_void, alt);
        }
    }

    pub fn get_alternate_label_count(&self) -> u32 {
        unsafe {
            ffi::xbpar_PARGraphNode_GetAlternateLabelCount(self as *const PARGraphNode as *const c_void)
        }
    }

    pub fn get_alternate_label(&self, i: u32) -> u32 {
        unsafe {
            ffi::xbpar_PARGraphNode_GetAlternateLabel(self as *const PARGraphNode as *const c_void, i)
        }
    }

    pub fn matches_label(&self, target: u32) -> bool {
        unsafe {
            ffi::xbpar_PARGraphNode_MatchesLabel(self as *const PARGraphNode as *const c_void, target) != 0
        }
    }

    pub fn get_edge_count(&self) -> u32 {
        unsafe {
            ffi::xbpar_PARGraphNode_GetEdgeCount(self as *const PARGraphNode as *const c_void)
        }
    }

    pub fn get_edge_by_index(&self, index: u32) -> &PARGraphEdge {
        unsafe {
            &*(ffi::xbpar_PARGraphNode_GetEdgeByIndex(
                self as *const PARGraphNode as *const c_void, index) as *const PARGraphEdge)
        }
    }

    // Careful careful: We are handing out a &mut edge so that we can get &mut nodes back out of that
    // but the object we have from C is actually const. This is "safe" because there aren't actually any methods we
    // can call to mutate edges
    pub fn get_edge_by_index_mut(&mut self, index: u32) -> &mut PARGraphEdge {
        unsafe {
            &mut*(ffi::xbpar_PARGraphNode_GetEdgeByIndex(
                self as *const PARGraphNode as *const c_void, index) as *mut PARGraphEdge)
        }
    }
}

pub struct PARGraph {
    ffi_graph: *mut c_void,
}

impl Deref for PARGraph {
    type Target = PARGraph_;

    fn deref(&self) -> &PARGraph_ {
        unsafe {
            &*(self.ffi_graph as *const PARGraph_)
        }
    }
}

impl DerefMut for PARGraph {
    fn deref_mut(&mut self) -> &mut PARGraph_ {
        unsafe {
            &mut*(self.ffi_graph as *mut PARGraph_)
        }
    }
}

impl Drop for PARGraph {
    fn drop(&mut self) {
        unsafe {
            ffi::xbpar_PARGraph_Destroy(self.ffi_graph);
        }
    }
}

pub struct PARGraph_ (
    UnsafeCell<()>
);

// Having this helps prevent crossing nodes between arbitrary and definitely wrong graphs
pub struct PARGraphPair(PARGraph, PARGraph);
pub struct PARGraphRefPair<'a> {
    pub d: &'a PARGraph_,
    pub n: &'a PARGraph_
}

impl PARGraphPair {
    pub fn new_pair() -> PARGraphPair {
        unsafe {
            PARGraphPair(
                PARGraph {
                    ffi_graph: ffi::xbpar_PARGraph_Create(),
                },
                PARGraph {
                    ffi_graph: ffi::xbpar_PARGraph_Create(),
                }
            )
        }
    }

    pub fn borrow(&self) -> PARGraphRefPair {
        PARGraphRefPair{d: &self.0, n: &self.1}
    }

    pub fn borrow_mut_d(&mut self) -> &mut PARGraph_ {
        &mut self.0
    }

    pub fn borrow_mut_n(&mut self) -> &mut PARGraph_ {
        &mut self.1
    }
}

impl PARGraph_ {
    pub fn allocate_label(&mut self) -> u32 {
        unsafe {
            ffi::xbpar_PARGraph_AllocateLabel(self as *mut PARGraph_ as *mut c_void)
        }
    }

    pub fn get_max_label(&self) -> u32 {
        unsafe {
            ffi::xbpar_PARGraph_GetMaxLabel(self as *const PARGraph_ as *const c_void)
        }
    }

    pub fn get_num_nodes_with_label(&self, label: u32) -> u32 {
        unsafe {
            ffi::xbpar_PARGraph_GetNumNodesWithLabel(self as *const PARGraph_ as *const c_void, label)
        }
    }

    pub fn index_nodes_by_label(&mut self) {
        unsafe {
            ffi::xbpar_PARGraph_IndexNodesByLabel(self as *mut PARGraph_ as *mut c_void);
        }
    }

    pub fn get_node_by_label_and_index(&self, label: u32, index: u32) -> &PARGraphNode {
        unsafe {
            &*(ffi::xbpar_PARGraph_GetNodeByLabelAndIndex(self as *const PARGraph_ as *const c_void, label, index)
                as *const PARGraphNode)
        }
    }

    pub fn get_node_by_label_and_index_mut(&mut self, label: u32, index: u32) -> &mut PARGraphNode {
        unsafe {
            &mut*(ffi::xbpar_PARGraph_GetNodeByLabelAndIndex(self as *const PARGraph_ as *const c_void, label, index)
                as *mut PARGraphNode)
        }
    }

    pub fn get_num_nodes(&self) -> u32 {
        unsafe {
            ffi::xbpar_PARGraph_GetNumNodes(self as *const PARGraph_ as *const c_void)
        }
    }

    pub fn get_node_by_index(&self, index: u32) -> &PARGraphNode {
        unsafe {
            &*(ffi::xbpar_PARGraph_GetNodeByIndex(self as *const PARGraph_ as *const c_void, index)
                as *const PARGraphNode)
        }
    }

    pub fn get_node_by_index_mut(&mut self, index: u32) -> &mut PARGraphNode {
        unsafe {
            &mut*(ffi::xbpar_PARGraph_GetNodeByIndex(self as *const PARGraph_ as *const c_void, index)
                as *mut PARGraphNode)
        }
    }

    // We can only safely add edges by indices because we can't give out a &mut and a & in the same graph because
    // it is possible to take one of them and traverse existing edges to get to another one and thereby violate
    // aliasing rules.
    pub fn add_edge(&mut self, source_idx: u32, srcport: &str, sink_idx: u32, dstport: &str) {
        unsafe {
            let source = ffi::xbpar_PARGraph_GetNodeByIndex(self as *const PARGraph_ as *const c_void, source_idx);
            let sink = ffi::xbpar_PARGraph_GetNodeByIndex(self as *const PARGraph_ as *const c_void, sink_idx);
            ffi::xbpar_PARGraphNode_AddEdge(source, srcport.as_ptr() as *const i8, srcport.len(),
                sink, dstport.as_ptr() as *const i8, dstport.len());
        }
    }

    pub fn remove_edge(&mut self, source_idx: u32, srcport: &str, sink_idx: u32, dstport: &str) {
        unsafe {
            let source = ffi::xbpar_PARGraph_GetNodeByIndex(self as *const PARGraph_ as *const c_void, source_idx);
            let sink = ffi::xbpar_PARGraph_GetNodeByIndex(self as *const PARGraph_ as *const c_void, sink_idx);
            ffi::xbpar_PARGraphNode_RemoveEdge(source, srcport.as_ptr() as *const i8, srcport.len(),
                sink, dstport.as_ptr() as *const i8, dstport.len());
        }
    }

    pub fn get_num_edges(&self) -> u32 {
        unsafe {
            ffi::xbpar_PARGraph_GetNumEdges(self as *const PARGraph_ as *const c_void)
        }
    }

    // Returns the index of the new node because that's basically the only way we can sanely do it here in rust
    pub fn add_new_node(&mut self, label: u32, p_data: *mut c_void) -> u32 {
        unsafe {
            let ffi_node = ffi::xbpar_PARGraphNode_Create(label, p_data);
            let ret_idx = ffi::xbpar_PARGraph_GetNumNodes(self as *const PARGraph_ as *const c_void);
            // This transfers ownership to the graph
            ffi::xbpar_PARGraph_AddNode(self as *mut PARGraph_ as *mut c_void, ffi_node);

            ret_idx
        }
    }
}

// The graph objects that the engine needs to work on must always live at least as long as the engine. This needs to be
// enforced on the trait itself because ??? (if you try to have it on each function separately, it will appear that
// some functions need to return a lifetime longer than the lifetime of the engine impl and that is weird). All graph
// lifetimes are the same because otherwise (more??) weird things can happen with mixing up different graphs???
// self is always &mut here because it represents the Rust side state that we can always mutate rather than
// representing the C++ state.
pub trait PAREngineImpl<'e, 'g: 'e> {
    fn set_base_engine(&'e mut self, base_engine: &'g mut BasePAREngine);

    // Overloads
    fn can_move_node(&'e mut self, node: &'g PARGraphNode,
        old_mate: &'g PARGraphNode, new_mate: &'g PARGraphNode) -> bool;
    fn get_new_placement_for_node(&'e mut self, pivot: &'g PARGraphNode) -> Option<&'g PARGraphNode>;
    fn find_suboptimal_placements(&'e mut self) -> Vec<&'g PARGraphNode>;
    fn compute_and_print_score(&'e mut self, iteration: u32) -> (u32, Vec<&'g PARGraphEdge>);
    fn print_unroutes(&'e mut self, unroutes: &[&'g PARGraphEdge]);
    fn compute_congestion_cost(&mut self) -> u32;
    fn compute_timing_cost(&mut self) -> u32;
    fn compute_unroutable_cost(&'e mut self) -> (u32, Vec<&'g PARGraphEdge>);
    fn sanity_check(&mut self) -> bool;
    fn initial_placement(&mut self) -> bool;
    fn initial_placement_core(&mut self) -> bool;
    fn optimize_placement(&'e mut self, badnodes: &[&'g PARGraphNode]) -> bool;
    fn compute_node_unroutable_cost(&'e mut self, pivot: &'g PARGraphNode, candidate: &'g PARGraphNode) -> u32;
    fn get_label_name(&mut self, label: u32) -> &str;
}

pub struct BasePAREngine (
    UnsafeCell<()>
);

impl<'e, 'g: 'e> PAREngineImpl<'e, 'g> for BasePAREngine {
    fn set_base_engine(&'e mut self, _: &'g mut BasePAREngine) {}

    fn can_move_node(&'e mut self, node: &'g PARGraphNode,
        old_mate: &'g PARGraphNode, new_mate: &'g PARGraphNode) -> bool {

        unsafe {
            ffi::xbpar_PAREngine_base_CanMoveNode(self as *const BasePAREngine as *const c_void,
                node as *const PARGraphNode as *const c_void,
                old_mate as *const PARGraphNode as *const c_void,
                new_mate as *const PARGraphNode as *const c_void) != 0
        }
    }

    fn get_new_placement_for_node(&'e mut self, _: &'g PARGraphNode) -> Option<&'g PARGraphNode> {
        panic!("pure virtual function call ;)");
    }

    fn find_suboptimal_placements(&'e mut self) -> Vec<&'g PARGraphNode> {
        panic!("pure virtual function call ;)");
    }

    fn compute_and_print_score(&'e mut self, iteration: u32) -> (u32, Vec<&'g PARGraphEdge>) {
        unsafe {
            let mut unroutes_len: usize = 0;
            let mut unroutes_ptr: *const*const c_void = ptr::null_mut();
            let ret = ffi::xbpar_PAREngine_base_ComputeAndPrintScore(self as *const BasePAREngine as *const c_void,
                &mut unroutes_ptr, &mut unroutes_len, iteration);
            let unroutes_slice = slice::from_raw_parts(unroutes_ptr, unroutes_len);
            let unroutes = unroutes_slice.iter().map(|&x| &*(x as *const PARGraphEdge)).collect();
            ffi::xbpar_ffi_free_object(unroutes_ptr as *mut c_void);

            (ret, unroutes)
        }
    }

    fn print_unroutes(&'e mut self, unroutes: &[&'g PARGraphEdge]) {
        unsafe {
            ffi::xbpar_PAREngine_base_PrintUnroutes(self as *mut BasePAREngine as *mut c_void,
                unroutes.as_ptr() as *const*const c_void, unroutes.len());
        }
    }

    fn compute_congestion_cost(&mut self) -> u32 {
        unsafe {
            ffi::xbpar_PAREngine_base_ComputeCongestionCost(self as *const BasePAREngine as *const c_void)
        }
    }

    fn compute_timing_cost(&mut self) -> u32 {
        unsafe {
            ffi::xbpar_PAREngine_base_ComputeTimingCost(self as *const BasePAREngine as *const c_void)
        }
    }

    fn compute_unroutable_cost(&'e mut self) -> (u32, Vec<&'g PARGraphEdge>) {
        unsafe {
            let mut unroutes_len: usize = 0;
            let mut unroutes_ptr: *const*const c_void = ptr::null_mut();
            let ret = ffi::xbpar_PAREngine_base_ComputeUnroutableCost(self as *const BasePAREngine as *const c_void,
                &mut unroutes_ptr, &mut unroutes_len);
            let unroutes_slice = slice::from_raw_parts(unroutes_ptr, unroutes_len);
            let unroutes = unroutes_slice.iter().map(|&x| &*(x as *const PARGraphEdge)).collect();
            ffi::xbpar_ffi_free_object(unroutes_ptr as *mut c_void);

            (ret, unroutes)
        }
    }

    fn sanity_check(&mut self) -> bool {
        unsafe {
            ffi::xbpar_PAREngine_base_SanityCheck(self as *const BasePAREngine as *const c_void) != 0
        }
    }

    fn initial_placement(&mut self) -> bool {
        unsafe {
            ffi::xbpar_PAREngine_base_InitialPlacement(self as *mut BasePAREngine as *mut c_void) != 0
        }
    }

    fn initial_placement_core(&mut self) -> bool {
        panic!("pure virtual function call ;)");
    }

    fn optimize_placement(&'e mut self, badnodes: &[&'g PARGraphNode]) -> bool {
        unsafe {
            ffi::xbpar_PAREngine_base_OptimizePlacement(self as *mut BasePAREngine as *mut c_void,
                badnodes.as_ptr() as *const*const c_void, badnodes.len()) != 0
        }
    }

    fn compute_node_unroutable_cost(&'e mut self, pivot: &'g PARGraphNode, candidate: &'g PARGraphNode) -> u32 {
        unsafe {
            ffi::xbpar_PAREngine_base_ComputeNodeUnroutableCost(self as *mut BasePAREngine as *mut c_void,
                pivot as *const PARGraphNode as *const c_void,
                candidate as *const PARGraphNode as *const c_void)
        }
    }

    fn get_label_name(&mut self, _: u32) -> &str {
        panic!("pure virtual function call ;)");
    }
}

impl<'e, 'g: 'e> BasePAREngine {
    pub fn get_graphs(&'e self) -> (&'g PARGraph_, &'g PARGraph_) {
        unsafe {
            (
                &*(ffi::xbpar_PAREngine_base_get_m_device(self as *const BasePAREngine as *const c_void)
                    as *const PARGraph_),
                &*(ffi::xbpar_PAREngine_base_get_m_netlist(self as *const BasePAREngine as *const c_void)
                    as *const PARGraph_)
            )
        }
    }

    pub fn get_m_netlist_mut(&'e mut self) -> &'g mut PARGraph_ {
        unsafe {
            &mut*(ffi::xbpar_PAREngine_base_get_m_netlist(self as *const BasePAREngine as *const c_void)
                as *mut PARGraph_)
        }
    }

    pub fn get_m_device_mut(&'e mut self) -> &'g mut PARGraph_ {
        unsafe {
            &mut*(ffi::xbpar_PAREngine_base_get_m_device(self as *const BasePAREngine as *const c_void)
                as *mut PARGraph_)
        }
    }

    pub fn get_both_netlists_mut(&'e mut self) -> (&'g mut PARGraph_, &'g mut PARGraph_) {
        unsafe {
            (&mut*(ffi::xbpar_PAREngine_base_get_m_netlist(self as *const BasePAREngine as *const c_void)
                as *mut PARGraph_),
                &mut*(ffi::xbpar_PAREngine_base_get_m_device(self as *const BasePAREngine as *const c_void)
                    as *mut PARGraph_))
        }
    }

    // FIXME: Is this interior mutability correct?
    pub fn random_number(&self) -> u32 {
        unsafe {
            ffi::xbpar_PAREngine_RandomNumber(self as *const BasePAREngine as *mut c_void)
        }
    }
}

pub struct PAREngine<'e, 'g: 'e, T: 'e + PAREngineImpl<'e, 'g>> {
    ffi_engine: *mut c_void,
    inner_impl: *mut T,
    _marker1: PhantomData<&'g ()>,
    _marker3: PhantomData<&'e mut T>,
}

impl<'e, 'g: 'e, T: 'e + PAREngineImpl<'e, 'g>> Drop for PAREngine<'e, 'g, T> {
    fn drop(&mut self) {
        unsafe {
            Box::from_raw(self.inner_impl);
            ffi::xbpar_PAREngine_Destroy(self.ffi_engine);
        }
    }
}

impl<'e, 'g: 'e, T: 'e + PAREngineImpl<'e, 'g>> PAREngine<'e, 'g, T> {
    pub fn new(inner_impl: T, graphs: &'g mut PARGraphPair) -> Self {
        unsafe {
            let boxed_impl = Box::into_raw(Box::new(inner_impl));

            let ffi_engine = ffi::xbpar_PAREngine_Create(
                boxed_impl as *mut c_void,
                graphs.1.ffi_graph, graphs.0.ffi_graph,
                Some(PAREngine::<T>::get_new_placement_for_node),
                Some(PAREngine::<T>::find_suboptimal_placements),
                Some(PAREngine::<T>::initial_placement_core),
                Some(PAREngine::<T>::get_label_name),
                Some(PAREngine::<T>::can_move_node),
                Some(PAREngine::<T>::compute_and_print_score),
                Some(PAREngine::<T>::print_unroutes),
                Some(PAREngine::<T>::compute_congestion_cost),
                Some(PAREngine::<T>::compute_timing_cost),
                Some(PAREngine::<T>::compute_unroutable_cost),
                Some(PAREngine::<T>::sanity_check),
                Some(PAREngine::<T>::initial_placement),
                Some(PAREngine::<T>::optimize_placement),
                Some(PAREngine::<T>::compute_node_unroutable_cost),
                Some(PAREngine::<T>::_free_edgevec),
                Some(PAREngine::<T>::_free_nodevec));

            (*boxed_impl).set_base_engine(&mut*(ffi_engine as *mut BasePAREngine));

            PAREngine {
                ffi_engine: ffi_engine,
                inner_impl: boxed_impl,
                _marker1: PhantomData,
                _marker3: PhantomData,
            }
        }
    }

    pub fn place_and_route(&mut self, seed: u32) -> bool {
        unsafe {
            ffi::xbpar_PAREngine_PlaceAndRoute(self.ffi_engine, seed) != 0
        }
    }

    pub fn compute_cost(&self) -> u32 {
        unsafe {
            ffi::xbpar_PAREngine_ComputeCost(self.ffi_engine)
        }
    }

    // Overloads
    unsafe extern "C" fn can_move_node(ffiengine: *mut c_void, node: *const c_void,
        old_mate: *const c_void, new_mate: *const c_void) -> i32 {

        (*(ffiengine as *mut T)).can_move_node(&*(node as *const PARGraphNode),
            &*(old_mate as *const PARGraphNode), &*(new_mate as *const PARGraphNode)) as i32
    }

    unsafe extern "C" fn get_new_placement_for_node(ffiengine: *mut c_void, pivot: *const c_void) -> *mut c_void {
        let ret = (*(ffiengine as *mut T)).get_new_placement_for_node(&*(pivot as *const PARGraphNode));

        if let Some(ret) = ret {
            ret as *const PARGraphNode as *mut c_void
        } else {
            ptr::null_mut()
        }
    }

    unsafe extern "C" fn find_suboptimal_placements(ffiengine: *mut c_void,
        bad_nodes_ptr: *mut*const*const c_void, bad_nodes_len: *mut usize, bad_nodes_capacity: *mut usize) {

        let bad_nodes = (*(ffiengine as *mut T)).find_suboptimal_placements();

        *bad_nodes_ptr = bad_nodes.as_ptr() as *const*const PARGraphNode as *const*const c_void;
        *bad_nodes_len = bad_nodes.len();
        *bad_nodes_capacity = bad_nodes.capacity();
        mem::forget(bad_nodes);
    }

    unsafe extern "C" fn compute_and_print_score(ffiengine: *mut c_void,
        unroutes_ptr: *mut*const*const c_void, unroutes_len: *mut usize, unroutes_capacity: *mut usize,
        iteration: u32) -> u32 {

        let (ret, unroutes) = (*(ffiengine as *mut T)).compute_and_print_score(iteration);

        *unroutes_ptr = unroutes.as_ptr() as *const*const PARGraphEdge as *const*const c_void;
        *unroutes_len = unroutes.len();
        *unroutes_capacity = unroutes.capacity();
        mem::forget(unroutes);

        ret
    }

    unsafe extern "C" fn print_unroutes(ffiengine: *mut c_void,
        unroutes_ptr: *const*const c_void, unroutes_len: usize) {

        let unroutes = slice::from_raw_parts(unroutes_ptr as *const&PARGraphEdge, unroutes_len);
        (*(ffiengine as *mut T)).print_unroutes(unroutes);
    }

    unsafe extern "C" fn compute_congestion_cost(ffiengine: *mut c_void) -> u32 {
        (*(ffiengine as *mut T)).compute_congestion_cost()
    }

    unsafe extern "C" fn compute_timing_cost(ffiengine: *mut c_void) -> u32 {
        (*(ffiengine as *mut T)).compute_timing_cost()
    }

    unsafe extern "C" fn compute_unroutable_cost(ffiengine: *mut c_void,
        unroutes_ptr: *mut*const*const c_void, unroutes_len: *mut usize, unroutes_capacity: *mut usize) -> u32 {

        let (ret, unroutes) = (*(ffiengine as *mut T)).compute_unroutable_cost();

        *unroutes_ptr = unroutes.as_ptr() as *const*const PARGraphEdge as *const*const c_void;
        *unroutes_len = unroutes.len();
        *unroutes_capacity = unroutes.capacity();
        mem::forget(unroutes);

        ret
    }

    unsafe extern "C" fn sanity_check(ffiengine: *mut c_void) -> i32 {
        (*(ffiengine as *mut T)).sanity_check() as i32
    }

    unsafe extern "C" fn initial_placement(ffiengine: *mut c_void) -> i32 {
        (*(ffiengine as *mut T)).initial_placement() as i32
    }

    unsafe extern "C" fn initial_placement_core(ffiengine: *mut c_void) -> i32 {
        (*(ffiengine as *mut T)).initial_placement_core() as i32
    }

    unsafe extern "C" fn optimize_placement(ffiengine: *mut c_void,
        badnodes_ptr: *const*const c_void, badnodes_len: usize) -> i32 {

        let badnodes = slice::from_raw_parts(badnodes_ptr as *const&PARGraphNode, badnodes_len);
        (*(ffiengine as *mut T)).optimize_placement(badnodes) as i32
    }

    unsafe extern "C" fn get_label_name(ffiengine: *mut c_void, label: u32) -> *const i8 {
        (*(ffiengine as *mut T)).get_label_name(label).as_ptr() as *const i8
    }

    unsafe extern "C" fn compute_node_unroutable_cost(ffiengine: *mut c_void,
        pivot: *const c_void, candidate: *const c_void) -> u32 {
        (*(ffiengine as *mut T)).compute_node_unroutable_cost(
            &*(pivot as *const PARGraphNode), &*(candidate as *const PARGraphNode))
    }

    unsafe extern "C" fn _free_edgevec(v: *const*const c_void, len: usize, capacity: usize) {
        Vec::from_raw_parts(v as *mut&PARGraphEdge, len, capacity);
    }

    unsafe extern "C" fn _free_nodevec(v: *const*const c_void, len: usize, capacity: usize) {
        Vec::from_raw_parts(v as *mut&PARGraphNode, len, capacity);
    }
}
