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

use std::ascii::AsciiExt;
use std::collections::HashMap;

use *;
use objpool::*;

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
}

pub struct DeviceGraph {
    nodes: ObjPool<DeviceGraphNode>,
}

fn alloc_label(par_graphs: &mut PARGraphPair<ObjPoolIndex<DeviceGraphNode>, ()>,
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
    pub fn new(device_name: &str, par_graphs: &mut PARGraphPair<ObjPoolIndex<DeviceGraphNode>, ()>)
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

        let (num_fbs, num_iobs, has_inpad, iob_to_fb_ff, zia_table) = if device_name.eq_ignore_ascii_case("XC2C32A") {
            (2, 32, true, iob_num_to_fb_ff_num_32, ZIA_BIT_TO_CHOICE_32)
        } else {
            panic!("Unsupported device name!");
        };

        // Create the GCK buffer nodes
        let mut bufg_clk_par_idxs = [0u32; 3];
        for i in 0..bufg_clk_par_idxs.len() {
            let pool_idx = graph.nodes.insert(DeviceGraphNode::BufgClk{i: i as u32});
            bufg_clk_par_idxs[i] = par_graphs.borrow_mut_d().add_new_node(bufg_l, pool_idx);
        }

        // Create the GTS buffer nodes
        let mut bufg_gts_par_idxs = [0u32; 4];
        for i in 0..bufg_gts_par_idxs.len() {
            let pool_idx = graph.nodes.insert(DeviceGraphNode::BufgGTS{i: i as u32});
            bufg_gts_par_idxs[i] = par_graphs.borrow_mut_d().add_new_node(bufg_l, pool_idx);
        }

        // Create the GSR buffer nodes
        let bufg_gsr_par_idx = {
            let pool_idx = graph.nodes.insert(DeviceGraphNode::BufgGSR);
            par_graphs.borrow_mut_d().add_new_node(bufg_l, pool_idx)
        };

        // FIXME: The Vec(s) aren't the most efficient thing possible, but meh
        // Function Block stuff
        let fb_related_par_idxs = (0..num_fbs).map(|fb| {
            // Create nodes

            // 56 AND term sites
            let mut andterm_par_idxs = [0u32; 56];
            for andterm_i in 0..andterm_par_idxs.len() {
                let andterm_pool_idx = graph.nodes.insert(DeviceGraphNode::AndTerm{fb: fb, i: andterm_i as u32});
                andterm_par_idxs[andterm_i] = par_graphs.borrow_mut_d().add_new_node(andterm_l, andterm_pool_idx);
            }

            // 16 OR term sites
            let mut orterm_par_idxs = [0u32; 16];
            for orterm_i in 0..orterm_par_idxs.len() {
                let orterm_pool_idx = graph.nodes.insert(DeviceGraphNode::OrTerm{fb: fb, i: orterm_i as u32});
                orterm_par_idxs[orterm_i] = par_graphs.borrow_mut_d().add_new_node(orterm_l, orterm_pool_idx);
            }

            // 16 XOR sites
            let mut xor_par_idxs = [0u32; 16];
            for xor_i in 0..xor_par_idxs.len() {
                let xor_pool_idx = graph.nodes.insert(DeviceGraphNode::Xor{fb: fb, i: xor_i as u32});
                xor_par_idxs[xor_i] = par_graphs.borrow_mut_d().add_new_node(xor_l, xor_pool_idx);
            }

            // 16 register sites
            let mut reg_par_idxs = [0u32; 16];
            for reg_i in 0..reg_par_idxs.len() {
                let reg_pool_idx = graph.nodes.insert(DeviceGraphNode::Reg{fb: fb, i: reg_i as u32});
                reg_par_idxs[reg_i] = par_graphs.borrow_mut_d().add_new_node(reg_l, reg_pool_idx);
            }

            // Create edges that exist within the FB

            // AND term to OR term
            for orterm_i in 0..orterm_par_idxs.len() {
                // one edge from each AND term
                for andterm_i in 0..andterm_par_idxs.len() {
                    par_graphs.borrow_mut_d().add_edge(andterm_par_idxs[andterm_i], "OUT",
                        orterm_par_idxs[orterm_i], &format!("OR_IN_{}", andterm_i));
                }
            }

            // OR term and PTC to XOR
            for i in 0..16 {
                par_graphs.borrow_mut_d().add_edge(orterm_par_idxs[i], "OUT", xor_par_idxs[i], "IN_ORTERM");
                par_graphs.borrow_mut_d().add_edge(andterm_par_idxs[get_ptc(i as u32) as usize], "OUT",
                    xor_par_idxs[i], "IN_PTC");
            }

            // XOR to register (only, not the direct input)
            for i in 0..16 {
                par_graphs.borrow_mut_d().add_edge(xor_par_idxs[i], "OUT", reg_par_idxs[i], "D/T");
            }

            // The other register control signals
            for i in 0..16 {
                // CE
                par_graphs.borrow_mut_d().add_edge(andterm_par_idxs[get_ptc(i as u32) as usize], "OUT",
                    reg_par_idxs[i], "CE");

                // Clocks
                // GCK
                for j in 0..bufg_clk_par_idxs.len() {
                    par_graphs.borrow_mut_d().add_edge(bufg_clk_par_idxs[j], "OUT", reg_par_idxs[i], "CLK");
                }
                // CTC
                par_graphs.borrow_mut_d().add_edge(andterm_par_idxs[get_ctc() as usize], "OUT",
                    reg_par_idxs[i], "CLK");
                // PTC
                par_graphs.borrow_mut_d().add_edge(andterm_par_idxs[get_ptc(i as u32) as usize], "OUT",
                    reg_par_idxs[i], "CLK");

                // Set
                // GSR
                par_graphs.borrow_mut_d().add_edge(bufg_gsr_par_idx, "OUT", reg_par_idxs[i], "S");
                // CTS
                par_graphs.borrow_mut_d().add_edge(andterm_par_idxs[get_cts() as usize], "OUT",
                    reg_par_idxs[i], "CLK");
                // PTA
                par_graphs.borrow_mut_d().add_edge(andterm_par_idxs[get_pta(i as u32) as usize], "OUT",
                    reg_par_idxs[i], "CLK");

                // Reset
                // GSR
                par_graphs.borrow_mut_d().add_edge(bufg_gsr_par_idx, "OUT", reg_par_idxs[i], "S");
                // CTR
                par_graphs.borrow_mut_d().add_edge(andterm_par_idxs[get_ctr() as usize], "OUT",
                    reg_par_idxs[i], "CLK");
                // PTA
                par_graphs.borrow_mut_d().add_edge(andterm_par_idxs[get_pta(i as u32) as usize], "OUT",
                    reg_par_idxs[i], "CLK");
            }

            (andterm_par_idxs, xor_par_idxs, reg_par_idxs)
        }).collect::<Vec<_>>();

        // IO buffer stuff
        let iob_par_idxs = (0..num_iobs).map(|iob| {
            // Create node
            let iob_pool_idx = graph.nodes.insert(DeviceGraphNode::IOBuf{i: iob as u32});
            let iob_par_idx = par_graphs.borrow_mut_d().add_new_node(iopad_l, iob_pool_idx);

            // Create the edges that don't need ZIA-related knowledge (OE, direct input, output data)
            let (fb, ff) = iob_to_fb_ff(iob).unwrap();

            // GTS
            for j in 0..bufg_gts_par_idxs.len() {
                par_graphs.borrow_mut_d().add_edge(bufg_gts_par_idxs[j], "OUT", iob_par_idx, "OE");
            }
            // CTE
            par_graphs.borrow_mut_d().add_edge(fb_related_par_idxs[fb as usize].0[get_cte() as usize], "OUT",
                iob_par_idx, "OE");
            // PTB
            par_graphs.borrow_mut_d().add_edge(fb_related_par_idxs[fb as usize].0[get_ptb(ff as u32) as usize], "OUT",
                iob_par_idx, "OE");

            // output data
            par_graphs.borrow_mut_d().add_edge(fb_related_par_idxs[fb as usize].1[ff as usize], "OUT",
                iob_par_idx, "IN");
            par_graphs.borrow_mut_d().add_edge(fb_related_par_idxs[fb as usize].2[ff as usize], "Q",
                iob_par_idx, "IN");

            // direct input
            par_graphs.borrow_mut_d().add_edge(iob_par_idx, "OUT",
                fb_related_par_idxs[fb as usize].2[ff as usize], "D/T");

            iob_par_idx
        }).collect::<Vec<_>>();

        // Input-only pads (in reality only one)
        let inpad_par_idx = if has_inpad {
            // Create node
            let inpad_pool_idx = graph.nodes.insert(DeviceGraphNode::InBuf);
            Some(par_graphs.borrow_mut_d().add_new_node(inpad_l, inpad_pool_idx))
        } else {
            None
        };

        // Now create all of the wonderful ZIA interconnects
        for zia_row_i in 0..zia_table.len() {
            let zia_row = &zia_table[zia_row_i];

            // Into each FB from ZIA
            for fb in 0..num_fbs {
                // Into every AND term
                for andterm_i in 0..fb_related_par_idxs[fb as usize].0.len() {
                    for zia_choice in zia_row {
                        match zia_choice {
                            &XC2ZIAInput::Macrocell{fb, ff} => {
                                // From the XOR gate
                                par_graphs.borrow_mut_d().add_edge(
                                    fb_related_par_idxs[fb as usize].1[ff as usize], "OUT",
                                    fb_related_par_idxs[fb as usize].0[andterm_i], &format!("AND_IN_{}", zia_row_i));
                                // From the register
                                par_graphs.borrow_mut_d().add_edge(
                                    fb_related_par_idxs[fb as usize].2[ff as usize], "Q",
                                    fb_related_par_idxs[fb as usize].0[andterm_i], &format!("AND_IN_{}", zia_row_i));
                            },
                            &XC2ZIAInput::IBuf{ibuf} => {
                                let (fb, ff) = iob_to_fb_ff(ibuf).unwrap();
                                // From the pad
                                par_graphs.borrow_mut_d().add_edge(
                                    iob_par_idxs[ibuf as usize], "OUT",
                                    fb_related_par_idxs[fb as usize].0[andterm_i], &format!("AND_IN_{}", zia_row_i));
                                // From the register
                                par_graphs.borrow_mut_d().add_edge(
                                    fb_related_par_idxs[fb as usize].2[ff as usize], "Q",
                                    fb_related_par_idxs[fb as usize].0[andterm_i], &format!("AND_IN_{}", zia_row_i));
                            },
                            &XC2ZIAInput::DedicatedInput => {
                                par_graphs.borrow_mut_d().add_edge(
                                    inpad_par_idx.unwrap(), "OUT",
                                    fb_related_par_idxs[fb as usize].0[andterm_i], &format!("AND_IN_{}", zia_row_i));
                            },
                            // These cannot be in the choices table; they are special cases
                            _ => unreachable!(),
                        }
                    }
                }
            }
        }

        (graph, lmap)
    }
}
