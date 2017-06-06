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
    let mut fb_bits = [XC2BistreamFB::default(); 2];
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
                if let NetlistGraphNodeVariant::AndTerm{output, ..} = ngraph_node_rs.variant {
                    let out_net = ngraph_rs.nets.get(output);
                    for &sink_or_term in &out_net.sinks {
                        let sink_or_node_ngraph = ngraph_rs.nodes.get(sink_or_term.0);
                        let sink_or_node_dgraph = dgraph_rs.nodes.get(
                            *ngraph.get_node_by_index(sink_or_node_ngraph.par_idx.unwrap()).get_mate().unwrap()
                            .get_associated_data());

                        if let &DeviceGraphNode::OrTerm{fb: fb_or, i: i_or} = sink_or_node_dgraph {
                            if fb_and != fb_or {
                                panic!("mismatched FBs");
                            }

                            // Finally, we know that we are connecting this AND gate to an OR gate, and we know which
                            // one
                            fb_bits[fb_and as usize].or_terms[i_or as usize].input[i_and as usize] = true;
                        } else {
                            panic!("mismatched graph node types");
                        }
                    }
                } else {
                    panic!("mismatched graph node types");
                }
            },
            &DeviceGraphNode::OrTerm{fb: fb_or, i: i_or} => {
                // This is hard-wired and we don't actually have to do anything, but we validate just in case
                if let NetlistGraphNodeVariant::OrTerm{output, ..} = ngraph_node_rs.variant {
                    let out_net = ngraph_rs.nets.get(output);
                    if out_net.sinks.len() != 1 {
                        panic!("OR output incorrectly conected");
                    }
                    let sink_xor = out_net.sinks[0];
                    let sink_xor_node_ngraph = ngraph_rs.nodes.get(sink_xor.0);
                    let sink_xor_node_dgraph = dgraph_rs.nodes.get(
                        *ngraph.get_node_by_index(sink_xor_node_ngraph.par_idx.unwrap()).get_mate().unwrap()
                        .get_associated_data());

                    if let &DeviceGraphNode::Xor{fb: fb_xor, i: i_xor} = sink_xor_node_dgraph {
                        if fb_or != fb_xor {
                            panic!("mismatched FBs");
                        }
                        if i_or != i_xor {
                            panic!("mismatched FFs");
                        }
                    } else {
                        panic!("mismatched graph node types");
                    }
                } else {
                    panic!("mismatched graph node types");
                }
            },
            &DeviceGraphNode::Xor{fb: fb_xor, i: i_xor} => {
                if let NetlistGraphNodeVariant::Xor{output, invert_out, andterm_input, ..} = ngraph_node_rs.variant {
                    let out_net = ngraph_rs.nets.get(output);
                    if out_net.sinks.len() != 1 && out_net.sinks.len() != 2 {
                        panic!("XOR output incorrectly conected");
                    }
                    for &sink_node in &out_net.sinks {
                        let sink_node_ngraph = ngraph_rs.nodes.get(sink_node.0);
                        let sink_node_dgraph = dgraph_rs.nodes.get(
                            *ngraph.get_node_by_index(sink_node_ngraph.par_idx.unwrap()).get_mate().unwrap()
                            .get_associated_data());

                        if let &DeviceGraphNode::Reg{fb: fb_reg, i: i_reg} = sink_node_dgraph {
                            if fb_xor != fb_reg {
                                panic!("mismatched FBs");
                            }
                            if i_xor != i_reg {
                                panic!("mismatched FFs");
                            }

                            fb_bits[fb_xor as usize].ffs[i_xor as usize].ff_in_ibuf = false;
                        } else if let &DeviceGraphNode::IOBuf{i: i_iob} = sink_node_dgraph {
                            let (fb_iob, ff_iob) = iob_num_to_fb_ff_num_32(i_iob).unwrap();

                            if fb_xor != fb_iob {
                                panic!("mismatched FBs");
                            }
                            if i_xor != ff_iob {
                                panic!("mismatched FFs");
                            }

                            // TODO
                            iob_bits[i_iob as usize].obuf_uses_ff = false;
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
                    fb_bits[fb_xor as usize].ffs[i_xor as usize].xor_mode = xormode;
                } else {
                    panic!("mismatched graph node types");
                }
            },
            &DeviceGraphNode::IOBuf{i: i_iob} => {
                let (fb_iob, ff_iob) = iob_num_to_fb_ff_num_32(i_iob).unwrap();
                if let NetlistGraphNodeVariant::IOBuf{output, oe, input} = ngraph_node_rs.variant {
                    if input.is_some() {
                        // pin is used as an OUTPUT
                        iob_bits[i_iob as usize].termination_enabled = false;
                        iob_bits[i_iob as usize].obuf_mode = XC2MCOBufMode::PushPull;
                    }

                    if oe.is_some() {
                        panic!("not implemented");
                    }

                    if output.is_some() {
                        // pin is used as an input
                        let out_net = ngraph_rs.nets.get(output.unwrap());
                        if out_net.sinks.len() != 1 {
                            panic!("IOB output incorrectly conected");
                        }
                        let sink_node_net = out_net.sinks[0];
                        let sink_node_ngraph = ngraph_rs.nodes.get(sink_node_net.0);
                        let sink_node_dgraph = dgraph_rs.nodes.get(
                            *ngraph.get_node_by_index(sink_node_ngraph.par_idx.unwrap()).get_mate().unwrap()
                            .get_associated_data());

                        if let &DeviceGraphNode::Reg{fb: fb_reg, i: i_reg} = sink_node_dgraph {
                            if fb_iob != fb_reg {
                                panic!("mismatched FBs");
                            }
                            if i_iob != i_reg {
                                panic!("mismatched FFs");
                            }

                            fb_bits[fb_iob as usize].ffs[ff_iob as usize].ff_in_ibuf = true;
                        } else if let &DeviceGraphNode::AndTerm{fb: fb_and, i: i_and} = sink_node_dgraph {
                            // TODO

                            // FIXME FIXME FIXME FIXME THIS IS TOTALLY BOGUS
                            for zia_row in 0..40 {
                                let maybe_zia_data = encode_32_zia_choice(zia_row, XC2ZIAInput::IBuf{ibuf: i_iob});
                                if maybe_zia_data.is_some() {
                                    fb_bits[fb_and as usize].zia_bits[zia_row as usize] = XC2ZIARowPiece {
                                        selected: XC2ZIAInput::IBuf{ibuf: i_iob},
                                    };
                                    // XXX where the heck did invert go?
                                    fb_bits[fb_and as usize].and_terms[i_and as usize].input[zia_row as usize] = true;

                                    break;
                                }
                            }

                            iob_bits[i_iob as usize].zia_mode = XC2IOBZIAMode::PAD;
                        } else {
                            panic!("mismatched graph node types");
                        }
                    }
                }
            },
            _ => unreachable!(),
        }
    }

    XC2Bitstream {
        speed_grade: String::from("6"),
        package: String::from("VQ44"),
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
