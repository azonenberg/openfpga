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

use std::marker::PhantomData;
use std::os::raw::*;
use std::slice;
use std::str;

pub struct PARGraphEdge<'a> {
    ffi_edge: *const c_void,
    _marker: PhantomData<&'a ()>,
}

impl<'a> PARGraphEdge<'a> {
    pub fn sourceport(&self) -> String {
        unsafe {
            let mut str_len: usize = 0;
            let str_ptr = ffi::xbpar_PARGraphEdge_get_sourceport(self.ffi_edge, &mut str_len);
            let str_slice = slice::from_raw_parts(str_ptr as *const u8, str_len);
            let str_str = str::from_utf8(str_slice).unwrap();
            str_str.to_owned()
        }
    }

    pub fn destport(&self) -> String {
        unsafe {
            let mut str_len: usize = 0;
            let str_ptr = ffi::xbpar_PARGraphEdge_get_destport(self.ffi_edge, &mut str_len);
            let str_slice = slice::from_raw_parts(str_ptr as *const u8, str_len);
            let str_str = str::from_utf8(str_slice).unwrap();
            str_str.to_owned()
        }
    }

    pub fn sourcenode(&'a self) -> PARGraphNode<'a> {
        unsafe {
            PARGraphNode {
                ffi_node: ffi::xbpar_PARGraphEdge_get_sourcenode(self.ffi_edge),
                _marker: PhantomData,
            }
        }
    }

    pub fn destnode(&'a self) -> PARGraphNode<'a> {
        unsafe {
            PARGraphNode {
                ffi_node: ffi::xbpar_PARGraphEdge_get_destnode(self.ffi_edge),
                _marker: PhantomData,
            }
        }
    }
}

// We are never actually allowed to own the C++ PARGraphNode object, so we never have to drop it here on the Rust side
pub struct PARGraphNode<'a> {
    ffi_node: *mut c_void,
    _marker: PhantomData<&'a ()>,
}

impl<'a> PARGraphNode<'a> {
    pub fn mate_with(&'a mut self, mate: &'a PARGraphNode) {
        unsafe {
            ffi::xbpar_PARGraphNode_MateWith(self.ffi_node, mate.ffi_node);
        }
    }

    pub fn get_mate(&'a self) -> PARGraphNode<'a> {
        unsafe {
            PARGraphNode {
                ffi_node: ffi::xbpar_PARGraphNode_GetMate(self.ffi_node),
                _marker: PhantomData,
            }
        }
    }

    pub fn relabel(&mut self, label: u32) {
        unsafe {
            ffi::xbpar_PARGraphNode_Relabel(self.ffi_node, label);
        }
    }

    pub fn get_label(&self) -> u32 {
        unsafe {
            ffi::xbpar_PARGraphNode_GetLabel(self.ffi_node)
        }
    }

    pub fn get_data_pointer_raw(&self) -> *mut c_void {
        unsafe {
            ffi::xbpar_PARGraphNode_GetData(self.ffi_node)
        }
    }

    pub fn add_alternate_label(&mut self, alt: u32) {
        unsafe {
            ffi::xbpar_PARGraphNode_AddAlternateLabel(self.ffi_node, alt);
        }
    }

    pub fn get_alternate_label_count(&self) -> u32 {
        unsafe {
            ffi::xbpar_PARGraphNode_GetAlternateLabelCount(self.ffi_node)
        }
    }

    pub fn get_alternate_label(&self, i: u32) -> u32 {
        unsafe {
            ffi::xbpar_PARGraphNode_GetAlternateLabel(self.ffi_node, i)
        }
    }

    pub fn matches_label(&self, target: u32) -> bool {
        unsafe {
            ffi::xbpar_PARGraphNode_MatchesLabel(self.ffi_node, target) != 0
        }
    }

    pub fn get_edge_count(&self) -> u32 {
        unsafe {
            ffi::xbpar_PARGraphNode_GetEdgeCount(self.ffi_node)
        }
    }

    pub fn get_edge_by_index(&'a self, index: u32) -> PARGraphEdge<'a> {
        unsafe {
            PARGraphEdge {
                ffi_edge: ffi::xbpar_PARGraphNode_GetEdgeByIndex(self.ffi_node, index),
                _marker: PhantomData,
            }
        }
    }

    pub fn add_edge(&'a mut self, srcport: &str, sink: PARGraphNode<'a>, dstport: &str) {
        unsafe {
            ffi::xbpar_PARGraphNode_AddEdge(self.ffi_node, srcport.as_ptr() as *const i8, srcport.len(),
                sink.ffi_node, dstport.as_ptr() as *const i8, dstport.len());
        }
    }

    pub fn remove_edge(&'a mut self, srcport: &str, sink: PARGraphNode<'a>, dstport: &str) {
        unsafe {
            ffi::xbpar_PARGraphNode_RemoveEdge(self.ffi_node, srcport.as_ptr() as *const i8, srcport.len(),
                sink.ffi_node, dstport.as_ptr() as *const i8, dstport.len());
        }
    }
}

pub struct PARGraph<'a> {
    ffi_graph: *mut c_void,
    _marker: PhantomData<&'a ()>,
}

impl<'a> Drop for PARGraph<'a> {
    fn drop(&mut self) {
        unsafe {
            ffi::xbpar_PARGraph_Destroy(self.ffi_graph);
        }
    }
}

impl<'a> PARGraph<'a> {
    pub fn new() -> Self {
        unsafe {
            PARGraph {
                ffi_graph: ffi::xbpar_PARGraph_Create(),
                _marker: PhantomData,
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

    pub fn get_node_by_label_and_index(&'a self, label: u32, index: u32) -> PARGraphNode<'a> {
        unsafe {
            PARGraphNode {
                ffi_node: ffi::xbpar_PARGraph_GetNodeByLabelAndIndex(self.ffi_graph, label, index),
                _marker: PhantomData,
            }
        }
    }

    pub fn get_num_nodes(&self) -> u32 {
        unsafe {
            ffi::xbpar_PARGraph_GetNumNodes(self.ffi_graph)
        }
    }

    pub fn get_node_by_index(&'a self, index: u32) -> PARGraphNode<'a> {
        unsafe {
            PARGraphNode {
                ffi_node: ffi::xbpar_PARGraph_GetNodeByIndex(self.ffi_graph, index),
                _marker: PhantomData,
            }
        }
    }

    pub fn get_num_edges(&self) -> u32 {
        unsafe {
            ffi::xbpar_PARGraph_GetNumEdges(self.ffi_graph)
        }
    }

    pub fn add_new_node(&'a mut self, label: u32, p_data: *mut c_void) -> PARGraphNode<'a> {
        unsafe {
            let ffi_node = ffi::xbpar_PARGraphNode_Create(label, p_data);
            // This transfers ownership to the graph
            ffi::xbpar_PARGraph_AddNode(self.ffi_graph, ffi_node);

            PARGraphNode {
                ffi_node: ffi_node,
                _marker: PhantomData,
            }
        }
    }
}
