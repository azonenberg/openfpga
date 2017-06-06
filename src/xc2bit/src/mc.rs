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

pub enum XC2MCFFClkSrc {
    GCK0,
    GCK1,
    GCK2,
    PTC,
    CTC,
}

pub enum XC2MCFFRSSrc {
    DISABLED,
    PTA,
    GSR,
    CTS,
}

pub enum XC2MCFFMode {
    DFF,
    LATCH,
    TFF,
    DFF_CE,
}

pub enum XC2MCFeedbackMode {
    DISABLED,
    COMB,
    REG,
}

pub enum XC2MCXorMode {
    ZERO,
    ONE,
    PTC,
    PTC_B,
}

pub enum XC2MCOBufMode {
    DISABLED,
    PUSH_PULL,
    OPEN_DRAIN,
    TRI_STATE_GTS0,
    TRI_STATE_GTS1,
    TRI_STATE_GTS2,
    TRI_STATE_GTS3,
    TRI_STATE_PTB,
    TRI_STATE_CTE,
    CGND,
}

pub struct XC2MCFF {
    clk_src: XC2MCFFClkSrc,
    // false = rising edge triggered, true = falling edge triggered
    falling_edge: bool,
    is_ddr: bool,
    r_src: XC2MCFFRSSrc,
    s_src: XC2MCFFRSSrc,
    // false = init to 0, true = init to 1
    init_state: bool,
    ff_mode: XC2MCFFMode,
    fb_mode: XC2MCFeedbackMode,
    // false = use xor gate/PLA, true = use IOB direct path
    // true is illegal for buried FFs
    ff_in_ibuf: bool,
    xor_mode: XC2MCXorMode,
}

pub struct XC2MCSmallIOB {
    // FIXME: Mystery bit here
    ibuf_to_zia: bool,
    schmitt_trigger: bool,
    // false = uses xor gate, true = uses FF output
    obuf_uses_ff: bool,
    obuf_mode: XC2MCOBufMode,
    termination_enabled: bool,
    slew_is_fast: bool,
}

// Weird additional input-only pin
pub struct XC2ExtraIBuf {
    schmitt_trigger: bool,
    termination_enabled: bool,
}
