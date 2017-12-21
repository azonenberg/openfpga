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
    placements: &[(PARFBAssignment, PARZIAAssignment, PARPTermZIARows)]) -> XC2Bitstream {

    // FIXME: Don't hardcode
    let mut fb_bits = [XC2BitstreamFB::default(); 2];
    let mut iob_bits = [XC2MCSmallIOB::default(); 32];

    let mut global_nets = XC2GlobalNets::default();

    let mut extra_inpin = XC2ExtraIBuf::default();

    // ZIA settings
    for fb_i in 0..placements.len() {
        for zia_i in 0..INPUTS_PER_ANDTERM {
            fb_bits[fb_i].zia_bits[zia_i] = XC2ZIARowPiece{selected: placements[fb_i].1[zia_i]};
        }
    }

    // AND terms
    for fb_i in 0..placements.len() {
        for andterm_i in 0..ANDTERMS_PER_FB {
            let andterm_rows = &placements[fb_i].2[andterm_i];

            for &x in &andterm_rows.0 {
                fb_bits[fb_i].and_terms[andterm_i].input[x as usize] = true;
            }
            for &x in &andterm_rows.1 {
                fb_bits[fb_i].and_terms[andterm_i].input_b[x as usize] = true;
            }
        }
    }

    // Input pins
    for fb_i in 0..placements.len() {
        for mc_i in 0..MCS_PER_FB {
            if let PARMCAssignment::MC(mc_idx) = placements[fb_i].0[mc_i].1 {
                // FIXME: This code is duplicated
                let mc = g.mcs.get(mc_idx);

                assert!(mc.reg_bits.is_none() && mc.xor_bits.is_none());
                if let Some(ref io_bits) = mc.io_bits {
                    assert!(io_bits.input.is_none() && io_bits.oe.is_none());

                    if true && fb_i == 2 && mc_i == 0 {
                        // Special input-only pin
                        extra_inpin.schmitt_trigger = io_bits.schmitt_trigger;
                        extra_inpin.termination_enabled = io_bits.termination_enabled;
                    } else {
                        let i_iob = fb_mc_num_to_iob_num(device_type, fb_i as u32, mc_i as u32).unwrap();

                        iob_bits[i_iob as usize].schmitt_trigger = io_bits.schmitt_trigger;
                        iob_bits[i_iob as usize].termination_enabled = io_bits.termination_enabled;
                        if mc.io_feedback_used {
                            // We need to set the ZIA mode to use the IO pad.
                            iob_bits[i_iob as usize].zia_mode = XC2IOBZIAMode::PAD;
                        }
                    }
                }
            }
        }
    }

    // Output and buried pins
    for fb_i in 0..placements.len() {
        for mc_i in 0..MCS_PER_FB {
            if let PARMCAssignment::MC(mc_idx) = placements[fb_i].0[mc_i].0 {
                let mc = g.mcs.get(mc_idx);

                if let Some(ref io_bits) = mc.io_bits {
                    let i_iob = fb_mc_num_to_iob_num(device_type, fb_i as u32, mc_i as u32).unwrap();

                    iob_bits[i_iob as usize].schmitt_trigger = io_bits.schmitt_trigger;
                    iob_bits[i_iob as usize].termination_enabled = io_bits.termination_enabled;
                    iob_bits[i_iob as usize].slew_is_fast = io_bits.slew_is_fast;
                    if mc.io_feedback_used {
                        // We need to set the ZIA mode to use the IO pad.
                        iob_bits[i_iob as usize].zia_mode = XC2IOBZIAMode::PAD;
                    } else if mc.xor_feedback_used && mc.reg_feedback_used {
                        // If both of these are used, then the register _has_ to come from here. Otherwise use the
                        // "normal" one.
                        iob_bits[i_iob as usize].zia_mode = XC2IOBZIAMode::REG;
                    }
                    if let Some(input) = io_bits.input {
                        iob_bits[i_iob as usize].obuf_uses_ff = input == InputGraphIOInputType::Reg;
                        iob_bits[i_iob as usize].obuf_mode = match io_bits.oe {
                            None => XC2IOBOBufMode::PushPull,
                            Some(InputGraphIOOEType::OpenDrain) => XC2IOBOBufMode::OpenDrain,
                            Some(InputGraphIOOEType::PTerm(pterm)) => {
                                let pterm = g.pterms.get(pterm);
                                let pt_loc = pterm.loc.unwrap();
                                assert!(pt_loc.fb == mc.loc.unwrap().fb);

                                if pt_loc.i == CTE {
                                    XC2IOBOBufMode::TriStateCTE
                                } else if pt_loc.i == get_ptb(mc_i as u32) {
                                    XC2IOBOBufMode::TriStatePTB
                                } else {
                                    panic!("Internal error - not CTE or PTB");
                                }
                            },
                            Some(InputGraphIOOEType::GTS(gts)) => {
                                let gts = g.bufg_gts.get(gts);
                                let which = gts.loc.unwrap().i;
                                match which {
                                    0 => XC2IOBOBufMode::TriStateGTS0,
                                    1 => XC2IOBOBufMode::TriStateGTS1,
                                    2 => XC2IOBOBufMode::TriStateGTS2,
                                    3 => XC2IOBOBufMode::TriStateGTS3,
                                    _ => panic!("Internal error - bad GTS"),
                                }
                            },
                        }
                    }
                }
            }
        }
    }

    // OR/XOR gates
    for fb_i in 0..placements.len() {
        for mc_i in 0..MCS_PER_FB {
            if let PARMCAssignment::MC(mc_idx) = placements[fb_i].0[mc_i].0 {
                let mc = g.mcs.get(mc_idx);

                if let Some(ref xor_bits) = mc.xor_bits {
                    // XOR
                    fb_bits[fb_i].mcs[mc_i].xor_mode = match (xor_bits.invert_out, xor_bits.andterm_input) {
                        (false, None) => XC2MCXorMode::ZERO,
                        (true, None) => XC2MCXorMode::ONE,
                        (false, Some(_)) => XC2MCXorMode::PTC,
                        (true, Some(_)) => XC2MCXorMode::PTCB,
                    };

                    // OR
                    for &and_to_or_idx in &xor_bits.orterm_inputs {
                        let pterm = g.pterms.get(and_to_or_idx);
                        let pt_loc = pterm.loc.unwrap();
                        assert!(pt_loc.fb == mc.loc.unwrap().fb);
                        fb_bits[fb_i].or_terms[mc_i].input[pt_loc.i as usize] = true;
                    }

                    // Feedback
                    if mc.xor_feedback_used {
                        fb_bits[fb_i].mcs[mc_i].fb_mode = XC2MCFeedbackMode::COMB;
                    } else if mc.reg_feedback_used {
                        fb_bits[fb_i].mcs[mc_i].fb_mode = XC2MCFeedbackMode::REG;
                    }
                }
            }
        }
    }

    // Register bits
    for fb_i in 0..placements.len() {
        for mc_i in 0..MCS_PER_FB {
            if let PARMCAssignment::MC(mc_idx) = placements[fb_i].0[mc_i].0 {
                let mc = g.mcs.get(mc_idx);

                if let Some(ref reg_bits) = mc.reg_bits {
                    fb_bits[fb_i].mcs[mc_i].reg_mode = reg_bits.mode;
                    fb_bits[fb_i].mcs[mc_i].clk_invert_pol = reg_bits.clkinv;
                    fb_bits[fb_i].mcs[mc_i].is_ddr = reg_bits.clkddr;
                    fb_bits[fb_i].mcs[mc_i].init_state = reg_bits.init_state;

                    // Set
                    fb_bits[fb_i].mcs[mc_i].s_src = match reg_bits.set_input {
                        None => XC2MCRegSetSrc::Disabled,
                        Some(InputGraphRegRSType::GSR(_)) => XC2MCRegSetSrc::GSR,
                        Some(InputGraphRegRSType::PTerm(pterm)) => {
                            let pterm = g.pterms.get(pterm);
                            let pt_loc = pterm.loc.unwrap();
                            assert!(pt_loc.fb == mc.loc.unwrap().fb);

                            if pt_loc.i == CTS {
                                XC2MCRegSetSrc::CTS
                            } else if pt_loc.i == get_pta(mc_i as u32) {
                                XC2MCRegSetSrc::PTA
                            } else {
                                panic!("Internal error - not CTS or PTA");
                            }
                        },
                    };

                    // Reset
                    fb_bits[fb_i].mcs[mc_i].r_src = match reg_bits.reset_input {
                        None => XC2MCRegResetSrc::Disabled,
                        Some(InputGraphRegRSType::GSR(_)) => XC2MCRegResetSrc::GSR,
                        Some(InputGraphRegRSType::PTerm(pterm)) => {
                            let pterm = g.pterms.get(pterm);
                            let pt_loc = pterm.loc.unwrap();
                            assert!(pt_loc.fb == mc.loc.unwrap().fb);

                            if pt_loc.i == CTR {
                                XC2MCRegResetSrc::CTR
                            } else if pt_loc.i == get_pta(mc_i as u32) {
                                XC2MCRegResetSrc::PTA
                            } else {
                                panic!("Internal error - not CTR or PTA");
                            }
                        },
                    };

                    // D/T input
                    fb_bits[fb_i].mcs[mc_i].ff_in_ibuf = reg_bits.dt_input == InputGraphRegInputType::Pin;

                    // Clock input
                    fb_bits[fb_i].mcs[mc_i].clk_src = match reg_bits.clk_input {
                        InputGraphRegClockType::PTerm(pterm) => {
                            let pterm = g.pterms.get(pterm);
                            let pt_loc = pterm.loc.unwrap();
                            assert!(pt_loc.fb == mc.loc.unwrap().fb);

                            if pt_loc.i == CTC {
                                XC2MCRegClkSrc::CTC
                            } else if pt_loc.i == get_ptc(mc_i as u32) {
                                XC2MCRegClkSrc::PTC
                            } else {
                                panic!("Internal error - not CTC or PTC");
                            }
                        },
                        InputGraphRegClockType::GCK(gck) => {
                            let gck = g.bufg_clks.get(gck);
                            let which = gck.loc.unwrap().i;
                            match which {
                                0 => XC2MCRegClkSrc::GCK0,
                                1 => XC2MCRegClkSrc::GCK1,
                                2 => XC2MCRegClkSrc::GCK2,
                                _ => panic!("Internal error - bad GCK"),
                            }
                        },
                    }
                }
            }
        }
    }

    // Global buffers
    for gck in g.bufg_clks.iter() {
        let which = gck.loc.unwrap().i;
        global_nets.gck_enable[which as usize] = true;
    }
    for gts in g.bufg_gts.iter() {
        let which = gts.loc.unwrap().i;
        global_nets.gts_enable[which as usize] = true;
        global_nets.gts_invert[which as usize] = gts.invert;
    }
    for gsr in g.bufg_gsr.iter() {
        global_nets.gsr_enable = true;
        global_nets.gsr_invert = gsr.invert;
    }

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
