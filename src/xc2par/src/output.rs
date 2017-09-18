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

pub fn produce_bitstream(device_type: XC2Device,
    par_graphs: &PARGraphPair<ObjPoolIndex<DeviceGraphNode>, ObjPoolIndex<NetlistGraphNode>>,
    dgraph_rs: &DeviceGraph, ngraph_rs: &NetlistGraph) -> XC2Bitstream {

    let graphs = par_graphs.borrow();
    let ngraph = graphs.n;

    // FIXME: Don't hardcode
    let mut fb_bits = [XC2BitstreamFB::default(); 2];
    let mut iob_bits = [XC2MCSmallIOB::default(); 32];

    let mut global_nets = XC2GlobalNets::default();

    let mut extra_inpin = XC2ExtraIBuf::default();

    // Walk all netlist graph nodes
    for i in 0..ngraph.get_num_nodes() {
        let ngraph_node = ngraph.get_node_by_index(i);

        let ngraph_node_rs = ngraph_rs.nodes.get(*ngraph_node.get_associated_data());
        println!("{:?}", ngraph_node_rs);
        let dgraph_node_rs = dgraph_rs.nodes.get(*ngraph_node.get_mate().unwrap().get_associated_data());
        println!("{:?}", dgraph_node_rs);


        let get_source_ngraph = |net_idx| {
            let net_obj = ngraph_rs.nets.get(net_idx);
            ngraph_rs.nodes.get(net_obj.source.unwrap().0)
        };

        let get_source_dgraph = |net_idx| {
            let net_obj = ngraph_rs.nets.get(net_idx);
            let source_node_ngraph = ngraph_rs.nodes.get(net_obj.source.unwrap().0);
            dgraph_rs.nodes.get(
                *ngraph.get_node_by_index(source_node_ngraph.par_idx.unwrap()).get_mate().unwrap()
                .get_associated_data())
        };

        match dgraph_node_rs {
            &DeviceGraphNode::AndTerm{fb: fb_and, i: i_and} => {
                if let NetlistGraphNodeVariant::AndTerm{ref inputs_true, ref inputs_comp, ..} = ngraph_node_rs.variant {
                    for &input in inputs_true {
                        if let &DeviceGraphNode::ZIADummyBuf{fb: fb_zia, row: zia_row} = get_source_dgraph(input) {
                            if fb_zia != fb_and {
                                panic!("mismatched FBs");
                            }

                            // We are connecting a particular ZIA row into this and term's true input
                            fb_bits[fb_and as usize].and_terms[i_and as usize].input[zia_row as usize] = true;
                        } else {
                            panic!("mismatched graph node types");
                        }
                    }
                    for &input in inputs_comp {
                        if let &DeviceGraphNode::ZIADummyBuf{fb: fb_zia, row: zia_row} = get_source_dgraph(input) {
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
                    for &input in inputs {
                        if let &DeviceGraphNode::AndTerm{fb: fb_and, i: i_and} = get_source_dgraph(input) {
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
                        if let &DeviceGraphNode::OrTerm{fb: fb_or, i: i_or} = get_source_dgraph(input) {
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
                        if let &DeviceGraphNode::AndTerm{fb: fb_and, i: i_and} = get_source_dgraph(input) {
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
                if let NetlistGraphNodeVariant::IOBuf{oe, input, output,
                        schmitt_trigger, termination_enabled, slew_is_fast, ..} = ngraph_node_rs.variant {

                    iob_bits[i_iob as usize].schmitt_trigger = schmitt_trigger;
                    iob_bits[i_iob as usize].termination_enabled = termination_enabled;
                    iob_bits[i_iob as usize].slew_is_fast = slew_is_fast;

                    if output.is_some() {
                        // If the input side of the IOB is being used, we need to set the ZIA mode to use the IO pad.
                        // The structure of the device graph should prevent any conflicts.
                        iob_bits[i_iob as usize].zia_mode = XC2IOBZIAMode::PAD;
                    }

                    if input.is_some() {
                        // The output side of the IOB is being used.

                        // TODO: OE
                        iob_bits[i_iob as usize].obuf_mode = XC2IOBOBufMode::PushPull;

                        // What is feeding the output?
                        let input_src = get_source_ngraph(input.unwrap());
                        if let NetlistGraphNodeVariant::Xor{..} = input_src.variant {
                            iob_bits[i_iob as usize].obuf_uses_ff = false;
                        } else if let NetlistGraphNodeVariant::Reg{..} = input_src.variant {
                            iob_bits[i_iob as usize].obuf_uses_ff = true;
                        } else {
                            panic!("mismatched graph node types");
                        }
                    }

                    if oe.is_some() {
                        panic!("not implemented");
                    }
                } else if let NetlistGraphNodeVariant::InBuf{schmitt_trigger, termination_enabled, ..} = ngraph_node_rs.variant {
                    iob_bits[i_iob as usize].schmitt_trigger = schmitt_trigger;
                    iob_bits[i_iob as usize].termination_enabled = termination_enabled;

                    // We need to set the ZIA mode to use the IO pad.
                    iob_bits[i_iob as usize].zia_mode = XC2IOBZIAMode::PAD;
                } else {
                    panic!("mismatched graph node types");
                }
            },
            &DeviceGraphNode::InBuf => {
                if let NetlistGraphNodeVariant::InBuf{schmitt_trigger, termination_enabled, ..} = ngraph_node_rs.variant {
                    extra_inpin.schmitt_trigger = schmitt_trigger;
                    extra_inpin.termination_enabled = termination_enabled;
                } else {
                    panic!("mismatched graph node types");
                }
            },
            &DeviceGraphNode::ZIADummyBuf{fb: fb_zia, row: zia_row} => {
                if let NetlistGraphNodeVariant::ZIADummyBuf{input, ..} = ngraph_node_rs.variant {
                    let source_node_dgraph = get_source_dgraph(input);

                    if let &DeviceGraphNode::IOBuf{i: i_iob} = source_node_dgraph {
                        fb_bits[fb_zia as usize].zia_bits[zia_row as usize] = XC2ZIARowPiece {
                            selected: XC2ZIAInput::IBuf{ibuf: i_iob},
                        };
                    } else if let &DeviceGraphNode::InBuf = source_node_dgraph {
                        fb_bits[fb_zia as usize].zia_bits[zia_row as usize] = XC2ZIARowPiece {
                            selected: XC2ZIAInput::DedicatedInput,
                        };
                    } else {
                        unimplemented!();
                    }
                } else {
                    panic!("mismatched graph node types");
                }
            },
            &DeviceGraphNode::BufgClk{i} => {
                if let NetlistGraphNodeVariant::BufgClk{..} = ngraph_node_rs.variant {
                    global_nets.gck_enable[i as usize] = true;
                } else {
                    panic!("mismatched graph node types");
                }
            },
            &DeviceGraphNode::BufgGTS{i} => {
                if let NetlistGraphNodeVariant::BufgGTS{invert, ..} = ngraph_node_rs.variant {
                    global_nets.gts_enable[i as usize] = true;
                    global_nets.gts_invert[i as usize] = invert;
                } else {
                    panic!("mismatched graph node types");
                }
            },
            &DeviceGraphNode::BufgGSR => {
                if let NetlistGraphNodeVariant::BufgGSR{invert, ..} = ngraph_node_rs.variant {
                    global_nets.gsr_enable = true;
                    global_nets.gsr_invert = invert;
                } else {
                    panic!("mismatched graph node types");
                }
            },
            &DeviceGraphNode::Reg{fb: fb_reg, i: i_reg} => {
                if let NetlistGraphNodeVariant::Reg{mode, clkinv, clkddr, init_state, set_input, reset_input,
                    dt_input, clk_input, ..} = ngraph_node_rs.variant {
                    // TODO: Validate??

                    fb_bits[fb_reg as usize].mcs[i_reg as usize].reg_mode = mode;
                    fb_bits[fb_reg as usize].mcs[i_reg as usize].clk_invert_pol = clkinv;
                    fb_bits[fb_reg as usize].mcs[i_reg as usize].is_ddr = clkddr;
                    fb_bits[fb_reg as usize].mcs[i_reg as usize].init_state = init_state;

                    if set_input.is_some() && set_input.unwrap() != ngraph_rs.vss_net {
                        unimplemented!();
                    }

                    if reset_input.is_some() && reset_input.unwrap() != ngraph_rs.vss_net {
                        unimplemented!();
                    }

                    // What is feeding the clock?
                    let clk_input_dg = get_source_dgraph(clk_input);
                    let clk_src_mux = if let &DeviceGraphNode::BufgClk{i: i_clk} = clk_input_dg {
                        match i_clk {
                            0 => XC2MCRegClkSrc::GCK0,
                            1 => XC2MCRegClkSrc::GCK1,
                            2 => XC2MCRegClkSrc::GCK2,
                            _ => panic!("illegal state"),
                        }
                    } else if let &DeviceGraphNode::AndTerm{fb: fb_and, i: i_and} = clk_input_dg {
                        if fb_and != fb_reg {
                            panic!("mismatched FBs");
                        }
                        if i_and == CTC {
                            XC2MCRegClkSrc::CTC
                        } else if i_and == get_ptc(i_reg) {
                            XC2MCRegClkSrc::PTC
                        } else {
                            panic!("mismatched FFs");
                        }
                    } else {
                        panic!("mismatched graph node types");
                    };
                    fb_bits[fb_reg as usize].mcs[i_reg as usize].clk_src = clk_src_mux;

                    // What is feeding the input?
                    let dt_input_dg = get_source_dgraph(dt_input);
                    let dt_mux = if let &DeviceGraphNode::IOBuf{..} = dt_input_dg {
                        true
                    } else if let &DeviceGraphNode::Xor{..} = dt_input_dg {
                        false
                    } else {
                        panic!("mismatched graph node types");
                    };
                    fb_bits[fb_reg as usize].mcs[i_reg as usize].ff_in_ibuf = dt_mux;

                    // TODO: Feedback?
                } else {
                    panic!("mismatched graph node types");
                }
            }
        }
    }

    XC2Bitstream {
        speed_grade: XC2Speed::Speed6,
        package: XC2Package::VQ44,
        bits: XC2BitstreamBits::XC2C32A {
            fb: fb_bits,
            iobs: iob_bits,
            inpin: extra_inpin,
            global_nets,
            legacy_ivoltage: false,
            legacy_ovoltage: false,
            ivoltage: [false, false],
            ovoltage: [false, false],
        }
    }
}
