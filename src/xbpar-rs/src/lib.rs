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
use std::marker::PhantomData;
use std::os::raw::*;
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

impl PARGraphNode {
    pub fn mate_with<'a>(&'a mut self, mate: &'a mut PARGraphNode) {
        unsafe {
            ffi::xbpar_PARGraphNode_MateWith(self as *mut PARGraphNode as *mut c_void,
                mate as *mut PARGraphNode as *mut c_void);
        }
    }

    pub fn get_mate(&self) -> &PARGraphNode {
        unsafe {
            &*(ffi::xbpar_PARGraphNode_GetMate(self as *const PARGraphNode as *const c_void) as *const PARGraphNode)
        }
    }

    pub fn get_mate_mut(&mut self) -> &mut PARGraphNode {
        unsafe {
            &mut*(ffi::xbpar_PARGraphNode_GetMate(self as *const PARGraphNode as *const c_void) as *mut PARGraphNode)
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

    // Careful careful: sink isn't a &mut, but it turns into a mutable pointer. This should be safe because we know
    // the object was "originally" mutable.
    pub fn add_edge<'a, 'b, 'c>(&'a mut self, srcport: &'b str, sink: &'a PARGraphNode, dstport: &'c str) {
        unsafe {
            ffi::xbpar_PARGraphNode_AddEdge(self as *mut PARGraphNode as *mut c_void,
                srcport.as_ptr() as *const i8, srcport.len(),
                sink as *const PARGraphNode as *mut c_void,
                dstport.as_ptr() as *const i8, dstport.len());
        }
    }

    pub fn remove_edge<'a, 'b, 'c>(&'a mut self, srcport: &'b str, sink: &'a PARGraphNode, dstport: &'c str) {
        unsafe {
            ffi::xbpar_PARGraphNode_RemoveEdge(self as *mut PARGraphNode as *mut c_void,
                srcport.as_ptr() as *const i8, srcport.len(),
                sink as *const PARGraphNode as *mut c_void,
                dstport.as_ptr() as *const i8, dstport.len());
        }
    }
}

pub struct PARGraph {
    ffi_graph: *mut c_void,
}

impl Drop for PARGraph {
    fn drop(&mut self) {
        unsafe {
            ffi::xbpar_PARGraph_Destroy(self.ffi_graph);
        }
    }
}

impl PARGraph {
    pub fn new() -> Self {
        unsafe {
            PARGraph {
                ffi_graph: ffi::xbpar_PARGraph_Create(),
            }
        }
    }

    pub fn allocate_label(&mut self) -> u32 {
        unsafe {
            ffi::xbpar_PARGraph_AllocateLabel(self.ffi_graph)
        }
    }

    pub fn get_max_label(&self) -> u32 {
        unsafe {
            ffi::xbpar_PARGraph_GetMaxLabel(self.ffi_graph)
        }
    }

    pub fn get_num_nodes_with_label(&self, label: u32) -> u32 {
        unsafe {
            ffi::xbpar_PARGraph_GetNumNodesWithLabel(self.ffi_graph, label)
        }
    }

    pub fn index_nodes_by_label(&mut self) {
        unsafe {
            ffi::xbpar_PARGraph_IndexNodesByLabel(self.ffi_graph);
        }
    }

    pub fn get_node_by_label_and_index(&self, label: u32, index: u32) -> &PARGraphNode {
        unsafe {
            &*(ffi::xbpar_PARGraph_GetNodeByLabelAndIndex(self.ffi_graph, label, index) as *const PARGraphNode)
        }
    }

    pub fn get_node_by_label_and_index_mut(&mut self, label: u32, index: u32) -> &mut PARGraphNode {
        unsafe {
            &mut*(ffi::xbpar_PARGraph_GetNodeByLabelAndIndex(self.ffi_graph, label, index) as *mut PARGraphNode)
        }
    }

    pub fn get_num_nodes(&self) -> u32 {
        unsafe {
            ffi::xbpar_PARGraph_GetNumNodes(self.ffi_graph)
        }
    }

    pub fn get_node_by_index(&self, index: u32) -> &PARGraphNode {
        unsafe {
            &*(ffi::xbpar_PARGraph_GetNodeByIndex(self.ffi_graph, index) as *const PARGraphNode)
        }
    }

    pub fn get_node_by_index_mut(&mut self, index: u32) -> &mut PARGraphNode {
        unsafe {
            &mut*(ffi::xbpar_PARGraph_GetNodeByIndex(self.ffi_graph, index) as *mut PARGraphNode)
        }
    }

    // This is a bit of a hack to allow us to actually create edges in the graph. Otherwise we won't be able to both
    // borrow one node mutably and a different node immutably.
    pub fn get_node_by_index_mut_pair<'a>(&'a mut self, index1: u32, index2: u32)
        -> (&'a mut PARGraphNode, &'a mut PARGraphNode) {

        // Because self is &mut, we only need to check whether the indices are the same and not if we have ever
        // loaned out a copy of the node
        if index1 == index2 {
            panic!("attempted to borrow the same node mutably twice");
        }
        unsafe {
            (&mut*(ffi::xbpar_PARGraph_GetNodeByIndex(self.ffi_graph, index1) as *mut PARGraphNode),
                &mut*(ffi::xbpar_PARGraph_GetNodeByIndex(self.ffi_graph, index2) as *mut PARGraphNode))
        }
    }

    pub fn get_num_edges(&self) -> u32 {
        unsafe {
            ffi::xbpar_PARGraph_GetNumEdges(self.ffi_graph)
        }
    }

    // Returns the index of the new node because that's basically the only way we can sanely do it here in rust
    pub fn add_new_node(&mut self, label: u32, p_data: *mut c_void) -> u32 {
        unsafe {
            let ffi_node = ffi::xbpar_PARGraphNode_Create(label, p_data);
            let ret_idx = ffi::xbpar_PARGraph_GetNumNodes(self.ffi_graph);
            // This transfers ownership to the graph
            ffi::xbpar_PARGraph_AddNode(self.ffi_graph, ffi_node);

            ret_idx
        }
    }
}

pub struct PAREngine<'a> {
    ffi_engine: *mut c_void,
    _marker: PhantomData<&'a ()>,
}

impl<'a> Drop for PAREngine<'a> {
    fn drop(&mut self) {
        unsafe {
            ffi::xbpar_PAREngine_Destroy(self.ffi_engine);
        }
    }
}

impl<'a> PAREngine<'a> {
    pub fn new(netlist: &'a mut PARGraph, device: &'a mut PARGraph) -> PAREngine<'a> {
        unsafe {
            PAREngine {
                ffi_engine: ffi::xbpar_PAREngine_Create(std::ptr::null_mut(), netlist.ffi_graph, device.ffi_graph,
                    None,
                    None,
                    None,
                    None,
                    None,
                    None,
                    None,
                    None,
                    None,
                    None,
                    None,
                    None,
                    None,
                    None,
                    None,
                    None),
                _marker: PhantomData,
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
}
