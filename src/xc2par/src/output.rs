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

extern crate xc2bit;
use self::xc2bit::*;

use *;
use objpool::*;

// XXX DELETE THIS
#[derive(Debug)]
pub enum NetlistMacrocell {
    PinOutput {
        // Index of the IOBUFE
        i: ObjPoolIndex<IntermediateGraphNode>,
    },
    PinInputUnreg {
        // Index of the IBUF
        i: ObjPoolIndex<IntermediateGraphNode>,
    },
    PinInputReg {
        // Index of the IBUF
        i: ObjPoolIndex<IntermediateGraphNode>,
    },
    BuriedComb {
        // Index of the XOR
        i: ObjPoolIndex<IntermediateGraphNode>,
    },
    BuriedReg {
        // Index of the register
        i: ObjPoolIndex<IntermediateGraphNode>,
        has_comb_fb: bool,
    }
}

pub fn produce_bitstream(device_type: XC2Device, g: &InputGraph,
    placements: &[(PARFBAssignment, [Option<ObjPoolIndex<InputGraphPTerm>>; ANDTERMS_PER_FB],
        [XC2ZIAInput; INPUTS_PER_ANDTERM], [(Vec<u32>, Vec<u32>); ANDTERMS_PER_FB])]) -> XC2Bitstream {

    // FIXME: Don't hardcode
    let mut fb_bits = [XC2BitstreamFB::default(); 2];
    let mut iob_bits = [XC2MCSmallIOB::default(); 32];

    let mut global_nets = XC2GlobalNets::default();

    let mut extra_inpin = XC2ExtraIBuf::default();

    // ZIA settings
    for fb_i in 0..placements.len() {
        for zia_i in 0..INPUTS_PER_ANDTERM {
            fb_bits[fb_i].zia_bits[zia_i] = XC2ZIARowPiece{selected: placements[fb_i].2[zia_i]};
        }
    }

    // AND terms
    for fb_i in 0..placements.len() {
        for andterm_i in 0..ANDTERMS_PER_FB {
            let andterm_rows = &placements[fb_i].3[andterm_i];

            for &x in &andterm_rows.0 {
                fb_bits[fb_i].and_terms[andterm_i].input[x as usize] = true;
            }
            for &x in &andterm_rows.1 {
                fb_bits[fb_i].and_terms[andterm_i].input_b[x as usize] = true;
            }
        }
    }

    // // Input pins
    // for fb_i in 0..placements.len() {
    //     for mc_i in 0..MCS_PER_FB {
    //         let i_iob = fb_mc_num_to_iob_num(device_type, fb_i as u32, mc_i as u32).unwrap();
    //         let gathered_mc_idx = placements[fb_i].0[mc_i].1;
    //         if gathered_mc_idx >= 0 {
    //             match mcs[gathered_mc_idx as usize] {
    //                 NetlistMacrocell::PinInputUnreg{i} => {
    //                     let ibuf_node = g.nodes.get(i);
    //                     if let IntermediateGraphNodeVariant::InBuf{schmitt_trigger, termination_enabled, ..} = ibuf_node.variant {
    //                         iob_bits[i_iob as usize].schmitt_trigger = schmitt_trigger;
    //                         iob_bits[i_iob as usize].termination_enabled = termination_enabled;

    //                         // We need to set the ZIA mode to use the IO pad.
    //                         iob_bits[i_iob as usize].zia_mode = XC2IOBZIAMode::PAD;
    //                     } else {
    //                         panic!("mismatched node type");
    //                     }
    //                 },
    //                 NetlistMacrocell::PinInputReg{i} => {
    //                     let ibuf_node = g.nodes.get(i);
    //                     if let IntermediateGraphNodeVariant::InBuf{schmitt_trigger, termination_enabled, output, ..} = ibuf_node.variant {
    //                         iob_bits[i_iob as usize].schmitt_trigger = schmitt_trigger;
    //                         iob_bits[i_iob as usize].termination_enabled = termination_enabled;

    //                         // We need to set the ZIA mode to use the IO pad.
    //                         iob_bits[i_iob as usize].zia_mode = XC2IOBZIAMode::PAD;

    //                         let reg_node = g.nodes.get(g.nets.get(output).source.unwrap().0);
    //                         if let IntermediateGraphNodeVariant::Reg{mode, clkinv, clkddr, init_state, set_input, reset_input,
    //                             dt_input, clk_input, ..} = reg_node.variant {

    //                             // TODO

    //                         } else {
    //                             panic!("mismatched node type");
    //                         }
    //                     } else {
    //                         panic!("mismatched node type");
    //                     }
    //                 },
    //                 _ => unreachable!(),
    //             }
    //         }
    //     }
    // }

    // // Output and buried pins
    // for fb_i in 0..placements.len() {
    //     for mc_i in 0..MCS_PER_FB {
    //         let gathered_mc_idx = placements[fb_i].0[mc_i].0;
    //         if gathered_mc_idx >= 0 {
    //             match mcs[gathered_mc_idx as usize] {
    //                 NetlistMacrocell::PinOutput{i} => {
    //                     let iobufe = g.nodes.get(i);
    //                     let i_iob = fb_mc_num_to_iob_num(device_type, fb_i as u32, mc_i as u32).unwrap();
    //                     if let IntermediateGraphNodeVariant::IOBuf{oe, input, output,
    //                             schmitt_trigger, termination_enabled, slew_is_fast, ..} = iobufe.variant {

    //                         iob_bits[i_iob as usize].schmitt_trigger = schmitt_trigger;
    //                         iob_bits[i_iob as usize].termination_enabled = termination_enabled;
    //                         iob_bits[i_iob as usize].slew_is_fast = slew_is_fast;

    //                         if output.is_some() {
    //                             // If the input side of the IOB is being used, we need to set the ZIA mode to use the IO pad.
    //                             // The structure of the device graph should prevent any conflicts.
    //                             iob_bits[i_iob as usize].zia_mode = XC2IOBZIAMode::PAD;
    //                         }

    //                         if input.is_some() {
    //                             // The output side of the IOB is being used.

    //                             // For now, assume it's push-pull and override it later.
    //                             iob_bits[i_iob as usize].obuf_mode = XC2IOBOBufMode::PushPull;

    //                             // What is feeding the output?
    //                             if input.unwrap() != g.vss_net {
    //                                 let input_src = g.nodes.get(g.nets.get(input.unwrap()).source.unwrap().0);
    //                                 if let IntermediateGraphNodeVariant::Xor{..} = input_src.variant {
    //                                     iob_bits[i_iob as usize].obuf_uses_ff = false;
    //                                 } else if let IntermediateGraphNodeVariant::Reg{..} = input_src.variant {
    //                                     iob_bits[i_iob as usize].obuf_uses_ff = true;
    //                                 } else {
    //                                     panic!("mismatched graph node types");
    //                                 }
    //                             }
    //                         }

    //                         if oe.is_some() {
    //                             unimplemented!();
    //                         }
    //                     } else {
    //                         panic!("mismatched graph node types");
    //                     }
    //                 },
    //                 NetlistMacrocell::BuriedComb{i} => {
    //                     unimplemented!();
    //                 },
    //                 NetlistMacrocell::BuriedReg{i, ..} => {
    //                     unimplemented!();
    //                 }
    //                 _ => unreachable!(),
    //             }
    //         }
    //     }
    // }

    // XXX TODO other bits

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
