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
    pub fn dump_human_readable(&self, device: XC2Device, my_idx: u32, writer: &mut Write) -> Result<(), io::Error> {
        write!(writer, "\n")?;
        let (fb, ff) = iob_num_to_fb_ff_num(device, my_idx).unwrap();
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

/// Input mode selection on larger parts with VREF
#[derive(Copy, Clone, Eq, PartialEq, Debug)]
pub enum XC2IOBIbufMode {
    /// This input buffer is not using VREF, and it is also not using the Schmitt trigger
    NoVrefNoSt,
    /// This input buffer is not using VREF, but it is using the Schmitt trigger
    NoVrefSt,
    /// This input buffer is using VREF (supposedly it always has the Schmitt trigger?)
    UsesVref,
    /// This input pin is serving as VREF
    IsVref,
}

/// Represents an I/O pin on "large" (128 and greater macrocell) devices.
#[derive(Copy, Clone)]
pub struct XC2MCLargeIOB {
    /// Mux selection for the ZIA input for this pin
    pub zia_mode: XC2IOBZIAMode,
    /// Selects the input mode for this pin
    pub ibuf_mode: XC2IOBIbufMode,
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
    /// Whether this pin is making use of the DataGate feature
    pub uses_data_gate: bool,
}

impl Default for XC2MCLargeIOB {
    /// Returns a "default" I/O pin configuration. The default state is for the output and the input into the ZIA
    /// to be disabled.

    // FIXME: Do the other defaults come from the particular way I invoked the Xilinx tools??
    fn default() -> XC2MCLargeIOB {
        XC2MCLargeIOB {
            zia_mode: XC2IOBZIAMode::Disabled,
            ibuf_mode: XC2IOBIbufMode::NoVrefSt,
            obuf_uses_ff: false,
            obuf_mode: XC2IOBOBufMode::Disabled,
            termination_enabled: true,
            slew_is_fast: true,
            uses_data_gate: false,
        }
    }
}

impl XC2MCLargeIOB {
    /// Dump a human-readable explanation of the settings for this pin to the given `writer` object.
    /// `my_idx` must be the index of this I/O pin in the internal numbering scheme.
    pub fn dump_human_readable(&self, device: XC2Device, my_idx: u32, writer: &mut Write) -> Result<(), io::Error> {
        write!(writer, "\n")?;
        let (fb, ff) = iob_num_to_fb_ff_num(device, my_idx).unwrap();
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
        write!(writer, "input mode: {}\n", match self.ibuf_mode {
            XC2IOBIbufMode::NoVrefNoSt => "no VREF, no Schmitt trigger",
            XC2IOBIbufMode::NoVrefSt => "no VREF, Schmitt trigger",
            XC2IOBIbufMode::UsesVref => "uses VREF (HSTL/SSTL)",
            XC2IOBIbufMode::IsVref => "is a VREF pin",
        })?;
        write!(writer, "output comes from {}\n", if self.obuf_uses_ff {"FF"} else {"XOR gate"})?;
        write!(writer, "slew rate: {}\n", if self.slew_is_fast {"fast"} else {"slow"})?;
        write!(writer, "ZIA driven from: {}\n", match self.zia_mode {
            XC2IOBZIAMode::Disabled => "disabled",
            XC2IOBZIAMode::PAD => "input pad",
            XC2IOBZIAMode::REG => "register",
        })?;
        write!(writer, "termination: {}\n", if self.termination_enabled {"yes"} else {"no"})?;
        write!(writer, "DataGate used: {}\n", if self.uses_data_gate {"yes"} else {"no"})?;

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
pub fn iob_num_to_fb_ff_num(device: XC2Device, iob: u32) -> Option<(u32, u32)> {
    match device {
        XC2Device::XC2C32 | XC2Device::XC2C32A => {
            if iob >= 32 {
                None
            } else {
                Some((iob / MCS_PER_FB as u32, iob % MCS_PER_FB as u32))
            }
        },
        XC2Device::XC2C64 | XC2Device::XC2C64A => {
            if iob >= 64 {
                None
            } else {
                Some((iob / MCS_PER_FB as u32, iob % MCS_PER_FB as u32))
            }
        },
        XC2Device::XC2C128 => {
            match iob {
                // "Missing" 4 IOBs
                 0... 5 => Some((0, iob -  0 +  0)),
                 6...11 => Some((0, iob -  6 + 10)),
                12...17 => Some((1, iob - 12 +  0)),
                18...23 => Some((1, iob - 18 + 10)),
                // "Missing" 3 IOBs
                24...30 => Some((2, iob - 24 +  0)),
                31...36 => Some((2, iob - 31 + 10)),
                37...43 => Some((3, iob - 37 +  0)),
                44...49 => Some((3, iob - 44 + 10)),
                50...56 => Some((4, iob - 50 +  0)),
                57...62 => Some((4, iob - 57 + 10)),
                // "Missing" 4 IOBs
                63...68 => Some((5, iob - 63 +  0)),
                69...74 => Some((5, iob - 69 + 10)),
                // "Missing" 3 IOBs
                75...81 => Some((6, iob - 75 +  0)),
                82...87 => Some((6, iob - 82 + 10)),
                // "Missing" 4 IOBs
                88...93 => Some((7, iob - 88 +  0)),
                94...99 => Some((7, iob - 94 + 10)),
                _ => None,
            }
        }
        _ => unreachable!(),
    }
}

/// Function to map from a function block and macrocell number to the internal numbering scheme for I/O pins.
pub fn fb_ff_num_to_iob_num(device: XC2Device, fb: u32, ff: u32) -> Option<u32> {
    match device {
        XC2Device::XC2C32 | XC2Device::XC2C32A => {
            if fb >= 2 || ff >= MCS_PER_FB as u32 {
                None
            } else {
                Some(fb * MCS_PER_FB as u32 + ff)
            }
        },
        XC2Device::XC2C64 | XC2Device::XC2C64A => {
            if fb >= 4 || ff >= MCS_PER_FB as u32 {
                None
            } else {
                Some(fb * MCS_PER_FB as u32 + ff)
            }
        },
        XC2Device::XC2C128 => {
            match fb {
                // "Missing" 4 IOBs
                0 => match ff {
                    0...5   => Some(0 + ff),
                    10...15 => Some(6 + (ff - 10)),
                    _ => None,
                },
                1 => match ff {
                    0...5   => Some(12 + ff),
                    10...15 => Some(18 + (ff - 10)),
                    _ => None,
                },
                // "Missing" 3 IOBs
                2 => match ff {
                    0...6   => Some(24 + ff),
                    10...15 => Some(31 + (ff - 10)),
                    _ => None,
                },
                3 => match ff {
                    0...6   => Some(37 + ff),
                    10...15 => Some(44 + (ff - 10)),
                    _ => None,
                },
                4 => match ff {
                    0...6   => Some(50 + ff),
                    10...15 => Some(57 + (ff - 10)),
                    _ => None,
                },
                // "Missing" 4 IOBs
                5 => match ff {
                    0...5   => Some(63 + ff),
                    10...15 => Some(69 + (ff - 10)),
                    _ => None,
                },
                // "Missing" 3 IOBs
                6 => match ff {
                    0...6   => Some(75 + ff),
                    10...15 => Some(82 + (ff - 10)),
                    _ => None,
                },
                // "Missing" 4 IOBs
                7 => match ff {
                    0...5   => Some(88 + ff),
                    10...15 => Some(94 + (ff - 10)),
                    _ => None,
                },
                _ => None,
            }
        },
        XC2Device::XC2C256 => {
            match fb {
                // "Missing" 5 IOBs
                0 => match ff {
                    0...5   => Some(0 + ff),
                    11...15 => Some(6 + (ff - 11)),
                    _ => None,
                },
                1 => match ff {
                    0...5   => Some(11 + ff),
                    11...15 => Some(17 + (ff - 11)),
                    _ => None,
                },
                2 => match ff {
                    0...5   => Some(22 + ff),
                    11...15 => Some(28 + (ff - 11)),
                    _ => None,
                },
                3 => match ff {
                    0...5   => Some(33 + ff),
                    11...15 => Some(39 + (ff - 11)),
                    _ => None,
                },
                4 => match ff {
                    0...5   => Some(44 + ff),
                    11...15 => Some(50 + (ff - 11)),
                    _ => None,
                },
                5 => match ff {
                    0...5   => Some(55 + ff),
                    11...15 => Some(61 + (ff - 11)),
                    _ => None,
                },
                // "Missing" 4 IOBs
                6 => match ff {
                    0...5   => Some(66 + ff),
                    10...15 => Some(72 + (ff - 10)),
                    _ => None,
                },
                7 => match ff {
                    0...5   => Some(78 + ff),
                    10...15 => Some(84 + (ff - 10)),
                    _ => None,
                },
                8 => match ff {
                    0...5   => Some(90 + ff),
                    10...15 => Some(96 + (ff - 10)),
                    _ => None,
                },
                9 => match ff {
                    0...5   => Some(102 + ff),
                    10...15 => Some(108 + (ff - 10)),
                    _ => None,
                },
                10 => match ff {
                    0...5   => Some(114 + ff),
                    10...15 => Some(120 + (ff - 10)),
                    _ => None,
                },
                11 => match ff {
                    0...5   => Some(126 + ff),
                    10...15 => Some(132 + (ff - 10)),
                    _ => None,
                },
                // "Missing" 5 IOBs
                12 => match ff {
                    0...5   => Some(138 + ff),
                    11...15 => Some(144 + (ff - 11)),
                    _ => None,
                },
                13 => match ff {
                    0...5   => Some(149 + ff),
                    11...15 => Some(155 + (ff - 11)),
                    _ => None,
                },
                // "Missing" 4 IOBs
                14 => match ff {
                    0...5   => Some(160 + ff),
                    10...15 => Some(166 + (ff - 10)),
                    _ => None,
                },
                15 => match ff {
                    0...5   => Some(172 + ff),
                    10...15 => Some(178 + (ff - 10)),
                    _ => None,
                },
                _ => None,
            }
        },
        _ => unreachable!(),
    }
}

/// Internal function that reads only the IO-related bits from the macrocell configuration
pub fn read_small_iob_logical(fuses: &[bool], fuse_idx: usize) -> Result<XC2MCSmallIOB, &'static str> {
    let inz = (fuses[fuse_idx + 11],
               fuses[fuse_idx + 12]);
    let input_to_zia = match inz {
        (false, false) => XC2IOBZIAMode::PAD,
        (true, false) => XC2IOBZIAMode::REG,
        (_, true) => XC2IOBZIAMode::Disabled,
    };

    let st = fuses[fuse_idx + 16];
    let regcom = fuses[fuse_idx + 19];

    let oe = (fuses[fuse_idx + 20],
              fuses[fuse_idx + 21],
              fuses[fuse_idx + 22],
              fuses[fuse_idx + 23]);
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

    let tm = fuses[fuse_idx + 24];
    let slw = fuses[fuse_idx + 25];

    Ok(XC2MCSmallIOB {
        zia_mode: input_to_zia,
        schmitt_trigger: st,
        obuf_uses_ff: !regcom,
        obuf_mode: output_mode,
        termination_enabled: tm,
        slew_is_fast: !slw,
    })
}

/// Internal function that reads only the IO-related bits from the macrocell configuration
pub fn read_large_iob_logical(fuses: &[bool], fuse_idx: usize) -> Result<XC2MCLargeIOB, &'static str> {
    let dg = fuses[fuse_idx + 5];

    let inmod = (fuses[fuse_idx + 8],
                 fuses[fuse_idx + 9]);
    let input_mode = match inmod {
        (false, false) => XC2IOBIbufMode::NoVrefNoSt,
        (false, true)  => XC2IOBIbufMode::IsVref,
        (true, false)  => XC2IOBIbufMode::UsesVref,
        (true, true)   => XC2IOBIbufMode::NoVrefSt,
    };

    let inz = (fuses[fuse_idx + 11],
               fuses[fuse_idx + 12]);
    let input_to_zia = match inz {
        (false, false) => XC2IOBZIAMode::PAD,
        (true, false) => XC2IOBZIAMode::REG,
        (_, true) => XC2IOBZIAMode::Disabled,
    };

    let oe = (fuses[fuse_idx + 13],
              fuses[fuse_idx + 14],
              fuses[fuse_idx + 15],
              fuses[fuse_idx + 16]);
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

    let regcom = fuses[fuse_idx + 20];

    let slw = fuses[fuse_idx + 25];
    let tm = fuses[fuse_idx + 26];

    Ok(XC2MCLargeIOB {
        zia_mode: input_to_zia,
        ibuf_mode: input_mode,
        obuf_uses_ff: !regcom,
        obuf_mode: output_mode,
        termination_enabled: tm,
        slew_is_fast: !slw,
        uses_data_gate: dg,
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
