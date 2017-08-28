/*
Copyright (c) 2016-2017, Robert Ou <rqou@robertou.com> and contributors
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

extern crate xbpar_rs;
use self::xbpar_rs::*;

extern crate xc2bit;
use self::xc2bit::*;

use std::cell::RefCell;
use std::collections::HashMap;
use std::collections::HashSet;

use *;
use objpool::*;

#[derive(Debug)]
pub enum DeviceGraphNode {
    AndTerm {
        fb: u32,
        i: u32,
    },
    OrTerm {
        fb: u32,
        i: u32,
    },
    Xor {
        fb: u32,
        i: u32,
    },
    Reg {
        fb: u32,
        i: u32,
    },
    BufgClk {
        i: u32,
    },
    BufgGTS {
        i: u32,
    },
    BufgGSR,
    IOBuf {
        i: u32,
    },
    InBuf,
    ZIADummyBuf {
        fb: u32,
        row: u32,
    }
}

#[derive(Debug)]
pub struct DeviceGraph {
    pub nodes: ObjPool<DeviceGraphNode>,
}

fn alloc_label(par_graphs: &mut PARGraphPair<ObjPoolIndex<DeviceGraphNode>, ObjPoolIndex<NetlistGraphNode>>,
    lmap: &mut HashMap<u32, &'static str>, label: &'static str) -> u32 {

    let lbl_d = par_graphs.borrow_mut_d().allocate_label();
    let lbl_n = par_graphs.borrow_mut_n().allocate_label();
    assert_eq!(lbl_d, lbl_n);
    lmap.insert(lbl_d, label);

    lbl_d
}

impl DeviceGraph {
    // Somewhat strange API - mutates an already created PAR engine graph but returns a new native graph as well as
    // the label map.
    pub fn new(device_name: &str,
        par_graphs: &mut PARGraphPair<ObjPoolIndex<DeviceGraphNode>, ObjPoolIndex<NetlistGraphNode>>)
        -> (DeviceGraph, HashMap<u32, &'static str>) {

        let mut graph = DeviceGraph {
            nodes: ObjPool::new(),
        };

        let mut lmap = HashMap::new();

        // Allocate labels for all the possible site types
        let iopad_l = alloc_label(par_graphs, &mut lmap, "IOPAD");
        let inpad_l = alloc_label(par_graphs, &mut lmap, "INPAD");
        let reg_l = alloc_label(par_graphs, &mut lmap, "REG");
        let xor_l = alloc_label(par_graphs, &mut lmap, "XOR");
        let orterm_l = alloc_label(par_graphs, &mut lmap, "ORTERM");
        let andterm_l = alloc_label(par_graphs, &mut lmap, "ANDTERM");
        let bufg_l = alloc_label(par_graphs, &mut lmap, "BUFG");
        let ziabuf_l = alloc_label(par_graphs, &mut lmap, "ZIA dummy buffer");

        let wire_map = RefCell::new(Vec::new());
        let wire_idx = RefCell::new(0);

        let (device_type, _, _) = parse_part_name_string(device_name).expect("invalid device name");

        // Need to create ZIA dummy buffers
        let zia_dummy_bufs = (0..device_type.num_fbs()).map(|fb| {
            (0..INPUTS_PER_ANDTERM).map(|zia_row_i| {
                let pool_idx = graph.nodes.insert(DeviceGraphNode::ZIADummyBuf{fb: fb as u32, row: zia_row_i as u32});
                par_graphs.borrow_mut_d().add_new_node(ziabuf_l, pool_idx)
            }).collect::<Vec<_>>()
        }).collect::<Vec<_>>();

        // Call the giant callback structure obtainer
        get_device_structure(device_type,
            |_: &str, node_type: &str, fb: u32, idx: u32| {
                let (pool_idx, label) = match node_type {
                    "BUFG" => {
                        let pool_idx = graph.nodes.insert(DeviceGraphNode::BufgClk{i: idx as u32});
                        (pool_idx, bufg_l)
                    },
                    "BUFGSR" => {
                        let pool_idx = graph.nodes.insert(DeviceGraphNode::BufgGSR);
                        (pool_idx, bufg_l)
                    },
                    "BUFGTS" => {
                        let pool_idx = graph.nodes.insert(DeviceGraphNode::BufgGTS{i: idx as u32});
                        (pool_idx, bufg_l)
                    },
                    "IOBUFE" => {
                        let pool_idx = graph.nodes.insert(DeviceGraphNode::IOBuf{i: idx as u32});
                        (pool_idx, iopad_l)
                    },
                    "IBUF" => {
                        let pool_idx = graph.nodes.insert(DeviceGraphNode::InBuf);
                        (pool_idx, inpad_l)
                    },
                    "ANDTERM" => {
                        let pool_idx = graph.nodes.insert(DeviceGraphNode::AndTerm{fb: fb as u32, i: idx as u32});
                        (pool_idx, andterm_l)
                    },
                    "ORTERM" => {
                        let pool_idx = graph.nodes.insert(DeviceGraphNode::OrTerm{fb: fb as u32, i: idx as u32});
                        (pool_idx, orterm_l)
                    },
                    "MACROCELL_XOR" => {
                        let pool_idx = graph.nodes.insert(DeviceGraphNode::Xor{fb: fb as u32, i: idx as u32});
                        (pool_idx, xor_l)
                    },
                    "REG" => {
                        let pool_idx = graph.nodes.insert(DeviceGraphNode::Reg{fb: fb as u32, i: idx as u32});
                        (pool_idx, reg_l)
                    },
                    _ => unreachable!(),
                };
                let par_idx = par_graphs.borrow_mut_d().add_new_node(label, pool_idx);
                if node_type == "IOBUFE" {
                    par_graphs.borrow_mut_d().get_node_by_index_mut(par_idx).add_alternate_label(inpad_l);
                }

                // FIXME: As usual, there is a hack for the ZIA
                if node_type == "ANDTERM" {
                    // Connect the ZIA buffers to this AND term
                    for i in 0..INPUTS_PER_ANDTERM {
                        par_graphs.borrow_mut_d().add_edge(zia_dummy_bufs[fb as usize][i], "OUT", par_idx, "IN");
                    }

                    0x8000000000000000 | ((fb as usize) << 32) | (par_idx as usize)
                } else {
                    par_idx as usize
                }
            },
            |_: &str| {
                let orig_wire_idx = {*wire_idx.borrow()};
                *wire_idx.borrow_mut() += 1;
                wire_map.borrow_mut().push((Vec::new(), Vec::new()));
                orig_wire_idx
            },
            |node_ref: usize, wire_ref: usize, port_name: &'static str, port_idx: u32, extra_data: (u32, u32)| {
                let mut wire_map = wire_map.borrow_mut();
                let wire_vecs = &mut wire_map[wire_ref];

                let is_source = match port_name {
                    "O" | "OUT" | "Q" => true,
                    "I" | "E" | "IN" | "IN_PTC" | "IN_ORTERM" | "D/T" | "CE" | "CLK" | "S" | "R" => false,
                    _ => unreachable!(),
                };

                if is_source {
                    wire_vecs.0.push((node_ref, port_name, port_idx, extra_data));
                } else {
                    wire_vecs.1.push((node_ref, port_name, port_idx, extra_data));
                }
            },
        );

        // Create the edges that were buffered up
        let mut zia_term_used_already_hack = HashSet::new();
        for (sources, sinks) in wire_map.into_inner().into_iter() {
            for &(source_node_ref, source_port_name, _, _) in &sources {
                for &(sink_node_ref, sink_port_name, sink_port_idx, sink_extra_data) in &sinks {
                    if sink_node_ref & 0x8000000000000000 == 0 {
                        // Not going into the AND terms

                        // However, it might be coming from the AND terms
                        let source_node_ref = if source_node_ref & 0x8000000000000000 != 0 {
                            source_node_ref & 0xFFFFFFFF
                        } else { source_node_ref };

                        par_graphs.borrow_mut_d().add_edge(source_node_ref as u32, source_port_name,
                            sink_node_ref as u32, sink_port_name);
                    } else {
                        // AND terms are special as usual
                        let sink_node_fb = (sink_node_ref & !0x8000000000000000) >> 32;
                        let sink_node_ref = zia_dummy_bufs[sink_node_fb][sink_port_idx as usize];

                        // "Intercept" wires going from ZIA to P-terms and only put one set.
                        // The actual fanning out is done with the ZIA dummy buffers.
                        if !zia_term_used_already_hack.contains(
                            &(sink_node_ref, sink_port_name, sink_port_idx, sink_extra_data)) {

                            zia_term_used_already_hack.insert(
                                (sink_node_ref, sink_port_name, sink_port_idx, sink_extra_data));

                            par_graphs.borrow_mut_d().add_edge(source_node_ref as u32, source_port_name,
                                sink_node_ref, sink_port_name);
                        }
                    }
                }
            }
        }

        (graph, lmap)
    }
}
