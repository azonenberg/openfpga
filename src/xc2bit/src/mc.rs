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

extern crate jedec;
use self::jedec::*;

use *;
use fusemap_physical::{mc_block_loc};
use util::{LinebreakSet};
use zia::{zia_get_row_width};

/// Clock source for the register in a macrocell
#[derive(Copy, Clone, Eq, PartialEq, Hash, Debug, Serialize, Deserialize)]
#[derive(BitPattern)]
pub enum XC2MCRegClkSrc {
    #[bits = "x00"]
    GCK0,
    #[bits = "x10"]
    GCK1,
    #[bits = "x01"]
    GCK2,
    #[bits = "011"]
    PTC,
    #[bits = "111"]
    CTC,
}

/// Reset source for the register in a macrocell
#[derive(Copy, Clone, Eq, PartialEq, Hash, Debug, Serialize, Deserialize)]
#[derive(BitPattern)]
pub enum XC2MCRegResetSrc {
    #[bits = "11"]
    Disabled,
    #[bits = "00"]
    PTA,
    #[bits = "01"]
    GSR,
    #[bits = "10"]
    CTR,
}

/// Set source for the register in a macrocell
#[derive(Copy, Clone, Eq, PartialEq, Hash, Debug, Serialize, Deserialize)]
#[derive(BitPattern)]
pub enum XC2MCRegSetSrc {
    #[bits = "11"]
    Disabled,
    #[bits = "00"]
    PTA,
    #[bits = "01"]
    GSR,
    #[bits = "10"]
    CTS,
}

/// Mode of the register in a macrocell.
#[derive(Copy, Clone, Eq, PartialEq, Hash, Debug, Serialize, Deserialize)]
#[derive(BitPattern)]
pub enum XC2MCRegMode {
    /// D-type flip-flop
    #[bits = "00"]
    DFF,
    /// Transparent latch
    #[bits = "01"]
    LATCH,
    /// Toggle flip-flop
    #[bits = "10"]
    TFF,
    /// D-type flip-flop with clock-enable pin
    #[bits = "11"]
    DFFCE,
}

/// Mux selection for the ZIA input from this macrocell. The ZIA input can be chosen to come from either the XOR gate
/// or from the output of the register.
#[derive(Copy, Clone, Eq, PartialEq, Hash, Debug, Serialize, Deserialize)]
#[derive(BitPattern)]
pub enum XC2MCFeedbackMode {
    #[bits = "X1"]
    Disabled,
    #[bits = "00"]
    COMB,
    #[bits = "10"]
    REG,
}

/// Mux selection for the "not from OR gate" input to the XOR gate. The XOR gate in a macrocell contains two inputs,
/// the output of the corresponding OR term from the PLA and a specific dedicated AND term from the PLA.
#[derive(Copy, Clone, Eq, PartialEq, Hash, Debug, Serialize, Deserialize)]
#[derive(BitPattern)]
pub enum XC2MCXorMode {
    /// A constant zero which results in this XOR outputting the value of the OR term
    #[bits = "00"]
    ZERO,
    /// A constant one which results in this XOR outputting the complement of the OR term
    #[bits = "11"]
    ONE,
    /// XOR the OR term with the special product term C
    #[bits = "10"]
    PTC,
    /// XNOR the OR term with the special product term C
    #[bits = "01"]
    PTCB,
}

/// Represents a macrocell.
#[derive(Copy, Clone, Eq, PartialEq, Hash, Debug, Serialize, Deserialize)]
#[derive(BitTwiddler)]
#[bittwiddler = "jed_internal_small"]
#[bittwiddler = "jed_internal_large"]
#[bittwiddler = "jed_internal_large_buried"]
#[bittwiddler = "crbit32 mirror0"]
#[bittwiddler = "crbit64 mirror0"]
#[bittwiddler = "crbit256 mirror0"]
#[bittwiddler = "crbit_large mirror0"]
pub struct XC2Macrocell {
    /// Clock source for the register
    #[bittwiddler_field = "jed_internal_small 0 2 3"]
    #[bittwiddler_field = "jed_internal_large 0 1 2"]
    #[bittwiddler_field = "jed_internal_large_buried 0 1 2"]
    #[bittwiddler_field = "crbit32 0|0 2|0 3|0"]
    #[bittwiddler_field = "crbit64 8|0 5|0 6|0"]
    #[bittwiddler_field = "crbit256 9|0 7|0 8|0"]
    #[bittwiddler_field = "crbit_large 8|0 9|0 10|0"]
    pub clk_src: XC2MCRegClkSrc,
    /// Specifies the clock polarity for the register
    ///
    /// `false` = rising edge triggered flip-flop, transparent-when-high latch
    ///
    /// `true` = falling edge triggered flip-flop, transparent-when-low latch
    #[bittwiddler_field = "jed_internal_small 1"]
    #[bittwiddler_field = "jed_internal_large 4"]
    #[bittwiddler_field = "jed_internal_large_buried 4"]
    #[bittwiddler_field = "crbit32 1|0"]
    #[bittwiddler_field = "crbit64 7|0"]
    #[bittwiddler_field = "crbit256 5|0"]
    #[bittwiddler_field = "crbit_large 12|0"]
    pub clk_invert_pol: bool,
    /// Specifies whether flip-flop are triggered on both clock edges
    ///
    /// It is currently unknown what happens when this is used on a transparent latch
    #[bittwiddler_field = "jed_internal_small 4"]
    #[bittwiddler_field = "jed_internal_large 3"]
    #[bittwiddler_field = "jed_internal_large_buried 3"]
    #[bittwiddler_field = "crbit32 4|0"]
    #[bittwiddler_field = "crbit64 4|0"]
    #[bittwiddler_field = "crbit256 6|0"]
    #[bittwiddler_field = "crbit_large 11|0"]
    pub is_ddr: bool,
    /// Reset source for the register
    #[bittwiddler_field = "jed_internal_small 5 6"]
    #[bittwiddler_field = "jed_internal_large 23 24"]
    #[bittwiddler_field = "jed_internal_large_buried 12 13"]
    #[bittwiddler_field = "crbit32 5|0 6|0"]
    #[bittwiddler_field = "crbit64 2|0 3|0"]
    #[bittwiddler_field = "crbit256 4|2 5|2"]
    #[bittwiddler_field = "crbit_large 11|1 12|1"]
    pub r_src: XC2MCRegResetSrc,
    /// Set source for the register
    #[bittwiddler_field = "jed_internal_small 7 8"]
    #[bittwiddler_field = "jed_internal_large 17 18"]
    #[bittwiddler_field = "jed_internal_large_buried 7 8"]
    #[bittwiddler_field = "crbit32 7|0 8|0"]
    #[bittwiddler_field = "crbit64 0|0 1|0"]
    #[bittwiddler_field = "crbit256 1|1 2|1"]
    #[bittwiddler_field = "crbit_large 13|1 14|1"]
    pub s_src: XC2MCRegSetSrc,
    /// Power-up state of the register
    ///
    /// `false` = init to 0, `true` = init to 1
    #[bittwiddler_field = "jed_internal_small !26"]
    #[bittwiddler_field = "jed_internal_large !19"]
    #[bittwiddler_field = "jed_internal_large_buried !9"]
    #[bittwiddler_field = "crbit32 !8|2"]
    #[bittwiddler_field = "crbit64 !0|2"]
    #[bittwiddler_field = "crbit256 !0|1"]
    #[bittwiddler_field = "crbit_large !14|0"]
    pub init_state: bool,
    /// Register mode
    #[bittwiddler_field = "jed_internal_small 9 10"]
    #[bittwiddler_field = "jed_internal_large 21 22"]
    #[bittwiddler_field = "jed_internal_large_buried 10 11"]
    #[bittwiddler_field = "crbit32 0|1 1|1"]
    #[bittwiddler_field = "crbit64 7|1 8|1"]
    #[bittwiddler_field = "crbit256 6|2 7|2"]
    #[bittwiddler_field = "crbit_large 9|1 10|1"]
    pub reg_mode: XC2MCRegMode,
    /// ZIA input mode for feedback from this macrocell
    #[bittwiddler_field = "jed_internal_small 13 14"]
    #[bittwiddler_field = "jed_internal_large 6 7"]
    #[bittwiddler_field = "jed_internal_large_buried 5 6"]
    #[bittwiddler_field = "crbit32 4|1 5|1"]
    #[bittwiddler_field = "crbit64 3|1 4|1"]
    #[bittwiddler_field = "crbit256 2|0 3|0"]
    #[bittwiddler_field = "crbit_large 2|0 3|0"]
    pub fb_mode: XC2MCFeedbackMode,
    /// Controls the input for the register
    ///
    /// `false` = use the output of the XOR gate (combinatorial path), `true` = use IOB direct path
    /// (`true` is illegal for buried macrocells in the larger devices)
    #[bittwiddler_field = "jed_internal_small !15"]
    #[bittwiddler_field = "jed_internal_large !10"]
    #[bittwiddler_field = "jed_internal_large_buried F"]
    #[bittwiddler_field = "crbit32 !6|1"]
    #[bittwiddler_field = "crbit64 !2|1"]
    #[bittwiddler_field = "crbit256 !9|1"]
    #[bittwiddler_field = "crbit_large !13|0"]
    pub ff_in_ibuf: bool,
    /// Controls the "other" (not from the OR term) input to the XOR gate
    #[bittwiddler_field = "jed_internal_small 17 18"]
    #[bittwiddler_field = "jed_internal_large 27 28"]
    #[bittwiddler_field = "jed_internal_large_buried 14 15"]
    #[bittwiddler_field = "crbit32 8|1 0|2"]
    #[bittwiddler_field = "crbit64 7|2 8|2"]
    #[bittwiddler_field = "crbit256 0|2 1|2"]
    #[bittwiddler_field = "crbit_large 0|1 1|1"]
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
    /// `fb` and `mc` must be the function block number and macrocell number of this macrocell.
    pub fn dump_human_readable<W: Write>(&self, fb: u32, mc: u32, mut writer: W) -> Result<(), io::Error> {
        write!(writer, "\n")?;
        write!(writer, "FF configuration for FB{}_{}\n", fb + 1, mc + 1)?;
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
        match device {
            XC2Device::XC2C32 | XC2Device::XC2C32A => {
                // The "32" variant
                // each macrocell is 3 rows high
                let y = y + (mc as usize) * 3;
                self.encode_crbit32(fuse_array, (x, y), mirror);
            },
            XC2Device::XC2C64 | XC2Device::XC2C64A => {
                // The "64" variant
                // each macrocell is 3 rows high
                let y = y + (mc as usize) * 3;
                self.encode_crbit64(fuse_array, (x, y), mirror);
            },
            XC2Device::XC2C256 => {
                // The "256" variant
                // each macrocell is 3 rows high
                let y = y + (mc as usize) * 3;
                self.encode_crbit256(fuse_array, (x, y), mirror);
            },
            XC2Device::XC2C128 | XC2Device::XC2C384 | XC2Device::XC2C512 => {
                // The "common large macrocell" variant
                // we need this funny lookup table, but otherwise macrocells are 2x15
                let y = y + MC_TO_ROW_MAP_LARGE[mc as usize];
                self.encode_crbit_large(fuse_array, (x, y), mirror);
            }
        }
    }

    /// Reads the crbit representation of this macrocell from the given `fuse_array`.
    pub fn from_crbit(device: XC2Device, fb: u32, mc: u32, fuse_array: &FuseArray) -> Self {
        let (x, y, mirror) = mc_block_loc(device, fb);
        match device {
            XC2Device::XC2C32 | XC2Device::XC2C32A => {
                // The "32" variant
                // each macrocell is 3 rows high
                let y = y + (mc as usize) * 3;
                Self::decode_crbit32(fuse_array, (x, y), mirror)
            },
            XC2Device::XC2C64 | XC2Device::XC2C64A => {
                // The "64" variant
                // each macrocell is 3 rows high
                let y = y + (mc as usize) * 3;
                Self::decode_crbit64(fuse_array, (x, y), mirror)
            },
            XC2Device::XC2C256 => {
                // The "256" variant
                // each macrocell is 3 rows high
                let y = y + (mc as usize) * 3;
                Self::decode_crbit256(fuse_array, (x, y), mirror)
            },
            XC2Device::XC2C128 | XC2Device::XC2C384 | XC2Device::XC2C512 => {
                // The "common large macrocell" variant
                // we need this funny lookup table, but otherwise macrocells are 2x15
                let y = y + MC_TO_ROW_MAP_LARGE[mc as usize];
                Self::decode_crbit_large(fuse_array, (x, y), mirror)
            }
        }
    }

    ///  Internal function that reads only the macrocell-related bits from the macrcocell configuration
    pub fn from_jed_small(fuses: &[bool], block_idx: usize, mc_idx: usize) -> Self {
        Self::decode_jed_internal_small(fuses, block_idx + mc_idx * 27)
    }

    ///  Internal function that reads only the macrocell-related bits from the macrcocell configuration
    pub fn from_jed_large(fuses: &[bool], fuse_idx: usize) -> Self {
        Self::decode_jed_internal_large(fuses, fuse_idx)
    }

    ///  Internal function that reads only the macrocell-related bits from the macrcocell configuration
    pub fn from_jed_large_buried(fuses: &[bool], fuse_idx: usize) -> Self {
        Self::decode_jed_internal_large_buried(fuses, fuse_idx)
    }

    /// Helper that prints the IOB and macrocell configuration on the "small" parts
    pub fn to_jed_small(jed: &mut JEDECFile, linebreaks: &mut LinebreakSet,
        device: XC2Device, fb: &XC2BitstreamFB, iobs: &[XC2MCSmallIOB], fb_i: usize, fuse_base: usize) {

        let zia_row_width = zia_get_row_width(device);

        for i in 0..MCS_PER_FB {
            let mc_fuse_base = fuse_base + zia_row_width * INPUTS_PER_ANDTERM +
                ANDTERMS_PER_FB * INPUTS_PER_ANDTERM * 2 + ANDTERMS_PER_FB * MCS_PER_FB + i * 27;
                
            linebreaks.add(mc_fuse_base);
            if i == 0 {
                linebreaks.add(mc_fuse_base);
            }

            let iob = fb_mc_num_to_iob_num(device, fb_i as u32, i as u32).unwrap() as usize;
            fb.mcs[i].encode_jed_internal_small(&mut jed.f, mc_fuse_base);
            iobs[iob].encode_jed_internal(&mut jed.f, mc_fuse_base);
        }
    }

    /// Helper that prints the IOB and macrocell configuration on the "large" parts
    pub fn to_jed_large(jed: &mut JEDECFile, linebreaks: &mut LinebreakSet,
        device: XC2Device, fb: &XC2BitstreamFB, iobs: &[XC2MCLargeIOB], fb_i: usize, fuse_base: usize) {

        let zia_row_width = zia_get_row_width(device);

        let mut current_fuse_offset = fuse_base + zia_row_width * INPUTS_PER_ANDTERM +
            ANDTERMS_PER_FB * INPUTS_PER_ANDTERM * 2 + ANDTERMS_PER_FB * MCS_PER_FB;

        linebreaks.add(current_fuse_offset);

        for i in 0..MCS_PER_FB {
            linebreaks.add(current_fuse_offset);

            let iob = fb_mc_num_to_iob_num(device, fb_i as u32, i as u32);

            if iob.is_some() {
                let iob = iob.unwrap() as usize;

                iobs[iob].encode_jed_internal(&mut jed.f, current_fuse_offset);
                fb.mcs[i].encode_jed_internal_large(&mut jed.f, current_fuse_offset);
                current_fuse_offset += 29;
            } else {
                fb.mcs[i].encode_jed_internal_large_buried(&mut jed.f, current_fuse_offset);
                current_fuse_offset += 16;
            }
        }
    }
}
