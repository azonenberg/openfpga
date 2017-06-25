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

use *;
use objpool::*;

pub fn produce_bitstream(par_graphs: &PARGraphPair<ObjPoolIndex<DeviceGraphNode>, ObjPoolIndex<NetlistGraphNode>>,
    dgraph_rs: &DeviceGraph, ngraph_rs: &NetlistGraph) -> XC2Bitstream {

    let graphs = par_graphs.borrow();
    let dgraph = graphs.d;
    let ngraph = graphs.n;

    // FIXME: Don't hardcode
    let mut fb_bits = [XC2BitstreamFB::default(); 2];
    let mut iob_bits = [XC2MCSmallIOB::default(); 32];

    // Walk all device graph nodes
    for i in 0..dgraph.get_num_nodes() {
        let dgraph_node = dgraph.get_node_by_index(i);
        if dgraph_node.get_mate().is_none() {
            // Not being used by the netlist; skip this
            continue;
        }

        let dgraph_node_rs = dgraph_rs.nodes.get(*dgraph_node.get_associated_data());
        println!("{:?}", dgraph_node_rs);
        let ngraph_node_rs = ngraph_rs.nodes.get(*dgraph_node.get_mate().unwrap().get_associated_data());
        println!("{:?}", ngraph_node_rs);

        match dgraph_node_rs {
            &DeviceGraphNode::AndTerm{fb: fb_and, i: i_and} => {
                if let NetlistGraphNodeVariant::AndTerm{ref inputs_true, ref inputs_comp, ..} = ngraph_node_rs.variant {
                    for input in inputs_true {
                        let in_net = ngraph_rs.nets.get(*input);
                        let source_node_ngraph = ngraph_rs.nodes.get(in_net.source.unwrap().0);
                        let source_node_dgraph = dgraph_rs.nodes.get(
                            *ngraph.get_node_by_index(source_node_ngraph.par_idx.unwrap()).get_mate().unwrap()
                            .get_associated_data());

                        if let &DeviceGraphNode::ZIADummyBuf{fb: fb_zia, row: zia_row} = source_node_dgraph {
                            if fb_zia != fb_and {
                                panic!("mismatched FBs");
                            }

                            // We are connecting a particular ZIA row into this and term's true input
                            fb_bits[fb_and as usize].and_terms[i_and as usize].input[zia_row as usize] = true;
                        } else {
                            panic!("mismatched graph node types");
                        }
                    }
                    for input in inputs_comp {
                        let in_net = ngraph_rs.nets.get(*input);
                        let source_node_ngraph = ngraph_rs.nodes.get(in_net.source.unwrap().0);
                        let source_node_dgraph = dgraph_rs.nodes.get(
                            *ngraph.get_node_by_index(source_node_ngraph.par_idx.unwrap()).get_mate().unwrap()
                            .get_associated_data());

                        if let &DeviceGraphNode::ZIADummyBuf{fb: fb_zia, row: zia_row} = source_node_dgraph {
                            if fb_zia != fb_and {
                                panic!("mismatched FBs");
                            }
                            
                            // We are connecting a particular ZIA row into this and term's complement input
                            fb_bits[fb_and as usize].and_terms[i_and as usize].input_b[zia_row as usize] = true;
                        } else {
                            panic!("mismatched graph node types");
                        }
                    }
                } else {
                    panic!("mismatched graph node types");
                }
            },
            &DeviceGraphNode::OrTerm{fb: fb_or, i: i_or} => {
                if let NetlistGraphNodeVariant::OrTerm{ref inputs, ..} = ngraph_node_rs.variant {
                    for input in inputs {
                        let in_net = ngraph_rs.nets.get(*input);
                        let source_node_ngraph = ngraph_rs.nodes.get(in_net.source.unwrap().0);
                        let source_node_dgraph = dgraph_rs.nodes.get(
                            *ngraph.get_node_by_index(source_node_ngraph.par_idx.unwrap()).get_mate().unwrap()
                            .get_associated_data());

                        if let &DeviceGraphNode::AndTerm{fb: fb_and, i: i_and} = source_node_dgraph {
                            if fb_and != fb_or {
                                panic!("mismatched FBs");
                            }

                            // We are connecting a particular AND term into this OR term's input
                            fb_bits[fb_and as usize].or_terms[i_or as usize].input[i_and as usize] = true;
                        } else {
                            panic!("mismatched graph node types");
                        }
                    }
                } else {
                    panic!("mismatched graph node types");
                }
            },
            &DeviceGraphNode::Xor{fb: fb_xor, i: i_xor} => {
                if let NetlistGraphNodeVariant::Xor{orterm_input, andterm_input, invert_out, ..} = ngraph_node_rs.variant {
                    // Validate
                    if let Some(input) = orterm_input {
                        let in_net = ngraph_rs.nets.get(input);
                        let source_node_ngraph = ngraph_rs.nodes.get(in_net.source.unwrap().0);
                        let source_node_dgraph = dgraph_rs.nodes.get(
                            *ngraph.get_node_by_index(source_node_ngraph.par_idx.unwrap()).get_mate().unwrap()
                            .get_associated_data());

                        if let &DeviceGraphNode::OrTerm{fb: fb_or, i: i_or} = source_node_dgraph {
                            if fb_xor != fb_or {
                                panic!("mismatched FBs");
                            }
                            if i_xor != i_or {
                                panic!("mismatched FFs");
                            }
                        } else {
                            panic!("mismatched graph node types");
                        }
                    }
                    if let Some(input) = andterm_input {
                        let in_net = ngraph_rs.nets.get(input);
                        let source_node_ngraph = ngraph_rs.nodes.get(in_net.source.unwrap().0);
                        let source_node_dgraph = dgraph_rs.nodes.get(
                            *ngraph.get_node_by_index(source_node_ngraph.par_idx.unwrap()).get_mate().unwrap()
                            .get_associated_data());

                        if let &DeviceGraphNode::AndTerm{fb: fb_and, i: i_and} = source_node_dgraph {
                            if fb_xor != fb_and {
                                panic!("mismatched FBs");
                            }
                            if get_ptc(i_xor) != i_and {
                                panic!("mismatched FFs");
                            }
                        } else {
                            panic!("mismatched graph node types");
                        }
                    }

                    // Mode
                    let xormode = if andterm_input.is_none() && !invert_out {
                        XC2MCXorMode::ZERO
                    } else if andterm_input.is_none() && invert_out {
                        XC2MCXorMode::ONE
                    } else if andterm_input.is_some() && !invert_out {
                        XC2MCXorMode::PTC
                    } else {
                        XC2MCXorMode::PTCB
                    };
                    fb_bits[fb_xor as usize].mcs[i_xor as usize].xor_mode = xormode;
                } else {
                    panic!("mismatched graph node types");
                }
            },
            &DeviceGraphNode::IOBuf{i: i_iob} => {
                if let NetlistGraphNodeVariant::IOBuf{oe, input, ..} = ngraph_node_rs.variant {
                    if input.is_some() {
                        // pin is used as an OUTPUT
                        // TODO
                        iob_bits[i_iob as usize].obuf_uses_ff = false;
                        iob_bits[i_iob as usize].termination_enabled = false;
                        iob_bits[i_iob as usize].obuf_mode = XC2IOBOBufMode::PushPull;
                    }

                    if oe.is_some() {
                        panic!("not implemented");
                    }
                } else if let NetlistGraphNodeVariant::InBuf{..} = ngraph_node_rs.variant {
                    // TODO???
                    iob_bits[i_iob as usize].termination_enabled = false;
                } else {
                    panic!("mismatched graph node types");
                }
            },
            &DeviceGraphNode::ZIADummyBuf{fb: fb_zia, row: zia_row} => {
                if let NetlistGraphNodeVariant::ZIADummyBuf{input, ..} = ngraph_node_rs.variant {
                    let in_net = ngraph_rs.nets.get(input);
                    let source_node_ngraph = ngraph_rs.nodes.get(in_net.source.unwrap().0);
                    let source_node_dgraph = dgraph_rs.nodes.get(
                        *ngraph.get_node_by_index(source_node_ngraph.par_idx.unwrap()).get_mate().unwrap()
                        .get_associated_data());

                    if let &DeviceGraphNode::IOBuf{i: i_iob} = source_node_dgraph {
                        fb_bits[fb_zia as usize].zia_bits[zia_row as usize] = XC2ZIARowPiece {
                            selected: XC2ZIAInput::IBuf{ibuf: i_iob},
                        };
                    } else {
                        unimplemented!();
                    }
                } else {
                    panic!("mismatched graph node types");
                }
            },
            _ => unreachable!(),
        }
    }

    XC2Bitstream {
        speed_grade: XC2Speed::Speed6,
        package: XC2Package::VQ44,
        bits: XC2BitstreamBits::XC2C32A {
            fb: fb_bits,
            iobs: iob_bits,
            inpin: XC2ExtraIBuf::default(),
            global_nets: XC2GlobalNets::default(),
            legacy_ivoltage: false,
            legacy_ovoltage: false,
            ivoltage: [false, false],
            ovoltage: [false, false],
        }
    }
}
