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

// I/Os

use std::io::Write;

use *;

#[derive(Copy, Clone, Eq, PartialEq, Debug)]
pub enum XC2IOBZIAMode {
    Disabled,
    PAD,
    REG,
}

#[derive(Copy, Clone, Eq, PartialEq, Debug)]
pub enum XC2MCOBufMode {
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

#[derive(Copy, Clone)]
pub struct XC2MCSmallIOB {
    pub zia_mode: XC2IOBZIAMode,
    pub schmitt_trigger: bool,
    // false = uses xor gate, true = uses FF output
    pub obuf_uses_ff: bool,
    pub obuf_mode: XC2MCOBufMode,
    pub termination_enabled: bool,
    pub slew_is_fast: bool,
}

impl Default for XC2MCSmallIOB {
    fn default() -> XC2MCSmallIOB {
        XC2MCSmallIOB {
            zia_mode: XC2IOBZIAMode::Disabled,
            schmitt_trigger: true,
            obuf_uses_ff: false,
            obuf_mode: XC2MCOBufMode::Disabled,
            termination_enabled: true,
            slew_is_fast: true,
        }
    }
}

impl XC2MCSmallIOB {
    pub fn dump_human_readable(&self, my_idx: u32, writer: &mut Write) {
        write!(writer, "\n").unwrap();
        let (fb, ff) = iob_num_to_fb_ff_num_32(my_idx).unwrap();
        write!(writer, "I/O configuration for FB{}_{}\n", fb + 1, ff + 1).unwrap();
        write!(writer, "output mode: {}\n", match self.obuf_mode {
            XC2MCOBufMode::Disabled => "disabled",
            XC2MCOBufMode::PushPull => "push-pull",
            XC2MCOBufMode::OpenDrain => "open-drain",
            XC2MCOBufMode::TriStateGTS0 => "GTS0-controlled tri-state",
            XC2MCOBufMode::TriStateGTS1 => "GTS1-controlled tri-state",
            XC2MCOBufMode::TriStateGTS2 => "GTS2-controlled tri-state",
            XC2MCOBufMode::TriStateGTS3 => "GTS3-controlled tri-state",
            XC2MCOBufMode::TriStatePTB => "PTB-controlled tri-state",
            XC2MCOBufMode::TriStateCTE => "CTE-controlled tri-state",
            XC2MCOBufMode::CGND => "CGND",
        }).unwrap();
        write!(writer, "output comes from {}\n", if self.obuf_uses_ff {"FF"} else {"XOR gate"}).unwrap();
        write!(writer, "slew rate: {}\n", if self.slew_is_fast {"fast"} else {"slow"}).unwrap();
        write!(writer, "ZIA driven from: {}\n", match self.zia_mode {
            XC2IOBZIAMode::Disabled => "disabled",
            XC2IOBZIAMode::PAD => "input pad",
            XC2IOBZIAMode::REG => "register",
        }).unwrap();
        write!(writer, "Schmitt trigger input: {}\n", if self.schmitt_trigger {"yes"} else {"no"}).unwrap();
        write!(writer, "termination: {}\n", if self.termination_enabled {"yes"} else {"no"}).unwrap();
    }
}

// Weird additional input-only pin
pub struct XC2ExtraIBuf {
    pub schmitt_trigger: bool,
    pub termination_enabled: bool,
}

impl Default for XC2ExtraIBuf {
    fn default() -> XC2ExtraIBuf {
        XC2ExtraIBuf {
            schmitt_trigger: true,
            termination_enabled: true,
        }
    }
}

impl XC2ExtraIBuf {
    pub fn dump_human_readable(&self, writer: &mut Write) {
        write!(writer, "\n").unwrap();
        write!(writer, "I/O configuration for input-only pin\n").unwrap();
        write!(writer, "Schmitt trigger input: {}\n", if self.schmitt_trigger {"yes"} else {"no"}).unwrap();
        write!(writer, "termination: {}\n", if self.termination_enabled {"yes"} else {"no"}).unwrap();
    }
}

pub fn iob_num_to_fb_ff_num_32(iob: u32) -> Option<(u32, u32)> {
    if iob >= 32 {
        None
    } else {
        Some((iob / MCS_PER_FB as u32, iob % MCS_PER_FB as u32))
    }
}

pub fn fb_ff_num_to_iob_num_32(fb: u32, ff: u32) -> Option<u32> {
    if fb >= 2 || ff >= MCS_PER_FB as u32 {
        None
    } else {
        Some(fb * MCS_PER_FB as u32 + ff)
    }
}

// Read only the IO-related bits
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
        (false, false, false, false) => XC2MCOBufMode::PushPull,
        (false, false, false, true)  => XC2MCOBufMode::OpenDrain,
        (false, false, true, false)  => XC2MCOBufMode::TriStateGTS1,
        (false, true, false, false)  => XC2MCOBufMode::TriStatePTB,
        (false, true, true, false)   => XC2MCOBufMode::TriStateGTS3,
        (true, false, false, false)  => XC2MCOBufMode::TriStateCTE,
        (true, false, true, false)   => XC2MCOBufMode::TriStateGTS2,
        (true, true, false, false)   => XC2MCOBufMode::TriStateGTS0,
        (true, true, true, false)    => XC2MCOBufMode::CGND,
        (true, true, true, true)     => XC2MCOBufMode::Disabled,
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

pub fn read_32_extra_ibuf_logical(fuses: &[bool]) -> XC2ExtraIBuf {
    let st = fuses[12272];
    let tm = fuses[12273];

    XC2ExtraIBuf {
        schmitt_trigger: st,
        termination_enabled: tm,
    }
}
