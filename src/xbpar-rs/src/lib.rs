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

pub struct PARGraphEdge<D, Dother> (
    // <eddyb> it prevents the compiler from assuming there can only be truly immutable data inside
    UnsafeCell<()>,
    PhantomData<D>,
    // FIXME: Is this correct for variance/dropcheck?
    PhantomData<Dother>
);

impl<D, Dother> PARGraphEdge<D, Dother> {
    pub fn sourceport(&self) -> String {
        unsafe {
            let mut str_len: usize = 0;
            let str_ptr = ffi::xbpar_PARGraphEdge_get_sourceport(
                self as *const PARGraphEdge<D, Dother> as *const c_void, &mut str_len);
            let str_slice = slice::from_raw_parts(str_ptr as *const u8, str_len);
            let str_str = str::from_utf8(str_slice).unwrap();
            str_str.to_owned()
        }
    }

    pub fn destport(&self) -> String {
        unsafe {
            let mut str_len: usize = 0;
            let str_ptr = ffi::xbpar_PARGraphEdge_get_destport(
                self as *const PARGraphEdge<D, Dother> as *const c_void, &mut str_len);
            let str_slice = slice::from_raw_parts(str_ptr as *const u8, str_len);
            let str_str = str::from_utf8(str_slice).unwrap();
            str_str.to_owned()
        }
    }

    pub fn sourcenode(&self) -> &PARGraphNode<D, Dother> {
        unsafe {
            &*(ffi::xbpar_PARGraphEdge_get_sourcenode(self as *const PARGraphEdge<D, Dother> as *const c_void)
                as *const PARGraphNode<D, Dother>)
        }
    }

    pub fn destnode(&self) -> &PARGraphNode<D, Dother> {
        unsafe {
            &*(ffi::xbpar_PARGraphEdge_get_destnode(self as *const PARGraphEdge<D, Dother> as *const c_void)
                as *const PARGraphNode<D, Dother>)
        }
    }

    // We need to take in a &mut in order to hand out a &mut
    pub fn sourcenode_mut(&mut self) -> &mut PARGraphNode<D, Dother> {
        unsafe {
            &mut*(ffi::xbpar_PARGraphEdge_get_sourcenode(self as *const PARGraphEdge<D, Dother> as *const c_void)
                as *mut PARGraphNode<D, Dother>)
        }
    }

    pub fn destnode_mut(&mut self) -> &mut PARGraphNode<D, Dother> {
        unsafe {
            &mut*(ffi::xbpar_PARGraphEdge_get_destnode(self as *const PARGraphEdge<D, Dother> as *const c_void)
                as *mut PARGraphNode<D, Dother>)
        }
    }
}

// We are never actually allowed to own the C++ PARGraphNode object, so we never have to drop it here on the Rust side
pub struct PARGraphNode<D, Dother> (
    UnsafeCell<()>,
    PhantomData<D>,
    PhantomData<Dother>
);

pub struct PARGraphNodeData<D> {
    d: D,
    cpp_idx: u32,
}

impl<D, Dother> PartialEq for PARGraphNode<D, Dother> {
    fn eq(&self, other: &PARGraphNode<D, Dother>) -> bool {
        self as *const PARGraphNode<D, Dother> == other as *const PARGraphNode<D, Dother>
    }
}

impl<D, Dother> Eq for PARGraphNode<D, Dother> {}

impl<D, Dother> Hash for PARGraphNode<D, Dother> {
    fn hash<H: Hasher>(&self, state: &mut H) {
        (self as *const PARGraphNode<D, Dother>).hash(state);
    }
}

impl<D, Dother> PARGraphNode<D, Dother> {
    pub fn get_mate(&self) -> Option<&PARGraphNode<Dother, D>> {
        unsafe {
            let ret_ptr = ffi::xbpar_PARGraphNode_GetMate(
                self as *const PARGraphNode<D, Dother> as *const c_void) as *const PARGraphNode<Dother, D>;
            if ret_ptr.is_null() {
                None
            } else {
                Some(&*(ret_ptr))
            }
        }
    }

    pub fn get_mate_mut(&mut self) -> Option<&mut PARGraphNode<Dother, D>> {
        unsafe {
            let ret_ptr = ffi::xbpar_PARGraphNode_GetMate(
                self as *const PARGraphNode<D, Dother> as *const c_void) as *mut PARGraphNode<Dother, D>;
            if ret_ptr.is_null() {
                None
            } else {
                Some(&mut*(ret_ptr))
            }
        }
    }

    pub fn relabel(&mut self, label: u32) {
        unsafe {
            ffi::xbpar_PARGraphNode_Relabel(self as *mut PARGraphNode<D, Dother> as *mut c_void, label);
        }
    }

    pub fn get_label(&self) -> u32 {
        unsafe {
            ffi::xbpar_PARGraphNode_GetLabel(self as *const PARGraphNode<D, Dother> as *const c_void)
        }
    }

    fn get_data_pointer_raw(&self) -> *mut PARGraphNodeData<D> {
        unsafe {
            ffi::xbpar_PARGraphNode_GetData(self as *const PARGraphNode<D, Dother> as *const c_void)
                as *mut PARGraphNodeData<D>
        }
    }

    pub fn get_associated_data(&self) -> &D {
        unsafe {
            &(*self.get_data_pointer_raw()).d
        }
    }

    pub fn get_associated_data_mut(&mut self) -> &mut D {
        unsafe {
            &mut(*self.get_data_pointer_raw()).d
        }
    }

    pub fn get_index(&self) -> u32 {
        unsafe {
            (*self.get_data_pointer_raw()).cpp_idx
        }
    }

    pub fn add_alternate_label(&mut self, alt: u32) {
        unsafe {
            ffi::xbpar_PARGraphNode_AddAlternateLabel(self as *mut PARGraphNode<D, Dother> as *mut c_void, alt);
        }
    }

    pub fn get_alternate_label_count(&self) -> u32 {
        unsafe {
            ffi::xbpar_PARGraphNode_GetAlternateLabelCount(self as *const PARGraphNode<D, Dother> as *const c_void)
        }
    }

    pub fn get_alternate_label(&self, i: u32) -> u32 {
        unsafe {
            ffi::xbpar_PARGraphNode_GetAlternateLabel(self as *const PARGraphNode<D, Dother> as *const c_void, i)
        }
    }

    pub fn matches_label(&self, target: u32) -> bool {
        unsafe {
            ffi::xbpar_PARGraphNode_MatchesLabel(self as *const PARGraphNode<D, Dother> as *const c_void, target) != 0
        }
    }

    pub fn get_edge_count(&self) -> u32 {
        unsafe {
            ffi::xbpar_PARGraphNode_GetEdgeCount(self as *const PARGraphNode<D, Dother> as *const c_void)
        }
    }

    pub fn get_edge_by_index(&self, index: u32) -> &PARGraphEdge<D, Dother> {
        unsafe {
            &*(ffi::xbpar_PARGraphNode_GetEdgeByIndex(
                self as *const PARGraphNode<D, Dother> as *const c_void, index) as *const PARGraphEdge<D, Dother>)
        }
    }

    // Careful careful: We are handing out a &mut edge so that we can get &mut nodes back out of that
    // but the object we have from C is actually const. This is "safe" because there aren't actually any methods we
    // can call to mutate edges
    pub fn get_edge_by_index_mut(&mut self, index: u32) -> &mut PARGraphEdge<D, Dother> {
        unsafe {
            &mut*(ffi::xbpar_PARGraphNode_GetEdgeByIndex(
                self as *const PARGraphNode<D, Dother> as *const c_void, index) as *mut PARGraphEdge<D, Dother>)
        }
    }
}

pub struct PARGraph<D, Dother> {
    ffi_graph: *mut c_void,
    _marker1: PhantomData<D>,
    _marker2: PhantomData<Dother>
}

impl<D, Dother> Deref for PARGraph<D, Dother> {
    type Target = PARGraph_<D, Dother>;

    fn deref(&self) -> &PARGraph_<D, Dother> {
        unsafe {
            &*(self.ffi_graph as *const PARGraph_<D, Dother>)
        }
    }
}

impl<D, Dother> DerefMut for PARGraph<D, Dother> {
    fn deref_mut(&mut self) -> &mut PARGraph_<D, Dother> {
        unsafe {
            &mut*(self.ffi_graph as *mut PARGraph_<D, Dother>)
        }
    }
}

impl<D, Dother> Drop for PARGraph<D, Dother> {
    fn drop(&mut self) {
        unsafe {
            // Here we have to drop all of the associated data
            for i in 0..self.get_num_nodes() {
                Box::from_raw(self.get_node_by_index(i).get_data_pointer_raw());
            }
            ffi::xbpar_PARGraph_Destroy(self.ffi_graph);
        }
    }
}

pub struct PARGraph_<D, Dother> (
    UnsafeCell<()>,
    PhantomData<D>,
    PhantomData<Dother>
);

// Having this helps prevent crossing nodes between arbitrary and definitely wrong graphs
pub struct PARGraphPair<Dd, Dn>(PARGraph<Dd, Dn>, PARGraph<Dn, Dd>);
pub struct PARGraphRefPair<'a, Dd: 'a, Dn: 'a> {
    pub d: &'a PARGraph_<Dd, Dn>,
    pub n: &'a PARGraph_<Dn, Dd>
}

impl<Dd, Dn> PARGraphPair<Dd, Dn> {
    pub fn new_pair() -> Self {
        unsafe {
            PARGraphPair(
                PARGraph {
                    ffi_graph: ffi::xbpar_PARGraph_Create(),
                    _marker1: PhantomData,
                    _marker2: PhantomData
                },
                PARGraph {
                    ffi_graph: ffi::xbpar_PARGraph_Create(),
                    _marker1: PhantomData,
                    _marker2: PhantomData
                }
            )
        }
    }

    pub fn borrow(&self) -> PARGraphRefPair<Dd, Dn> {
        PARGraphRefPair{d: &self.0, n: &self.1}
    }

    pub fn borrow_mut_d(&mut self) -> &mut PARGraph_<Dd, Dn> {
        &mut self.0
    }

    pub fn borrow_mut_n(&mut self) -> &mut PARGraph_<Dn, Dd> {
        &mut self.1
    }
}

impl<D, Dother> PARGraph_<D, Dother> {
    pub fn allocate_label(&mut self) -> u32 {
        unsafe {
            ffi::xbpar_PARGraph_AllocateLabel(self as *mut PARGraph_<D, Dother> as *mut c_void)
        }
    }

    pub fn get_max_label(&self) -> u32 {
        unsafe {
            ffi::xbpar_PARGraph_GetMaxLabel(self as *const PARGraph_<D, Dother> as *const c_void)
        }
    }

    pub fn get_num_nodes_with_label(&self, label: u32) -> u32 {
        unsafe {
            ffi::xbpar_PARGraph_GetNumNodesWithLabel(self as *const PARGraph_<D, Dother> as *const c_void, label)
        }
    }

    pub fn index_nodes_by_label(&mut self) {
        unsafe {
            ffi::xbpar_PARGraph_IndexNodesByLabel(self as *mut PARGraph_<D, Dother> as *mut c_void);
        }
    }

    pub fn get_node_by_label_and_index(&self, label: u32, index: u32) -> &PARGraphNode<D, Dother> {
        unsafe {
            &*(ffi::xbpar_PARGraph_GetNodeByLabelAndIndex(
                self as *const PARGraph_<D, Dother> as *const c_void, label, index) as *const PARGraphNode<D, Dother>)
        }
    }

    pub fn get_node_by_label_and_index_mut(&mut self, label: u32, index: u32) -> &mut PARGraphNode<D, Dother> {
        unsafe {
            &mut*(ffi::xbpar_PARGraph_GetNodeByLabelAndIndex(
                self as *const PARGraph_<D, Dother> as *const c_void, label, index) as *mut PARGraphNode<D, Dother>)
        }
    }

    pub fn get_num_nodes(&self) -> u32 {
        unsafe {
            ffi::xbpar_PARGraph_GetNumNodes(self as *const PARGraph_<D, Dother> as *const c_void)
        }
    }

    pub fn get_node_by_index(&self, index: u32) -> &PARGraphNode<D, Dother> {
        unsafe {
            &*(ffi::xbpar_PARGraph_GetNodeByIndex(self as *const PARGraph_<D, Dother> as *const c_void, index)
                as *const PARGraphNode<D, Dother>)
        }
    }

    pub fn get_node_by_index_mut(&mut self, index: u32) -> &mut PARGraphNode<D, Dother> {
        unsafe {
            &mut*(ffi::xbpar_PARGraph_GetNodeByIndex(self as *const PARGraph_<D, Dother> as *const c_void, index)
                as *mut PARGraphNode<D, Dother>)
        }
    }

    // We can only safely add edges by indices because we can't give out a &mut and a & in the same graph because
    // it is possible to take one of them and traverse existing edges to get to another one and thereby violate
    // aliasing rules.
    pub fn add_edge(&mut self, source_idx: u32, srcport: &str, sink_idx: u32, dstport: &str) {
        unsafe {
            let source = ffi::xbpar_PARGraph_GetNodeByIndex(
                self as *const PARGraph_<D, Dother> as *const c_void, source_idx);
            let sink = ffi::xbpar_PARGraph_GetNodeByIndex(
                self as *const PARGraph_<D, Dother> as *const c_void, sink_idx);
            ffi::xbpar_PARGraphNode_AddEdge(source, srcport.as_ptr() as *const i8, srcport.len(),
                sink, dstport.as_ptr() as *const i8, dstport.len());
        }
    }

    pub fn remove_edge(&mut self, source_idx: u32, srcport: &str, sink_idx: u32, dstport: &str) {
        unsafe {
            let source = ffi::xbpar_PARGraph_GetNodeByIndex(
                self as *const PARGraph_<D, Dother> as *const c_void, source_idx);
            let sink = ffi::xbpar_PARGraph_GetNodeByIndex(
                self as *const PARGraph_<D, Dother> as *const c_void, sink_idx);
            ffi::xbpar_PARGraphNode_RemoveEdge(source, srcport.as_ptr() as *const i8, srcport.len(),
                sink, dstport.as_ptr() as *const i8, dstport.len());
        }
    }

    pub fn get_num_edges(&self) -> u32 {
        unsafe {
            ffi::xbpar_PARGraph_GetNumEdges(self as *const PARGraph_<D, Dother> as *const c_void)
        }
    }

    // Returns the index of the new node because that's basically the only way we can sanely do it here in rust
    pub fn add_new_node(&mut self, label: u32, p_data: D) -> u32 {
        unsafe {
            let associated_data = Box::into_raw(Box::new(PARGraphNodeData {
                d: p_data,
                cpp_idx: 0,
            }));
            let ffi_node = ffi::xbpar_PARGraphNode_Create(label, associated_data as *mut c_void);
            let ret_idx = ffi::xbpar_PARGraph_GetNumNodes(self as *const PARGraph_<D, Dother> as *const c_void);
            (*associated_data).cpp_idx = ret_idx;
            // This transfers ownership to the graph
            ffi::xbpar_PARGraph_AddNode(self as *mut PARGraph_<D, Dother> as *mut c_void, ffi_node);

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
pub trait PAREngineImpl<'e, 'g: 'e, Dd, Dn> {
    fn set_base_engine(&'e mut self, base_engine: &'g mut BasePAREngine<Dd, Dn>);

    // Overloads
    fn can_move_node(&'e mut self, node: &'g PARGraphNode<Dn, Dd>,
        old_mate: &'g PARGraphNode<Dd, Dn>, new_mate: &'g PARGraphNode<Dd, Dn>) -> bool;
    fn get_new_placement_for_node(&'e mut self, pivot: &'g PARGraphNode<Dn, Dd>) -> Option<&'g PARGraphNode<Dd, Dn>>;
    fn find_suboptimal_placements(&'e mut self) -> Vec<&'g PARGraphNode<Dn, Dd>>;
    fn compute_and_print_score(&'e mut self, iteration: u32) -> (u32, Vec<&'g PARGraphEdge<Dn, Dd>>);
    fn print_unroutes(&'e mut self, unroutes: &[&'g PARGraphEdge<Dn, Dd>]);
    fn compute_congestion_cost(&mut self) -> u32;
    fn compute_timing_cost(&mut self) -> u32;
    fn compute_unroutable_cost(&'e mut self) -> (u32, Vec<&'g PARGraphEdge<Dn, Dd>>);
    fn sanity_check(&mut self) -> bool;
    fn initial_placement(&mut self) -> bool;
    fn initial_placement_core(&mut self) -> bool;
    fn optimize_placement(&'e mut self, badnodes: &[&'g PARGraphNode<Dn, Dd>]) -> bool;
    fn compute_node_unroutable_cost(&'e mut self,
        pivot: &'g PARGraphNode<Dn, Dd>, candidate: &'g PARGraphNode<Dd, Dn>) -> u32;
    fn get_label_name(&mut self, label: u32) -> &str;
}

pub struct BasePAREngine<Dd, Dn> (
    UnsafeCell<()>,
    PhantomData<Dd>,
    PhantomData<Dn>
);

impl<'e, 'g: 'e, Dd, Dn> PAREngineImpl<'e, 'g, Dd, Dn> for BasePAREngine<Dd, Dn> {
    fn set_base_engine(&'e mut self, _: &'g mut BasePAREngine<Dd, Dn>) {}

    fn can_move_node(&'e mut self, node: &'g PARGraphNode<Dn, Dd>,
        old_mate: &'g PARGraphNode<Dd, Dn>, new_mate: &'g PARGraphNode<Dd, Dn>) -> bool {

        unsafe {
            ffi::xbpar_PAREngine_base_CanMoveNode(self as *const BasePAREngine<Dd, Dn> as *const c_void,
                node as *const PARGraphNode<Dn, Dd> as *const c_void,
                old_mate as *const PARGraphNode<Dd, Dn> as *const c_void,
                new_mate as *const PARGraphNode<Dd, Dn> as *const c_void) != 0
        }
    }

    fn get_new_placement_for_node(&'e mut self, _: &'g PARGraphNode<Dn, Dd>) -> Option<&'g PARGraphNode<Dd, Dn>> {
        panic!("pure virtual function call ;)");
    }

    fn find_suboptimal_placements(&'e mut self) -> Vec<&'g PARGraphNode<Dn, Dd>> {
        panic!("pure virtual function call ;)");
    }

    fn compute_and_print_score(&'e mut self, iteration: u32) -> (u32, Vec<&'g PARGraphEdge<Dn, Dd>>) {
        unsafe {
            let mut unroutes_len: usize = 0;
            let mut unroutes_ptr: *const*const c_void = ptr::null_mut();
            let ret = ffi::xbpar_PAREngine_base_ComputeAndPrintScore(
                self as *const BasePAREngine<Dd, Dn> as *const c_void,
                &mut unroutes_ptr, &mut unroutes_len, iteration);
            let unroutes_slice = slice::from_raw_parts(unroutes_ptr as *const&PARGraphEdge<Dn, Dd>, unroutes_len);
            let unroutes = unroutes_slice.to_owned();
            ffi::xbpar_ffi_free_object(unroutes_ptr as *mut c_void);

            (ret, unroutes)
        }
    }

    fn print_unroutes(&'e mut self, unroutes: &[&'g PARGraphEdge<Dn, Dd>]) {
        unsafe {
            ffi::xbpar_PAREngine_base_PrintUnroutes(self as *mut BasePAREngine<Dd, Dn> as *mut c_void,
                unroutes.as_ptr() as *const*const c_void, unroutes.len());
        }
    }

    fn compute_congestion_cost(&mut self) -> u32 {
        unsafe {
            ffi::xbpar_PAREngine_base_ComputeCongestionCost(self as *const BasePAREngine<Dd, Dn> as *const c_void)
        }
    }

    fn compute_timing_cost(&mut self) -> u32 {
        unsafe {
            ffi::xbpar_PAREngine_base_ComputeTimingCost(self as *const BasePAREngine<Dd, Dn> as *const c_void)
        }
    }

    fn compute_unroutable_cost(&'e mut self) -> (u32, Vec<&'g PARGraphEdge<Dn, Dd>>) {
        unsafe {
            let mut unroutes_len: usize = 0;
            let mut unroutes_ptr: *const*const c_void = ptr::null_mut();
            let ret = ffi::xbpar_PAREngine_base_ComputeUnroutableCost(
                self as *const BasePAREngine<Dd, Dn> as *const c_void,
                &mut unroutes_ptr, &mut unroutes_len);
            let unroutes_slice = slice::from_raw_parts(unroutes_ptr as *const&PARGraphEdge<Dn, Dd>, unroutes_len);
            let unroutes = unroutes_slice.to_owned();
            ffi::xbpar_ffi_free_object(unroutes_ptr as *mut c_void);

            (ret, unroutes)
        }
    }

    fn sanity_check(&mut self) -> bool {
        unsafe {
            ffi::xbpar_PAREngine_base_SanityCheck(self as *const BasePAREngine<Dd, Dn> as *const c_void) != 0
        }
    }

    fn initial_placement(&mut self) -> bool {
        unsafe {
            ffi::xbpar_PAREngine_base_InitialPlacement(self as *mut BasePAREngine<Dd, Dn> as *mut c_void) != 0
        }
    }

    fn initial_placement_core(&mut self) -> bool {
        panic!("pure virtual function call ;)");
    }

    fn optimize_placement(&'e mut self, badnodes: &[&'g PARGraphNode<Dn, Dd>]) -> bool {
        unsafe {
            ffi::xbpar_PAREngine_base_OptimizePlacement(self as *mut BasePAREngine<Dd, Dn> as *mut c_void,
                badnodes.as_ptr() as *const*const c_void, badnodes.len()) != 0
        }
    }

    fn compute_node_unroutable_cost(&'e mut self,
        pivot: &'g PARGraphNode<Dn, Dd>, candidate: &'g PARGraphNode<Dd, Dn>) -> u32 {

        unsafe {
            ffi::xbpar_PAREngine_base_ComputeNodeUnroutableCost(self as *mut BasePAREngine<Dd, Dn> as *mut c_void,
                pivot as *const PARGraphNode<Dn, Dd> as *const c_void,
                candidate as *const PARGraphNode<Dd, Dn> as *const c_void)
        }
    }

    fn get_label_name(&mut self, _: u32) -> &str {
        panic!("pure virtual function call ;)");
    }
}

impl<'e, 'g: 'e, Dd, Dn> BasePAREngine<Dd, Dn> {
    pub fn get_graphs(&'e self) -> PARGraphRefPair<'g, Dd, Dn> {// (&'g PARGraph_<Dd, Dn>, &'g PARGraph_<Dn, Dd>) {
        unsafe {
            PARGraphRefPair {
                d: &*(ffi::xbpar_PAREngine_base_get_m_device(self as *const BasePAREngine<Dd, Dn> as *const c_void)
                    as *const PARGraph_<Dd, Dn>),
                n: &*(ffi::xbpar_PAREngine_base_get_m_netlist(self as *const BasePAREngine<Dd, Dn> as *const c_void)
                    as *const PARGraph_<Dn, Dd>)
            }
        }
    }

    pub fn get_m_netlist_mut(&'e mut self) -> &'g mut PARGraph_<Dn, Dd> {
        unsafe {
            &mut*(ffi::xbpar_PAREngine_base_get_m_netlist(self as *const BasePAREngine<Dd, Dn> as *const c_void)
                as *mut PARGraph_<Dn, Dd>)
        }
    }

    pub fn get_m_device_mut(&'e mut self) -> &'g mut PARGraph_<Dd, Dn> {
        unsafe {
            &mut*(ffi::xbpar_PAREngine_base_get_m_device(self as *const BasePAREngine<Dd, Dn> as *const c_void)
                as *mut PARGraph_<Dd, Dn>)
        }
    }

    // FIXME: This is semantically bullshit. Why is it attached to this type?
    pub fn mate_nodes(&mut self, n_idx: u32, d_idx: u32) {
        unsafe {
            let m_netlist = &mut*(ffi::xbpar_PAREngine_base_get_m_netlist(
                self as *const BasePAREngine<Dd, Dn> as *const c_void) as *mut PARGraph_<Dn, Dd>);
            let m_device = &mut*(ffi::xbpar_PAREngine_base_get_m_device(
                self as *const BasePAREngine<Dd, Dn> as *const c_void) as *mut PARGraph_<Dd, Dn>);
            let node = m_netlist.get_node_by_index_mut(n_idx);
            let mate = m_device.get_node_by_index_mut(d_idx);
            ffi::xbpar_PARGraphNode_MateWith(node as *mut PARGraphNode<Dn, Dd> as *mut c_void,
                mate as *mut PARGraphNode<Dd, Dn> as *mut c_void);
        }
    }

    // FIXME: Is this interior mutability correct?
    pub fn random_number(&self) -> u32 {
        unsafe {
            ffi::xbpar_PAREngine_RandomNumber(self as *const BasePAREngine<Dd, Dn> as *mut c_void)
        }
    }
}

pub struct PAREngine<'e, 'g: 'e, Dd: 'g, Dn: 'g, T: 'e + PAREngineImpl<'e, 'g, Dd, Dn>> {
    ffi_engine: *mut c_void,
    inner_impl: *mut T,
    _marker1: PhantomData<&'g ()>,
    _marker2: PhantomData<&'e mut T>,
    // FIXME: WTF?
    _marker3: PhantomData<Dd>,
    _marker4: PhantomData<Dn>
}

impl<'e, 'g: 'e, Dd: 'g, Dn: 'g, T: 'e + PAREngineImpl<'e, 'g, Dd, Dn>> Drop for PAREngine<'e, 'g, Dd, Dn, T> {
    fn drop(&mut self) {
        unsafe {
            Box::from_raw(self.inner_impl);
            ffi::xbpar_PAREngine_Destroy(self.ffi_engine);
        }
    }
}

impl<'e, 'g: 'e, Dd: 'g, Dn: 'g, T: 'e + PAREngineImpl<'e, 'g, Dd, Dn>> PAREngine<'e, 'g, Dd, Dn, T> {
    pub fn new(inner_impl: T, graphs: &'g mut PARGraphPair<Dd, Dn>) -> Self {
        unsafe {
            let boxed_impl = Box::into_raw(Box::new(inner_impl));

            let ffi_engine = ffi::xbpar_PAREngine_Create(
                boxed_impl as *mut c_void,
                graphs.1.ffi_graph, graphs.0.ffi_graph,
                Some(PAREngine::<Dd, Dn, T>::get_new_placement_for_node),
                Some(PAREngine::<Dd, Dn, T>::find_suboptimal_placements),
                Some(PAREngine::<Dd, Dn, T>::initial_placement_core),
                Some(PAREngine::<Dd, Dn, T>::get_label_name),
                Some(PAREngine::<Dd, Dn, T>::can_move_node),
                Some(PAREngine::<Dd, Dn, T>::compute_and_print_score),
                Some(PAREngine::<Dd, Dn, T>::print_unroutes),
                Some(PAREngine::<Dd, Dn, T>::compute_congestion_cost),
                Some(PAREngine::<Dd, Dn, T>::compute_timing_cost),
                Some(PAREngine::<Dd, Dn, T>::compute_unroutable_cost),
                Some(PAREngine::<Dd, Dn, T>::sanity_check),
                Some(PAREngine::<Dd, Dn, T>::initial_placement),
                Some(PAREngine::<Dd, Dn, T>::optimize_placement),
                Some(PAREngine::<Dd, Dn, T>::compute_node_unroutable_cost),
                Some(PAREngine::<Dd, Dn, T>::_free_edgevec),
                Some(PAREngine::<Dd, Dn, T>::_free_nodevec));

            (*boxed_impl).set_base_engine(&mut*(ffi_engine as *mut BasePAREngine<Dd, Dn>));

            PAREngine {
                ffi_engine: ffi_engine,
                inner_impl: boxed_impl,
                _marker1: PhantomData,
                _marker2: PhantomData,
                _marker3: PhantomData,
                _marker4: PhantomData,
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

        (*(ffiengine as *mut T)).can_move_node(&*(node as *const PARGraphNode<Dn, Dd>),
            &*(old_mate as *const PARGraphNode<Dd, Dn>), &*(new_mate as *const PARGraphNode<Dd, Dn>)) as i32
    }

    unsafe extern "C" fn get_new_placement_for_node(ffiengine: *mut c_void, pivot: *const c_void) -> *mut c_void {
        let ret = (*(ffiengine as *mut T)).get_new_placement_for_node(&*(pivot as *const PARGraphNode<Dn, Dd>));

        if let Some(ret) = ret {
            ret as *const PARGraphNode<Dd, Dn> as *mut c_void
        } else {
            ptr::null_mut()
        }
    }

    unsafe extern "C" fn find_suboptimal_placements(ffiengine: *mut c_void,
        bad_nodes_ptr: *mut*const*const c_void, bad_nodes_len: *mut usize, bad_nodes_capacity: *mut usize) {

        let bad_nodes = (*(ffiengine as *mut T)).find_suboptimal_placements();

        *bad_nodes_ptr = bad_nodes.as_ptr() as *const*const PARGraphNode<Dn, Dd> as *const*const c_void;
        *bad_nodes_len = bad_nodes.len();
        *bad_nodes_capacity = bad_nodes.capacity();
        mem::forget(bad_nodes);
    }

    unsafe extern "C" fn compute_and_print_score(ffiengine: *mut c_void,
        unroutes_ptr: *mut*const*const c_void, unroutes_len: *mut usize, unroutes_capacity: *mut usize,
        iteration: u32) -> u32 {

        let (ret, unroutes) = (*(ffiengine as *mut T)).compute_and_print_score(iteration);

        *unroutes_ptr = unroutes.as_ptr() as *const*const PARGraphEdge<Dn, Dd> as *const*const c_void;
        *unroutes_len = unroutes.len();
        *unroutes_capacity = unroutes.capacity();
        mem::forget(unroutes);

        ret
    }

    unsafe extern "C" fn print_unroutes(ffiengine: *mut c_void,
        unroutes_ptr: *const*const c_void, unroutes_len: usize) {

        let unroutes = slice::from_raw_parts(unroutes_ptr as *const&PARGraphEdge<Dn, Dd>, unroutes_len);
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

        *unroutes_ptr = unroutes.as_ptr() as *const*const PARGraphEdge<Dn, Dd> as *const*const c_void;
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

        let badnodes = slice::from_raw_parts(badnodes_ptr as *const&PARGraphNode<Dn, Dd>, badnodes_len);
        (*(ffiengine as *mut T)).optimize_placement(badnodes) as i32
    }

    unsafe extern "C" fn get_label_name(ffiengine: *mut c_void, label: u32) -> *const i8 {
        (*(ffiengine as *mut T)).get_label_name(label).as_ptr() as *const i8
    }

    unsafe extern "C" fn compute_node_unroutable_cost(ffiengine: *mut c_void,
        pivot: *const c_void, candidate: *const c_void) -> u32 {
        (*(ffiengine as *mut T)).compute_node_unroutable_cost(
            &*(pivot as *const PARGraphNode<Dn, Dd>), &*(candidate as *const PARGraphNode<Dd, Dn>))
    }

    unsafe extern "C" fn _free_edgevec(v: *const*const c_void, len: usize, capacity: usize) {
        Vec::from_raw_parts(v as *mut&PARGraphEdge<Dn, Dd>, len, capacity);
    }

    unsafe extern "C" fn _free_nodevec(v: *const*const c_void, len: usize, capacity: usize) {
        Vec::from_raw_parts(v as *mut&PARGraphNode<Dn, Dd>, len, capacity);
    }
}
