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
use fusemap_physical::{mc_block_loc};
use util::{b2s};
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

impl XC2MCRegClkSrc {
    /// decodes the Aclk and Clk bits
    pub fn decode(aclk: bool, clk: (bool, bool)) -> Self {
        match clk {
            (false, false) => XC2MCRegClkSrc::GCK0,
            (false, true)  => XC2MCRegClkSrc::GCK2,
            (true, false)  => XC2MCRegClkSrc::GCK1,
            (true, true)   => match aclk {
                true => XC2MCRegClkSrc::CTC,
                false => XC2MCRegClkSrc::PTC,
            },
        }
    }

    /// encodes the Aclk bit
    pub fn encode_aclk(&self) -> bool {
        match *self {
            XC2MCRegClkSrc::CTC => true,
            _ => false,
        }
    }

    /// encodes the Clk bits
    pub fn encode_clk(&self) -> (bool, bool) {
        match *self {
            XC2MCRegClkSrc::GCK0 => (false, false),
            XC2MCRegClkSrc::GCK1 => (true, false),
            XC2MCRegClkSrc::GCK2 => (false, true),
            XC2MCRegClkSrc::PTC | XC2MCRegClkSrc::CTC => (true, true),
        }
    }
}

/// Reset source for the register in a macrocell
#[derive(Copy, Clone, Eq, PartialEq, Debug)]
pub enum XC2MCRegResetSrc {
    Disabled,
    PTA,
    GSR,
    CTR,
}

impl XC2MCRegResetSrc {
    /// decodes the R bits
    pub fn decode(r: (bool, bool)) -> Self {
        match r {
            (false, false) => XC2MCRegResetSrc::PTA,
            (false, true)  => XC2MCRegResetSrc::GSR,
            (true, false)  => XC2MCRegResetSrc::CTR,
            (true, true)   => XC2MCRegResetSrc::Disabled,
        }
    }

    /// encodes the R bits
    pub fn encode(&self) -> (bool, bool) {
        match *self {
            XC2MCRegResetSrc::PTA => (false, false),
            XC2MCRegResetSrc::GSR => (false, true),
            XC2MCRegResetSrc::CTR => (true, false),
            XC2MCRegResetSrc::Disabled => (true, true),
        }
    }
}

/// Set source for the register in a macrocell
#[derive(Copy, Clone, Eq, PartialEq, Debug)]
pub enum XC2MCRegSetSrc {
    Disabled,
    PTA,
    GSR,
    CTS,
}

impl XC2MCRegSetSrc {
    /// decodes the P bits
    pub fn decode(r: (bool, bool)) -> Self {
        match r {
            (false, false) => XC2MCRegSetSrc::PTA,
            (false, true)  => XC2MCRegSetSrc::GSR,
            (true, false)  => XC2MCRegSetSrc::CTS,
            (true, true)   => XC2MCRegSetSrc::Disabled,
        }
    }

    /// encodes the P bits
    pub fn encode(&self) -> (bool, bool) {
        match *self {
            XC2MCRegSetSrc::PTA => (false, false),
            XC2MCRegSetSrc::GSR => (false, true),
            XC2MCRegSetSrc::CTS => (true, false),
            XC2MCRegSetSrc::Disabled => (true, true),
        }
    }
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

impl XC2MCRegMode {
    /// decodes the RegMod bits
    pub fn decode(regmod: (bool, bool)) -> Self {
        match regmod {
            (false, false) => XC2MCRegMode::DFF,
            (false, true)  => XC2MCRegMode::LATCH,
            (true, false)  => XC2MCRegMode::TFF,
            (true, true)   => XC2MCRegMode::DFFCE,
        }
    }

    /// encodes the RegMod bits
    pub fn encode(&self) -> (bool, bool) {
        match *self {
            XC2MCRegMode::DFF => (false, false),
            XC2MCRegMode::LATCH => (false, true),
            XC2MCRegMode::TFF => (true, false),
            XC2MCRegMode::DFFCE => (true, true),
        }
    }
}

/// Mux selection for the ZIA input from this macrocell. The ZIA input can be chosen to come from either the XOR gate
/// or from the output of the register.
#[derive(Copy, Clone, Eq, PartialEq, Debug)]
pub enum XC2MCFeedbackMode {
    Disabled,
    COMB,
    REG,
}

impl XC2MCFeedbackMode {
    /// decodes the FB bits
    pub fn decode(fb: (bool, bool)) -> Self {
        match fb {
            (false, false) => XC2MCFeedbackMode::COMB,
            (true, false)  => XC2MCFeedbackMode::REG,
            (_, true)      => XC2MCFeedbackMode::Disabled,
        }
    }

    /// encodes the FB bits
    pub fn encode(&self) -> (bool, bool) {
        match *self {
            XC2MCFeedbackMode::COMB => (false, false),
            XC2MCFeedbackMode::REG => (true, false),
            XC2MCFeedbackMode::Disabled => (true, true),
        }
    }
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

impl XC2MCXorMode {
    /// decodes the XorIn bits
    pub fn decode(xorin: (bool, bool)) -> Self {
        match xorin {
            (false, false) => XC2MCXorMode::ZERO,
            (false, true)  => XC2MCXorMode::PTCB,
            (true, false)  => XC2MCXorMode::PTC,
            (true, true)   => XC2MCXorMode::ONE,
        }
    }

    /// encodes the XorIn bits
    pub fn encode(&self) -> (bool, bool) {
        match *self {
            XC2MCXorMode::ZERO => (false, false),
            XC2MCXorMode::PTCB => (false, true),
            XC2MCXorMode::PTC => (true, false),
            XC2MCXorMode::ONE => (true, true),
        }
    }
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
    fn default() -> Self {
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

pub static MC_TO_ROW_MAP_LARGE: [usize; MCS_PER_FB] = 
    [0, 3, 5, 8, 10, 13, 15, 18, 20, 23, 25, 28, 30, 33, 35, 38];

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

    /// Write the crbit representation of this macrocell to the given `fuse_array`.
    pub fn to_crbit(&self, device: XC2Device, fb: u32, mc: u32, fuse_array: &mut FuseArray) {
        let (x, y, mirror) = mc_block_loc(device, fb);
        // direction
        let x = x as i32;
        let d = if !mirror {1} else {-1};
        match device {
            XC2Device::XC2C32 | XC2Device::XC2C32A => {
                // The "32" variant
                // each macrocell is 3 rows high
                let y = y + (mc as usize) * 3;

                // aclk
                fuse_array.set((x + d * 0) as usize, y + 0, self.clk_src.encode_aclk());

                // clkop
                fuse_array.set((x + d * 1) as usize, y + 0, self.clk_invert_pol);

                // clk
                let clk = self.clk_src.encode_clk();
                fuse_array.set((x + d * 2) as usize, y + 0, clk.0);
                fuse_array.set((x + d * 3) as usize, y + 0, clk.1);

                // clkfreq
                fuse_array.set((x + d * 4) as usize, y + 0, self.is_ddr);

                // r
                let r = self.r_src.encode();
                fuse_array.set((x + d * 5) as usize, y + 0, r.0);
                fuse_array.set((x + d * 6) as usize, y + 0, r.1);

                // p
                let p = self.s_src.encode();
                fuse_array.set((x + d * 7) as usize, y + 0, p.0);
                fuse_array.set((x + d * 8) as usize, y + 0, p.1);

                // regmod
                let regmod = self.reg_mode.encode();
                fuse_array.set((x + d * 0) as usize, y + 1, regmod.0);
                fuse_array.set((x + d * 1) as usize, y + 1, regmod.1);

                // skipped INz (belongs to IOB)

                // fb
                let fb = self.fb_mode.encode();
                fuse_array.set((x + d * 4) as usize, y + 1, fb.0);
                fuse_array.set((x + d * 5) as usize, y + 1, fb.1);

                // inreg
                fuse_array.set((x + d * 6) as usize, y + 1, !self.ff_in_ibuf);

                // skipped St (belongs to IOB)

                // xorin
                let xorin = self.xor_mode.encode();
                fuse_array.set((x + d * 8) as usize, y + 1, xorin.0);
                fuse_array.set((x + d * 0) as usize, y + 2, xorin.1);

                // skipped RegCom (belongs to IOB)
                // skipped Oe (belongs to IOB)
                // skipped Tm (belongs to IOB)
                // skipped Slw (belongs to IOB)

                // pu
                fuse_array.set((x + d * 8) as usize, y + 2, !self.init_state);
            },
            XC2Device::XC2C64 | XC2Device::XC2C64A => {
                // The "64" variant
                // each macrocell is 3 rows high
                let y = y + (mc as usize) * 3;

                // aclk
                fuse_array.set((x + d * 8) as usize, y + 0, self.clk_src.encode_aclk());

                // clkop
                fuse_array.set((x + d * 7) as usize, y + 0, self.clk_invert_pol);

                // clk
                let clk = self.clk_src.encode_clk();
                fuse_array.set((x + d * 5) as usize, y + 0, clk.0);
                fuse_array.set((x + d * 6) as usize, y + 0, clk.1);

                // clkfreq
                fuse_array.set((x + d * 4) as usize, y + 0, self.is_ddr);

                // r
                let r = self.r_src.encode();
                fuse_array.set((x + d * 2) as usize, y + 0, r.0);
                fuse_array.set((x + d * 3) as usize, y + 0, r.1);

                // p
                let p = self.s_src.encode();
                fuse_array.set((x + d * 0) as usize, y + 0, p.0);
                fuse_array.set((x + d * 1) as usize, y + 0, p.1);

                // regmod
                let regmod = self.reg_mode.encode();
                fuse_array.set((x + d * 7) as usize, y + 1, regmod.0);
                fuse_array.set((x + d * 8) as usize, y + 1, regmod.1);

                // skipped INz (belongs to IOB)

                // fb
                let fb = self.fb_mode.encode();
                fuse_array.set((x + d * 3) as usize, y + 1, fb.0);
                fuse_array.set((x + d * 4) as usize, y + 1, fb.1);

                // inreg
                fuse_array.set((x + d * 2) as usize, y + 1, !self.ff_in_ibuf);

                // skipped St (belongs to IOB)

                // xorin
                let xorin = self.xor_mode.encode();
                fuse_array.set((x + d * 7) as usize, y + 2, xorin.0);
                fuse_array.set((x + d * 8) as usize, y + 2, xorin.1);

                // skipped RegCom (belongs to IOB)
                // skipped Oe (belongs to IOB)
                // skipped Tm (belongs to IOB)
                // skipped Slw (belongs to IOB)

                // pu
                fuse_array.set((x + d * 0) as usize, y + 2, !self.init_state);
            },
            XC2Device::XC2C256 => {
                // The "256" variant
                // each macrocell is 3 rows high
                let y = y + (mc as usize) * 3;

                // skipped InMod (belongs to IOB)

                // fb
                let fb = self.fb_mode.encode();
                fuse_array.set((x + d * 2) as usize, y + 0, fb.0);
                fuse_array.set((x + d * 3) as usize, y + 0, fb.1);

                // skipped DG (belongs to IOB)

                // clkop
                fuse_array.set((x + d * 5) as usize, y + 0, self.clk_invert_pol);

                // clkfreq
                fuse_array.set((x + d * 6) as usize, y + 0, self.is_ddr);

                // clk
                let clk = self.clk_src.encode_clk();
                fuse_array.set((x + d * 7) as usize, y + 0, clk.0);
                fuse_array.set((x + d * 8) as usize, y + 0, clk.1);

                // aclk
                fuse_array.set((x + d * 9) as usize, y + 0, self.clk_src.encode_aclk());

                // pu
                fuse_array.set((x + d * 0) as usize, y + 1, !self.init_state);

                // p
                let p = self.s_src.encode();
                fuse_array.set((x + d * 1) as usize, y + 1, p.0);
                fuse_array.set((x + d * 2) as usize, y + 1, p.1);

                // skipped Oe (belongs to IOB)
                // skipped INz (belongs to IOB)

                // inreg
                fuse_array.set((x + d * 9) as usize, y + 1, !self.ff_in_ibuf);

                // xorin
                let xorin = self.xor_mode.encode();
                fuse_array.set((x + d * 0) as usize, y + 2, xorin.0);
                fuse_array.set((x + d * 1) as usize, y + 2, xorin.1);

                // skipped Tm (belongs to IOB)
                // skipped Slw (belongs to IOB)

                // r
                let r = self.r_src.encode();
                fuse_array.set((x + d * 4) as usize, y + 2, r.0);
                fuse_array.set((x + d * 5) as usize, y + 2, r.1);

                // regmod
                let regmod = self.reg_mode.encode();
                fuse_array.set((x + d * 6) as usize, y + 2, regmod.0);
                fuse_array.set((x + d * 7) as usize, y + 2, regmod.1);

                // skipped RegCom (belongs to IOB)
            },
            XC2Device::XC2C128 | XC2Device::XC2C384 | XC2Device::XC2C512 => {
                // The "common large macrocell" variant
                // we need this funny lookup table, but otherwise macrocells are 2x15
                let y = y + MC_TO_ROW_MAP_LARGE[mc as usize];

                // skipped INz (belongs to IOB)

                // fb
                let fb = self.fb_mode.encode();
                fuse_array.set((x + d * 2) as usize, y + 0, fb.0);
                fuse_array.set((x + d * 3) as usize, y + 0, fb.1);

                // skipped DG (belongs to IOB)
                // skipped InMod (belongs to IOB)
                // skipped Tm (belongs to IOB)

                // aclk
                fuse_array.set((x + d * 8) as usize, y + 0, self.clk_src.encode_aclk());

                // clk
                let clk = self.clk_src.encode_clk();
                fuse_array.set((x + d * 9) as usize, y + 0, clk.0);
                fuse_array.set((x + d * 10) as usize, y + 0, clk.1);

                // clkfreq
                fuse_array.set((x + d * 11) as usize, y + 0, self.is_ddr);

                // clkop
                fuse_array.set((x + d * 12) as usize, y + 0, self.clk_invert_pol);

                // inreg
                fuse_array.set((x + d * 13) as usize, y + 0, !self.ff_in_ibuf);

                // pu
                fuse_array.set((x + d * 14) as usize, y + 0, !self.init_state);

                // xorin
                let xorin = self.xor_mode.encode();
                fuse_array.set((x + d * 0) as usize, y + 1, xorin.0);
                fuse_array.set((x + d * 1) as usize, y + 1, xorin.1);

                // skipped Oe (belongs to IOB)
                // skipped Slw (belongs to IOB)
                // skipped RegCom (belongs to IOB)

                // regmod
                let regmod = self.reg_mode.encode();
                fuse_array.set((x + d * 9) as usize, y + 1, regmod.0);
                fuse_array.set((x + d * 10) as usize, y + 1, regmod.1);

                // r
                let r = self.r_src.encode();
                fuse_array.set((x + d * 11) as usize, y + 1, r.0);
                fuse_array.set((x + d * 12) as usize, y + 1, r.1);

                // p
                let p = self.s_src.encode();
                fuse_array.set((x + d * 13) as usize, y + 1, p.0);
                fuse_array.set((x + d * 14) as usize, y + 1, p.1);
            }
        }
    }

    /// Reads the crbit representation of this macrocell from the given `fuse_array`.
    pub fn from_crbit(device: XC2Device, fb: u32, mc: u32, fuse_array: &FuseArray) -> Self {
        let (x, y, mirror) = mc_block_loc(device, fb);
        // direction
        let x = x as i32;
        let d = if !mirror {1} else {-1};
        match device {
            XC2Device::XC2C32 | XC2Device::XC2C32A => {
                // The "32" variant
                // each macrocell is 3 rows high
                let y = y + (mc as usize) * 3;

                // aclk
                let aclk = fuse_array.get((x + d * 0) as usize, y + 0);

                // clkop
                let clk_invert_pol = fuse_array.get((x + d * 1) as usize, y + 0);

                // clk
                let clk = (fuse_array.get((x + d * 2) as usize, y + 0),
                           fuse_array.get((x + d * 3) as usize, y + 0));

                // clkfreq
                let is_ddr = fuse_array.get((x + d * 4) as usize, y + 0);

                // r
                let r = (fuse_array.get((x + d * 5) as usize, y + 0),
                         fuse_array.get((x + d * 6) as usize, y + 0));

                // p
                let p = (fuse_array.get((x + d * 7) as usize, y + 0),
                         fuse_array.get((x + d * 8) as usize, y + 0));

                // regmod
                let regmod = (fuse_array.get((x + d * 0) as usize, y + 1),
                              fuse_array.get((x + d * 1) as usize, y + 1));

                // skipped INz (belongs to IOB)

                // fb
                let fb = (fuse_array.get((x + d * 4) as usize, y + 1),
                          fuse_array.get((x + d * 5) as usize, y + 1));

                // inreg
                let ff_in_ibuf = !fuse_array.get((x + d * 6) as usize, y + 1);

                // skipped St (belongs to IOB)

                // xorin
                let xorin = (fuse_array.get((x + d * 8) as usize, y + 1),
                             fuse_array.get((x + d * 0) as usize, y + 2));

                // skipped RegCom (belongs to IOB)
                // skipped Oe (belongs to IOB)
                // skipped Tm (belongs to IOB)
                // skipped Slw (belongs to IOB)

                // pu
                let init_state = !fuse_array.get((x + d * 8) as usize, y + 2);

                XC2Macrocell {
                    clk_src: XC2MCRegClkSrc::decode(aclk, clk),
                    clk_invert_pol,
                    is_ddr,
                    r_src: XC2MCRegResetSrc::decode(r),
                    s_src: XC2MCRegSetSrc::decode(p),
                    init_state,
                    reg_mode: XC2MCRegMode::decode(regmod),
                    fb_mode: XC2MCFeedbackMode::decode(fb),
                    ff_in_ibuf,
                    xor_mode: XC2MCXorMode::decode(xorin),
                }
            },
            XC2Device::XC2C64 | XC2Device::XC2C64A => {
                // The "64" variant
                // each macrocell is 3 rows high
                let y = y + (mc as usize) * 3;

                // aclk
                let aclk = fuse_array.get((x + d * 8) as usize, y + 0);

                // clkop
                let clk_invert_pol = fuse_array.get((x + d * 7) as usize, y + 0);

                // clk
                let clk = (fuse_array.get((x + d * 5) as usize, y + 0),
                           fuse_array.get((x + d * 6) as usize, y + 0));

                // clkfreq
                let is_ddr = fuse_array.get((x + d * 4) as usize, y + 0);

                // r
                let r = (fuse_array.get((x + d * 2) as usize, y + 0),
                         fuse_array.get((x + d * 3) as usize, y + 0));

                // p
                let p = (fuse_array.get((x + d * 0) as usize, y + 0),
                         fuse_array.get((x + d * 1) as usize, y + 0));

                // regmod
                let regmod = (fuse_array.get((x + d * 7) as usize, y + 1),
                              fuse_array.get((x + d * 8) as usize, y + 1));

                // skipped INz (belongs to IOB)

                // fb
                let fb = (fuse_array.get((x + d * 3) as usize, y + 1),
                          fuse_array.get((x + d * 4) as usize, y + 1));

                // inreg
                let ff_in_ibuf = !fuse_array.get((x + d * 2) as usize, y + 1);

                // skipped St (belongs to IOB)

                // xorin
                let xorin = (fuse_array.get((x + d * 7) as usize, y + 2),
                             fuse_array.get((x + d * 8) as usize, y + 2));

                // skipped RegCom (belongs to IOB)
                // skipped Oe (belongs to IOB)
                // skipped Tm (belongs to IOB)
                // skipped Slw (belongs to IOB)

                // pu
                let init_state = !fuse_array.get((x + d * 0) as usize, y + 2);

                XC2Macrocell {
                    clk_src: XC2MCRegClkSrc::decode(aclk, clk),
                    clk_invert_pol,
                    is_ddr,
                    r_src: XC2MCRegResetSrc::decode(r),
                    s_src: XC2MCRegSetSrc::decode(p),
                    init_state,
                    reg_mode: XC2MCRegMode::decode(regmod),
                    fb_mode: XC2MCFeedbackMode::decode(fb),
                    ff_in_ibuf,
                    xor_mode: XC2MCXorMode::decode(xorin),
                }
            },
            XC2Device::XC2C256 => {
                // The "256" variant
                // each macrocell is 3 rows high
                let y = y + (mc as usize) * 3;

                // skipped InMod (belongs to IOB)

                // fb
                let fb = (fuse_array.get((x + d * 2) as usize, y + 0),
                          fuse_array.get((x + d * 3) as usize, y + 0));

                // skipped DG (belongs to IOB)

                // clkop
                let clk_invert_pol = fuse_array.get((x + d * 5) as usize, y + 0);

                // clkfreq
                let is_ddr = fuse_array.get((x + d * 6) as usize, y + 0);

                // clk
                let clk = (fuse_array.get((x + d * 7) as usize, y + 0),
                           fuse_array.get((x + d * 8) as usize, y + 0));

                // aclk
                let aclk = fuse_array.get((x + d * 9) as usize, y + 0);

                // pu
                let init_state = !fuse_array.get((x + d * 0) as usize, y + 1);

                // p
                let p = (fuse_array.get((x + d * 1) as usize, y + 1),
                         fuse_array.get((x + d * 2) as usize, y + 1));

                // skipped Oe (belongs to IOB)
                // skipped INz (belongs to IOB)

                // inreg
                let ff_in_ibuf = !fuse_array.get((x + d * 9) as usize, y + 1);

                // xorin
                let xorin = (fuse_array.get((x + d * 0) as usize, y + 2),
                             fuse_array.get((x + d * 1) as usize, y + 2));

                // skipped Tm (belongs to IOB)
                // skipped Slw (belongs to IOB)

                // r
                let r = (fuse_array.get((x + d * 4) as usize, y + 2),
                         fuse_array.get((x + d * 5) as usize, y + 2));

                // regmod
                let regmod = (fuse_array.get((x + d * 6) as usize, y + 2),
                              fuse_array.get((x + d * 7) as usize, y + 2));

                // skipped RegCom (belongs to IOB)

                XC2Macrocell {
                    clk_src: XC2MCRegClkSrc::decode(aclk, clk),
                    clk_invert_pol,
                    is_ddr,
                    r_src: XC2MCRegResetSrc::decode(r),
                    s_src: XC2MCRegSetSrc::decode(p),
                    init_state,
                    reg_mode: XC2MCRegMode::decode(regmod),
                    fb_mode: XC2MCFeedbackMode::decode(fb),
                    ff_in_ibuf,
                    xor_mode: XC2MCXorMode::decode(xorin),
                }
            },
            XC2Device::XC2C128 | XC2Device::XC2C384 | XC2Device::XC2C512 => {
                // The "common large macrocell" variant
                // we need this funny lookup table, but otherwise macrocells are 2x15
                let y = y + MC_TO_ROW_MAP_LARGE[mc as usize];

                // skipped INz (belongs to IOB)

                // fb
                let fb = (fuse_array.get((x + d * 2) as usize, y + 0),
                          fuse_array.get((x + d * 3) as usize, y + 0));

                // skipped DG (belongs to IOB)
                // skipped InMod (belongs to IOB)
                // skipped Tm (belongs to IOB)

                // aclk
                let aclk = fuse_array.get((x + d * 8) as usize, y + 0);

                // clk
                let clk = (fuse_array.get((x + d * 9) as usize, y + 0),
                           fuse_array.get((x + d * 10) as usize, y + 0));

                // clkfreq
                let is_ddr = fuse_array.get((x + d * 11) as usize, y + 0);

                // clkop
                let clk_invert_pol = fuse_array.get((x + d * 12) as usize, y + 0);

                // inreg
                let ff_in_ibuf = !fuse_array.get((x + d * 13) as usize, y + 0);

                // pu
                let init_state = !fuse_array.get((x + d * 14) as usize, y + 0);

                // xorin
                let xorin = (fuse_array.get((x + d * 0) as usize, y + 1),
                             fuse_array.get((x + d * 1) as usize, y + 1));

                // skipped Oe (belongs to IOB)
                // skipped Slw (belongs to IOB)
                // skipped RegCom (belongs to IOB)

                // regmod
                let regmod = (fuse_array.get((x + d * 9) as usize, y + 1),
                              fuse_array.get((x + d * 10) as usize, y + 1));

                // r
                let r = (fuse_array.get((x + d * 11) as usize, y + 1),
                         fuse_array.get((x + d * 12) as usize, y + 1));

                // p
                let p = (fuse_array.get((x + d * 13) as usize, y + 1),
                         fuse_array.get((x + d * 14) as usize, y + 1));

                XC2Macrocell {
                    clk_src: XC2MCRegClkSrc::decode(aclk, clk),
                    clk_invert_pol,
                    is_ddr,
                    r_src: XC2MCRegResetSrc::decode(r),
                    s_src: XC2MCRegSetSrc::decode(p),
                    init_state,
                    reg_mode: XC2MCRegMode::decode(regmod),
                    fb_mode: XC2MCFeedbackMode::decode(fb),
                    ff_in_ibuf,
                    xor_mode: XC2MCXorMode::decode(xorin),
                }
            }
        }
    }
}

///  Internal function that reads only the macrocell-related bits from the macrcocell configuration
pub fn read_small_ff_logical(fuses: &[bool], block_idx: usize, ff_idx: usize) -> XC2Macrocell {
    let aclk = fuses[block_idx + ff_idx * 27 + 0];
    let clk = (fuses[block_idx + ff_idx * 27 + 2],
               fuses[block_idx + ff_idx * 27 + 3]);

    let clk_src = XC2MCRegClkSrc::decode(aclk, clk);

    let clkop = fuses[block_idx + ff_idx * 27 + 1];
    let clkfreq = fuses[block_idx + ff_idx * 27 + 4];

    let r = (fuses[block_idx + ff_idx * 27 + 5],
             fuses[block_idx + ff_idx * 27 + 6]);
    let reset_mode = XC2MCRegResetSrc::decode(r);

    let p = (fuses[block_idx + ff_idx * 27 + 7],
             fuses[block_idx + ff_idx * 27 + 8]);
    let set_mode = XC2MCRegSetSrc::decode(p);

    let regmod = (fuses[block_idx + ff_idx * 27 + 9],
                  fuses[block_idx + ff_idx * 27 + 10]);
    let reg_mode = XC2MCRegMode::decode(regmod);

    let fb = (fuses[block_idx + ff_idx * 27 + 13],
              fuses[block_idx + ff_idx * 27 + 14]);
    let fb_mode = XC2MCFeedbackMode::decode(fb);

    let inreg = fuses[block_idx + ff_idx * 27 + 15];

    let xorin = (fuses[block_idx + ff_idx * 27 + 17],
                 fuses[block_idx + ff_idx * 27 + 18]);
    let xormode = XC2MCXorMode::decode(xorin);

    let pu = fuses[block_idx + ff_idx * 27 + 26];

    XC2Macrocell {
        clk_src,
        clk_invert_pol: clkop,
        is_ddr: clkfreq,
        r_src: reset_mode,
        s_src: set_mode,
        init_state: !pu,
        reg_mode,
        fb_mode,
        ff_in_ibuf: !inreg,
        xor_mode: xormode,
    }
}

///  Internal function that reads only the macrocell-related bits from the macrcocell configuration
pub fn read_large_ff_logical(fuses: &[bool], fuse_idx: usize) -> XC2Macrocell {
    let aclk = fuses[fuse_idx + 0];

    let clk = (fuses[fuse_idx + 1],
               fuses[fuse_idx + 2]);

    let clk_src = XC2MCRegClkSrc::decode(aclk, clk);

    let clkfreq = fuses[fuse_idx + 3];
    let clkop = fuses[fuse_idx + 4];

    let fb = (fuses[fuse_idx + 6],
              fuses[fuse_idx + 7]);
    let fb_mode = XC2MCFeedbackMode::decode(fb);

    let inreg = fuses[fuse_idx + 10];

    let p = (fuses[fuse_idx + 17],
             fuses[fuse_idx + 18]);
    let set_mode = XC2MCRegSetSrc::decode(p);

    let pu = fuses[fuse_idx + 19];

    let regmod = (fuses[fuse_idx + 21],
                  fuses[fuse_idx + 22]);
    let reg_mode = XC2MCRegMode::decode(regmod);

    let r = (fuses[fuse_idx + 23],
             fuses[fuse_idx + 24]);
    let reset_mode = XC2MCRegResetSrc::decode(r);

    let xorin = (fuses[fuse_idx + 27],
                 fuses[fuse_idx + 28]);
    let xormode = XC2MCXorMode::decode(xorin);

    XC2Macrocell {
        clk_src,
        clk_invert_pol: clkop,
        is_ddr: clkfreq,
        r_src: reset_mode,
        s_src: set_mode,
        init_state: !pu,
        reg_mode,
        fb_mode,
        ff_in_ibuf: !inreg,
        xor_mode: xormode,
    }
}

///  Internal function that reads only the macrocell-related bits from the macrcocell configuration
pub fn read_large_buried_ff_logical(fuses: &[bool], fuse_idx: usize) -> XC2Macrocell {
    let aclk = fuses[fuse_idx + 0];

    let clk = (fuses[fuse_idx + 1],
               fuses[fuse_idx + 2]);

    let clk_src = XC2MCRegClkSrc::decode(aclk, clk);

    let clkfreq = fuses[fuse_idx + 3];
    let clkop = fuses[fuse_idx + 4];

    let fb = (fuses[fuse_idx + 5],
              fuses[fuse_idx + 6]);
    let fb_mode = XC2MCFeedbackMode::decode(fb);

    let p = (fuses[fuse_idx + 7],
             fuses[fuse_idx + 8]);
    let set_mode = XC2MCRegSetSrc::decode(p);

    let pu = fuses[fuse_idx + 9];

    let regmod = (fuses[fuse_idx + 10],
                  fuses[fuse_idx + 11]);
    let reg_mode = XC2MCRegMode::decode(regmod);

    let r = (fuses[fuse_idx + 12],
             fuses[fuse_idx + 13]);
    let reset_mode = XC2MCRegResetSrc::decode(r);

    let xorin = (fuses[fuse_idx + 14],
                 fuses[fuse_idx + 15]);
    let xormode = XC2MCXorMode::decode(xorin);

    XC2Macrocell {
        clk_src,
        clk_invert_pol: clkop,
        is_ddr: clkfreq,
        r_src: reset_mode,
        s_src: set_mode,
        init_state: !pu,
        reg_mode,
        fb_mode,
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
        write!(writer, "{}", b2s(fb.ffs[i].clk_src.encode_aclk()))?;

        // clkop
        write!(writer, "{}", b2s(fb.ffs[i].clk_invert_pol))?;

        // clk
        let clk = fb.ffs[i].clk_src.encode_clk();
        write!(writer, "{}{}", b2s(clk.0), b2s(clk.1))?;

        // clkfreq
        write!(writer, "{}", b2s(fb.ffs[i].is_ddr))?;

        // r
        let r = fb.ffs[i].r_src.encode();
        write!(writer, "{}{}", b2s(r.0), b2s(r.1))?;

        // p
        let p = fb.ffs[i].s_src.encode();
        write!(writer, "{}{}", b2s(p.0), b2s(p.1))?;

        // regmod
        let regmod = fb.ffs[i].reg_mode.encode();
        write!(writer, "{}{}", b2s(regmod.0), b2s(regmod.1))?;

        // inz
        let inz = iobs[iob].zia_mode.encode();
        write!(writer, "{}{}", b2s(inz.0), b2s(inz.1))?;

        // fb
        let fb_bits = fb.ffs[i].fb_mode.encode();
        write!(writer, "{}{}", b2s(fb_bits.0), b2s(fb_bits.1))?;

        // inreg
        write!(writer, "{}", b2s(!fb.ffs[i].ff_in_ibuf))?;

        // st
        write!(writer, "{}", b2s(iobs[iob].schmitt_trigger))?;

        // xorin
        let xorin = fb.ffs[i].xor_mode.encode();
        write!(writer, "{}{}", b2s(xorin.0), b2s(xorin.1))?;

        // regcom
        write!(writer, "{}", b2s(!iobs[iob].obuf_uses_ff))?;

        // oe
        let oe = iobs[iob].obuf_mode.encode();
        write!(writer, "{}{}{}{}",
            b2s(oe.0), b2s(oe.1),
            b2s(oe.2), b2s(oe.3))?;

        // tm
        write!(writer, "{}", b2s(iobs[iob].termination_enabled))?;

        // slw
        write!(writer, "{}", b2s(!iobs[iob].slew_is_fast))?;

        // pu
        write!(writer, "{}", b2s(!fb.ffs[i].init_state))?;

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
        write!(writer, "{}", b2s(fb.ffs[i].clk_src.encode_aclk()))?;

        // clk
        let clk = fb.ffs[i].clk_src.encode_clk();
        write!(writer, "{}{}", b2s(clk.0), b2s(clk.1))?;


        // clkfreq
        write!(writer, "{}", b2s(fb.ffs[i].is_ddr))?;

        // clkop
        write!(writer, "{}", b2s(fb.ffs[i].clk_invert_pol))?;

        // dg
        if iob.is_some() {
            write!(writer, "{}", b2s(iobs[iob.unwrap() as usize].uses_data_gate))?;
        }

        // fb
        let fb_bits = fb.ffs[i].fb_mode.encode();
        write!(writer, "{}{}", b2s(fb_bits.0), b2s(fb_bits.1))?;

        if iob.is_some() {
            let iob = iob.unwrap() as usize;

            // inmod
            let inmod = iobs[iob].ibuf_mode.encode();
            write!(writer, "{}{}", b2s(inmod.0), b2s(inmod.1))?;

            // inreg
            write!(writer, "{}", b2s(!fb.ffs[i].ff_in_ibuf))?;

            // inz
            let inz = iobs[iob].zia_mode.encode();
            write!(writer, "{}{}", b2s(inz.0), b2s(inz.1))?;

            // oe
            let oe = iobs[iob].obuf_mode.encode();
            write!(writer, "{}{}{}{}",
                b2s(oe.0), b2s(oe.1),
                b2s(oe.2), b2s(oe.3))?;
        }

        // p
        let p = fb.ffs[i].s_src.encode();
        write!(writer, "{}{}", b2s(p.0), b2s(p.1))?;

        // pu
        write!(writer, "{}", b2s(!fb.ffs[i].init_state))?;

        if iob.is_some() {
            // regcom
            write!(writer, "{}", b2s(!iobs[iob.unwrap() as usize].obuf_uses_ff))?;
        }

        // regmod
        let regmod = fb.ffs[i].reg_mode.encode();
        write!(writer, "{}{}", b2s(regmod.0), b2s(regmod.1))?;

        // r
        let r = fb.ffs[i].r_src.encode();
        write!(writer, "{}{}", b2s(r.0), b2s(r.1))?;

        if iob.is_some() {
            let iob = iob.unwrap() as usize;

            // slw
            write!(writer, "{}", b2s(!iobs[iob].slew_is_fast))?;

            // tm
            write!(writer, "{}", b2s(iobs[iob].termination_enabled))?;
        }

        // xorin
        let xorin = fb.ffs[i].xor_mode.encode();
        write!(writer, "{}{}", b2s(xorin.0), b2s(xorin.1))?;

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
