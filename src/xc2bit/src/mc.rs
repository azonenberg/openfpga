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

use std::io::Write;

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

#[derive(Copy, Clone)]
pub struct XC2MCReg {
    pub clk_src: XC2MCRegClkSrc,
    // false = rising edge triggered, true = falling edge triggered
    pub falling_edge: bool,
    pub is_ddr: bool,
    pub r_src: XC2MCRegResetSrc,
    pub s_src: XC2MCRegSetSrc,
    // false = init to 0, true = init to 1
    pub init_state: bool,
    pub ff_mode: XC2MCRegMode,
    pub fb_mode: XC2MCFeedbackMode,
    // false = use xor gate/PLA, true = use IOB direct path
    // true is illegal for buried FFs
    pub ff_in_ibuf: bool,
    pub xor_mode: XC2MCXorMode,
}

impl Default for XC2MCReg {
    fn default() -> XC2MCReg {
        XC2MCReg {
            clk_src: XC2MCRegClkSrc::GCK0,
            falling_edge: false,
            is_ddr: false,
            r_src: XC2MCRegResetSrc::Disabled,
            s_src: XC2MCRegSetSrc::Disabled,
            init_state: true,
            ff_mode: XC2MCRegMode::DFF,
            fb_mode: XC2MCFeedbackMode::Disabled,
            ff_in_ibuf: false,
            xor_mode: XC2MCXorMode::ZERO,
        }
    }
}

impl XC2MCReg {
    pub fn dump_human_readable(&self, fb: u32, ff: u32, writer: &mut Write) {
        write!(writer, "\n").unwrap();
        write!(writer, "FF configuration for FB{}_{}\n", fb + 1, ff + 1).unwrap();
        write!(writer, "FF mode: {}\n", match self.ff_mode {
            XC2MCRegMode::DFF => "D flip-flop",
            XC2MCRegMode::LATCH => "transparent latch",
            XC2MCRegMode::TFF => "T flip-flop",
            XC2MCRegMode::DFFCE => "D flip-flop with clock-enable",
        }).unwrap();
        write!(writer, "initial state: {}\n", if self.init_state {1} else {0}).unwrap();
        write!(writer, "{}-edge triggered\n", if self.falling_edge {"falling"} else {"rising"}).unwrap();
        write!(writer, "DDR: {}\n", if self.is_ddr {"yes"} else {"no"}).unwrap();
        write!(writer, "clock source: {}\n", match self.clk_src {
            XC2MCRegClkSrc::GCK0 => "GCK0",
            XC2MCRegClkSrc::GCK1 => "GCK1",
            XC2MCRegClkSrc::GCK2 => "GCK2",
            XC2MCRegClkSrc::PTC => "PTC",
            XC2MCRegClkSrc::CTC => "CTC",
        }).unwrap();
        write!(writer, "set source: {}\n", match self.s_src {
            XC2MCRegSetSrc::Disabled => "disabled",
            XC2MCRegSetSrc::PTA => "PTA",
            XC2MCRegSetSrc::GSR => "GSR",
            XC2MCRegSetSrc::CTS => "CTS",
        }).unwrap();
        write!(writer, "reset source: {}\n", match self.r_src {
            XC2MCRegResetSrc::Disabled => "disabled",
            XC2MCRegResetSrc::PTA => "PTA",
            XC2MCRegResetSrc::GSR => "GSR",
            XC2MCRegResetSrc::CTR => "CTR",
        }).unwrap();
        write!(writer, "using ibuf direct path: {}\n", if self.ff_in_ibuf {"yes"} else {"no"}).unwrap();
        write!(writer, "XOR gate input: {}\n", match self.xor_mode {
            XC2MCXorMode::ZERO => "0",
            XC2MCXorMode::ONE => "1",
            XC2MCXorMode::PTC => "PTC",
            XC2MCXorMode::PTCB => "~PTC",
        }).unwrap();
        write!(writer, "ZIA feedback: {}\n", match self.fb_mode {
            XC2MCFeedbackMode::Disabled => "disabled",
            XC2MCFeedbackMode::COMB => "combinatorial",
            XC2MCFeedbackMode::REG => "registered",
        }).unwrap();
    }
}


// Read only the FF-related bits
pub fn read_32_ff_logical(fuses: &[bool], block_idx: usize, ff_idx: usize) -> XC2MCReg {
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
    let ff_mode = match regmod {
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

    XC2MCReg {
        clk_src: clk_src,
        falling_edge: clkop,
        is_ddr: clkfreq,
        r_src: reset_mode,
        s_src: set_mode,
        init_state: !pu,
        ff_mode: ff_mode,
        fb_mode: fb_mode,
        ff_in_ibuf: !inreg,
        xor_mode: xormode,
    }
}
