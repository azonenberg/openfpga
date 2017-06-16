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
use fusemap_logical::{fb_fuse_idx, gck_fuse_idx, gsr_fuse_idx, gts_fuse_idx, global_term_fuse_idx,
                      total_logical_fuse_count, clock_div_fuse_idx};
use fusemap_physical::{fuse_array_dims, gck_fuse_coords, gsr_fuse_coords, gts_fuse_coords, global_term_fuse_coord,
                       clock_div_fuse_coord};
use iob::{read_small_iob_logical, read_large_iob_logical, read_32_extra_ibuf_logical, read_32_extra_ibuf_physical};
use mc::{write_small_mc_to_jed, write_large_mc_to_jed};
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

        write!(writer, "QF{}*\n", total_logical_fuse_count(self.bits.device_type()))?;
        write!(writer, "N DEVICE {}-{}-{}*\n\n", self.bits.device_type(), self.speed_grade, self.package)?;

        self.bits.write_jed(writer)?;

        write!(writer, "\x030000\n")?;

        Ok(())
    }

    /// Converts the bitstream into a FuseArray object so that it can be written to the native "crbit" format
    pub fn to_crbit(&self) -> FuseArray {
        let (w, h) = fuse_array_dims(self.bits.device_type());
        let mut fuse_array = FuseArray::from_dim(w, h);

        fuse_array.dev_name_str = Some(format!("{}-{}-{}", self.bits.device_type(), self.speed_grade, self.package));

        self.bits.to_crbit(&mut fuse_array);

        fuse_array
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
            XC2Device::XC2C256 => {
                Ok(XC2Bitstream {
                    speed_grade: speed_grade,
                    package: package,
                    bits: XC2BitstreamBits::XC2C256 {
                        fb: [XC2BitstreamFB::default(); 16],
                        iobs: [XC2MCLargeIOB::default(); 184],
                        global_nets: XC2GlobalNets::default(),
                        ivoltage: [false, false],
                        ovoltage: [false, false],
                        data_gate: false,
                        use_vref: false,
                        clock_div: XC2ClockDiv::default(),
                    }
                })
            },
            XC2Device::XC2C384 => {
                Ok(XC2Bitstream {
                    speed_grade: speed_grade,
                    package: package,
                    bits: XC2BitstreamBits::XC2C384 {
                        fb: [XC2BitstreamFB::default(); 24],
                        iobs: [XC2MCLargeIOB::default(); 240],
                        global_nets: XC2GlobalNets::default(),
                        ivoltage: [false, false, false, false],
                        ovoltage: [false, false, false, false],
                        data_gate: false,
                        use_vref: false,
                        clock_div: XC2ClockDiv::default(),
                    }
                })
            },
            XC2Device::XC2C512 => {
                Ok(XC2Bitstream {
                    speed_grade: speed_grade,
                    package: package,
                    bits: XC2BitstreamBits::XC2C512 {
                        fb: [XC2BitstreamFB::default(); 32],
                        iobs: [XC2MCLargeIOB::default(); 270],
                        global_nets: XC2GlobalNets::default(),
                        ivoltage: [false, false, false, false],
                        ovoltage: [false, false, false, false],
                        data_gate: false,
                        use_vref: false,
                        clock_div: XC2ClockDiv::default(),
                    }
                })
            }
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

    /// Write the crbit representation of the global net settings to the given `fuse_array`.
    pub fn to_crbit(&self, device: XC2Device, fuse_array: &mut FuseArray) {
        let ((gck0x, gck0y), (gck1x, gck1y), (gck2x, gck2y)) = gck_fuse_coords(device);
        fuse_array.set(gck0x, gck0y, self.gck_enable[0]);
        fuse_array.set(gck1x, gck1y, self.gck_enable[1]);
        fuse_array.set(gck2x, gck2y, self.gck_enable[2]);

        let ((gsren_x, gsren_y), (gsrinv_x, gsrinv_y)) = gsr_fuse_coords(device);
        fuse_array.set(gsren_x, gsren_y, self.gsr_enable);
        fuse_array.set(gsrinv_x, gsrinv_y, self.gsr_invert);

        let (((gts0en_x, gts0en_y), (gts0inv_x, gts0inv_y)), ((gts1en_x, gts1en_y), (gts1inv_x, gts1inv_y)),
             ((gts2en_x, gts2en_y), (gts2inv_x, gts2inv_y)), ((gts3en_x, gts3en_y), (gts3inv_x, gts3inv_y))) =
                gts_fuse_coords(device);
        fuse_array.set(gts0en_x, gts0en_y, !self.gts_enable[0]);
        fuse_array.set(gts0inv_x, gts0inv_y, self.gts_invert[0]);
        fuse_array.set(gts1en_x, gts1en_y, !self.gts_enable[1]);
        fuse_array.set(gts1inv_x, gts1inv_y, self.gts_invert[1]);
        fuse_array.set(gts2en_x, gts2en_y, !self.gts_enable[2]);
        fuse_array.set(gts2inv_x, gts2inv_y, self.gts_invert[2]);
        fuse_array.set(gts3en_x, gts3en_y, !self.gts_enable[3]);
        fuse_array.set(gts3inv_x, gts3inv_y, self.gts_invert[3]);

        let (term_x, term_y) = global_term_fuse_coord(device);
        fuse_array.set(term_x, term_y, self.global_pu);
    }
}

/// Internal function to read the global nets
fn read_global_nets_logical(device: XC2Device, fuses: &[bool]) -> XC2GlobalNets {
    XC2GlobalNets {
        gck_enable: [
            fuses[gck_fuse_idx(device) + 0],
            fuses[gck_fuse_idx(device) + 1],
            fuses[gck_fuse_idx(device) + 2],
        ],
        gsr_enable: fuses[gsr_fuse_idx(device) + 1],
        gsr_invert: fuses[gsr_fuse_idx(device) + 0],
        gts_enable: [
            !fuses[gts_fuse_idx(device) + 1],
            !fuses[gts_fuse_idx(device) + 3],
            !fuses[gts_fuse_idx(device) + 5],
            !fuses[gts_fuse_idx(device) + 7],
        ],
        gts_invert: [
            fuses[gts_fuse_idx(device) + 0],
            fuses[gts_fuse_idx(device) + 2],
            fuses[gts_fuse_idx(device) + 4],
            fuses[gts_fuse_idx(device) + 6],
        ],
        global_pu: fuses[global_term_fuse_idx(device)],
    }
}

/// Internal function to read the global nets
fn read_global_nets_physical(device: XC2Device, fuse_array: &FuseArray) -> XC2GlobalNets {
    let ((gck0x, gck0y), (gck1x, gck1y), (gck2x, gck2y)) = gck_fuse_coords(device);

    let ((gsren_x, gsren_y), (gsrinv_x, gsrinv_y)) = gsr_fuse_coords(device);

    let (((gts0en_x, gts0en_y), (gts0inv_x, gts0inv_y)), ((gts1en_x, gts1en_y), (gts1inv_x, gts1inv_y)),
         ((gts2en_x, gts2en_y), (gts2inv_x, gts2inv_y)), ((gts3en_x, gts3en_y), (gts3inv_x, gts3inv_y))) =
            gts_fuse_coords(device);

    let (term_x, term_y) = global_term_fuse_coord(device);

    XC2GlobalNets {
        gck_enable: [
            fuse_array.get(gck0x, gck0y),
            fuse_array.get(gck1x, gck1y),
            fuse_array.get(gck2x, gck2y),
        ],
        gsr_enable: fuse_array.get(gsren_x, gsren_y),
        gsr_invert: fuse_array.get(gsrinv_x, gsrinv_y),
        gts_enable: [
            !fuse_array.get(gts0en_x, gts0en_y),
            !fuse_array.get(gts1en_x, gts1en_y),
            !fuse_array.get(gts2en_x, gts2en_y),
            !fuse_array.get(gts3en_x, gts3en_y),
        ],
        gts_invert: [
            fuse_array.get(gts0inv_x, gts0inv_y),
            fuse_array.get(gts1inv_x, gts1inv_y),
            fuse_array.get(gts2inv_x, gts2inv_y),
            fuse_array.get(gts3inv_x, gts3inv_y),
        ],
        global_pu: fuse_array.get(term_x, term_y),
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
#[derive(Copy, Clone, Debug)]
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
fn read_clock_div_logical(device: XC2Device, fuses: &[bool]) -> XC2ClockDiv {
    let clock_fuse_block = clock_div_fuse_idx(device);

    XC2ClockDiv {
        delay: !fuses[clock_fuse_block + 4],
        enabled: !fuses[clock_fuse_block],
        div_ratio: match (fuses[clock_fuse_block + 1], fuses[clock_fuse_block + 2], fuses[clock_fuse_block + 3]) {
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
    },
    XC2C256 {
        fb: [XC2BitstreamFB; 16],
        iobs: [XC2MCLargeIOB; 184],
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
    },
    XC2C384 {
        fb: [XC2BitstreamFB; 24],
        iobs: [XC2MCLargeIOB; 240],
        global_nets: XC2GlobalNets,
        clock_div: XC2ClockDiv,
        /// Whether the DataGate feature is used
        data_gate: bool,
        /// Whether I/O standards with VREF are used
        use_vref: bool,
        /// Voltage level control for each I/O bank
        ///
        /// `false` = low, `true` = high
        ivoltage: [bool; 4],
        /// Voltage level control for each I/O bank
        ///
        /// `false` = low, `true` = high
        ovoltage: [bool; 4],
    },
    XC2C512 {
        fb: [XC2BitstreamFB; 32],
        iobs: [XC2MCLargeIOB; 270],
        global_nets: XC2GlobalNets,
        clock_div: XC2ClockDiv,
        /// Whether the DataGate feature is used
        data_gate: bool,
        /// Whether I/O standards with VREF are used
        use_vref: bool,
        /// Voltage level control for each I/O bank
        ///
        /// `false` = low, `true` = high
        ivoltage: [bool; 4],
        /// Voltage level control for each I/O bank
        ///
        /// `false` = low, `true` = high
        ovoltage: [bool; 4],
    }
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
            &XC2BitstreamBits::XC2C256{..} => XC2Device::XC2C256,
            &XC2BitstreamBits::XC2C384{..} => XC2Device::XC2C384,
            &XC2BitstreamBits::XC2C512{..} => XC2Device::XC2C512,
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
            &XC2BitstreamBits::XC2C256{ref fb, ..} => fb,
            &XC2BitstreamBits::XC2C384{ref fb, ..} => fb,
            &XC2BitstreamBits::XC2C512{ref fb, ..} => fb,
        }
    }

    /// Helper to extract only the global net data without having to perform an explicit `match`
    pub fn get_global_nets(&self) -> &XC2GlobalNets {
        match self {
            &XC2BitstreamBits::XC2C32{ref global_nets, ..} => global_nets,
            &XC2BitstreamBits::XC2C32A{ref global_nets, ..} => global_nets,
            &XC2BitstreamBits::XC2C64{ref global_nets, ..} => global_nets,
            &XC2BitstreamBits::XC2C64A{ref global_nets, ..} => global_nets,
            &XC2BitstreamBits::XC2C128{ref global_nets, ..} => global_nets,
            &XC2BitstreamBits::XC2C256{ref global_nets, ..} => global_nets,
            &XC2BitstreamBits::XC2C384{ref global_nets, ..} => global_nets,
            &XC2BitstreamBits::XC2C512{ref global_nets, ..} => global_nets,
        }
    }

    pub fn get_clock_div(&self) -> Option<&XC2ClockDiv> {
        match self {
            &XC2BitstreamBits::XC2C32{..} => None,
            &XC2BitstreamBits::XC2C32A{..} => None,
            &XC2BitstreamBits::XC2C64{..} => None,
            &XC2BitstreamBits::XC2C64A{..} => None,
            &XC2BitstreamBits::XC2C128{ref clock_div, ..} => Some(clock_div),
            &XC2BitstreamBits::XC2C256{ref clock_div, ..} => Some(clock_div),
            &XC2BitstreamBits::XC2C384{ref clock_div, ..} => Some(clock_div),
            &XC2BitstreamBits::XC2C512{ref clock_div, ..} => Some(clock_div),
        }
    }

    /// Convert the actual bitstream bits to crbit format
    pub fn to_crbit(&self, fuse_array: &mut FuseArray) {
        // FBs
        for i in 0..self.device_type().num_fbs() {
            self.get_fb()[i].to_crbit(self.device_type(), i as u32, fuse_array);
        }

        // IOBs
        match self {
            // This match is needed because the different sizes have different IOB structures, and fixed-sized arrays
            // are gimped enough that returning an array of trait objects is hard.
            &XC2BitstreamBits::XC2C32 {ref iobs, ..} |
            &XC2BitstreamBits::XC2C32A {ref iobs, ..} => {
                for i in 0..self.device_type().num_iobs() {
                    iobs[i].to_crbit(self.device_type(), i as u32, fuse_array);
                }
            },
            &XC2BitstreamBits::XC2C64 {ref iobs, ..} |
            &XC2BitstreamBits::XC2C64A {ref iobs, ..} => {
                for i in 0..self.device_type().num_iobs() {
                    iobs[i].to_crbit(self.device_type(), i as u32, fuse_array);
                }
            },
            &XC2BitstreamBits::XC2C128 {ref iobs, ..} => {
                for i in 0..self.device_type().num_iobs() {
                    iobs[i].to_crbit(self.device_type(), i as u32, fuse_array);
                }
            },
            &XC2BitstreamBits::XC2C256 {ref iobs, ..} => {
                for i in 0..self.device_type().num_iobs() {
                    iobs[i].to_crbit(self.device_type(), i as u32, fuse_array);
                }
            },
            &XC2BitstreamBits::XC2C384 {ref iobs, ..} => {
                for i in 0..self.device_type().num_iobs() {
                    iobs[i].to_crbit(self.device_type(), i as u32, fuse_array);
                }
            },
            &XC2BitstreamBits::XC2C512 {ref iobs, ..} => {
                for i in 0..self.device_type().num_iobs() {
                    iobs[i].to_crbit(self.device_type(), i as u32, fuse_array);
                }
            },
        }

        // Weird extra input-only pin
        match self {
            &XC2BitstreamBits::XC2C32 {ref inpin, ..} |
            &XC2BitstreamBits::XC2C32A {ref inpin, ..} => {
                fuse_array.set(131, 24, inpin.schmitt_trigger);
                fuse_array.set(132, 24, inpin.termination_enabled);
            },
            _ => {}
        }

        // Global nets
        self.get_global_nets().to_crbit(self.device_type(), fuse_array);

        // Clock divider
        if let Some(clock_div) = self.get_clock_div() {
            let ((clken_x, clken_y), (clkdiv0_x, clkdiv0_y), (clkdiv1_x, clkdiv1_y), (clkdiv2_x, clkdiv2_y),
                (clkdelay_x, clkdelay_y)) = clock_div_fuse_coord(self.device_type());

            fuse_array.set(clken_x, clken_y, !clock_div.enabled);

            let divratio = match clock_div.div_ratio {
                XC2ClockDivRatio::Div2  => (false, false, false),
                XC2ClockDivRatio::Div4  => (false, false, true),
                XC2ClockDivRatio::Div6  => (false, true, false),
                XC2ClockDivRatio::Div8  => (false, true, true),
                XC2ClockDivRatio::Div10 => (true, false, false),
                XC2ClockDivRatio::Div12 => (true, false, true),
                XC2ClockDivRatio::Div14 => (true, true, false),
                XC2ClockDivRatio::Div16 => (true, true, true),
            };
            fuse_array.set(clkdiv0_x, clkdiv0_y, divratio.0);
            fuse_array.set(clkdiv1_x, clkdiv1_y, divratio.1);
            fuse_array.set(clkdiv2_x, clkdiv2_y, divratio.2);

            fuse_array.set(clkdelay_x, clkdelay_y, !clock_div.delay);
        }

        // Bank voltages and miscellaneous
        match self {
            &XC2BitstreamBits::XC2C32 {ref ivoltage, ref ovoltage, ..} |
            &XC2BitstreamBits::XC2C32A {legacy_ivoltage: ref ivoltage, legacy_ovoltage: ref ovoltage, ..} => {
                fuse_array.set(130, 24, !*ovoltage);
                fuse_array.set(130, 25, !*ivoltage);
            }
            &XC2BitstreamBits::XC2C64 {ref ivoltage, ref ovoltage, ..} |
            &XC2BitstreamBits::XC2C64A {legacy_ivoltage: ref ivoltage, legacy_ovoltage: ref ovoltage, ..} => {
                fuse_array.set(137, 23, !*ovoltage);
                fuse_array.set(138, 23, !*ivoltage);
            }
            &XC2BitstreamBits::XC2C128 {ref ivoltage, ref ovoltage, ref data_gate, ref use_vref, ..}  => {
                fuse_array.set(371, 67, !*data_gate);

                fuse_array.set(8, 67, !ivoltage[0]);
                fuse_array.set(9, 67, !ovoltage[0]);
                fuse_array.set(368, 67, !ivoltage[1]);
                fuse_array.set(369, 67, !ovoltage[1]);

                fuse_array.set(10, 67, !*use_vref);
            }
            &XC2BitstreamBits::XC2C256 {ref ivoltage, ref ovoltage, ref data_gate, ref use_vref, ..}  => {
                fuse_array.set(518, 23, !*data_gate);

                fuse_array.set(175, 23, !ivoltage[0]);
                fuse_array.set(176, 23, !ovoltage[0]);
                fuse_array.set(515, 23, !ivoltage[1]);
                fuse_array.set(516, 23, !ovoltage[1]);
                
                fuse_array.set(177, 23, !*use_vref);
            }
            &XC2BitstreamBits::XC2C384 {ref ivoltage, ref ovoltage, ref data_gate, ref use_vref, ..}  => {
                fuse_array.set(932, 17, !*data_gate);

                fuse_array.set(936, 17, !ivoltage[0]);
                fuse_array.set(937, 17, !ovoltage[0]);
                fuse_array.set(1864, 17, !ivoltage[1]);
                fuse_array.set(1865, 17, !ovoltage[1]);
                fuse_array.set(1, 17, !ivoltage[2]);
                fuse_array.set(2, 17, !ovoltage[2]);
                fuse_array.set(929, 17, !ivoltage[3]);
                fuse_array.set(930, 17, !ovoltage[3]);
                
                fuse_array.set(3, 17, !*use_vref);
            }
            &XC2BitstreamBits::XC2C512 {ref ivoltage, ref ovoltage, ref data_gate, ref use_vref, ..}  => {
                fuse_array.set(982, 147, !*data_gate);

                fuse_array.set(992, 147, ivoltage[0]);
                fuse_array.set(991, 147, ovoltage[0]);
                fuse_array.set(1965, 147, ivoltage[1]);
                fuse_array.set(1964, 147, ovoltage[1]);
                fuse_array.set(3, 147, ivoltage[2]);
                fuse_array.set(2, 147, ovoltage[2]);
                fuse_array.set(985, 147, ivoltage[3]);
                fuse_array.set(984, 147, ovoltage[3]);
                
                fuse_array.set(1, 147, !*use_vref);
            }
        }

        // A-variant bank voltages
        match self {
            &XC2BitstreamBits::XC2C32A {ref ivoltage, ref ovoltage, ..} => {
                fuse_array.set(131, 25, !ivoltage[0]);
                fuse_array.set(132, 25, !ovoltage[0]);
                fuse_array.set(133, 25, !ivoltage[1]);
                fuse_array.set(134, 25, !ovoltage[1]);
            },
            &XC2BitstreamBits::XC2C64A {ref ivoltage, ref ovoltage, ..} => {
                fuse_array.set(139, 23, !ivoltage[0]);
                fuse_array.set(140, 23, !ovoltage[0]);
                fuse_array.set(141, 23, !ivoltage[1]);
                fuse_array.set(142, 23, !ovoltage[1]);
            },
            _ => {}
        }
    }

    /// Dump a human-readable explanation of the bitstream to the given `writer` object.
    pub fn dump_human_readable(&self, writer: &mut Write) -> Result<(), io::Error> {
        write!(writer, "device type: {}\n", self.device_type())?;

        // Bank voltages
        match self {
            &XC2BitstreamBits::XC2C32 {ref ivoltage, ref ovoltage, ..} |
            &XC2BitstreamBits::XC2C64 {ref ivoltage, ref ovoltage, ..} => {
                write!(writer, "output voltage range: {}\n", if *ovoltage {"high"} else {"low"})?;
                write!(writer, "input voltage range: {}\n", if *ivoltage {"high"} else {"low"})?;
            },
            &XC2BitstreamBits::XC2C32A {ref legacy_ivoltage, ref legacy_ovoltage, ref ivoltage, ref ovoltage, ..} |
            &XC2BitstreamBits::XC2C64A {ref legacy_ivoltage, ref legacy_ovoltage, ref ivoltage, ref ovoltage, ..} => {
                write!(writer, "legacy output voltage range: {}\n", if *legacy_ovoltage {"high"} else {"low"})?;
                write!(writer, "legacy input voltage range: {}\n", if *legacy_ivoltage {"high"} else {"low"})?;
                write!(writer, "bank 0 output voltage range: {}\n", if ovoltage[0] {"high"} else {"low"})?;
                write!(writer, "bank 1 output voltage range: {}\n", if ovoltage[1] {"high"} else {"low"})?;
                write!(writer, "bank 0 input voltage range: {}\n", if ivoltage[0] {"high"} else {"low"})?;
                write!(writer, "bank 1 input voltage range: {}\n", if ivoltage[1] {"high"} else {"low"})?;
            },
            &XC2BitstreamBits::XC2C128 {ref ivoltage, ref ovoltage, ref data_gate, ref use_vref, ..} |
            &XC2BitstreamBits::XC2C256 {ref ivoltage, ref ovoltage, ref data_gate, ref use_vref, ..} => {
                write!(writer, "bank 0 output voltage range: {}\n", if ovoltage[0] {"high"} else {"low"})?;
                write!(writer, "bank 1 output voltage range: {}\n", if ovoltage[1] {"high"} else {"low"})?;
                write!(writer, "bank 0 input voltage range: {}\n", if ivoltage[0] {"high"} else {"low"})?;
                write!(writer, "bank 1 input voltage range: {}\n", if ivoltage[1] {"high"} else {"low"})?;
                write!(writer, "DataGate used: {}\n", if *data_gate {"yes"} else {"no"})?;
                write!(writer, "VREF used: {}\n", if *use_vref {"yes"} else {"no"})?;
            },
            &XC2BitstreamBits::XC2C384 {ref ivoltage, ref ovoltage, ref data_gate, ref use_vref, ..} |
            &XC2BitstreamBits::XC2C512 {ref ivoltage, ref ovoltage, ref data_gate, ref use_vref, ..} => {
                write!(writer, "bank 0 output voltage range: {}\n", if ovoltage[0] {"high"} else {"low"})?;
                write!(writer, "bank 1 output voltage range: {}\n", if ovoltage[1] {"high"} else {"low"})?;
                write!(writer, "bank 2 output voltage range: {}\n", if ovoltage[2] {"high"} else {"low"})?;
                write!(writer, "bank 3 output voltage range: {}\n", if ovoltage[3] {"high"} else {"low"})?;
                write!(writer, "bank 0 input voltage range: {}\n", if ivoltage[0] {"high"} else {"low"})?;
                write!(writer, "bank 1 input voltage range: {}\n", if ivoltage[1] {"high"} else {"low"})?;
                write!(writer, "bank 2 input voltage range: {}\n", if ivoltage[2] {"high"} else {"low"})?;
                write!(writer, "bank 3 input voltage range: {}\n", if ivoltage[3] {"high"} else {"low"})?;
                write!(writer, "DataGate used: {}\n", if *data_gate {"yes"} else {"no"})?;
                write!(writer, "VREF used: {}\n", if *use_vref {"yes"} else {"no"})?;
            }
        }

        // Clock divider
        if let Some(clock_div) = self.get_clock_div() {
            clock_div.dump_human_readable(writer)?;
        }

        // Global net configuration
        self.get_global_nets().dump_human_readable(writer)?;

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
            &XC2BitstreamBits::XC2C256 {ref iobs, ..} => {
                for i in 0..self.device_type().num_iobs() {
                    iobs[i].dump_human_readable(self.device_type(), i as u32, writer)?;
                }
            },
            &XC2BitstreamBits::XC2C384 {ref iobs, ..} => {
                for i in 0..self.device_type().num_iobs() {
                    iobs[i].dump_human_readable(self.device_type(), i as u32, writer)?;
                }
            },
            &XC2BitstreamBits::XC2C512 {ref iobs, ..} => {
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
        // FBs
        match self {
            &XC2BitstreamBits::XC2C32 {ref fb, ref iobs, ..} |
            &XC2BitstreamBits::XC2C32A {ref fb, ref iobs, ..} => {
                // Each FB
                for fb_i in 0..2 {
                    let fuse_base = fb_fuse_idx(XC2Device::XC2C32, fb_i as u32);

                    fb[fb_i].write_to_jed(XC2Device::XC2C32, fuse_base, writer)?;

                    // Macrocells
                    write_small_mc_to_jed(writer, XC2Device::XC2C32, &fb[fb_i], iobs, fb_i, fuse_base)?;
                }
            }
            &XC2BitstreamBits::XC2C64 {ref fb, ref iobs, ..} |
            &XC2BitstreamBits::XC2C64A {ref fb, ref iobs, ..} => {
                // Each FB
                for fb_i in 0..4 {
                    let fuse_base = fb_fuse_idx(XC2Device::XC2C64, fb_i as u32);

                    fb[fb_i].write_to_jed(XC2Device::XC2C64, fuse_base, writer)?;

                    // Macrocells
                    write_small_mc_to_jed(writer, XC2Device::XC2C64, &fb[fb_i], iobs, fb_i, fuse_base)?;
                }
            }
            &XC2BitstreamBits::XC2C128 {ref fb, ref iobs, ..}  => {
                // Each FB
                for fb_i in 0..8 {
                    let fuse_base = fb_fuse_idx(XC2Device::XC2C128, fb_i as u32);

                    fb[fb_i].write_to_jed(XC2Device::XC2C128, fuse_base, writer)?;

                    // Macrocells
                    write_large_mc_to_jed(writer, XC2Device::XC2C128, &fb[fb_i], iobs, fb_i, fuse_base)?;
                }
            }
            &XC2BitstreamBits::XC2C256 {ref fb, ref iobs, ..}  => {
                // Each FB
                for fb_i in 0..16 {
                    let fuse_base = fb_fuse_idx(XC2Device::XC2C256, fb_i as u32);

                    fb[fb_i].write_to_jed(XC2Device::XC2C256, fuse_base, writer)?;

                    // Macrocells
                    write_large_mc_to_jed(writer, XC2Device::XC2C256, &fb[fb_i], iobs, fb_i, fuse_base)?;
                }
            }
            &XC2BitstreamBits::XC2C384 {ref fb, ref iobs, ..}  => {
                // Each FB
                for fb_i in 0..24 {
                    let fuse_base = fb_fuse_idx(XC2Device::XC2C384, fb_i as u32);

                    fb[fb_i].write_to_jed(XC2Device::XC2C384, fuse_base, writer)?;

                    // Macrocells
                    write_large_mc_to_jed(writer, XC2Device::XC2C384, &fb[fb_i], iobs, fb_i, fuse_base)?;
                }
            }
            &XC2BitstreamBits::XC2C512 {ref fb, ref iobs, ..}  => {
                // Each FB
                for fb_i in 0..32 {
                    let fuse_base = fb_fuse_idx(XC2Device::XC2C512, fb_i as u32);

                    fb[fb_i].write_to_jed(XC2Device::XC2C512, fuse_base, writer)?;

                    // Macrocells
                    write_large_mc_to_jed(writer, XC2Device::XC2C512, &fb[fb_i], iobs, fb_i, fuse_base)?;
                }
            }
        }

        // GCK
        write!(writer, "L{:06} {}{}{}*\n",
            gck_fuse_idx(self.device_type()),
            if self.get_global_nets().gck_enable[0] {"1"} else {"0"},
            if self.get_global_nets().gck_enable[1] {"1"} else {"0"},
            if self.get_global_nets().gck_enable[2] {"1"} else {"0"})?;

        // Clock divider
        if let Some(clock_div) = self.get_clock_div() {
            let clock_fuse_block = clock_div_fuse_idx(self.device_type());

            write!(writer, "L{:06} {}{}*\n",
                clock_fuse_block,
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
            write!(writer, "L{:06} {}*\n",
                clock_fuse_block + 4,
                if clock_div.delay {"0"} else {"1"})?;
        }

        // GSR
        write!(writer, "L{:06} {}{}*\n",
            gsr_fuse_idx(self.device_type()),
            if self.get_global_nets().gsr_invert {"1"} else {"0"},
            if self.get_global_nets().gsr_enable {"1"} else {"0"})?;

        // GTS
        write!(writer, "L{:06} {}{}{}{}{}{}{}{}*\n",
            gts_fuse_idx(self.device_type()),
            if self.get_global_nets().gts_invert[0] {"1"} else {"0"},
            if self.get_global_nets().gts_enable[0] {"0"} else {"1"},
            if self.get_global_nets().gts_invert[1] {"1"} else {"0"},
            if self.get_global_nets().gts_enable[1] {"0"} else {"1"},
            if self.get_global_nets().gts_invert[2] {"1"} else {"0"},
            if self.get_global_nets().gts_enable[2] {"0"} else {"1"},
            if self.get_global_nets().gts_invert[3] {"1"} else {"0"},
            if self.get_global_nets().gts_enable[3] {"0"} else {"1"})?;

        // Global termination
        write!(writer, "L{:06} {}*\n",
            global_term_fuse_idx(self.device_type()),
            if self.get_global_nets().global_pu {"1"} else {"0"})?;

        // Bank voltages and miscellaneous
        match self {
            &XC2BitstreamBits::XC2C32 {ref inpin, ref ivoltage, ref ovoltage, ..} |
            &XC2BitstreamBits::XC2C32A {ref inpin, legacy_ivoltage: ref ivoltage,
                legacy_ovoltage: ref ovoltage, ..} => {

                write!(writer, "L012270 {}*\n", if *ovoltage {"0"} else {"1"})?;
                write!(writer, "L012271 {}*\n", if *ivoltage {"0"} else {"1"})?;

                write!(writer, "L012272 {}{}*\n",
                    if inpin.schmitt_trigger {"1"} else {"0"},
                    if inpin.termination_enabled {"1"} else {"0"})?;
            }
            &XC2BitstreamBits::XC2C64 {ref ivoltage, ref ovoltage, ..} |
            &XC2BitstreamBits::XC2C64A {legacy_ivoltage: ref ivoltage, legacy_ovoltage: ref ovoltage, ..} => {
                write!(writer, "L025806 {}*\n", if *ovoltage {"0"} else {"1"})?;
                write!(writer, "L025807 {}*\n", if *ivoltage {"0"} else {"1"})?;
            }
            &XC2BitstreamBits::XC2C128 {ref ivoltage, ref ovoltage, ref data_gate, ref use_vref, ..}  => {
                write!(writer, "L055335 {}*\n", if *data_gate {"0"} else {"1"})?;

                write!(writer, "L055336 {}{}*\n", if ivoltage[0] {"0"} else {"1"}, if ivoltage[1] {"0"} else {"1"})?;
                write!(writer, "L055338 {}{}*\n", if ovoltage[0] {"0"} else {"1"}, if ovoltage[1] {"0"} else {"1"})?;

                write!(writer, "L055340 {}*\n", if *use_vref {"0"} else {"1"})?;
            }
            &XC2BitstreamBits::XC2C256 {ref ivoltage, ref ovoltage, ref data_gate, ref use_vref, ..}  => {
                write!(writer, "L123243 {}*\n", if *data_gate {"0"} else {"1"})?;

                write!(writer, "L123244 {}{}*\n", if ivoltage[0] {"0"} else {"1"}, if ivoltage[1] {"0"} else {"1"})?;
                write!(writer, "L123246 {}{}*\n", if ovoltage[0] {"0"} else {"1"}, if ovoltage[1] {"0"} else {"1"})?;

                write!(writer, "L123248 {}*\n", if *use_vref {"0"} else {"1"})?;
            }
            &XC2BitstreamBits::XC2C384 {ref ivoltage, ref ovoltage, ref data_gate, ref use_vref, ..}  => {
                write!(writer, "L209347 {}*\n", if *data_gate {"0"} else {"1"})?;

                write!(writer, "L209348 {}{}{}{}*\n",
                    if ivoltage[0] {"0"} else {"1"}, if ivoltage[1] {"0"} else {"1"},
                    if ivoltage[2] {"0"} else {"1"}, if ivoltage[3] {"0"} else {"1"})?;
                write!(writer, "L209352 {}{}{}{}*\n",
                    if ovoltage[0] {"0"} else {"1"}, if ovoltage[1] {"0"} else {"1"},
                    if ovoltage[2] {"0"} else {"1"}, if ovoltage[3] {"0"} else {"1"})?;

                write!(writer, "L209356 {}*\n", if *use_vref {"0"} else {"1"})?;
            }
            &XC2BitstreamBits::XC2C512 {ref ivoltage, ref ovoltage, ref data_gate, ref use_vref, ..}  => {
                write!(writer, "L296393 {}*\n", if *data_gate {"0"} else {"1"})?;

                write!(writer, "L296394 {}{}{}{}*\n",
                    if ivoltage[0] {"1"} else {"0"}, if ivoltage[1] {"1"} else {"0"},
                    if ivoltage[2] {"1"} else {"0"}, if ivoltage[3] {"1"} else {"0"})?;
                write!(writer, "L296398 {}{}{}{}*\n",
                    if ovoltage[0] {"1"} else {"0"}, if ovoltage[1] {"1"} else {"0"},
                    if ovoltage[2] {"1"} else {"0"}, if ovoltage[3] {"1"} else {"0"})?;

                write!(writer, "L296402 {}*\n", if *use_vref {"0"} else {"1"})?;
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

/// Common logic for reading bitstreams on "small" devices
pub fn read_bitstream_logical_common_small(fuses: &[bool], device: XC2Device,
    fb: &mut [XC2BitstreamFB], iobs: &mut [XC2MCSmallIOB]) -> Result<(), &'static str> {

    for i in 0..fb.len() {
        let base_fuse = fb_fuse_idx(device, i as u32);
        let res = read_fb_logical(device, fuses, i as u32, base_fuse)?;
        fb[i] = res;

        let zia_row_width = zia_get_row_width(device);
        let size_of_zia = zia_row_width * INPUTS_PER_ANDTERM;
        let size_of_and = INPUTS_PER_ANDTERM * 2 * ANDTERMS_PER_FB;
        let size_of_or = ANDTERMS_PER_FB * MCS_PER_FB;
        let mut iob_fuse = base_fuse + size_of_zia + size_of_and + size_of_or;
        for ff in 0..MCS_PER_FB {
            let iob = fb_ff_num_to_iob_num(device, i as u32, ff as u32);
            let res = read_small_iob_logical(fuses, iob_fuse)?;
            iobs[iob.unwrap() as usize] = res;
            iob_fuse += 27;
        }
    };

    Ok(())
}

/// Common logic for reading bitstreams on "large" devices
pub fn read_bitstream_logical_common_large(fuses: &[bool], device: XC2Device,
    fb: &mut [XC2BitstreamFB], iobs: &mut [XC2MCLargeIOB]) -> Result<(), &'static str> {

    for i in 0..fb.len() {
        let base_fuse = fb_fuse_idx(device, i as u32);
        let res = read_fb_logical(device, fuses, i as u32, base_fuse)?;
        fb[i] = res;

        let zia_row_width = zia_get_row_width(device);
        let size_of_zia = zia_row_width * INPUTS_PER_ANDTERM;
        let size_of_and = INPUTS_PER_ANDTERM * 2 * ANDTERMS_PER_FB;
        let size_of_or = ANDTERMS_PER_FB * MCS_PER_FB;
        let mut iob_fuse = base_fuse + size_of_zia + size_of_and + size_of_or;
        for ff in 0..MCS_PER_FB {
            let iob = fb_ff_num_to_iob_num(device, i as u32, ff as u32);
            if iob.is_some() {
                let res = read_large_iob_logical(fuses, iob_fuse)?;
                iobs[iob.unwrap() as usize] = res;
                // Must be not a buried macrocell
                iob_fuse += 29;
            } else {
                // Buried
                iob_fuse += 16;
            }
        }
    };

    Ok(())
}
/// Internal function for parsing an XC2C32 bitstream
pub fn read_32_bitstream_logical(fuses: &[bool]) -> Result<XC2BitstreamBits, &'static str> {
    let mut fb = [XC2BitstreamFB::default(); 2];
    let mut iobs = [XC2MCSmallIOB::default(); 32];
    
    read_bitstream_logical_common_small(fuses, XC2Device::XC2C32, &mut fb, &mut iobs)?;

    let inpin = read_32_extra_ibuf_logical(fuses);

    let global_nets = read_global_nets_logical(XC2Device::XC2C32, fuses);

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
    let mut iobs = [XC2MCSmallIOB::default(); 32];
    
    read_bitstream_logical_common_small(fuses, XC2Device::XC2C32A, &mut fb, &mut iobs)?;

    let inpin = read_32_extra_ibuf_logical(fuses);

    let global_nets = read_global_nets_logical(XC2Device::XC2C32A, fuses);

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
    let mut iobs = [XC2MCSmallIOB::default(); 64];
    
    read_bitstream_logical_common_small(fuses, XC2Device::XC2C64, &mut fb, &mut iobs)?;

    let global_nets = read_global_nets_logical(XC2Device::XC2C64, fuses);

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
    let mut iobs = [XC2MCSmallIOB::default(); 64];
    
    read_bitstream_logical_common_small(fuses, XC2Device::XC2C64A, &mut fb, &mut iobs)?;

    let global_nets = read_global_nets_logical(XC2Device::XC2C64A, fuses);

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
    
    read_bitstream_logical_common_large(fuses, XC2Device::XC2C128, &mut fb, &mut iobs)?;

    let global_nets = read_global_nets_logical(XC2Device::XC2C128, fuses);

    Ok(XC2BitstreamBits::XC2C128 {
        fb: fb,
        iobs: iobs,
        global_nets: global_nets,
        clock_div: read_clock_div_logical(XC2Device::XC2C128, fuses),
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

/// Internal function for parsing an XC2C256 bitstream
pub fn read_256_bitstream_logical(fuses: &[bool]) -> Result<XC2BitstreamBits, &'static str> {
    let mut fb = [XC2BitstreamFB::default(); 16];
    let mut iobs = [XC2MCLargeIOB::default(); 184];
    
    read_bitstream_logical_common_large(fuses, XC2Device::XC2C256, &mut fb, &mut iobs)?;

    let global_nets = read_global_nets_logical(XC2Device::XC2C256, fuses);

    Ok(XC2BitstreamBits::XC2C256 {
        fb: fb,
        iobs: iobs,
        global_nets: global_nets,
        clock_div: read_clock_div_logical(XC2Device::XC2C256, fuses),
        data_gate: !fuses[123243],
        use_vref: !fuses[123248],
        ivoltage: [
            !fuses[123244],
            !fuses[123245],
        ],
        ovoltage: [
            !fuses[123246],
            !fuses[123247],
        ]
    })
}

/// Internal function for parsing an XC2C384 bitstream
pub fn read_384_bitstream_logical(fuses: &[bool]) -> Result<XC2BitstreamBits, &'static str> {
    let mut fb = [XC2BitstreamFB::default(); 24];
    let mut iobs = [XC2MCLargeIOB::default(); 240];
    
    read_bitstream_logical_common_large(fuses, XC2Device::XC2C384, &mut fb, &mut iobs)?;

    let global_nets = read_global_nets_logical(XC2Device::XC2C384, fuses);

    Ok(XC2BitstreamBits::XC2C384 {
        fb: fb,
        iobs: iobs,
        global_nets: global_nets,
        clock_div: read_clock_div_logical(XC2Device::XC2C384, fuses),
        data_gate: !fuses[209347],
        use_vref: !fuses[209356],
        ivoltage: [
            !fuses[209348],
            !fuses[209349],
            !fuses[209350],
            !fuses[209351],
        ],
        ovoltage: [
            !fuses[209352],
            !fuses[209353],
            !fuses[209354],
            !fuses[209355],
        ]
    })
}

/// Internal function for parsing an XC2C512 bitstream
pub fn read_512_bitstream_logical(fuses: &[bool]) -> Result<XC2BitstreamBits, &'static str> {
    let mut fb = [XC2BitstreamFB::default(); 32];
    let mut iobs = [XC2MCLargeIOB::default(); 270];
    
    read_bitstream_logical_common_large(fuses, XC2Device::XC2C512, &mut fb, &mut iobs)?;

    let global_nets = read_global_nets_logical(XC2Device::XC2C512, fuses);

    Ok(XC2BitstreamBits::XC2C512 {
        fb: fb,
        iobs: iobs,
        global_nets: global_nets,
        clock_div: read_clock_div_logical(XC2Device::XC2C512, fuses),
        data_gate: !fuses[296393],
        use_vref: !fuses[296402],
        ivoltage: [
            fuses[296394],
            fuses[296395],
            fuses[296396],
            fuses[296397],
        ],
        ovoltage: [
            fuses[296398],
            fuses[296399],
            fuses[296400],
            fuses[296401],
        ]
    })
}

/// Common logic for reading bitstreams on "small" devices
pub fn read_bitstream_physical_common_small(fuse_array: &FuseArray, device: XC2Device,
    fb: &mut [XC2BitstreamFB], iobs: &mut [XC2MCSmallIOB]) -> Result<(), &'static str> {

    for i in 0..fb.len() {
        fb[i] = XC2BitstreamFB::from_crbit(device, i as u32, fuse_array)?;
    };

    for i in 0..iobs.len() {
        iobs[i] = XC2MCSmallIOB::from_crbit(device, i as u32, fuse_array)?;
    }

    Ok(())
}

/// Internal function for parsing an XC2C32A bitstream
pub fn read_32a_bitstream_physical(fuse_array: &FuseArray) -> Result<XC2BitstreamBits, &'static str> {
    let mut fb = [XC2BitstreamFB::default(); 2];
    let mut iobs = [XC2MCSmallIOB::default(); 32];
    
    read_bitstream_physical_common_small(fuse_array, XC2Device::XC2C32A, &mut fb, &mut iobs)?;

    let inpin = read_32_extra_ibuf_physical(fuse_array);

    let global_nets = read_global_nets_physical(XC2Device::XC2C32A, fuse_array);

    Ok(XC2BitstreamBits::XC2C32A {
        fb: fb,
        iobs: iobs,
        inpin: inpin,
        global_nets: global_nets,
        legacy_ovoltage: !fuse_array.get(130, 24),
        legacy_ivoltage: !fuse_array.get(130, 25),
        ivoltage: [
            !fuse_array.get(131, 25),
            !fuse_array.get(133, 25),
        ],
        ovoltage: [
            !fuse_array.get(132, 25),
            !fuse_array.get(134, 25),
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

    if fuses.len() != total_logical_fuse_count(part) {
        return Err("wrong number of fuses");
    }

    match part {
        XC2Device::XC2C32 => {
            let bits = read_32_bitstream_logical(fuses)?;
            Ok(XC2Bitstream {
                speed_grade: spd,
                package: pkg,
                bits: bits,
            })
        },
        XC2Device::XC2C32A => {
            let bits = read_32a_bitstream_logical(fuses)?;
            Ok(XC2Bitstream {
                speed_grade: spd,
                package: pkg,
                bits: bits,
            })
        },
        XC2Device::XC2C64 => {
            let bits = read_64_bitstream_logical(fuses)?;
            Ok(XC2Bitstream {
                speed_grade: spd,
                package: pkg,
                bits: bits,
            })
        },
        XC2Device::XC2C64A => {
            let bits = read_64a_bitstream_logical(fuses)?;
            Ok(XC2Bitstream {
                speed_grade: spd,
                package: pkg,
                bits: bits,
            })
        },
        XC2Device::XC2C128 => {
            let bits = read_128_bitstream_logical(fuses)?;
            Ok(XC2Bitstream {
                speed_grade: spd,
                package: pkg,
                bits: bits,
            })
        },
        XC2Device::XC2C256 => {
            let bits = read_256_bitstream_logical(fuses)?;
            Ok(XC2Bitstream {
                speed_grade: spd,
                package: pkg,
                bits: bits,
            })
        },
        XC2Device::XC2C384 => {
            let bits = read_384_bitstream_logical(fuses)?;
            Ok(XC2Bitstream {
                speed_grade: spd,
                package: pkg,
                bits: bits,
            })
        },
        XC2Device::XC2C512 => {
            let bits = read_512_bitstream_logical(fuses)?;
            Ok(XC2Bitstream {
                speed_grade: spd,
                package: pkg,
                bits: bits,
            })
        },
    }
}

/// Processes a fuse array (in physical addressing) into a bitstream object
pub fn process_crbit(fuse_array: &FuseArray) -> Result<XC2Bitstream, &'static str> {
    // FIXME: Can we guess the device type from the dimensions?
    if fuse_array.dev_name_str.is_none() {
        return Err("unspecified device name");
    }

    let device_combination = parse_part_name_string(fuse_array.dev_name_str.as_ref().unwrap());
    if device_combination.is_none() {
        return Err("malformed device name");
    }

    let (part, spd, pkg) = device_combination.unwrap();

    if fuse_array.dim() != fuse_array_dims(part) {
        return Err("wrong number of fuses");
    }


    match part {
        XC2Device::XC2C32 => {
            unimplemented!();
            // let bits = read_32_bitstream_physical(fuse_array)?;
            // Ok(XC2Bitstream {
            //     speed_grade: spd,
            //     package: pkg,
            //     bits: bits,
            // })
        },
        XC2Device::XC2C32A => {
            let bits = read_32a_bitstream_physical(fuse_array)?;
            Ok(XC2Bitstream {
                speed_grade: spd,
                package: pkg,
                bits: bits,
            })
        },
        XC2Device::XC2C64 => {
            unimplemented!();
            // let bits = read_64_bitstream_physical(fuse_array)?;
            // Ok(XC2Bitstream {
            //     speed_grade: spd,
            //     package: pkg,
            //     bits: bits,
            // })
        },
        XC2Device::XC2C64A => {
            unimplemented!();
            // let bits = read_64a_bitstream_physical(fuse_array)?;
            // Ok(XC2Bitstream {
            //     speed_grade: spd,
            //     package: pkg,
            //     bits: bits,
            // })
        },
        XC2Device::XC2C128 => {
            unimplemented!();
            // let bits = read_128_bitstream_physical(fuse_array)?;
            // Ok(XC2Bitstream {
            //     speed_grade: spd,
            //     package: pkg,
            //     bits: bits,
            // })
        },
        XC2Device::XC2C256 => {
            unimplemented!();
            // let bits = read_256_bitstream_physical(fuse_array)?;
            // Ok(XC2Bitstream {
            //     speed_grade: spd,
            //     package: pkg,
            //     bits: bits,
            // })
        },
        XC2Device::XC2C384 => {
            unimplemented!();
            // let bits = read_384_bitstream_physical(fuse_array)?;
            // Ok(XC2Bitstream {
            //     speed_grade: spd,
            //     package: pkg,
            //     bits: bits,
            // })
        },
        XC2Device::XC2C512 => {
            unimplemented!();
            // let bits = read_512_bitstream_physical(fuse_array)?;
            // Ok(XC2Bitstream {
            //     speed_grade: spd,
            //     package: pkg,
            //     bits: bits,
            // })
        },
    }
}
