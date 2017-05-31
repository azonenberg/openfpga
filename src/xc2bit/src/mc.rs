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

// Macrocell stuff

#[derive(Copy, Clone)]
pub enum XC2MCFFClkSrc {
    GCK0,
    GCK1,
    GCK2,
    PTC,
    CTC,
}

#[derive(Copy, Clone)]
pub enum XC2MCFFResetSrc {
    Disabled,
    PTA,
    GSR,
    CTR,
}

#[derive(Copy, Clone)]
pub enum XC2MCFFSetSrc {
    Disabled,
    PTA,
    GSR,
    CTS,
}

#[derive(Copy, Clone)]
pub enum XC2MCFFMode {
    DFF,
    LATCH,
    TFF,
    DFFCE,
}

#[derive(Copy, Clone)]
pub enum XC2MCFeedbackMode {
    Disabled,
    COMB,
    REG,
}

#[derive(Copy, Clone)]
pub enum XC2MCXorMode {
    ZERO,
    ONE,
    PTC,
    PTCB,
}

#[derive(Copy, Clone)]
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
pub struct XC2MCFF {
    pub clk_src: XC2MCFFClkSrc,
    // false = rising edge triggered, true = falling edge triggered
    pub falling_edge: bool,
    pub is_ddr: bool,
    pub r_src: XC2MCFFResetSrc,
    pub s_src: XC2MCFFSetSrc,
    // false = init to 0, true = init to 1
    pub init_state: bool,
    pub ff_mode: XC2MCFFMode,
    pub fb_mode: XC2MCFeedbackMode,
    // false = use xor gate/PLA, true = use IOB direct path
    // true is illegal for buried FFs
    pub ff_in_ibuf: bool,
    pub xor_mode: XC2MCXorMode,
}

impl Default for XC2MCFF {
    fn default() -> XC2MCFF {
        XC2MCFF {
            clk_src: XC2MCFFClkSrc::GCK0,
            falling_edge: false,
            is_ddr: false,
            r_src: XC2MCFFResetSrc::Disabled,
            s_src: XC2MCFFSetSrc::Disabled,
            init_state: false,
            ff_mode: XC2MCFFMode::DFF,
            fb_mode: XC2MCFeedbackMode::Disabled,
            ff_in_ibuf: false,
            xor_mode: XC2MCXorMode::ZERO,
        }
    }
}

#[derive(Copy, Clone)]
pub struct XC2MCSmallIOB {
    // FIXME: Mystery bit here
    pub ibuf_to_zia: bool,
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
            ibuf_to_zia: false,
            schmitt_trigger: true,
            obuf_uses_ff: false,
            obuf_mode: XC2MCOBufMode::Disabled,
            termination_enabled: true,
            slew_is_fast: true,
        }
    }
}

// Weird additional input-only pin
pub struct XC2ExtraIBuf {
    pub schmitt_trigger: bool,
    pub termination_enabled: bool,
}

// Read only the FF-related bits
pub fn read_32_ff_logical(fuses: &[bool], block_idx: usize, ff_idx: usize) -> XC2MCFF {
    let aclk = fuses[block_idx + ff_idx * 27 + 0];
    let clk = (fuses[block_idx + ff_idx * 27 + 2],
               fuses[block_idx + ff_idx * 27 + 3]);

    let clk_src = match clk {
        (false, false) => XC2MCFFClkSrc::GCK0,
        (false, true)  => XC2MCFFClkSrc::GCK1,
        (true, false)  => XC2MCFFClkSrc::GCK2,
        (true, true)   => match aclk {
            true => XC2MCFFClkSrc::CTC,
            false => XC2MCFFClkSrc::PTC,
        },
    };

    let clkop = fuses[block_idx + ff_idx * 27 + 1];
    let clkfreq = fuses[block_idx + ff_idx * 27 + 4];

    let r = (fuses[block_idx + ff_idx * 27 + 5],
             fuses[block_idx + ff_idx * 27 + 6]);
    let reset_mode = match r {
        (false, false) => XC2MCFFResetSrc::PTA,
        (false, true)  => XC2MCFFResetSrc::GSR,
        (true, false)  => XC2MCFFResetSrc::CTR,
        (true, true)   => XC2MCFFResetSrc::Disabled,
    };

    let p = (fuses[block_idx + ff_idx * 27 + 7],
             fuses[block_idx + ff_idx * 27 + 8]);
    let set_mode = match p {
        (false, false) => XC2MCFFSetSrc::PTA,
        (false, true)  => XC2MCFFSetSrc::GSR,
        (true, false)  => XC2MCFFSetSrc::CTS,
        (true, true)   => XC2MCFFSetSrc::Disabled,
    };

    let regmod = (fuses[block_idx + ff_idx * 27 + 9],
                  fuses[block_idx + ff_idx * 27 + 10]);
    let ff_mode = match regmod {
        (false, false) => XC2MCFFMode::DFF,
        (false, true)  => XC2MCFFMode::LATCH,
        (true, false)  => XC2MCFFMode::TFF,
        (true, true)   => XC2MCFFMode::DFFCE,
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

    XC2MCFF {
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

// Read only the IO-related bits
pub fn read_32_iob_logical(fuses: &[bool], block_idx: usize, io_idx: usize) -> Result<XC2MCSmallIOB, &'static str> {
    let inz = (fuses[block_idx + io_idx * 27 + 11],
               fuses[block_idx + io_idx * 27 + 12]);
    let input_to_zia = match inz {
        (false, false) => true,
        (true, true) => false,
        _ => return Err("unknown INz mode used"),
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
        ibuf_to_zia: input_to_zia,
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
