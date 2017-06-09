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

//! Contains functions pertaining to the I/O pins

use std::io;
use std::io::Write;

use *;

/// Mux selection for the ZIA input from this I/O pin's input. The ZIA input can be chosen to come from either the
/// input pin directly or from the output of the register in the macrocell corresponding to this I/O pin. The latter
/// is used to allow for buried combinatorial feedback in a macrocell without "wasting" the register.
#[derive(Copy, Clone, Eq, PartialEq, Debug)]
pub enum XC2IOBZIAMode {
    Disabled,
    PAD,
    REG,
}

/// Mode selection for the I/O pin's output buffer. See the Xilinx Coolrunner-II documentation for more information.
#[derive(Copy, Clone, Eq, PartialEq, Debug)]
pub enum XC2IOBOBufMode {
    Disabled,
    PushPull,
    OpenDrain,
    TriStateGTS0,
    TriStateGTS1,
    TriStateGTS2,
    TriStateGTS3,
    TriStatePTB,
    TriStateCTE,
    CGND,
}

/// Represents an I/O pin on "small" (32 and 64 macrocell) devices.
#[derive(Copy, Clone)]
pub struct XC2MCSmallIOB {
    /// Mux selection for the ZIA input for this pin
    pub zia_mode: XC2IOBZIAMode,
    /// Whether the Schmitt trigger is being used on this pin's input
    pub schmitt_trigger: bool,
    /// Selects the source used to drive this pin's output (if the output is enabled).
    /// `false` selects the XOR gate in the macrocell (combinatorial output), and `true` selects the register output
    /// (registered output).
    pub obuf_uses_ff: bool,
    /// Selects the output mode for this pin
    pub obuf_mode: XC2IOBOBufMode,
    /// Selects if the global termination (bus hold or pull-up) is enabled on this pin
    pub termination_enabled: bool,
    /// Selects if fast slew rate is used on this pin
    pub slew_is_fast: bool,
}

impl Default for XC2MCSmallIOB {
    /// Returns a "default" I/O pin configuration. The default state is for the output and the input into the ZIA
    /// to be disabled.

    // FIXME: Do the other defaults come from the particular way I invoked the Xilinx tools??
    fn default() -> XC2MCSmallIOB {
        XC2MCSmallIOB {
            zia_mode: XC2IOBZIAMode::Disabled,
            schmitt_trigger: true,
            obuf_uses_ff: false,
            obuf_mode: XC2IOBOBufMode::Disabled,
            termination_enabled: true,
            slew_is_fast: true,
        }
    }
}

impl XC2MCSmallIOB {
    /// Dump a human-readable explanation of the settings for this pin to the given `writer` object.
    /// `my_idx` must be the index of this I/O pin in the internal numbering scheme.
    pub fn dump_human_readable(&self, my_idx: u32, writer: &mut Write) -> Result<(), io::Error> {
        write!(writer, "\n")?;
        let (fb, ff) = iob_num_to_fb_ff_num_32(my_idx).unwrap();
        write!(writer, "I/O configuration for FB{}_{}\n", fb + 1, ff + 1)?;
        write!(writer, "output mode: {}\n", match self.obuf_mode {
            XC2IOBOBufMode::Disabled => "disabled",
            XC2IOBOBufMode::PushPull => "push-pull",
            XC2IOBOBufMode::OpenDrain => "open-drain",
            XC2IOBOBufMode::TriStateGTS0 => "GTS0-controlled tri-state",
            XC2IOBOBufMode::TriStateGTS1 => "GTS1-controlled tri-state",
            XC2IOBOBufMode::TriStateGTS2 => "GTS2-controlled tri-state",
            XC2IOBOBufMode::TriStateGTS3 => "GTS3-controlled tri-state",
            XC2IOBOBufMode::TriStatePTB => "PTB-controlled tri-state",
            XC2IOBOBufMode::TriStateCTE => "CTE-controlled tri-state",
            XC2IOBOBufMode::CGND => "CGND",
        })?;
        write!(writer, "output comes from {}\n", if self.obuf_uses_ff {"FF"} else {"XOR gate"})?;
        write!(writer, "slew rate: {}\n", if self.slew_is_fast {"fast"} else {"slow"})?;
        write!(writer, "ZIA driven from: {}\n", match self.zia_mode {
            XC2IOBZIAMode::Disabled => "disabled",
            XC2IOBZIAMode::PAD => "input pad",
            XC2IOBZIAMode::REG => "register",
        })?;
        write!(writer, "Schmitt trigger input: {}\n", if self.schmitt_trigger {"yes"} else {"no"})?;
        write!(writer, "termination: {}\n", if self.termination_enabled {"yes"} else {"no"})?;

        Ok(())
    }
}

/// Represents the one additional special input-only pin on 32-macrocell devices.
pub struct XC2ExtraIBuf {
    pub schmitt_trigger: bool,
    pub termination_enabled: bool,
}

impl Default for XC2ExtraIBuf {
    /// Returns a "default" pin configuration.

    // FIXME: Do the other defaults come from the particular way I invoked the Xilinx tools??
    fn default() -> XC2ExtraIBuf {
        XC2ExtraIBuf {
            schmitt_trigger: true,
            termination_enabled: true,
        }
    }
}

impl XC2ExtraIBuf {
    /// Dump a human-readable explanation of the settings for this pin to the given `writer` object.
    pub fn dump_human_readable(&self, writer: &mut Write) -> Result<(), io::Error> {
        write!(writer, "\n")?;
        write!(writer, "I/O configuration for input-only pin\n")?;
        write!(writer, "Schmitt trigger input: {}\n", if self.schmitt_trigger {"yes"} else {"no"})?;
        write!(writer, "termination: {}\n", if self.termination_enabled {"yes"} else {"no"})?;

        Ok(())
    }
}

/// Function to map from the internal numbering scheme for I/O pins to a function block and macrocell number.
/// This function is for the 32-macrocell devices.
pub fn iob_num_to_fb_ff_num_32(iob: u32) -> Option<(u32, u32)> {
    if iob >= 32 {
        None
    } else {
        Some((iob / MCS_PER_FB as u32, iob % MCS_PER_FB as u32))
    }
}

/// Function to map from a function block and macrocell number to the internal numbering scheme for I/O pins.
/// This function is for the 32-macrocell devices.
pub fn fb_ff_num_to_iob_num_32(fb: u32, ff: u32) -> Option<u32> {
    if fb >= 2 || ff >= MCS_PER_FB as u32 {
        None
    } else {
        Some(fb * MCS_PER_FB as u32 + ff)
    }
}

/// Function to map from the internal numbering scheme for I/O pins to a function block and macrocell number.
/// This function is for the 64-macrocell devices.
pub fn iob_num_to_fb_ff_num_64(iob: u32) -> Option<(u32, u32)> {
    if iob >= 64 {
        None
    } else {
        Some((iob / MCS_PER_FB as u32, iob % MCS_PER_FB as u32))
    }
}

/// Function to map from a function block and macrocell number to the internal numbering scheme for I/O pins.
/// This function is for the 64-macrocell devices.
pub fn fb_ff_num_to_iob_num_64(fb: u32, ff: u32) -> Option<u32> {
    if fb >= 4 || ff >= MCS_PER_FB as u32 {
        None
    } else {
        Some(fb * MCS_PER_FB as u32 + ff)
    }
}

/// Internal function that reads only the IO-related bits from the macrocell configuration
pub fn read_32_iob_logical(fuses: &[bool], block_idx: usize, io_idx: usize) -> Result<XC2MCSmallIOB, &'static str> {
    let inz = (fuses[block_idx + io_idx * 27 + 11],
               fuses[block_idx + io_idx * 27 + 12]);
    let input_to_zia = match inz {
        (false, false) => XC2IOBZIAMode::PAD,
        (true, false) => XC2IOBZIAMode::REG,
        (_, true) => XC2IOBZIAMode::Disabled,
    };

    let st = fuses[block_idx + io_idx * 27 + 16];
    let regcom = fuses[block_idx + io_idx * 27 + 19];

    let oe = (fuses[block_idx + io_idx * 27 + 20],
              fuses[block_idx + io_idx * 27 + 21],
              fuses[block_idx + io_idx * 27 + 22],
              fuses[block_idx + io_idx * 27 + 23]);
    let output_mode = match oe {
        (false, false, false, false) => XC2IOBOBufMode::PushPull,
        (false, false, false, true)  => XC2IOBOBufMode::OpenDrain,
        (false, false, true, false)  => XC2IOBOBufMode::TriStateGTS1,
        (false, true, false, false)  => XC2IOBOBufMode::TriStatePTB,
        (false, true, true, false)   => XC2IOBOBufMode::TriStateGTS3,
        (true, false, false, false)  => XC2IOBOBufMode::TriStateCTE,
        (true, false, true, false)   => XC2IOBOBufMode::TriStateGTS2,
        (true, true, false, false)   => XC2IOBOBufMode::TriStateGTS0,
        (true, true, true, false)    => XC2IOBOBufMode::CGND,
        (true, true, true, true)     => XC2IOBOBufMode::Disabled,
        _ => return Err("unknown Oe mode used"),
    };

    let tm = fuses[block_idx + io_idx * 27 + 24];
    let slw = fuses[block_idx + io_idx * 27 + 25];

    Ok(XC2MCSmallIOB {
        zia_mode: input_to_zia,
        schmitt_trigger: st,
        obuf_uses_ff: !regcom,
        obuf_mode: output_mode,
        termination_enabled: tm,
        slew_is_fast: !slw,
    })
}

/// Internal function that reads only the input-only pin configuration
pub fn read_32_extra_ibuf_logical(fuses: &[bool]) -> XC2ExtraIBuf {
    let st = fuses[12272];
    let tm = fuses[12273];

    XC2ExtraIBuf {
        schmitt_trigger: st,
        termination_enabled: tm,
    }
}
