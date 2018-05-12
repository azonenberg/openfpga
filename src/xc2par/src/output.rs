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

use xc2bit::*;

use *;
use objpool::*;

pub fn produce_bitstream(device_type: XC2DeviceSpeedPackage, g: &InputGraph, go: &OutputGraph) -> XC2Bitstream {
    let mut result = XC2Bitstream::blank_bitstream(device_type);

    {
        let fb_bits = result.bits.get_fb_mut();

        // ZIA settings
        for fb_i in 0..go.zia.len() {
            for zia_i in 0..INPUTS_PER_ANDTERM {
                *fb_bits[fb_i].get_mut_zia(zia_i) = go.zia[fb_i].x[zia_i];
            }
        }

        // AND terms
        for andterm_idx in g.pterms.iter_idx() {
            let andterm_go = go.pterms.get(ObjPoolIndex::from(andterm_idx));
            let fb_i = andterm_go.loc.unwrap().fb as usize;
            let andterm_i = andterm_go.loc.unwrap().i as usize;

            for &x in &andterm_go.inputs_true_zia {
                fb_bits[fb_i].get_mut_andterm(andterm_i).set(x as usize, true);
            }
            for &x in &andterm_go.inputs_comp_zia {
                fb_bits[fb_i].get_mut_andterm(andterm_i).set_b(x as usize, true);
            }
        }
    }

    // IO bits
    for mc_idx in g.mcs.iter_idx() {
        let mc = g.mcs.get(mc_idx);
        let mc_go = go.mcs.get(ObjPoolIndex::from(mc_idx));
        let fb_i = mc_go.loc.unwrap().fb;
        let mc_i = mc_go.loc.unwrap().i;

        if let Some(ref io_bits) = mc.io_bits {
            if (device_type.dev == XC2Device::XC2C32 || device_type.dev == XC2Device::XC2C32A) && fb_i == 2 && mc_i == 0 {
                // Special input-only pin
                match result.bits {
                    XC2BitstreamBits::XC2C32{inpin: ref mut extra_inpin, ..} |
                    XC2BitstreamBits::XC2C32A{inpin: ref mut extra_inpin, ..} => {
                        extra_inpin.schmitt_trigger = io_bits.schmitt_trigger;
                        extra_inpin.termination_enabled = io_bits.termination_enabled;
                    },
                    _ => unreachable!(),
                }
            } else {
                let i_iob = fb_mc_num_to_iob_num(device_type.dev, fb_i as u32, mc_i as u32).unwrap();

                macro_rules! output_iob_common {
                    ($iob_bit:expr) => {{
                        $iob_bit.termination_enabled = io_bits.termination_enabled;
                        $iob_bit.slew_is_fast = io_bits.slew_is_fast;
                        if mc.io_feedback_used {
                            // We need to set the ZIA mode to use the IO pad.
                            $iob_bit.zia_mode = XC2IOBZIAMode::PAD;
                        } else if mc.xor_feedback_used && mc.reg_feedback_used {
                            // If both of these are used, then the register _has_ to come from here. Otherwise use the
                            // "normal" one.
                            $iob_bit.zia_mode = XC2IOBZIAMode::REG;
                        }
                        if let Some(input) = io_bits.input {
                            $iob_bit.obuf_uses_ff = input == InputGraphIOInputType::Reg;
                            $iob_bit.obuf_mode = match io_bits.oe {
                                None => XC2IOBOBufMode::PushPull,
                                Some(InputGraphIOOEType::OpenDrain) => XC2IOBOBufMode::OpenDrain,
                                Some(InputGraphIOOEType::PTerm(pterm)) => {
                                    let pterm = go.pterms.get(ObjPoolIndex::from(pterm));
                                    let pt_loc = pterm.loc.unwrap();
                                    assert!(pt_loc.fb == fb_i);

                                    if pt_loc.i == CTE {
                                        XC2IOBOBufMode::TriStateCTE
                                    } else if pt_loc.i == get_ptb(mc_i as u32) {
                                        XC2IOBOBufMode::TriStatePTB
                                    } else {
                                        panic!("Internal error - not CTE or PTB");
                                    }
                                },
                                Some(InputGraphIOOEType::GTS(gts)) => {
                                    let gts = go.bufg_gts.get(ObjPoolIndex::from(gts));
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
                    }}
                }

                if device_type.dev.is_small_iob() {
                    let iob_bit = result.bits.get_mut_small_iob(i_iob as usize).unwrap();

                    iob_bit.schmitt_trigger = io_bits.schmitt_trigger;
                    output_iob_common!(iob_bit);
                } else {
                    let iob_bit = result.bits.get_mut_large_iob(i_iob as usize).unwrap();

                    // TODO: Pins that are/use Vref
                    iob_bit.ibuf_mode = if io_bits.schmitt_trigger {
                        XC2IOBIbufMode::NoVrefSt
                    } else {
                        XC2IOBIbufMode::NoVrefNoSt
                    };
                    iob_bit.uses_data_gate = io_bits.uses_data_gate;
                    output_iob_common!(iob_bit);
                }
            }
        }
    }

    {
        let fb_bits = result.bits.get_fb_mut();

        // OR/XOR gates
        for mc_idx in g.mcs.iter_idx() {
            let mc = g.mcs.get(mc_idx);
            let mc_go = go.mcs.get(ObjPoolIndex::from(mc_idx));
            let fb_i = mc_go.loc.unwrap().fb as usize;
            let mc_i = mc_go.loc.unwrap().i as usize;

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
                    let pterm = go.pterms.get(ObjPoolIndex::from(and_to_or_idx));
                    let pt_loc = pterm.loc.unwrap();
                    assert!(pt_loc.fb as usize == fb_i);
                    fb_bits[fb_i].or_terms[mc_i].set(pt_loc.i as usize, true);
                }

                // Feedback
                if mc.xor_feedback_used {
                    fb_bits[fb_i].mcs[mc_i].fb_mode = XC2MCFeedbackMode::COMB;
                } else if mc.reg_feedback_used {
                    fb_bits[fb_i].mcs[mc_i].fb_mode = XC2MCFeedbackMode::REG;
                }
            }
        }

        // Register bits
        for mc_idx in g.mcs.iter_idx() {
            let mc = g.mcs.get(mc_idx);
            let mc_go = go.mcs.get(ObjPoolIndex::from(mc_idx));
            let fb_i = mc_go.loc.unwrap().fb as usize;
            let mc_i = mc_go.loc.unwrap().i as usize;

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
                        let pterm = go.pterms.get(ObjPoolIndex::from(pterm));
                        let pt_loc = pterm.loc.unwrap();
                        assert!(pt_loc.fb as usize == fb_i);

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
                        let pterm = go.pterms.get(ObjPoolIndex::from(pterm));
                        let pt_loc = pterm.loc.unwrap();
                        assert!(pt_loc.fb as usize == fb_i);

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
                        let pterm = go.pterms.get(ObjPoolIndex::from(pterm));
                        let pt_loc = pterm.loc.unwrap();
                        assert!(pt_loc.fb as usize == fb_i);

                        if pt_loc.i == CTC {
                            XC2MCRegClkSrc::CTC
                        } else if pt_loc.i == get_ptc(mc_i as u32) {
                            XC2MCRegClkSrc::PTC
                        } else {
                            panic!("Internal error - not CTC or PTC");
                        }
                    },
                    InputGraphRegClockType::GCK(gck) => {
                        let gck = go.bufg_clks.get(ObjPoolIndex::from(gck));
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

    {
        let global_nets = result.bits.get_global_nets_mut();

        // Global buffers
        for gck in go.bufg_clks.iter() {
            let which = gck.loc.unwrap().i;
            global_nets.gck_enable[which as usize] = true;
        }
        for gts_idx in g.bufg_gts.iter_idx() {
            let gts = g.bufg_gts.get(gts_idx);
            let gts_go = go.bufg_gts.get(ObjPoolIndex::from(gts_idx));
            let which = gts_go.loc.unwrap().i;
            global_nets.gts_enable[which as usize] = true;
            global_nets.gts_invert[which as usize] = gts.invert;
        }
        for gsr in g.bufg_gsr.iter() {
            global_nets.gsr_enable = true;
            global_nets.gsr_invert = gsr.invert;
        }
    }

    // XXX TODO other global bits
    if let XC2BitstreamBits::XC2C32A{ref mut legacy_ivoltage, ref mut legacy_ovoltage,
        ref mut ivoltage, ref mut ovoltage, ..} = result.bits {

        *legacy_ivoltage = false;
        *legacy_ovoltage = false;
        *ivoltage = [false, false];
        *ovoltage = [false, false];
    }

    result
}
