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

//! Contains functions pertaining to macrocells

use std::io;
use std::io::Write;

use *;
use zia::{zia_get_row_width};

/// Clock source for the register in a macrocell
#[derive(Copy, Clone, Eq, PartialEq, Debug)]
pub enum XC2MCRegClkSrc {
    GCK0,
    GCK1,
    GCK2,
    PTC,
    CTC,
}

/// Reset source for the register in a macrocell
#[derive(Copy, Clone, Eq, PartialEq, Debug)]
pub enum XC2MCRegResetSrc {
    Disabled,
    PTA,
    GSR,
    CTR,
}

/// Set source for the register in a macrocell
#[derive(Copy, Clone, Eq, PartialEq, Debug)]
pub enum XC2MCRegSetSrc {
    Disabled,
    PTA,
    GSR,
    CTS,
}

/// Mode of the register in a macrocell.
#[derive(Copy, Clone, Eq, PartialEq, Debug)]
pub enum XC2MCRegMode {
    /// D-type flip-flop
    DFF,
    /// Transparent latch
    LATCH,
    /// Toggle flip-flop
    TFF,
    /// D-type flip-flop with clock-enable pin
    DFFCE,
}

/// Mux selection for the ZIA input from this macrocell. The ZIA input can be chosen to come from either the XOR gate
/// or from the output of the register.
#[derive(Copy, Clone, Eq, PartialEq, Debug)]
pub enum XC2MCFeedbackMode {
    Disabled,
    COMB,
    REG,
}

/// Mux selection for the "not from OR gate" input to the XOR gate. The XOR gate in a macrocell contains two inputs,
/// the output of the corresponding OR term from the PLA and a specific dedicated AND term from the PLA.
#[derive(Copy, Clone, Eq, PartialEq, Debug)]
pub enum XC2MCXorMode {
    /// A constant zero which results in this XOR outputting the value of the OR term
    ZERO,
    /// A constant one which results in this XOR outputting the complement of the OR term
    ONE,
    /// XOR the OR term with the special product term C
    PTC,
    /// XNOR the OR term with the special product term C
    PTCB,
}

/// Represents a macrocell.
#[derive(Copy, Clone)]
pub struct XC2Macrocell {
    /// Clock source for the register
    pub clk_src: XC2MCRegClkSrc,
    /// Specifies the clock polarity for the register
    ///
    /// `false` = rising edge triggered flip-flop, transparent-when-high latch
    ///
    /// `true` = falling edge triggered flip-flop, transparent-when-low latch
    pub clk_invert_pol: bool,
    /// Specifies whether flip-flop are triggered on both clock edges
    ///
    /// It is currently unknown what happens when this is used on a transparent latch
    pub is_ddr: bool,
    /// Reset source for the register
    pub r_src: XC2MCRegResetSrc,
    /// Set source for the register
    pub s_src: XC2MCRegSetSrc,
    /// Power-up state of the register
    ///
    /// `false` = init to 0, `true` = init to 1
    pub init_state: bool,
    /// Register mode
    pub reg_mode: XC2MCRegMode,
    /// ZIA input mode for feedback from this macrocell
    pub fb_mode: XC2MCFeedbackMode,
    /// Controls the input for the register
    ///
    /// `false` = use the output of the XOR gate (combinatorial path), `true` = use IOB direct path
    /// (`true` is illegal for buried macrocells in the larger devices)
    pub ff_in_ibuf: bool,
    /// Controls the "other" (not from the OR term) input to the XOR gate
    pub xor_mode: XC2MCXorMode,
}

impl Default for XC2Macrocell {
    /// Returns a "default" macrocell configuration.
    // XXX what should the default state be???
    fn default() -> XC2Macrocell {
        XC2Macrocell {
            clk_src: XC2MCRegClkSrc::GCK0,
            clk_invert_pol: false,
            is_ddr: false,
            r_src: XC2MCRegResetSrc::Disabled,
            s_src: XC2MCRegSetSrc::Disabled,
            init_state: true,
            reg_mode: XC2MCRegMode::DFF,
            fb_mode: XC2MCFeedbackMode::Disabled,
            ff_in_ibuf: false,
            xor_mode: XC2MCXorMode::ZERO,
        }
    }
}

impl XC2Macrocell {
    /// Dump a human-readable explanation of the settings for this macrocell to the given `writer` object.
    /// `fb` and `ff` must be the function block number and macrocell number of this macrocell.
    pub fn dump_human_readable(&self, fb: u32, ff: u32, writer: &mut Write) -> Result<(), io::Error> {
        write!(writer, "\n")?;
        write!(writer, "FF configuration for FB{}_{}\n", fb + 1, ff + 1)?;
        write!(writer, "FF mode: {}\n", match self.reg_mode {
            XC2MCRegMode::DFF => "D flip-flop",
            XC2MCRegMode::LATCH => "transparent latch",
            XC2MCRegMode::TFF => "T flip-flop",
            XC2MCRegMode::DFFCE => "D flip-flop with clock-enable",
        })?;
        write!(writer, "initial state: {}\n", if self.init_state {1} else {0})?;
        write!(writer, "{}-edge triggered\n", if self.clk_invert_pol {"falling"} else {"rising"})?;
        write!(writer, "DDR: {}\n", if self.is_ddr {"yes"} else {"no"})?;
        write!(writer, "clock source: {}\n", match self.clk_src {
            XC2MCRegClkSrc::GCK0 => "GCK0",
            XC2MCRegClkSrc::GCK1 => "GCK1",
            XC2MCRegClkSrc::GCK2 => "GCK2",
            XC2MCRegClkSrc::PTC => "PTC",
            XC2MCRegClkSrc::CTC => "CTC",
        })?;
        write!(writer, "set source: {}\n", match self.s_src {
            XC2MCRegSetSrc::Disabled => "disabled",
            XC2MCRegSetSrc::PTA => "PTA",
            XC2MCRegSetSrc::GSR => "GSR",
            XC2MCRegSetSrc::CTS => "CTS",
        })?;
        write!(writer, "reset source: {}\n", match self.r_src {
            XC2MCRegResetSrc::Disabled => "disabled",
            XC2MCRegResetSrc::PTA => "PTA",
            XC2MCRegResetSrc::GSR => "GSR",
            XC2MCRegResetSrc::CTR => "CTR",
        })?;
        write!(writer, "using ibuf direct path: {}\n", if self.ff_in_ibuf {"yes"} else {"no"})?;
        write!(writer, "XOR gate input: {}\n", match self.xor_mode {
            XC2MCXorMode::ZERO => "0",
            XC2MCXorMode::ONE => "1",
            XC2MCXorMode::PTC => "PTC",
            XC2MCXorMode::PTCB => "~PTC",
        })?;
        write!(writer, "ZIA feedback: {}\n", match self.fb_mode {
            XC2MCFeedbackMode::Disabled => "disabled",
            XC2MCFeedbackMode::COMB => "combinatorial",
            XC2MCFeedbackMode::REG => "registered",
        })?;

        Ok(())
    }
}


///  Internal function that reads only the macrocell-related bits from the macrcocell configuration
pub fn read_small_ff_logical(fuses: &[bool], block_idx: usize, ff_idx: usize) -> XC2Macrocell {
    let aclk = fuses[block_idx + ff_idx * 27 + 0];
    let clk = (fuses[block_idx + ff_idx * 27 + 2],
               fuses[block_idx + ff_idx * 27 + 3]);

    let clk_src = match clk {
        (false, false) => XC2MCRegClkSrc::GCK0,
        (false, true)  => XC2MCRegClkSrc::GCK1,
        (true, false)  => XC2MCRegClkSrc::GCK2,
        (true, true)   => match aclk {
            true => XC2MCRegClkSrc::CTC,
            false => XC2MCRegClkSrc::PTC,
        },
    };

    let clkop = fuses[block_idx + ff_idx * 27 + 1];
    let clkfreq = fuses[block_idx + ff_idx * 27 + 4];

    let r = (fuses[block_idx + ff_idx * 27 + 5],
             fuses[block_idx + ff_idx * 27 + 6]);
    let reset_mode = match r {
        (false, false) => XC2MCRegResetSrc::PTA,
        (false, true)  => XC2MCRegResetSrc::GSR,
        (true, false)  => XC2MCRegResetSrc::CTR,
        (true, true)   => XC2MCRegResetSrc::Disabled,
    };

    let p = (fuses[block_idx + ff_idx * 27 + 7],
             fuses[block_idx + ff_idx * 27 + 8]);
    let set_mode = match p {
        (false, false) => XC2MCRegSetSrc::PTA,
        (false, true)  => XC2MCRegSetSrc::GSR,
        (true, false)  => XC2MCRegSetSrc::CTS,
        (true, true)   => XC2MCRegSetSrc::Disabled,
    };

    let regmod = (fuses[block_idx + ff_idx * 27 + 9],
                  fuses[block_idx + ff_idx * 27 + 10]);
    let reg_mode = match regmod {
        (false, false) => XC2MCRegMode::DFF,
        (false, true)  => XC2MCRegMode::LATCH,
        (true, false)  => XC2MCRegMode::TFF,
        (true, true)   => XC2MCRegMode::DFFCE,
    };

    let fb = (fuses[block_idx + ff_idx * 27 + 13],
              fuses[block_idx + ff_idx * 27 + 14]);
    let fb_mode = match fb {
        (false, false) => XC2MCFeedbackMode::COMB,
        (true, false)  => XC2MCFeedbackMode::REG,
        (_, true)      => XC2MCFeedbackMode::Disabled,
    };

    let inreg = fuses[block_idx + ff_idx * 27 + 15];

    let xorin = (fuses[block_idx + ff_idx * 27 + 17],
                 fuses[block_idx + ff_idx * 27 + 18]);
    let xormode = match xorin {
        (false, false) => XC2MCXorMode::ZERO,
        (false, true)  => XC2MCXorMode::PTCB,
        (true, false)  => XC2MCXorMode::PTC,
        (true, true)   => XC2MCXorMode::ONE,
    };

    let pu = fuses[block_idx + ff_idx * 27 + 26];

    XC2Macrocell {
        clk_src: clk_src,
        clk_invert_pol: clkop,
        is_ddr: clkfreq,
        r_src: reset_mode,
        s_src: set_mode,
        init_state: !pu,
        reg_mode: reg_mode,
        fb_mode: fb_mode,
        ff_in_ibuf: !inreg,
        xor_mode: xormode,
    }
}

///  Internal function that reads only the macrocell-related bits from the macrcocell configuration
pub fn read_large_ff_logical(fuses: &[bool], fuse_idx: usize) -> XC2Macrocell {
    let aclk = fuses[fuse_idx + 0];

    let clk = (fuses[fuse_idx + 1],
               fuses[fuse_idx + 2]);
    let clk_src = match clk {
        (false, false) => XC2MCRegClkSrc::GCK0,
        (false, true)  => XC2MCRegClkSrc::GCK1,
        (true, false)  => XC2MCRegClkSrc::GCK2,
        (true, true)   => match aclk {
            true => XC2MCRegClkSrc::CTC,
            false => XC2MCRegClkSrc::PTC,
        },
    };

    let clkfreq = fuses[fuse_idx + 3];
    let clkop = fuses[fuse_idx + 4];

    let fb = (fuses[fuse_idx + 6],
              fuses[fuse_idx + 7]);
    let fb_mode = match fb {
        (false, false) => XC2MCFeedbackMode::COMB,
        (true, false)  => XC2MCFeedbackMode::REG,
        (_, true)      => XC2MCFeedbackMode::Disabled,
    };

    let inreg = fuses[fuse_idx + 10];

    let p = (fuses[fuse_idx + 17],
             fuses[fuse_idx + 18]);
    let set_mode = match p {
        (false, false) => XC2MCRegSetSrc::PTA,
        (false, true)  => XC2MCRegSetSrc::GSR,
        (true, false)  => XC2MCRegSetSrc::CTS,
        (true, true)   => XC2MCRegSetSrc::Disabled,
    };

    let pu = fuses[fuse_idx + 19];

    let regmod = (fuses[fuse_idx + 21],
                  fuses[fuse_idx + 22]);
    let reg_mode = match regmod {
        (false, false) => XC2MCRegMode::DFF,
        (false, true)  => XC2MCRegMode::LATCH,
        (true, false)  => XC2MCRegMode::TFF,
        (true, true)   => XC2MCRegMode::DFFCE,
    };

    let r = (fuses[fuse_idx + 23],
             fuses[fuse_idx + 24]);
    let reset_mode = match r {
        (false, false) => XC2MCRegResetSrc::PTA,
        (false, true)  => XC2MCRegResetSrc::GSR,
        (true, false)  => XC2MCRegResetSrc::CTR,
        (true, true)   => XC2MCRegResetSrc::Disabled,
    };

    let xorin = (fuses[fuse_idx + 27],
                 fuses[fuse_idx + 28]);
    let xormode = match xorin {
        (false, false) => XC2MCXorMode::ZERO,
        (false, true)  => XC2MCXorMode::PTCB,
        (true, false)  => XC2MCXorMode::PTC,
        (true, true)   => XC2MCXorMode::ONE,
    };

    XC2Macrocell {
        clk_src: clk_src,
        clk_invert_pol: clkop,
        is_ddr: clkfreq,
        r_src: reset_mode,
        s_src: set_mode,
        init_state: !pu,
        reg_mode: reg_mode,
        fb_mode: fb_mode,
        ff_in_ibuf: !inreg,
        xor_mode: xormode,
    }
}

///  Internal function that reads only the macrocell-related bits from the macrcocell configuration
pub fn read_large_buried_ff_logical(fuses: &[bool], fuse_idx: usize) -> XC2Macrocell {
    let aclk = fuses[fuse_idx + 0];

    let clk = (fuses[fuse_idx + 1],
               fuses[fuse_idx + 2]);
    let clk_src = match clk {
        (false, false) => XC2MCRegClkSrc::GCK0,
        (false, true)  => XC2MCRegClkSrc::GCK1,
        (true, false)  => XC2MCRegClkSrc::GCK2,
        (true, true)   => match aclk {
            true => XC2MCRegClkSrc::CTC,
            false => XC2MCRegClkSrc::PTC,
        },
    };

    let clkfreq = fuses[fuse_idx + 3];
    let clkop = fuses[fuse_idx + 4];

    let fb = (fuses[fuse_idx + 5],
              fuses[fuse_idx + 6]);
    let fb_mode = match fb {
        (false, false) => XC2MCFeedbackMode::COMB,
        (true, false)  => XC2MCFeedbackMode::REG,
        (_, true)      => XC2MCFeedbackMode::Disabled,
    };

    let p = (fuses[fuse_idx + 7],
             fuses[fuse_idx + 8]);
    let set_mode = match p {
        (false, false) => XC2MCRegSetSrc::PTA,
        (false, true)  => XC2MCRegSetSrc::GSR,
        (true, false)  => XC2MCRegSetSrc::CTS,
        (true, true)   => XC2MCRegSetSrc::Disabled,
    };

    let pu = fuses[fuse_idx + 9];

    let regmod = (fuses[fuse_idx + 10],
                  fuses[fuse_idx + 11]);
    let reg_mode = match regmod {
        (false, false) => XC2MCRegMode::DFF,
        (false, true)  => XC2MCRegMode::LATCH,
        (true, false)  => XC2MCRegMode::TFF,
        (true, true)   => XC2MCRegMode::DFFCE,
    };

    let r = (fuses[fuse_idx + 12],
             fuses[fuse_idx + 13]);
    let reset_mode = match r {
        (false, false) => XC2MCRegResetSrc::PTA,
        (false, true)  => XC2MCRegResetSrc::GSR,
        (true, false)  => XC2MCRegResetSrc::CTR,
        (true, true)   => XC2MCRegResetSrc::Disabled,
    };

    let xorin = (fuses[fuse_idx + 14],
                 fuses[fuse_idx + 15]);
    let xormode = match xorin {
        (false, false) => XC2MCXorMode::ZERO,
        (false, true)  => XC2MCXorMode::PTCB,
        (true, false)  => XC2MCXorMode::PTC,
        (true, true)   => XC2MCXorMode::ONE,
    };

    XC2Macrocell {
        clk_src: clk_src,
        clk_invert_pol: clkop,
        is_ddr: clkfreq,
        r_src: reset_mode,
        s_src: set_mode,
        init_state: !pu,
        reg_mode: reg_mode,
        fb_mode: fb_mode,
        ff_in_ibuf: false,
        xor_mode: xormode,
    }
}

/// Helper that prints the IOB and macrocell configuration on the "small" parts
pub fn write_small_mc_to_jed(writer: &mut Write, device: XC2Device, fb: &XC2BitstreamFB, iobs: &[XC2MCSmallIOB],
    fb_i: usize, fuse_base: usize) -> Result<(), io::Error> {

    let zia_row_width = zia_get_row_width(device);

    for i in 0..MCS_PER_FB {
        write!(writer, "L{:06} ",
            fuse_base + zia_row_width * INPUTS_PER_ANDTERM +
            ANDTERMS_PER_FB * INPUTS_PER_ANDTERM * 2 + ANDTERMS_PER_FB * MCS_PER_FB + i * 27)?;

        let iob = fb_ff_num_to_iob_num(device, fb_i as u32, i as u32).unwrap() as usize;

        // aclk
        write!(writer, "{}", match fb.ffs[i].clk_src {
            XC2MCRegClkSrc::CTC => "1",
            _ => "0",
        })?;

        // clkop
        write!(writer, "{}", if fb.ffs[i].clk_invert_pol {"1"} else {"0"})?;

        // clk
        write!(writer, "{}", match fb.ffs[i].clk_src {
            XC2MCRegClkSrc::GCK0 => "00",
            XC2MCRegClkSrc::GCK1 => "01",
            XC2MCRegClkSrc::GCK2 => "10",
            XC2MCRegClkSrc::PTC | XC2MCRegClkSrc::CTC => "11",
        })?;

        // clkfreq
        write!(writer, "{}", if fb.ffs[i].is_ddr {"1"} else {"0"})?;

        // r
        write!(writer, "{}", match fb.ffs[i].r_src {
            XC2MCRegResetSrc::PTA => "00",
            XC2MCRegResetSrc::GSR => "01",
            XC2MCRegResetSrc::CTR => "10",
            XC2MCRegResetSrc::Disabled => "11",
        })?;

        // p
        write!(writer, "{}", match fb.ffs[i].s_src {
            XC2MCRegSetSrc::PTA => "00",
            XC2MCRegSetSrc::GSR => "01",
            XC2MCRegSetSrc::CTS => "10",
            XC2MCRegSetSrc::Disabled => "11",
        })?;

        // regmod
        write!(writer, "{}", match fb.ffs[i].reg_mode {
            XC2MCRegMode::DFF => "00",
            XC2MCRegMode::LATCH => "01",
            XC2MCRegMode::TFF => "10",
            XC2MCRegMode::DFFCE => "11",
        })?;

        // inz
        write!(writer, "{}", match iobs[iob].zia_mode {
            XC2IOBZIAMode::PAD => "00",
            XC2IOBZIAMode::REG => "10",
            XC2IOBZIAMode::Disabled => "11",
        })?;

        // fb
        write!(writer, "{}", match fb.ffs[i].fb_mode {
            XC2MCFeedbackMode::COMB => "00",
            XC2MCFeedbackMode::REG => "10",
            XC2MCFeedbackMode::Disabled => "11",
        })?;

        // inreg
        write!(writer, "{}", if fb.ffs[i].ff_in_ibuf {"0"} else {"1"})?;

        // st
        write!(writer, "{}", if iobs[iob].schmitt_trigger {"1"} else {"0"})?;

        // xorin
        write!(writer, "{}", match fb.ffs[i].xor_mode {
            XC2MCXorMode::ZERO => "00",
            XC2MCXorMode::PTCB => "01",
            XC2MCXorMode::PTC => "10",
            XC2MCXorMode::ONE => "11",
        })?;

        // regcom
        write!(writer, "{}", if iobs[iob].obuf_uses_ff {"0"} else {"1"})?;

        // oe
        write!(writer, "{}", match iobs[iob].obuf_mode {
            XC2IOBOBufMode::PushPull => "0000",
            XC2IOBOBufMode::OpenDrain => "0001",
            XC2IOBOBufMode::TriStateGTS1 => "0010",
            XC2IOBOBufMode::TriStatePTB => "0100",
            XC2IOBOBufMode::TriStateGTS3 => "0110",
            XC2IOBOBufMode::TriStateCTE => "1000",
            XC2IOBOBufMode::TriStateGTS2 => "1010",
            XC2IOBOBufMode::TriStateGTS0 => "1100",
            XC2IOBOBufMode::CGND => "1110",
            XC2IOBOBufMode::Disabled => "1111",
        })?;

        // tm
        write!(writer, "{}", if iobs[iob].termination_enabled {"1"} else {"0"})?;

        // slw
        write!(writer, "{}", if iobs[iob].slew_is_fast {"0"} else {"1"})?;

        // pu
        write!(writer, "{}", if fb.ffs[i].init_state {"0"} else {"1"})?;

        write!(writer, "*\n")?;
    }
    write!(writer, "\n")?;

    Ok(())
}

/// Helper that prints the IOB and macrocell configuration on the "large" parts
pub fn write_large_mc_to_jed(writer: &mut Write, device: XC2Device, fb: &XC2BitstreamFB, iobs: &[XC2MCLargeIOB],
    fb_i: usize, fuse_base: usize) -> Result<(), io::Error> {

    let zia_row_width = zia_get_row_width(device);

    let mut current_fuse_offset = fuse_base + zia_row_width * INPUTS_PER_ANDTERM +
        ANDTERMS_PER_FB * INPUTS_PER_ANDTERM * 2 + ANDTERMS_PER_FB * MCS_PER_FB;

    for i in 0..MCS_PER_FB {
        write!(writer, "L{:06} ", current_fuse_offset)?;

        let iob = fb_ff_num_to_iob_num(device, fb_i as u32, i as u32);

        // aclk
        write!(writer, "{}", match fb.ffs[i].clk_src {
            XC2MCRegClkSrc::CTC => "1",
            _ => "0",
        })?;

        // clk
        write!(writer, "{}", match fb.ffs[i].clk_src {
            XC2MCRegClkSrc::GCK0 => "00",
            XC2MCRegClkSrc::GCK1 => "01",
            XC2MCRegClkSrc::GCK2 => "10",
            XC2MCRegClkSrc::PTC | XC2MCRegClkSrc::CTC => "11",
        })?;

        // clkfreq
        write!(writer, "{}", if fb.ffs[i].is_ddr {"1"} else {"0"})?;

        // clkop
        write!(writer, "{}", if fb.ffs[i].clk_invert_pol {"1"} else {"0"})?;

        // dg
        if iob.is_some() {
            write!(writer, "{}", if iobs[iob.unwrap() as usize].uses_data_gate {"1"} else {"0"})?;
        }

        // fb
        write!(writer, "{}", match fb.ffs[i].fb_mode {
            XC2MCFeedbackMode::COMB => "00",
            XC2MCFeedbackMode::REG => "10",
            XC2MCFeedbackMode::Disabled => "11",
        })?;

        if iob.is_some() {
            let iob = iob.unwrap() as usize;

            // inmod
            write!(writer, "{}", match iobs[iob].ibuf_mode {
                XC2IOBIbufMode::NoVrefNoSt => "00",
                XC2IOBIbufMode::IsVref => "01",
                XC2IOBIbufMode::UsesVref => "10",
                XC2IOBIbufMode::NoVrefSt => "11",
            })?;

            // inreg
            write!(writer, "{}", if fb.ffs[i].ff_in_ibuf {"0"} else {"1"})?;

            // inz
            write!(writer, "{}", match iobs[iob].zia_mode {
                XC2IOBZIAMode::PAD => "00",
                XC2IOBZIAMode::REG => "10",
                XC2IOBZIAMode::Disabled => "11",
            })?;

            // oe
            write!(writer, "{}", match iobs[iob].obuf_mode {
                XC2IOBOBufMode::PushPull => "0000",
                XC2IOBOBufMode::OpenDrain => "0001",
                XC2IOBOBufMode::TriStateGTS1 => "0010",
                XC2IOBOBufMode::TriStatePTB => "0100",
                XC2IOBOBufMode::TriStateGTS3 => "0110",
                XC2IOBOBufMode::TriStateCTE => "1000",
                XC2IOBOBufMode::TriStateGTS2 => "1010",
                XC2IOBOBufMode::TriStateGTS0 => "1100",
                XC2IOBOBufMode::CGND => "1110",
                XC2IOBOBufMode::Disabled => "1111",
            })?;
        }

        // p
        write!(writer, "{}", match fb.ffs[i].s_src {
            XC2MCRegSetSrc::PTA => "00",
            XC2MCRegSetSrc::GSR => "01",
            XC2MCRegSetSrc::CTS => "10",
            XC2MCRegSetSrc::Disabled => "11",
        })?;

        // pu
        write!(writer, "{}", if fb.ffs[i].init_state {"0"} else {"1"})?;

        if iob.is_some() {
            // regcom
            write!(writer, "{}", if iobs[iob.unwrap() as usize].obuf_uses_ff {"0"} else {"1"})?;
        }

        // regmod
        write!(writer, "{}", match fb.ffs[i].reg_mode {
            XC2MCRegMode::DFF => "00",
            XC2MCRegMode::LATCH => "01",
            XC2MCRegMode::TFF => "10",
            XC2MCRegMode::DFFCE => "11",
        })?;

        // r
        write!(writer, "{}", match fb.ffs[i].r_src {
            XC2MCRegResetSrc::PTA => "00",
            XC2MCRegResetSrc::GSR => "01",
            XC2MCRegResetSrc::CTR => "10",
            XC2MCRegResetSrc::Disabled => "11",
        })?;

        if iob.is_some() {
            let iob = iob.unwrap() as usize;

            // slw
            write!(writer, "{}", if iobs[iob].slew_is_fast {"0"} else {"1"})?;

            // tm
            write!(writer, "{}", if iobs[iob].termination_enabled {"1"} else {"0"})?;
        }

        // xorin
        write!(writer, "{}", match fb.ffs[i].xor_mode {
            XC2MCXorMode::ZERO => "00",
            XC2MCXorMode::PTCB => "01",
            XC2MCXorMode::PTC => "10",
            XC2MCXorMode::ONE => "11",
        })?;

        write!(writer, "*\n")?;

        if iob.is_some() {
            current_fuse_offset += 29;
        } else {
            current_fuse_offset += 16;
        }
    }
    write!(writer, "\n")?;

    Ok(())
}
