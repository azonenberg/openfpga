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

// Toplevel bitstrem stuff

use std::io::Write;

use *;
use fb::{read_32_fb_logical};
use mc::{read_32_iob_logical, read_32_extra_ibuf_logical, fb_ff_num_to_iob_num_32};
use zia::{encode_32_zia_choice};

pub struct XC2Bitstream {
    pub speed_grade: String,
    pub package: String,
    pub bits: XC2BitstreamBits,
}

impl XC2Bitstream {
    pub fn dump_human_readable(&self, writer: &mut Write) {
        write!(writer, "xc2bit dump\n").unwrap();
        write!(writer, "device speed grade: {}\n", self.speed_grade).unwrap();
        write!(writer, "device package: {}\n", self.package).unwrap();
        self.bits.dump_human_readable(writer);
    }

    pub fn write_jed(&self, writer: &mut Write) {
        write!(writer, ".JED fuse map written by xc2bit\n").unwrap();
        write!(writer, "https://github.com/azonenberg/openfpga\n\n").unwrap();
        write!(writer, "\x02").unwrap();

        match self.bits {
            XC2BitstreamBits::XC2C32{..} => {
                write!(writer, "QF12274*\n").unwrap();
                write!(writer, "N DEVICE XC2C32-{}-{}*\n\n", self.speed_grade, self.package).unwrap();
            },
            XC2BitstreamBits::XC2C32A{..} => {
                write!(writer, "QF12278*\n").unwrap();
                write!(writer, "N DEVICE XC2C32A-{}-{}*\n\n", self.speed_grade, self.package).unwrap();
            },
        }

        self.bits.write_jed(writer);

        write!(writer, "\x030000\n").unwrap();
    }

    pub fn blank_bitstream(device: &str, speed_grade: &str, package: &str) -> Result<XC2Bitstream, &'static str> {
        // TODO: Validate speed_grade and package

        match device {
            "XC2C32" => {
                Ok(XC2Bitstream {
                    speed_grade: speed_grade.to_owned(),
                    package: package.to_owned(),
                    bits: XC2BitstreamBits::XC2C32 {
                        fb: [XC2BistreamFB::default(); 2],
                        iobs: [XC2MCSmallIOB::default(); 32],
                        inpin: XC2ExtraIBuf::default(),
                        global_nets: XC2GlobalNets::default(),
                        ivoltage: false,
                        ovoltage: false,
                    }
                })
            },
            "XC2C32A" => {
                Ok(XC2Bitstream {
                    speed_grade: speed_grade.to_owned(),
                    package: package.to_owned(),
                    bits: XC2BitstreamBits::XC2C32A {
                        fb: [XC2BistreamFB::default(); 2],
                        iobs: [XC2MCSmallIOB::default(); 32],
                        inpin: XC2ExtraIBuf::default(),
                        global_nets: XC2GlobalNets::default(),
                        legacy_ivoltage: false,
                        legacy_ovoltage: false,
                        ivoltage: [false, false],
                        ovoltage: [false, false],
                    }
                })
            },
            _ => Err("invalid device")
        }
    }
}

pub struct XC2GlobalNets {
    pub gck_enable: [bool; 3],
    pub gsr_enable: bool,
    // false = active low, true = active high
    pub gsr_invert: bool,
    pub gts_enable: [bool; 4],
    // false = used as T, true = used as !T
    pub gts_invert: [bool; 4],
    // false = keeper, true = pull-up
    pub global_pu: bool,
}

impl Default for XC2GlobalNets {
    fn default() -> XC2GlobalNets {
        XC2GlobalNets {
            gck_enable: [false; 3],
            gsr_enable: false,
            gsr_invert: false,
            gts_enable: [false; 4],
            gts_invert: [true; 4],
            global_pu: true,
        }
    }
}

impl XC2GlobalNets {
    pub fn dump_human_readable(&self, writer: &mut Write) {
        write!(writer, "\n").unwrap();
        write!(writer, "GCK0 {}\n", if self.gck_enable[0] {"enabled"} else {"disabled"}).unwrap();
        write!(writer, "GCK1 {}\n", if self.gck_enable[1] {"enabled"} else {"disabled"}).unwrap();
        write!(writer, "GCK2 {}\n", if self.gck_enable[2] {"enabled"} else {"disabled"}).unwrap();

        write!(writer, "GSR {}, active {}\n",
            if self.gsr_enable {"enabled"} else {"disabled"},
            if self.gsr_invert {"high"} else {"low"}).unwrap();

        write!(writer, "GTS0 {}, acts as {}\n",
            if self.gts_enable[0] {"enabled"} else {"disabled"},
            if self.gts_invert[0] {"!T"} else {"T"}).unwrap();
        write!(writer, "GTS1 {}, acts as {}\n",
            if self.gts_enable[1] {"enabled"} else {"disabled"},
            if self.gts_invert[1] {"!T"} else {"T"}).unwrap();
        write!(writer, "GTS2 {}, acts as {}\n",
            if self.gts_enable[2] {"enabled"} else {"disabled"},
            if self.gts_invert[2] {"!T"} else {"T"}).unwrap();
        write!(writer, "GTS3 {}, acts as {}\n",
            if self.gts_enable[3] {"enabled"} else {"disabled"},
            if self.gts_invert[3] {"!T"} else {"T"}).unwrap();

        write!(writer, "global termination is {}\n", if self.global_pu {"pull-up"} else {"bus hold"}).unwrap();
    }
}

fn read_32_global_nets_logical(fuses: &[bool]) -> XC2GlobalNets {
    XC2GlobalNets {
        gck_enable: [
            fuses[12256],
            fuses[12257],
            fuses[12258],
        ],
        gsr_enable: fuses[12260],
        gsr_invert: fuses[12259],
        gts_enable: [
            !fuses[12262],
            !fuses[12264],
            !fuses[12266],
            !fuses[12268],
        ],
        gts_invert: [
            fuses[12261],
            fuses[12263],
            fuses[12265],
            fuses[12267],
        ],
        global_pu: fuses[12269],
    }
}

pub enum XC2BitstreamBits {
    XC2C32 {
        fb: [XC2BistreamFB; 2],
        iobs: [XC2MCSmallIOB; 32],
        inpin: XC2ExtraIBuf,
        global_nets: XC2GlobalNets,
        // false = low, true = high
        ivoltage: bool,
        ovoltage: bool,
    },
    XC2C32A {
        fb: [XC2BistreamFB; 2],
        iobs: [XC2MCSmallIOB; 32],
        inpin: XC2ExtraIBuf,
        global_nets: XC2GlobalNets,
        // false = low, true = high
        legacy_ivoltage: bool,
        legacy_ovoltage: bool,
        ivoltage: [bool; 2],
        ovoltage: [bool; 2],
    },
}

impl XC2BitstreamBits {
    pub fn dump_human_readable(&self, writer: &mut Write) {
        match self {
            &XC2BitstreamBits::XC2C32 {
                ref fb, ref iobs, ref inpin, ref global_nets, ref ivoltage, ref ovoltage} => {

                write!(writer, "device type: XC2C32\n").unwrap();
                write!(writer, "output voltage range: {}\n", if *ovoltage {"high"} else {"low"}).unwrap();
                write!(writer, "input voltage range: {}\n", if *ivoltage {"high"} else {"low"}).unwrap();
                global_nets.dump_human_readable(writer);

                for i in 0..32 {
                    iobs[i].dump_human_readable(i as u32, writer);
                }

                inpin.dump_human_readable(writer);

                fb[0].dump_human_readable(0, writer);
                fb[1].dump_human_readable(1, writer);
            },
            &XC2BitstreamBits::XC2C32A {
                ref fb, ref iobs, ref inpin, ref global_nets, ref legacy_ivoltage, ref legacy_ovoltage,
                ref ivoltage, ref ovoltage} => {

                write!(writer, "device type: XC2C32A\n").unwrap();
                write!(writer, "legacy output voltage range: {}\n", if *legacy_ovoltage {"high"} else {"low"}).unwrap();
                write!(writer, "legacy input voltage range: {}\n", if *legacy_ivoltage {"high"} else {"low"}).unwrap();
                write!(writer, "bank 0 output voltage range: {}\n", if ovoltage[0] {"high"} else {"low"}).unwrap();
                write!(writer, "bank 1 output voltage range: {}\n", if ovoltage[1] {"high"} else {"low"}).unwrap();
                write!(writer, "bank 0 input voltage range: {}\n", if ivoltage[0] {"high"} else {"low"}).unwrap();
                write!(writer, "bank 1 input voltage range: {}\n", if ivoltage[1] {"high"} else {"low"}).unwrap();
                global_nets.dump_human_readable(writer);

                for i in 0..32 {
                    iobs[i].dump_human_readable(i as u32, writer);
                }

                inpin.dump_human_readable(writer);

                fb[0].dump_human_readable(0, writer);
                fb[1].dump_human_readable(1, writer);
            },
        }
    }

    pub fn write_jed(&self, writer: &mut Write) {
        match self {
            &XC2BitstreamBits::XC2C32 {
                ref fb, ref iobs, ref inpin, ref global_nets, ref ivoltage, ref ovoltage, ..
            } | &XC2BitstreamBits::XC2C32A {
                ref fb, ref iobs, ref inpin, ref global_nets, legacy_ivoltage: ref ivoltage,
                legacy_ovoltage: ref ovoltage, ..
            } => {

                // Each FB
                for fb_i in 0..2 {
                    let fuse_base = if fb_i == 0 {0} else {6128};

                    // ZIA
                    for i in 0..40 {
                        write!(writer, "L{:06} ", fuse_base + i * 8).unwrap();
                        let zia_choice_bits =
                            encode_32_zia_choice(i as u32, fb[fb_i].zia_bits[i].selected)
                            .expect("invalid ZIA input");
                        write!(writer, "{}{}{}{}{}{}{}{}",
                            if zia_choice_bits[7] {"1"} else {"0"},
                            if zia_choice_bits[6] {"1"} else {"0"},
                            if zia_choice_bits[5] {"1"} else {"0"},
                            if zia_choice_bits[4] {"1"} else {"0"},
                            if zia_choice_bits[3] {"1"} else {"0"},
                            if zia_choice_bits[2] {"1"} else {"0"},
                            if zia_choice_bits[1] {"1"} else {"0"},
                            if zia_choice_bits[0] {"1"} else {"0"}).unwrap();
                        write!(writer, "*\n").unwrap();
                    }
                    write!(writer, "\n").unwrap();

                    // AND terms
                    for i in 0..56 {
                        write!(writer, "L{:06} ", fuse_base + 8 * 40 + i * 80).unwrap();
                        for j in 0..40 {
                            if fb[fb_i].and_terms[i].input[j] {
                                write!(writer, "0").unwrap();
                            } else {
                                write!(writer, "1").unwrap();
                            }
                            if fb[fb_i].and_terms[i].input_b[j] {
                                write!(writer, "0").unwrap();
                            } else {
                                write!(writer, "1").unwrap();
                            }
                        }
                        write!(writer, "*\n").unwrap();
                    }
                    write!(writer, "\n").unwrap();

                    // OR terms
                    for i in 0..56 {
                        write!(writer, "L{:06} ", fuse_base + 8 * 40 + 56 * 80 + i * 16).unwrap();
                        for j in 0..16 {
                            if fb[fb_i].or_terms[j].input[i] {
                                write!(writer, "0").unwrap();
                            } else {
                                write!(writer, "1").unwrap();
                            }
                        }
                        write!(writer, "*\n").unwrap();
                    }
                    write!(writer, "\n").unwrap();

                    // Macrocells
                    for i in 0..16 {
                        write!(writer, "L{:06} ", fuse_base + 8 * 40 + 56 * 80 + 56 * 16 + i * 27).unwrap();

                        let iob = fb_ff_num_to_iob_num_32(fb_i as u32, i as u32).unwrap() as usize;

                        // aclk
                        write!(writer, "{}", match fb[fb_i].ffs[i].clk_src {
                            XC2MCFFClkSrc::CTC => "1",
                            _ => "0",
                        }).unwrap();

                        // clkop
                        write!(writer, "{}", if fb[fb_i].ffs[i].falling_edge {"1"} else {"0"}).unwrap();

                        // clk
                        write!(writer, "{}", match fb[fb_i].ffs[i].clk_src {
                            XC2MCFFClkSrc::GCK0 => "00",
                            XC2MCFFClkSrc::GCK1 => "01",
                            XC2MCFFClkSrc::GCK2 => "10",
                            XC2MCFFClkSrc::PTC | XC2MCFFClkSrc::CTC => "11",
                        }).unwrap();

                        // clkfreq
                        write!(writer, "{}", if fb[fb_i].ffs[i].is_ddr {"1"} else {"0"}).unwrap();

                        // r
                        write!(writer, "{}", match fb[fb_i].ffs[i].r_src {
                            XC2MCFFResetSrc::PTA => "00",
                            XC2MCFFResetSrc::GSR => "01",
                            XC2MCFFResetSrc::CTR => "10",
                            XC2MCFFResetSrc::Disabled => "11",
                        }).unwrap();

                        // p
                        write!(writer, "{}", match fb[fb_i].ffs[i].s_src {
                            XC2MCFFSetSrc::PTA => "00",
                            XC2MCFFSetSrc::GSR => "01",
                            XC2MCFFSetSrc::CTS => "10",
                            XC2MCFFSetSrc::Disabled => "11",
                        }).unwrap();

                        // regmod
                        write!(writer, "{}", match fb[fb_i].ffs[i].ff_mode {
                            XC2MCFFMode::DFF => "00",
                            XC2MCFFMode::LATCH => "01",
                            XC2MCFFMode::TFF => "10",
                            XC2MCFFMode::DFFCE => "11",
                        }).unwrap();

                        // inz
                        write!(writer, "{}", if iobs[iob].ibuf_to_zia {"00"} else {"11"}).unwrap();

                        // fb
                        write!(writer, "{}", match fb[fb_i].ffs[i].fb_mode {
                            XC2MCFeedbackMode::COMB => "00",
                            XC2MCFeedbackMode::REG => "10",
                            XC2MCFeedbackMode::Disabled => "11",
                        }).unwrap();

                        // inreg
                        write!(writer, "{}", if fb[fb_i].ffs[i].ff_in_ibuf {"0"} else {"1"}).unwrap();

                        // st
                        write!(writer, "{}", if iobs[iob].schmitt_trigger {"1"} else {"0"}).unwrap();

                        // xorin
                        write!(writer, "{}", match fb[fb_i].ffs[i].xor_mode {
                            XC2MCXorMode::ZERO => "00",
                            XC2MCXorMode::PTCB => "01",
                            XC2MCXorMode::PTC => "10",
                            XC2MCXorMode::ONE => "11",
                        }).unwrap();

                        // regcom
                        write!(writer, "{}", if iobs[iob].obuf_uses_ff {"0"} else {"1"}).unwrap();

                        // oe
                        write!(writer, "{}", match iobs[iob].obuf_mode {
                            XC2MCOBufMode::PushPull => "0000",
                            XC2MCOBufMode::OpenDrain => "0001",
                            XC2MCOBufMode::TriStateGTS1 => "0010",
                            XC2MCOBufMode::TriStatePTB => "0100",
                            XC2MCOBufMode::TriStateGTS3 => "0110",
                            XC2MCOBufMode::TriStateCTE => "1000",
                            XC2MCOBufMode::TriStateGTS2 => "1010",
                            XC2MCOBufMode::TriStateGTS0 => "1100",
                            XC2MCOBufMode::CGND => "1110",
                            XC2MCOBufMode::Disabled => "1111",
                        }).unwrap();

                        // tm
                        write!(writer, "{}", if iobs[iob].termination_enabled {"1"} else {"0"}).unwrap();

                        // slw
                        write!(writer, "{}", if iobs[iob].slew_is_fast {"0"} else {"1"}).unwrap();

                        // pu
                        write!(writer, "{}", if fb[fb_i].ffs[i].init_state {"0"} else {"1"}).unwrap();

                        write!(writer, "*\n").unwrap();
                    }
                    write!(writer, "\n").unwrap();
                }

                // "other stuff" except bank voltages
                write!(writer, "L012256 {}{}{}*\n",
                    if global_nets.gck_enable[0] {"1"} else {"0"},
                    if global_nets.gck_enable[1] {"1"} else {"0"},
                    if global_nets.gck_enable[2] {"1"} else {"0"}).unwrap();

                write!(writer, "L012259 {}{}*\n",
                    if global_nets.gsr_invert {"1"} else {"0"},
                    if global_nets.gsr_enable {"1"} else {"0"}).unwrap();

                write!(writer, "L012261 {}{}{}{}{}{}{}{}*\n",
                    if global_nets.gts_invert[0] {"1"} else {"0"},
                    if global_nets.gts_enable[0] {"0"} else {"1"},
                    if global_nets.gts_invert[1] {"1"} else {"0"},
                    if global_nets.gts_enable[1] {"0"} else {"1"},
                    if global_nets.gts_invert[2] {"1"} else {"0"},
                    if global_nets.gts_enable[2] {"0"} else {"1"},
                    if global_nets.gts_invert[3] {"1"} else {"0"},
                    if global_nets.gts_enable[3] {"0"} else {"1"}).unwrap();

                write!(writer, "L012269 {}*\n", if global_nets.global_pu {"1"} else {"0"}).unwrap();

                write!(writer, "L012270 {}*\n", if *ovoltage {"0"} else {"1"}).unwrap();
                write!(writer, "L012271 {}*\n", if *ivoltage {"0"} else {"1"}).unwrap();

                write!(writer, "L012272 {}{}*\n",
                    if inpin.schmitt_trigger {"1"} else {"0"},
                    if inpin.termination_enabled {"1"} else {"0"}).unwrap();
            }
        }

        // A-variant bank voltages
        match self {
            &XC2BitstreamBits::XC2C32A {ref ivoltage, ref ovoltage, ..} => {
                write!(writer, "L012274 {}*\n", if ivoltage[0] {"0"} else {"1"}).unwrap();
                write!(writer, "L012275 {}*\n", if ovoltage[0] {"0"} else {"1"}).unwrap();
                write!(writer, "L012276 {}*\n", if ivoltage[1] {"0"} else {"1"}).unwrap();
                write!(writer, "L012277 {}*\n", if ovoltage[1] {"0"} else {"1"}).unwrap();
            },
            _ => {}
        }
    }
}

pub fn read_32_bitstream_logical(fuses: &[bool]) -> Result<XC2BitstreamBits, &'static str> {
    let mut fb = [XC2BistreamFB::default(); 2];
    for i in 0..fb.len() {
        let res = read_32_fb_logical(fuses, i);
        if let Err(err) = res {
            return Err(err);
        }
        fb[i] = res.unwrap();
    };

    let mut iobs = [XC2MCSmallIOB::default(); 32];
    for i in 0..iobs.len() {
        let base_fuse = if i < 16 {
            5696
        } else {
            11824
        };
        let res = read_32_iob_logical(fuses, base_fuse, i % 16);
        if let Err(err) = res {
            return Err(err);
        }
        iobs[i] = res.unwrap();
    }

    let inpin = read_32_extra_ibuf_logical(fuses);

    let global_nets = read_32_global_nets_logical(fuses);

    Ok(XC2BitstreamBits::XC2C32 {
        fb: fb,
        iobs: iobs,
        inpin: inpin,
        global_nets: global_nets,
        ovoltage: !fuses[12270],
        ivoltage: !fuses[12271],
    })
}


pub fn read_32a_bitstream_logical(fuses: &[bool]) -> Result<XC2BitstreamBits, &'static str> {
    let mut fb = [XC2BistreamFB::default(); 2];
    for i in 0..fb.len() {
        let res = read_32_fb_logical(fuses, i);
        if let Err(err) = res {
            return Err(err);
        }
        fb[i] = res.unwrap();
    };

    let mut iobs = [XC2MCSmallIOB::default(); 32];
    for i in 0..iobs.len() {
        let base_fuse = if i < 16 {
            5696
        } else {
            11824
        };
        let res = read_32_iob_logical(fuses, base_fuse, i % 16);
        if let Err(err) = res {
            return Err(err);
        }
        iobs[i] = res.unwrap();
    }

    let inpin = read_32_extra_ibuf_logical(fuses);

    let global_nets = read_32_global_nets_logical(fuses);

    Ok(XC2BitstreamBits::XC2C32A {
        fb: fb,
        iobs: iobs,
        inpin: inpin,
        global_nets: global_nets,
        legacy_ovoltage: !fuses[12270],
        legacy_ivoltage: !fuses[12271],
        ivoltage: [
            !fuses[12274],
            !fuses[12276],
        ],
        ovoltage: [
            !fuses[12275],
            !fuses[12277],
        ]
    })
}
