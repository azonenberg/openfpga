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

use std::io;
use std::io::Write;

use *;
use fb::{read_fb_logical};
use iob::{read_small_iob_logical, read_large_iob_logical, read_32_extra_ibuf_logical};
use zia::{zia_get_row_width};

/// Toplevel struct representing an entire Coolrunner-II bitstream
pub struct XC2Bitstream {
    pub speed_grade: XC2Speed,
    pub package: XC2Package,
    pub bits: XC2BitstreamBits,
}

impl XC2Bitstream {
    /// Dump a human-readable explanation of the bitstream to the given `writer` object.
    pub fn dump_human_readable(&self, writer: &mut Write) -> Result<(), io::Error> {
        write!(writer, "xc2bit dump\n")?;
        write!(writer, "device speed grade: {}\n", self.speed_grade)?;
        write!(writer, "device package: {}\n", self.package)?;
        self.bits.dump_human_readable(writer)?;

        Ok(())
    }

    /// Write a .jed representation of the bitstream to the given `writer` object.
    pub fn write_jed(&self, writer: &mut Write) -> Result<(), io::Error> {
        write!(writer, ".JED fuse map written by xc2bit\n")?;
        write!(writer, "https://github.com/azonenberg/openfpga\n\n")?;
        write!(writer, "\x02")?;

        match self.bits {
            XC2BitstreamBits::XC2C32{..} => {
                write!(writer, "QF12274*\n")?;
                write!(writer, "N DEVICE XC2C32-{}-{}*\n\n", self.speed_grade, self.package)?;
            },
            XC2BitstreamBits::XC2C32A{..} => {
                write!(writer, "QF12278*\n")?;
                write!(writer, "N DEVICE XC2C32A-{}-{}*\n\n", self.speed_grade, self.package)?;
            },
            XC2BitstreamBits::XC2C64{..} => {
                write!(writer, "QF25808*\n")?;
                write!(writer, "N DEVICE XC2C64-{}-{}*\n\n", self.speed_grade, self.package)?;
            },
            XC2BitstreamBits::XC2C64A{..} => {
                write!(writer, "QF25812*\n")?;
                write!(writer, "N DEVICE XC2C64A-{}-{}*\n\n", self.speed_grade, self.package)?;
            },
            XC2BitstreamBits::XC2C128{..} => {
                write!(writer, "QF55341*\n")?;
                write!(writer, "N DEVICE XC2C128-{}-{}*\n\n", self.speed_grade, self.package)?;
            },
        }

        self.bits.write_jed(writer)?;

        write!(writer, "\x030000\n")?;

        Ok(())
    }

    /// Construct a new blank bitstream of the given part
    pub fn blank_bitstream(device: XC2Device, speed_grade: XC2Speed, package: XC2Package)
        -> Result<XC2Bitstream, &'static str> {

        if !is_valid_part_combination(device, speed_grade, package) {
            return Err("requested device/speed/package is not a legal combination")
        }

        match device {
            XC2Device::XC2C32 => {
                Ok(XC2Bitstream {
                    speed_grade: speed_grade,
                    package: package,
                    bits: XC2BitstreamBits::XC2C32 {
                        fb: [XC2BitstreamFB::default(); 2],
                        iobs: [XC2MCSmallIOB::default(); 32],
                        inpin: XC2ExtraIBuf::default(),
                        global_nets: XC2GlobalNets::default(),
                        ivoltage: false,
                        ovoltage: false,
                    }
                })
            },
            XC2Device::XC2C32A => {
                Ok(XC2Bitstream {
                    speed_grade: speed_grade,
                    package: package,
                    bits: XC2BitstreamBits::XC2C32A {
                        fb: [XC2BitstreamFB::default(); 2],
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
            XC2Device::XC2C64 => {
                Ok(XC2Bitstream {
                    speed_grade: speed_grade,
                    package: package,
                    bits: XC2BitstreamBits::XC2C64 {
                        fb: [XC2BitstreamFB::default(); 4],
                        iobs: [XC2MCSmallIOB::default(); 64],
                        global_nets: XC2GlobalNets::default(),
                        ivoltage: false,
                        ovoltage: false,
                    }
                })
            },
            XC2Device::XC2C64A => {
                Ok(XC2Bitstream {
                    speed_grade: speed_grade,
                    package: package,
                    bits: XC2BitstreamBits::XC2C64A {
                        fb: [XC2BitstreamFB::default(); 4],
                        iobs: [XC2MCSmallIOB::default(); 64],
                        global_nets: XC2GlobalNets::default(),
                        legacy_ivoltage: false,
                        legacy_ovoltage: false,
                        ivoltage: [false, false],
                        ovoltage: [false, false],
                    }
                })
            },
            XC2Device::XC2C128 => {
                Ok(XC2Bitstream {
                    speed_grade: speed_grade,
                    package: package,
                    bits: XC2BitstreamBits::XC2C128 {
                        fb: [XC2BitstreamFB::default(); 8],
                        iobs: [XC2MCLargeIOB::default(); 100],
                        global_nets: XC2GlobalNets::default(),
                        ivoltage: [false, false],
                        ovoltage: [false, false],
                        data_gate: false,
                        use_vref: false,
                        clock_div: XC2ClockDiv::default(),
                    }
                })
            },
            _ => Err("invalid device")
        }
    }
}

/// Represents the configuration of the global nets. Coolrunner-II parts have various global control signals that have
/// dedicated low-skew paths.
pub struct XC2GlobalNets {
    /// Controls whether the three global clock nets are enabled or not
    pub gck_enable: [bool; 3],
    /// Controls whether the global set/reset net is enabled or not
    pub gsr_enable: bool,
    /// Controls the polarity of the global set/reset signal
    ///
    /// `false` = active low, `true` = active high
    pub gsr_invert: bool,
    /// Controls whether the four global tristate nets are enabled or not
    pub gts_enable: [bool; 4],
    /// Controls the polarity of the global tristate signal
    ///
    /// `false` = used as T, `true` = used as !T
    pub gts_invert: [bool; 4],
    /// Controls the mode of the global termination
    ///
    /// `false` = keeper, `true` = pull-up
    pub global_pu: bool,
}

impl Default for XC2GlobalNets {
    /// Returns a "default" global net configuration which has everything disabled.
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
    /// Dump a human-readable explanation of the global net configuration to the given `writer` object.
    pub fn dump_human_readable(&self, writer: &mut Write) -> Result<(), io::Error> {
        write!(writer, "\n")?;
        write!(writer, "GCK0 {}\n", if self.gck_enable[0] {"enabled"} else {"disabled"})?;
        write!(writer, "GCK1 {}\n", if self.gck_enable[1] {"enabled"} else {"disabled"})?;
        write!(writer, "GCK2 {}\n", if self.gck_enable[2] {"enabled"} else {"disabled"})?;

        write!(writer, "GSR {}, active {}\n",
            if self.gsr_enable {"enabled"} else {"disabled"},
            if self.gsr_invert {"high"} else {"low"})?;

        write!(writer, "GTS0 {}, acts as {}\n",
            if self.gts_enable[0] {"enabled"} else {"disabled"},
            if self.gts_invert[0] {"!T"} else {"T"})?;
        write!(writer, "GTS1 {}, acts as {}\n",
            if self.gts_enable[1] {"enabled"} else {"disabled"},
            if self.gts_invert[1] {"!T"} else {"T"})?;
        write!(writer, "GTS2 {}, acts as {}\n",
            if self.gts_enable[2] {"enabled"} else {"disabled"},
            if self.gts_invert[2] {"!T"} else {"T"})?;
        write!(writer, "GTS3 {}, acts as {}\n",
            if self.gts_enable[3] {"enabled"} else {"disabled"},
            if self.gts_invert[3] {"!T"} else {"T"})?;

        write!(writer, "global termination is {}\n", if self.global_pu {"pull-up"} else {"bus hold"})?;

        Ok(())
    }
}

/// Internal function to read the global nets from a 32-macrocell part
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

/// Internal function to read the global nets from a 64-macrocell part
fn read_64_global_nets_logical(fuses: &[bool]) -> XC2GlobalNets {
    XC2GlobalNets {
        gck_enable: [
            fuses[25792],
            fuses[25793],
            fuses[25794],
        ],
        gsr_enable: fuses[25796],
        gsr_invert: fuses[25795],
        gts_enable: [
            !fuses[25798],
            !fuses[25800],
            !fuses[25802],
            !fuses[25804],
        ],
        gts_invert: [
            fuses[25797],
            fuses[25799],
            fuses[25801],
            fuses[25803],
        ],
        global_pu: fuses[25805],
    }
}

/// Internal function to read the global nets from a 128-macrocell part
fn read_128_global_nets_logical(fuses: &[bool]) -> XC2GlobalNets {
    XC2GlobalNets {
        gck_enable: [
            fuses[55316],
            fuses[55317],
            fuses[55318],
        ],
        gsr_enable: fuses[55325],
        gsr_invert: fuses[55324],
        gts_enable: [
            !fuses[55327],
            !fuses[55329],
            !fuses[55331],
            !fuses[55333],
        ],
        gts_invert: [
            fuses[55326],
            fuses[55328],
            fuses[55330],
            fuses[55332],
        ],
        global_pu: fuses[55334],
    }
}

/// Possible clock divide ratios for the programmable clock divider
#[derive(Copy, Clone, Eq, PartialEq, Debug)]
pub enum XC2ClockDivRatio {
    Div2,
    Div4,
    Div6,
    Div8,
    Div10,
    Div12,
    Div14,
    Div16,
}

/// Represents the configuration of the programmable clock divider in devices with 128 macrocells or more. This is
/// hard-wired onto the GCK2 clock pin.
pub struct XC2ClockDiv {
    /// Ratio that input clock is divided by
    pub div_ratio: XC2ClockDivRatio,
    /// Whether the "delay" feature is enabled
    pub delay: bool,
    /// Whether the clock divider is enabled (other settings are ignored if not)
    pub enabled: bool,
}

impl XC2ClockDiv {
    /// Dump a human-readable explanation of the clock divider to the given `writer` object.
    pub fn dump_human_readable(&self, writer: &mut Write) -> Result<(), io::Error> {
        write!(writer, "\n")?;
        write!(writer, "GCK2 clock divider {}\n", if self.enabled {"enabled"} else {"disabled"})?;
        write!(writer, "clock divider delay {}\n", if self.delay {"enabled"} else {"disabled"})?;

        write!(writer, "clock division ratio: {}\n", match self.div_ratio {
            XC2ClockDivRatio::Div2 => "2",
            XC2ClockDivRatio::Div4 => "4",
            XC2ClockDivRatio::Div6 => "6",
            XC2ClockDivRatio::Div8 => "8",
            XC2ClockDivRatio::Div10 => "10",
            XC2ClockDivRatio::Div12 => "12",
            XC2ClockDivRatio::Div14 => "14",
            XC2ClockDivRatio::Div16 => "16",
        })?;

        Ok(())
    }
}

impl Default for XC2ClockDiv {
    /// Returns a "default" clock divider configuration, which is one that is not used
    fn default() -> Self {
        XC2ClockDiv {
            div_ratio: XC2ClockDivRatio::Div16,
            delay: false,
            enabled: false,
        }
    }
}

/// Internal function to read the clock divider configuration from a 128-macrocell part
fn read_128_clock_div_logical(fuses: &[bool]) -> XC2ClockDiv {
    XC2ClockDiv {
        delay: !fuses[55323],
        enabled: !fuses[55319],
        div_ratio: match (fuses[55320], fuses[55321], fuses[55322]) {
            (false, false, false) => XC2ClockDivRatio::Div2,
            (false, false,  true) => XC2ClockDivRatio::Div4,
            (false,  true, false) => XC2ClockDivRatio::Div6,
            (false,  true,  true) => XC2ClockDivRatio::Div8,
            ( true, false, false) => XC2ClockDivRatio::Div10,
            ( true, false,  true) => XC2ClockDivRatio::Div12,
            ( true,  true, false) => XC2ClockDivRatio::Div14,
            ( true,  true,  true) => XC2ClockDivRatio::Div16,
        }
    }
}


/// The actual bitstream bits for each possible Coolrunner-II part
pub enum XC2BitstreamBits {
    XC2C32 {
        fb: [XC2BitstreamFB; 2],
        iobs: [XC2MCSmallIOB; 32],
        inpin: XC2ExtraIBuf,
        global_nets: XC2GlobalNets,
        /// Voltage level control
        ///
        /// `false` = low, `true` = high
        ivoltage: bool,
        /// Voltage level control
        ///
        /// `false` = low, `true` = high
        ovoltage: bool,
    },
    XC2C32A {
        fb: [XC2BitstreamFB; 2],
        iobs: [XC2MCSmallIOB; 32],
        inpin: XC2ExtraIBuf,
        global_nets: XC2GlobalNets,
        /// Legacy voltage level control, should almost always be set to `false`
        ///
        /// `false` = low, `true` = high
        legacy_ivoltage: bool,
        /// Legacy voltage level control, should almost always be set to `false`
        ///
        /// `false` = low, `true` = high
        legacy_ovoltage: bool,
        /// Voltage level control for each I/O bank
        ///
        /// `false` = low, `true` = high
        ivoltage: [bool; 2],
        /// Voltage level control for each I/O bank
        ///
        /// `false` = low, `true` = high
        ovoltage: [bool; 2],
    },
    XC2C64 {
        fb: [XC2BitstreamFB; 4],
        iobs: [XC2MCSmallIOB; 64],
        global_nets: XC2GlobalNets,
        /// Voltage level control
        ///
        /// `false` = low, `true` = high
        ivoltage: bool,
        /// Voltage level control
        ///
        /// `false` = low, `true` = high
        ovoltage: bool,
    },
    XC2C64A {
        fb: [XC2BitstreamFB; 4],
        iobs: [XC2MCSmallIOB; 64],
        global_nets: XC2GlobalNets,
        /// Legacy voltage level control, should almost always be set to `false`
        ///
        /// `false` = low, `true` = high
        legacy_ivoltage: bool,
        /// Legacy voltage level control, should almost always be set to `false`
        ///
        /// `false` = low, `true` = high
        legacy_ovoltage: bool,
        /// Voltage level control for each I/O bank
        ///
        /// `false` = low, `true` = high
        ivoltage: [bool; 2],
        /// Voltage level control for each I/O bank
        ///
        /// `false` = low, `true` = high
        ovoltage: [bool; 2],
    },
    XC2C128 {
        fb: [XC2BitstreamFB; 8],
        iobs: [XC2MCLargeIOB; 100],
        global_nets: XC2GlobalNets,
        clock_div: XC2ClockDiv,
        /// Whether the DataGate feature is used
        data_gate: bool,
        /// Whether I/O standards with VREF are used
        use_vref: bool,
        /// Voltage level control for each I/O bank
        ///
        /// `false` = low, `true` = high
        ivoltage: [bool; 2],
        /// Voltage level control for each I/O bank
        ///
        /// `false` = low, `true` = high
        ovoltage: [bool; 2],
    }
}

/// Helper that prints the IOB and macrocell configuration on the "small" parts
fn write_small_mc_to_jed(writer: &mut Write, device: XC2Device, fb: &XC2BitstreamFB, iobs: &[XC2MCSmallIOB],
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
fn write_large_mc_to_jed(writer: &mut Write, device: XC2Device, fb: &XC2BitstreamFB, iobs: &[XC2MCLargeIOB],
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

impl XC2BitstreamBits {
    /// Helper to convert ourself into a `XC2Device` enum because an `XC2Device` enum has various useful methods
    pub fn device_type(&self) -> XC2Device {
        match self {
            &XC2BitstreamBits::XC2C32{..} => XC2Device::XC2C32,
            &XC2BitstreamBits::XC2C32A{..} => XC2Device::XC2C32A,
            &XC2BitstreamBits::XC2C64{..} => XC2Device::XC2C64,
            &XC2BitstreamBits::XC2C64A{..} => XC2Device::XC2C64A,
            &XC2BitstreamBits::XC2C128{..} => XC2Device::XC2C128,
        }
    }

    /// Helper to extract only the function block data without having to perform an explicit `match`
    pub fn get_fb(&self) -> &[XC2BitstreamFB] {
        match self {
            &XC2BitstreamBits::XC2C32{ref fb, ..} => fb,
            &XC2BitstreamBits::XC2C32A{ref fb, ..} => fb,
            &XC2BitstreamBits::XC2C64{ref fb, ..} => fb,
            &XC2BitstreamBits::XC2C64A{ref fb, ..} => fb,
            &XC2BitstreamBits::XC2C128{ref fb, ..} => fb,
        }
    }

    /// Dump a human-readable explanation of the bitstream to the given `writer` object.
    pub fn dump_human_readable(&self, writer: &mut Write) -> Result<(), io::Error> {
        match self {
            &XC2BitstreamBits::XC2C32 {ref global_nets, ref ivoltage, ref ovoltage, ..} => {

                write!(writer, "device type: XC2C32\n")?;
                write!(writer, "output voltage range: {}\n", if *ovoltage {"high"} else {"low"})?;
                write!(writer, "input voltage range: {}\n", if *ivoltage {"high"} else {"low"})?;
                global_nets.dump_human_readable(writer)?;
            },
            &XC2BitstreamBits::XC2C32A {ref global_nets, ref legacy_ivoltage, ref legacy_ovoltage,
                ref ivoltage, ref ovoltage, ..} => {

                write!(writer, "device type: XC2C32A\n")?;
                write!(writer, "legacy output voltage range: {}\n", if *legacy_ovoltage {"high"} else {"low"})?;
                write!(writer, "legacy input voltage range: {}\n", if *legacy_ivoltage {"high"} else {"low"})?;
                write!(writer, "bank 0 output voltage range: {}\n", if ovoltage[0] {"high"} else {"low"})?;
                write!(writer, "bank 1 output voltage range: {}\n", if ovoltage[1] {"high"} else {"low"})?;
                write!(writer, "bank 0 input voltage range: {}\n", if ivoltage[0] {"high"} else {"low"})?;
                write!(writer, "bank 1 input voltage range: {}\n", if ivoltage[1] {"high"} else {"low"})?;
                global_nets.dump_human_readable(writer)?;
            },
            &XC2BitstreamBits::XC2C64 {ref global_nets, ref ivoltage, ref ovoltage, ..} => {

                write!(writer, "device type: XC2C64\n")?;
                write!(writer, "output voltage range: {}\n", if *ovoltage {"high"} else {"low"})?;
                write!(writer, "input voltage range: {}\n", if *ivoltage {"high"} else {"low"})?;
                global_nets.dump_human_readable(writer)?;
            },
            &XC2BitstreamBits::XC2C64A {ref global_nets, ref legacy_ivoltage, ref legacy_ovoltage,
                ref ivoltage, ref ovoltage, ..} => {

                write!(writer, "device type: XC2C64A\n")?;
                write!(writer, "legacy output voltage range: {}\n", if *legacy_ovoltage {"high"} else {"low"})?;
                write!(writer, "legacy input voltage range: {}\n", if *legacy_ivoltage {"high"} else {"low"})?;
                write!(writer, "bank 0 output voltage range: {}\n", if ovoltage[0] {"high"} else {"low"})?;
                write!(writer, "bank 1 output voltage range: {}\n", if ovoltage[1] {"high"} else {"low"})?;
                write!(writer, "bank 0 input voltage range: {}\n", if ivoltage[0] {"high"} else {"low"})?;
                write!(writer, "bank 1 input voltage range: {}\n", if ivoltage[1] {"high"} else {"low"})?;
                global_nets.dump_human_readable(writer)?;
            },
            &XC2BitstreamBits::XC2C128 {ref global_nets, ref ivoltage, ref ovoltage, ref clock_div, ref data_gate,
                ref use_vref, ..}  => {

                write!(writer, "device type: XC2C128\n")?;
                write!(writer, "bank 0 output voltage range: {}\n", if ovoltage[0] {"high"} else {"low"})?;
                write!(writer, "bank 1 output voltage range: {}\n", if ovoltage[1] {"high"} else {"low"})?;
                write!(writer, "bank 0 input voltage range: {}\n", if ivoltage[0] {"high"} else {"low"})?;
                write!(writer, "bank 1 input voltage range: {}\n", if ivoltage[1] {"high"} else {"low"})?;
                write!(writer, "DataGate used: {}\n", if *data_gate {"high"} else {"low"})?;
                write!(writer, "VREF used: {}\n", if *use_vref {"high"} else {"low"})?;
                clock_div.dump_human_readable(writer)?;
                global_nets.dump_human_readable(writer)?;
            }
        }

        // IOBs
        match self {
            // This match is needed because the different sizes have different IOB structures, and fixed-sized arrays
            // are gimped enough that returning an array of trait objects is hard.
            &XC2BitstreamBits::XC2C32 {ref iobs, ..} |
            &XC2BitstreamBits::XC2C32A {ref iobs, ..} => {
                for i in 0..self.device_type().num_iobs() {
                    iobs[i].dump_human_readable(self.device_type(), i as u32, writer)?;
                }
            },
            &XC2BitstreamBits::XC2C64 {ref iobs, ..} |
            &XC2BitstreamBits::XC2C64A {ref iobs, ..} => {
                for i in 0..self.device_type().num_iobs() {
                    iobs[i].dump_human_readable(self.device_type(), i as u32, writer)?;
                }
            },
            &XC2BitstreamBits::XC2C128 {ref iobs, ..} => {
                for i in 0..self.device_type().num_iobs() {
                    iobs[i].dump_human_readable(self.device_type(), i as u32, writer)?;
                }
            },
        }

        // Input-only pin
        match self {
            &XC2BitstreamBits::XC2C32 {ref inpin, ..} | &XC2BitstreamBits::XC2C32A {ref inpin, ..} => {
                inpin.dump_human_readable(writer)?;
            },
            _ => {}
        }

        // FBs
        for i in 0..self.device_type().num_fbs() {
            self.get_fb()[i].dump_human_readable(self.device_type(), i as u32, writer)?;
        }

        Ok(())
    }

    /// Write a .jed representation of the bitstream to the given `writer` object.
    pub fn write_jed(&self, writer: &mut Write) -> Result<(), io::Error> {
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

                    fb[fb_i].write_to_jed(XC2Device::XC2C32, fuse_base, writer)?;

                    // Macrocells
                    write_small_mc_to_jed(writer, XC2Device::XC2C32, &fb[fb_i], iobs, fb_i, fuse_base)?;
                }

                // "other stuff" except bank voltages
                write!(writer, "L012256 {}{}{}*\n",
                    if global_nets.gck_enable[0] {"1"} else {"0"},
                    if global_nets.gck_enable[1] {"1"} else {"0"},
                    if global_nets.gck_enable[2] {"1"} else {"0"})?;

                write!(writer, "L012259 {}{}*\n",
                    if global_nets.gsr_invert {"1"} else {"0"},
                    if global_nets.gsr_enable {"1"} else {"0"})?;

                write!(writer, "L012261 {}{}{}{}{}{}{}{}*\n",
                    if global_nets.gts_invert[0] {"1"} else {"0"},
                    if global_nets.gts_enable[0] {"0"} else {"1"},
                    if global_nets.gts_invert[1] {"1"} else {"0"},
                    if global_nets.gts_enable[1] {"0"} else {"1"},
                    if global_nets.gts_invert[2] {"1"} else {"0"},
                    if global_nets.gts_enable[2] {"0"} else {"1"},
                    if global_nets.gts_invert[3] {"1"} else {"0"},
                    if global_nets.gts_enable[3] {"0"} else {"1"})?;

                write!(writer, "L012269 {}*\n", if global_nets.global_pu {"1"} else {"0"})?;

                write!(writer, "L012270 {}*\n", if *ovoltage {"0"} else {"1"})?;
                write!(writer, "L012271 {}*\n", if *ivoltage {"0"} else {"1"})?;

                write!(writer, "L012272 {}{}*\n",
                    if inpin.schmitt_trigger {"1"} else {"0"},
                    if inpin.termination_enabled {"1"} else {"0"})?;
            }
            &XC2BitstreamBits::XC2C64 {
                ref fb, ref iobs, ref global_nets, ref ivoltage, ref ovoltage, ..
            } | &XC2BitstreamBits::XC2C64A {
                ref fb, ref iobs, ref global_nets, legacy_ivoltage: ref ivoltage,
                legacy_ovoltage: ref ovoltage, ..
            } => {

                // Each FB
                for fb_i in 0..4 {
                    let fuse_base = match fb_i {
                        0 => 0,
                        1 => 6448,
                        2 => 12896,
                        3 => 19344,
                        _ => unreachable!(),
                    };

                    fb[fb_i].write_to_jed(XC2Device::XC2C64, fuse_base, writer)?;

                    // Macrocells
                    write_small_mc_to_jed(writer, XC2Device::XC2C64, &fb[fb_i], iobs, fb_i, fuse_base)?;
                }

                // "other stuff" except bank voltages
                write!(writer, "L025792 {}{}{}*\n",
                    if global_nets.gck_enable[0] {"1"} else {"0"},
                    if global_nets.gck_enable[1] {"1"} else {"0"},
                    if global_nets.gck_enable[2] {"1"} else {"0"})?;

                write!(writer, "L025795 {}{}*\n",
                    if global_nets.gsr_invert {"1"} else {"0"},
                    if global_nets.gsr_enable {"1"} else {"0"})?;

                write!(writer, "L025797 {}{}{}{}{}{}{}{}*\n",
                    if global_nets.gts_invert[0] {"1"} else {"0"},
                    if global_nets.gts_enable[0] {"0"} else {"1"},
                    if global_nets.gts_invert[1] {"1"} else {"0"},
                    if global_nets.gts_enable[1] {"0"} else {"1"},
                    if global_nets.gts_invert[2] {"1"} else {"0"},
                    if global_nets.gts_enable[2] {"0"} else {"1"},
                    if global_nets.gts_invert[3] {"1"} else {"0"},
                    if global_nets.gts_enable[3] {"0"} else {"1"})?;

                write!(writer, "L025805 {}*\n", if global_nets.global_pu {"1"} else {"0"})?;

                write!(writer, "L025806 {}*\n", if *ovoltage {"0"} else {"1"})?;
                write!(writer, "L025807 {}*\n", if *ivoltage {"0"} else {"1"})?;
            }
            &XC2BitstreamBits::XC2C128 {
                ref fb, ref iobs, ref global_nets, ref ivoltage, ref ovoltage, ref clock_div, ref data_gate,
                ref use_vref}  => {

                // Each FB
                for fb_i in 0..8 {
                    let fuse_base = match fb_i {
                        0 => 0,
                        1 => 6908,
                        2 => 13816,
                        3 => 20737,
                        4 => 27658,
                        5 => 34579,
                        6 => 41487,
                        7 => 48408,
                        _ => unreachable!(),
                    };

                    fb[fb_i].write_to_jed(XC2Device::XC2C128, fuse_base, writer)?;

                    // Macrocells
                    write_large_mc_to_jed(writer, XC2Device::XC2C128, &fb[fb_i], iobs, fb_i, fuse_base)?;
                }

                // "other stuff"
                write!(writer, "L055316 {}{}{}*\n",
                    if global_nets.gck_enable[0] {"1"} else {"0"},
                    if global_nets.gck_enable[1] {"1"} else {"0"},
                    if global_nets.gck_enable[2] {"1"} else {"0"})?;

                write!(writer, "L055319 {}{}*\n",
                    if clock_div.enabled {"0"} else {"1"},
                    match clock_div.div_ratio {
                        XC2ClockDivRatio::Div2  => "000",
                        XC2ClockDivRatio::Div4  => "001",
                        XC2ClockDivRatio::Div6  => "010",
                        XC2ClockDivRatio::Div8  => "011",
                        XC2ClockDivRatio::Div10 => "100",
                        XC2ClockDivRatio::Div12 => "101",
                        XC2ClockDivRatio::Div14 => "110",
                        XC2ClockDivRatio::Div16 => "111",
                    })?;
                write!(writer, "L055323 {}*\n", if clock_div.delay {"0"} else {"1"})?;

                write!(writer, "L055324 {}{}*\n",
                    if global_nets.gsr_invert {"1"} else {"0"},
                    if global_nets.gsr_enable {"1"} else {"0"})?;

                write!(writer, "L055326 {}{}{}{}{}{}{}{}*\n",
                    if global_nets.gts_invert[0] {"1"} else {"0"},
                    if global_nets.gts_enable[0] {"0"} else {"1"},
                    if global_nets.gts_invert[1] {"1"} else {"0"},
                    if global_nets.gts_enable[1] {"0"} else {"1"},
                    if global_nets.gts_invert[2] {"1"} else {"0"},
                    if global_nets.gts_enable[2] {"0"} else {"1"},
                    if global_nets.gts_invert[3] {"1"} else {"0"},
                    if global_nets.gts_enable[3] {"0"} else {"1"})?;

                write!(writer, "L055334 {}*\n", if global_nets.global_pu {"1"} else {"0"})?;

                write!(writer, "L055335 {}*\n", if *data_gate {"0"} else {"1"})?;

                write!(writer, "L055336 {}{}*\n", if ivoltage[0] {"0"} else {"1"}, if ivoltage[1] {"0"} else {"1"})?;
                write!(writer, "L055338 {}{}*\n", if ovoltage[0] {"0"} else {"1"}, if ovoltage[1] {"0"} else {"1"})?;

                write!(writer, "L055340 {}*\n", if *use_vref {"0"} else {"1"})?;
            }
        }

        // A-variant bank voltages
        match self {
            &XC2BitstreamBits::XC2C32A {ref ivoltage, ref ovoltage, ..} => {
                write!(writer, "L012274 {}*\n", if ivoltage[0] {"0"} else {"1"})?;
                write!(writer, "L012275 {}*\n", if ovoltage[0] {"0"} else {"1"})?;
                write!(writer, "L012276 {}*\n", if ivoltage[1] {"0"} else {"1"})?;
                write!(writer, "L012277 {}*\n", if ovoltage[1] {"0"} else {"1"})?;
            },
            &XC2BitstreamBits::XC2C64A {ref ivoltage, ref ovoltage, ..} => {
                write!(writer, "L025808 {}*\n", if ivoltage[0] {"0"} else {"1"})?;
                write!(writer, "L025809 {}*\n", if ovoltage[0] {"0"} else {"1"})?;
                write!(writer, "L025810 {}*\n", if ivoltage[1] {"0"} else {"1"})?;
                write!(writer, "L025811 {}*\n", if ovoltage[1] {"0"} else {"1"})?;
            },
            _ => {}
        }

        Ok(())
    }
}

/// Internal function for parsing an XC2C32 bitstream
pub fn read_32_bitstream_logical(fuses: &[bool]) -> Result<XC2BitstreamBits, &'static str> {
    let mut fb = [XC2BitstreamFB::default(); 2];
    for i in 0..fb.len() {
        let base_fuse = if i < MCS_PER_FB {
            5696
        } else {
            11824
        };
        let res = read_fb_logical(XC2Device::XC2C32A, fuses, i as u32, base_fuse);
        if let Err(err) = res {
            return Err(err);
        }
        fb[i] = res.unwrap();
    };

    let mut iobs = [XC2MCSmallIOB::default(); 32];
    for i in 0..iobs.len() {
        let base_fuse = if i < MCS_PER_FB {
            5696
        } else {
            11824
        };
        let res = read_small_iob_logical(fuses, base_fuse, i % MCS_PER_FB);
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

/// Internal function for parsing an XC2C32A bitstream
pub fn read_32a_bitstream_logical(fuses: &[bool]) -> Result<XC2BitstreamBits, &'static str> {
    let mut fb = [XC2BitstreamFB::default(); 2];
    for i in 0..fb.len() {
        let base_fuse = if i < MCS_PER_FB {
            5696
        } else {
            11824
        };
        let res = read_fb_logical(XC2Device::XC2C32A, fuses, i as u32, base_fuse);
        if let Err(err) = res {
            return Err(err);
        }
        fb[i] = res.unwrap();
    };

    let mut iobs = [XC2MCSmallIOB::default(); 32];
    for i in 0..iobs.len() {
        let base_fuse = if i < MCS_PER_FB {
            5696
        } else {
            11824
        };
        let res = read_small_iob_logical(fuses, base_fuse, i % MCS_PER_FB);
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

/// Internal function for parsing an XC2C64 bitstream
pub fn read_64_bitstream_logical(fuses: &[bool]) -> Result<XC2BitstreamBits, &'static str> {
    let mut fb = [XC2BitstreamFB::default(); 4];
    for i in 0..fb.len() {
        let base_fuse = match i {
            0...15 => 6016,
            16...31 => 12464,
            32...47 => 18912,
            48...63 => 25360,
            _ => unreachable!(),
        };
        let res = read_fb_logical(XC2Device::XC2C64, fuses, i as u32, base_fuse);
        if let Err(err) = res {
            return Err(err);
        }
        fb[i] = res.unwrap();
    };

    let mut iobs = [XC2MCSmallIOB::default(); 64];
    for i in 0..iobs.len() {
        let base_fuse = match i {
            0...15 => 6016,
            16...31 => 12464,
            32...47 => 18912,
            48...63 => 25360,
            _ => unreachable!(),
        };
        let res = read_small_iob_logical(fuses, base_fuse, i % MCS_PER_FB);
        if let Err(err) = res {
            return Err(err);
        }
        iobs[i] = res.unwrap();
    }

    let global_nets = read_64_global_nets_logical(fuses);

    Ok(XC2BitstreamBits::XC2C64 {
        fb: fb,
        iobs: iobs,
        global_nets: global_nets,
        ovoltage: !fuses[25806],
        ivoltage: !fuses[25807],
    })
}

/// Internal function for parsing an XC2C64A bitstream
pub fn read_64a_bitstream_logical(fuses: &[bool]) -> Result<XC2BitstreamBits, &'static str> {
    let mut fb = [XC2BitstreamFB::default(); 4];
    for i in 0..fb.len() {
        let base_fuse = match i {
            0...15 => 6016,
            16...31 => 12464,
            32...47 => 18912,
            48...63 => 25360,
            _ => unreachable!(),
        };
        let res = read_fb_logical(XC2Device::XC2C64A, fuses, i as u32, base_fuse);
        if let Err(err) = res {
            return Err(err);
        }
        fb[i] = res.unwrap();
    };

    let mut iobs = [XC2MCSmallIOB::default(); 64];
    for i in 0..iobs.len() {
        let base_fuse = match i {
            0...15 => 6016,
            16...31 => 12464,
            32...47 => 18912,
            48...63 => 25360,
            _ => unreachable!(),
        };
        let res = read_small_iob_logical(fuses, base_fuse, i % MCS_PER_FB);
        if let Err(err) = res {
            return Err(err);
        }
        iobs[i] = res.unwrap();
    }

    let global_nets = read_64_global_nets_logical(fuses);

    Ok(XC2BitstreamBits::XC2C64A {
        fb: fb,
        iobs: iobs,
        global_nets: global_nets,
        legacy_ovoltage: !fuses[25806],
        legacy_ivoltage: !fuses[25807],
        ivoltage: [
            !fuses[25808],
            !fuses[25810],
        ],
        ovoltage: [
            !fuses[25809],
            !fuses[25811],
        ]
    })
}

/// Internal function for parsing an XC2C128 bitstream
pub fn read_128_bitstream_logical(fuses: &[bool]) -> Result<XC2BitstreamBits, &'static str> {
    let mut fb = [XC2BitstreamFB::default(); 8];
    let mut iobs = [XC2MCLargeIOB::default(); 100];
    for i in 0..fb.len() {
        let base_fuse = match i {
            0 => 0,
            1 => 6908,
            2 => 13816,
            3 => 20737,
            4 => 27658,
            5 => 34579,
            6 => 41487,
            7 => 48408,
            _ => unreachable!(),
        };
        let res = read_fb_logical(XC2Device::XC2C128, fuses, i as u32, base_fuse);
        if let Err(err) = res {
            return Err(err);
        }
        fb[i] = res.unwrap();

        let zia_row_width = zia_get_row_width(XC2Device::XC2C128);
        let mut iob_fuse = base_fuse + zia_row_width * INPUTS_PER_ANDTERM + INPUTS_PER_ANDTERM * 2 * ANDTERMS_PER_FB +
            ANDTERMS_PER_FB * MCS_PER_FB;
        for ff in 0..MCS_PER_FB {
            let iob = fb_ff_num_to_iob_num(XC2Device::XC2C128, i as u32, ff as u32);
            if iob.is_some() {
                let res = read_large_iob_logical(fuses, iob_fuse);
                if let Err(err) = res {
                    return Err(err);
                }
                iobs[iob.unwrap() as usize] = res.unwrap();
                iob_fuse += 29;
            } else {
                iob_fuse += 16;
            }
        }
    };

    let global_nets = read_128_global_nets_logical(fuses);

    Ok(XC2BitstreamBits::XC2C128 {
        fb: fb,
        iobs: iobs,
        global_nets: global_nets,
        clock_div: read_128_clock_div_logical(fuses),
        data_gate: !fuses[55335],
        use_vref: !fuses[55340],
        ivoltage: [
            !fuses[55336],
            !fuses[55337],
        ],
        ovoltage: [
            !fuses[55338],
            !fuses[55339],
        ]
    })
}

/// Processes a fuse array into a bitstream object
pub fn process_jed(fuses: &[bool], device: &str) -> Result<XC2Bitstream, &'static str> {

    let device_combination = parse_part_name_string(device);
    if device_combination.is_none() {
        return Err("malformed device name");
    }

    let (part, spd, pkg) = device_combination.unwrap();

    match part {
        XC2Device::XC2C32 => {
            if fuses.len() != 12274 {
                return Err("wrong number of fuses");
            }
            let bits = read_32_bitstream_logical(fuses);
            if let Err(err) = bits {
                return Err(err);
            }
            Ok(XC2Bitstream {
                speed_grade: spd,
                package: pkg,
                bits: bits.unwrap(),
            })
        },
        XC2Device::XC2C32A => {
            if fuses.len() != 12278 {
                return Err("wrong number of fuses");
            }
            let bits = read_32a_bitstream_logical(fuses);
            if let Err(err) = bits {
                return Err(err);
            }
            Ok(XC2Bitstream {
                speed_grade: spd,
                package: pkg,
                bits: bits.unwrap(),
            })
        },
        XC2Device::XC2C64 => {
            if fuses.len() != 25808 {
                return Err("wrong number of fuses");
            }
            let bits = read_64_bitstream_logical(fuses);
            if let Err(err) = bits {
                return Err(err);
            }
            Ok(XC2Bitstream {
                speed_grade: spd,
                package: pkg,
                bits: bits.unwrap(),
            })
        },
        XC2Device::XC2C64A => {
            if fuses.len() != 25812 {
                return Err("wrong number of fuses");
            }
            let bits = read_64a_bitstream_logical(fuses);
            if let Err(err) = bits {
                return Err(err);
            }
            Ok(XC2Bitstream {
                speed_grade: spd,
                package: pkg,
                bits: bits.unwrap(),
            })
        },
        XC2Device::XC2C128 => {
            if fuses.len() != 55341 {
                return Err("wrong number of fuses");
            }
            let bits = read_128_bitstream_logical(fuses);
            if let Err(err) = bits {
                return Err(err);
            }
            Ok(XC2Bitstream {
                speed_grade: spd,
                package: pkg,
                bits: bits.unwrap(),
            })
        },
        _ => Err("unsupported part"),
    }
}
