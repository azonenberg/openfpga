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

use jedec::*;

use crate::*;
use crate::fusemap_logical::{fb_fuse_idx, gck_fuse_idx, gsr_fuse_idx, gts_fuse_idx, global_term_fuse_idx,
                             total_logical_fuse_count, clock_div_fuse_idx};
use crate::fusemap_physical::{fuse_array_dims, clock_div_fuse_coord};
use crate::util::{LinebreakSet};
use crate::zia::{zia_get_row_width};

/// Toplevel struct representing an entire Coolrunner-II bitstream
#[derive(Serialize)]
pub struct XC2Bitstream {
    pub speed_grade: XC2Speed,
    pub package: XC2Package,
    pub bits: XC2BitstreamBits,
}

impl XC2Bitstream {
    /// Dump a human-readable explanation of the bitstream to the given `writer` object.
    pub fn dump_human_readable<W: Write>(&self, mut writer: W) -> Result<(), io::Error> {
        write!(writer, "xc2bit dump\n")?;
        write!(writer, "device speed grade: {}\n", self.speed_grade)?;
        write!(writer, "device package: {}\n", self.package)?;
        self.bits.dump_human_readable(&mut writer)?;

        Ok(())
    }

    /// Write a .jed representation of the bitstream to the given `writer` object.
    pub fn to_jed<W: Write>(&self, mut writer: W) -> Result<(), io::Error> {
        write!(writer, ".JED fuse map written by xc2bit\n")?;
        write!(writer, "https://github.com/azonenberg/openfpga\n\n")?;

        let mut linebreaks = LinebreakSet::new();
        let mut jed = JEDECFile::new(total_logical_fuse_count(self.bits.device_type()));
        jed.dev_name_str = Some(format!("{}-{}-{}", self.bits.device_type(), self.speed_grade, self.package));
        self.bits.to_jed(&mut jed, &mut linebreaks);

        jed.write_custom_linebreaks(&mut writer, linebreaks.iter())?;

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

    /// Processes a fuse array into a bitstream object
    pub fn from_jed(jed: &JEDECFile) -> Result<Self, XC2BitError> {
        if jed.dev_name_str.is_none() {
            return Err(XC2BitError::BadDeviceName(String::new()));
        }

        let device = jed.dev_name_str.as_ref().unwrap();

        let device_combination = XC2DeviceSpeedPackage::from_str(device);
        if device_combination.is_none() {
            return Err(XC2BitError::BadDeviceName(device.to_owned()));
        }

        let XC2DeviceSpeedPackage {
            dev, spd, pkg
        } = device_combination.unwrap();

        let fuses = &jed.f;

        if fuses.len() != total_logical_fuse_count(dev) {
            return Err(XC2BitError::WrongFuseCount);
        }

        match dev {
            XC2Device::XC2C32 => {
                let bits = read_32_bitstream_logical(fuses)?;
                Ok(XC2Bitstream {
                    speed_grade: spd,
                    package: pkg,
                    bits,
                })
            },
            XC2Device::XC2C32A => {
                let bits = read_32a_bitstream_logical(fuses)?;
                Ok(XC2Bitstream {
                    speed_grade: spd,
                    package: pkg,
                    bits,
                })
            },
            XC2Device::XC2C64 => {
                let bits = read_64_bitstream_logical(fuses)?;
                Ok(XC2Bitstream {
                    speed_grade: spd,
                    package: pkg,
                    bits,
                })
            },
            XC2Device::XC2C64A => {
                let bits = read_64a_bitstream_logical(fuses)?;
                Ok(XC2Bitstream {
                    speed_grade: spd,
                    package: pkg,
                    bits,
                })
            },
            XC2Device::XC2C128 => {
                let bits = read_128_bitstream_logical(fuses)?;
                Ok(XC2Bitstream {
                    speed_grade: spd,
                    package: pkg,
                    bits,
                })
            },
            XC2Device::XC2C256 => {
                let bits = read_256_bitstream_logical(fuses)?;
                Ok(XC2Bitstream {
                    speed_grade: spd,
                    package: pkg,
                    bits,
                })
            },
            XC2Device::XC2C384 => {
                let bits = read_384_bitstream_logical(fuses)?;
                Ok(XC2Bitstream {
                    speed_grade: spd,
                    package: pkg,
                    bits,
                })
            },
            XC2Device::XC2C512 => {
                let bits = read_512_bitstream_logical(fuses)?;
                Ok(XC2Bitstream {
                    speed_grade: spd,
                    package: pkg,
                    bits,
                })
            },
        }
    }

    /// Processes a fuse array (in physical addressing) into a bitstream object
    pub fn from_crbit(fuse_array: &FuseArray) -> Result<Self, XC2BitError> {
        // FIXME: Can we guess the device type from the dimensions?
        if fuse_array.dev_name_str.is_none() {
            return Err(XC2BitError::BadDeviceName(String::from("")));
        }

        let device_combination = XC2DeviceSpeedPackage::from_str(fuse_array.dev_name_str.as_ref().unwrap());
        if device_combination.is_none() {
            return Err(XC2BitError::BadDeviceName(fuse_array.dev_name_str.as_ref().unwrap().to_owned()));
        }

        let XC2DeviceSpeedPackage {
            dev, spd, pkg
        } = device_combination.unwrap();

        if fuse_array.dim() != fuse_array_dims(dev) {
            return Err(XC2BitError::WrongFuseCount);
        }


        match dev {
            XC2Device::XC2C32 => {
                let bits = read_32_bitstream_physical(fuse_array)?;
                Ok(XC2Bitstream {
                    speed_grade: spd,
                    package: pkg,
                    bits,
                })
            },
            XC2Device::XC2C32A => {
                let bits = read_32a_bitstream_physical(fuse_array)?;
                Ok(XC2Bitstream {
                    speed_grade: spd,
                    package: pkg,
                    bits,
                })
            },
            XC2Device::XC2C64 => {
                let bits = read_64_bitstream_physical(fuse_array)?;
                Ok(XC2Bitstream {
                    speed_grade: spd,
                    package: pkg,
                    bits,
                })
            },
            XC2Device::XC2C64A => {
                let bits = read_64a_bitstream_physical(fuse_array)?;
                Ok(XC2Bitstream {
                    speed_grade: spd,
                    package: pkg,
                    bits,
                })
            },
            XC2Device::XC2C128 => {
                let bits = read_128_bitstream_physical(fuse_array)?;
                Ok(XC2Bitstream {
                    speed_grade: spd,
                    package: pkg,
                    bits,
                })
            },
            XC2Device::XC2C256 => {
                let bits = read_256_bitstream_physical(fuse_array)?;
                Ok(XC2Bitstream {
                    speed_grade: spd,
                    package: pkg,
                    bits,
                })
            },
            XC2Device::XC2C384 => {
                let bits = read_384_bitstream_physical(fuse_array)?;
                Ok(XC2Bitstream {
                    speed_grade: spd,
                    package: pkg,
                    bits,
                })
            },
            XC2Device::XC2C512 => {
                let bits = read_512_bitstream_physical(fuse_array)?;
                Ok(XC2Bitstream {
                    speed_grade: spd,
                    package: pkg,
                    bits,
                })
            },
        }
    }

    /// Construct a new blank bitstream of the given part
    pub fn blank_bitstream(part_combination: XC2DeviceSpeedPackage) -> Self {
        let XC2DeviceSpeedPackage {
            dev: device, spd: speed_grade, pkg: package
        } = part_combination;

        match device {
            XC2Device::XC2C32 => {
                XC2Bitstream {
                    speed_grade,
                    package,
                    bits: XC2BitstreamBits::XC2C32 {
                        fb: [XC2BitstreamFB::default(); 2],
                        iobs: [XC2MCSmallIOB::default(); 32],
                        inpin: XC2ExtraIBuf::default(),
                        global_nets: XC2GlobalNets::default(),
                        ivoltage: false,
                        ovoltage: false,
                    }
                }
            },
            XC2Device::XC2C32A => {
                XC2Bitstream {
                    speed_grade,
                    package,
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
                }
            },
            XC2Device::XC2C64 => {
                XC2Bitstream {
                    speed_grade,
                    package,
                    bits: XC2BitstreamBits::XC2C64 {
                        fb: [XC2BitstreamFB::default(); 4],
                        iobs: [[XC2MCSmallIOB::default(); 32]; 2],
                        global_nets: XC2GlobalNets::default(),
                        ivoltage: false,
                        ovoltage: false,
                    }
                }
            },
            XC2Device::XC2C64A => {
                XC2Bitstream {
                    speed_grade,
                    package,
                    bits: XC2BitstreamBits::XC2C64A {
                        fb: [XC2BitstreamFB::default(); 4],
                        iobs: [[XC2MCSmallIOB::default(); 32]; 2],
                        global_nets: XC2GlobalNets::default(),
                        legacy_ivoltage: false,
                        legacy_ovoltage: false,
                        ivoltage: [false, false],
                        ovoltage: [false, false],
                    }
                }
            },
            XC2Device::XC2C128 => {
                XC2Bitstream {
                    speed_grade,
                    package,
                    bits: XC2BitstreamBits::XC2C128 {
                        fb: [XC2BitstreamFB::default(); 8],
                        iobs: [[XC2MCLargeIOB::default(); 25]; 4],
                        global_nets: XC2GlobalNets::default(),
                        ivoltage: [false, false],
                        ovoltage: [false, false],
                        data_gate: false,
                        use_vref: false,
                        clock_div: XC2ClockDiv::default(),
                    }
                }
            },
            XC2Device::XC2C256 => {
                XC2Bitstream {
                    speed_grade,
                    package,
                    bits: XC2BitstreamBits::XC2C256 {
                        fb: [XC2BitstreamFB::default(); 16],
                        iobs: [[XC2MCLargeIOB::default(); 23]; 8],
                        global_nets: XC2GlobalNets::default(),
                        ivoltage: [false, false],
                        ovoltage: [false, false],
                        data_gate: false,
                        use_vref: false,
                        clock_div: XC2ClockDiv::default(),
                    }
                }
            },
            XC2Device::XC2C384 => {
                XC2Bitstream {
                    speed_grade,
                    package,
                    bits: XC2BitstreamBits::XC2C384 {
                        fb: [XC2BitstreamFB::default(); 24],
                        iobs: [[XC2MCLargeIOB::default(); 24]; 10],
                        global_nets: XC2GlobalNets::default(),
                        ivoltage: [false, false, false, false],
                        ovoltage: [false, false, false, false],
                        data_gate: false,
                        use_vref: false,
                        clock_div: XC2ClockDiv::default(),
                    }
                }
            },
            XC2Device::XC2C512 => {
                XC2Bitstream {
                    speed_grade,
                    package,
                    bits: XC2BitstreamBits::XC2C512 {
                        fb: [XC2BitstreamFB::default(); 32],
                        iobs: [[XC2MCLargeIOB::default(); 27]; 10],
                        global_nets: XC2GlobalNets::default(),
                        ivoltage: [false, false, false, false],
                        ovoltage: [false, false, false, false],
                        data_gate: false,
                        use_vref: false,
                        clock_div: XC2ClockDiv::default(),
                    }
                }
            }
        }
    }
}

/// The actual bitstream bits for each possible Coolrunner-II part
#[derive(Copy, Clone, Eq, PartialEq, Hash, Debug, Serialize, Deserialize)]
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
        iobs: [[XC2MCSmallIOB; 32]; 2],
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
        iobs: [[XC2MCSmallIOB; 32]; 2],
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
        iobs: [[XC2MCLargeIOB; 25]; 4],
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
        iobs: [[XC2MCLargeIOB; 23]; 8],
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
        iobs: [[XC2MCLargeIOB; 24]; 10],
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
        iobs: [[XC2MCLargeIOB; 27]; 10],
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

    /// Helper to extract only the function block data without having to perform an explicit `match`
    pub fn get_fb_mut(&mut self) -> &mut [XC2BitstreamFB] {
        match self {
            &mut XC2BitstreamBits::XC2C32{ref mut fb, ..} => fb,
            &mut XC2BitstreamBits::XC2C32A{ref mut fb, ..} => fb,
            &mut XC2BitstreamBits::XC2C64{ref mut fb, ..} => fb,
            &mut XC2BitstreamBits::XC2C64A{ref mut fb, ..} => fb,
            &mut XC2BitstreamBits::XC2C128{ref mut fb, ..} => fb,
            &mut XC2BitstreamBits::XC2C256{ref mut fb, ..} => fb,
            &mut XC2BitstreamBits::XC2C384{ref mut fb, ..} => fb,
            &mut XC2BitstreamBits::XC2C512{ref mut fb, ..} => fb,
        }
    }

    /// Helper to extract only the I/O data without having to perform an explicit `match`
    pub fn get_small_iob(&self, i: usize) -> Option<&XC2MCSmallIOB> {
        match self {
            &XC2BitstreamBits::XC2C32{ref iobs, ..} => Some(&iobs[i]),
            &XC2BitstreamBits::XC2C32A{ref iobs, ..} => Some(&iobs[i]),
            &XC2BitstreamBits::XC2C64{ref iobs, ..} => Some(&iobs[i / 32][i % 32]),
            &XC2BitstreamBits::XC2C64A{ref iobs, ..} => Some(&iobs[i / 32][i % 32]),
            _ => None,
        }
    }

    /// Helper to extract only the I/O data without having to perform an explicit `match`
    pub fn get_mut_small_iob(&mut self, i: usize) -> Option<&mut XC2MCSmallIOB> {
        match self {
            &mut XC2BitstreamBits::XC2C32{ref mut iobs, ..} => Some(&mut iobs[i]),
            &mut XC2BitstreamBits::XC2C32A{ref mut iobs, ..} => Some(&mut iobs[i]),
            &mut XC2BitstreamBits::XC2C64{ref mut iobs, ..} => Some(&mut iobs[i / 32][i % 32]),
            &mut XC2BitstreamBits::XC2C64A{ref mut iobs, ..} => Some(&mut iobs[i / 32][i % 32]),
            _ => None,
        }
    }

    /// Helper to extract only the I/O data without having to perform an explicit `match`
    pub fn get_large_iob(&self, i: usize) -> Option<&XC2MCLargeIOB> {
        match self {
            &XC2BitstreamBits::XC2C128{ref iobs, ..} => Some(&iobs[i / 25][i % 25]),
            &XC2BitstreamBits::XC2C256{ref iobs, ..} => Some(&iobs[i / 23][i % 23]),
            &XC2BitstreamBits::XC2C384{ref iobs, ..} => Some(&iobs[i / 24][i % 24]),
            &XC2BitstreamBits::XC2C512{ref iobs, ..} => Some(&iobs[i / 27][i % 27]),
            _ => None,
        }
    }

    /// Helper to extract only the I/O data without having to perform an explicit `match`
    pub fn get_mut_large_iob(&mut self, i: usize) -> Option<&mut XC2MCLargeIOB> {
        match self {
            &mut XC2BitstreamBits::XC2C128{ref mut iobs, ..} => Some(&mut iobs[i / 25][i % 25]),
            &mut XC2BitstreamBits::XC2C256{ref mut iobs, ..} => Some(&mut iobs[i / 23][i % 23]),
            &mut XC2BitstreamBits::XC2C384{ref mut iobs, ..} => Some(&mut iobs[i / 24][i % 24]),
            &mut XC2BitstreamBits::XC2C512{ref mut iobs, ..} => Some(&mut iobs[i / 27][i % 27]),
            _ => None,
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

    /// Helper to extract only the global net data without having to perform an explicit `match`
    pub fn get_global_nets_mut(&mut self) -> &mut XC2GlobalNets {
        match self {
            &mut XC2BitstreamBits::XC2C32{ref mut global_nets, ..} => global_nets,
            &mut XC2BitstreamBits::XC2C32A{ref mut global_nets, ..} => global_nets,
            &mut XC2BitstreamBits::XC2C64{ref mut global_nets, ..} => global_nets,
            &mut XC2BitstreamBits::XC2C64A{ref mut global_nets, ..} => global_nets,
            &mut XC2BitstreamBits::XC2C128{ref mut global_nets, ..} => global_nets,
            &mut XC2BitstreamBits::XC2C256{ref mut global_nets, ..} => global_nets,
            &mut XC2BitstreamBits::XC2C384{ref mut global_nets, ..} => global_nets,
            &mut XC2BitstreamBits::XC2C512{ref mut global_nets, ..} => global_nets,
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
        for i in 0..self.device_type().num_iobs() {
            if let Some(iob) = self.get_small_iob(i) {
                iob.to_crbit(self.device_type(), i as u32, fuse_array);
            }
            if let Some(iob) = self.get_large_iob(i) {
                iob.to_crbit(self.device_type(), i as u32, fuse_array);
            }
        }

        // Weird extra input-only pin
        match self {
            &XC2BitstreamBits::XC2C32 {ref inpin, ..} |
            &XC2BitstreamBits::XC2C32A {ref inpin, ..} => {
                inpin.encode_crbit(fuse_array);
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

            let divratio = clock_div.div_ratio.encode();
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

        // Initialize security/done/usercode rows to all 1s
        for x in 0..fuse_array.dim().0 {
            fuse_array.set(x, fuse_array.dim().1 - 1, true);
            fuse_array.set(x, fuse_array.dim().1 - 2, true);
        }

        // Set done1 to 0
        match self {
            &XC2BitstreamBits::XC2C32{..} | &XC2BitstreamBits::XC2C32A{..} => {
                fuse_array.set(9, 48, false);
            },
            &XC2BitstreamBits::XC2C64{..} | &XC2BitstreamBits::XC2C64A{..} => {
                fuse_array.set(8, 96, false);
            },
            &XC2BitstreamBits::XC2C128{..} => {
                fuse_array.set(9, 80, false);
            },
            &XC2BitstreamBits::XC2C256{..} => {
                fuse_array.set(9, 96, false);
            },
            &XC2BitstreamBits::XC2C384{..} => {
                fuse_array.set(9, 120, false);
            },
            &XC2BitstreamBits::XC2C512{..} => {
                fuse_array.set(9, 160, false);
            },
        }

        // TODO: Security bits and USERCODE bits
    }

    /// Dump a human-readable explanation of the bitstream to the given `writer` object.
    pub fn dump_human_readable<W: Write>(&self, mut writer: W) -> Result<(), io::Error> {
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
            write!(writer, "\n{}", clock_div)?;
        }

        // Global net configuration
        write!(writer, "\n{}", self.get_global_nets())?;

        // IOBs
        for i in 0..self.device_type().num_iobs() {
            write!(writer, "\n")?;
            let (fb, mc) = iob_num_to_fb_mc_num(self.device_type(), i as u32).unwrap();
            write!(writer, "I/O configuration for FB{}_{}\n", fb + 1, mc + 1)?;
            if let Some(iob) = self.get_small_iob(i) {
                write!(writer, "{}", iob)?;
            }
            if let Some(iob) = self.get_large_iob(i) {
                write!(writer, "{}", iob)?;
            }
        }

        // Input-only pin
        match self {
            &XC2BitstreamBits::XC2C32 {ref inpin, ..} | &XC2BitstreamBits::XC2C32A {ref inpin, ..} => {
                write!(writer, "\n")?;
                write!(writer, "I/O configuration for input-only pin\n")?;
                write!(writer, "{}", inpin)?;
            },
            _ => {}
        }

        // FBs
        for i in 0..self.device_type().num_fbs() {
            self.get_fb()[i].dump_human_readable(self.device_type(), i as u32, &mut writer)?;
        }

        Ok(())
    }

    /// Write a .jed representation of the bitstream to the given `jed` object.
    pub fn to_jed(&self, jed: &mut JEDECFile, linebreaks: &mut LinebreakSet) {
        // FBs
        for fb_i in 0..self.device_type().num_fbs() {
            let fuse_base = fb_fuse_idx(self.device_type(), fb_i as u32);
            self.get_fb()[fb_i].to_jed(self.device_type(), fuse_base, jed, linebreaks);
        }

        // Macrocells
        for fb_i in 0..self.device_type().num_fbs() {
            let fuse_base = fb_fuse_idx(self.device_type(), fb_i as u32);
            let fb = self.get_fb();
            if self.device_type().is_small_iob() {
                XC2Macrocell::to_jed_small(jed, linebreaks, self.device_type(), &fb[fb_i], fuse_base);

                for i in 0..MCS_PER_FB {
                    let iob = fb_mc_num_to_iob_num(self.device_type(), fb_i as u32, i as u32).unwrap() as usize;
                    self.get_small_iob(iob).unwrap().to_jed(jed, self.device_type(), fuse_base, i);
                }
            }
            if self.device_type().is_large_iob() {
                XC2Macrocell::to_jed_large(jed, linebreaks, self.device_type(), &fb[fb_i], fb_i, fuse_base);

                let zia_row_width = zia_get_row_width(self.device_type());
                let mut current_fuse_offset = fuse_base + zia_row_width * INPUTS_PER_ANDTERM +
                    ANDTERMS_PER_FB * INPUTS_PER_ANDTERM * 2 + ANDTERMS_PER_FB * MCS_PER_FB;

                for i in 0..MCS_PER_FB {
                    let iob = fb_mc_num_to_iob_num(self.device_type(), fb_i as u32, i as u32);

                    if iob.is_some() {
                        let iob = iob.unwrap() as usize;

                        self.get_large_iob(iob).unwrap().to_jed(jed, current_fuse_offset);
                        current_fuse_offset += 29;
                    } else {
                        current_fuse_offset += 16;
                    }
                }
            }
        }

        // GCK
        linebreaks.add(gck_fuse_idx(self.device_type()));
        linebreaks.add(gck_fuse_idx(self.device_type()));
        jed.f[gck_fuse_idx(self.device_type()) + 0] = self.get_global_nets().gck_enable[0];
        jed.f[gck_fuse_idx(self.device_type()) + 1] = self.get_global_nets().gck_enable[1];
        jed.f[gck_fuse_idx(self.device_type()) + 2] = self.get_global_nets().gck_enable[2];

        // Clock divider
        if let Some(clock_div) = self.get_clock_div() {
            let clock_fuse_block = clock_div_fuse_idx(self.device_type());

            linebreaks.add(clock_fuse_block);
            jed.f[clock_fuse_block] = !clock_div.enabled;
            let clk_div_bits = clock_div.div_ratio.encode();
            jed.f[clock_fuse_block+1] = clk_div_bits.0;
            jed.f[clock_fuse_block+2] = clk_div_bits.1;
            jed.f[clock_fuse_block+3] = clk_div_bits.2;
            linebreaks.add(clock_fuse_block + 4);
            jed.f[clock_fuse_block + 4] = !clock_div.delay;
        }

        // GSR
        linebreaks.add(gsr_fuse_idx(self.device_type()));
        jed.f[gsr_fuse_idx(self.device_type()) + 0] = self.get_global_nets().gsr_invert;
        jed.f[gsr_fuse_idx(self.device_type()) + 1] = self.get_global_nets().gsr_enable;

        // GTS
        linebreaks.add(gts_fuse_idx(self.device_type()));
        jed.f[gts_fuse_idx(self.device_type()) + 0] = self.get_global_nets().gts_invert[0];
        jed.f[gts_fuse_idx(self.device_type()) + 1] = !self.get_global_nets().gts_enable[0];
        jed.f[gts_fuse_idx(self.device_type()) + 2] = self.get_global_nets().gts_invert[1];
        jed.f[gts_fuse_idx(self.device_type()) + 3] = !self.get_global_nets().gts_enable[1];
        jed.f[gts_fuse_idx(self.device_type()) + 4] = self.get_global_nets().gts_invert[2];
        jed.f[gts_fuse_idx(self.device_type()) + 5] = !self.get_global_nets().gts_enable[2];
        jed.f[gts_fuse_idx(self.device_type()) + 6] = self.get_global_nets().gts_invert[3];
        jed.f[gts_fuse_idx(self.device_type()) + 7] = !self.get_global_nets().gts_enable[3];

        // Global termination
        linebreaks.add(global_term_fuse_idx(self.device_type()));
        jed.f[global_term_fuse_idx(self.device_type())] = self.get_global_nets().global_pu;

        // Bank voltages and miscellaneous
        match self {
            &XC2BitstreamBits::XC2C32 {ref inpin, ref ivoltage, ref ovoltage, ..} |
            &XC2BitstreamBits::XC2C32A {ref inpin, legacy_ivoltage: ref ivoltage,
                legacy_ovoltage: ref ovoltage, ..} => {

                linebreaks.add(12270);
                jed.f[12270] = !ovoltage;
                linebreaks.add(12271);
                jed.f[12271] = !ivoltage;

                linebreaks.add(12272);
                inpin.encode_jed(&mut jed.f);
            }
            &XC2BitstreamBits::XC2C64 {ref ivoltage, ref ovoltage, ..} |
            &XC2BitstreamBits::XC2C64A {legacy_ivoltage: ref ivoltage, legacy_ovoltage: ref ovoltage, ..} => {
                linebreaks.add(25806);
                jed.f[25806] = !ovoltage;
                linebreaks.add(25807);
                jed.f[25807] = !ivoltage;
            }
            &XC2BitstreamBits::XC2C128 {ref ivoltage, ref ovoltage, ref data_gate, ref use_vref, ..}  => {
                linebreaks.add(55335);
                jed.f[55335] = !data_gate;

                linebreaks.add(55336);
                jed.f[55336] = !ivoltage[0];
                jed.f[55337] = !ivoltage[1];
                linebreaks.add(55338);
                jed.f[55338] = !ovoltage[0];
                jed.f[55339] = !ovoltage[1];

                linebreaks.add(55340);
                jed.f[55340] = !use_vref;
            }
            &XC2BitstreamBits::XC2C256 {ref ivoltage, ref ovoltage, ref data_gate, ref use_vref, ..}  => {
                linebreaks.add(123243);
                jed.f[123243] = !data_gate;

                linebreaks.add(123244);
                jed.f[123244] = !ivoltage[0];
                jed.f[123245] = !ivoltage[1];
                linebreaks.add(123246);
                jed.f[123246] = !ovoltage[0];
                jed.f[123247] = !ovoltage[1];

                linebreaks.add(123248);
                jed.f[123248] = !use_vref;
            }
            &XC2BitstreamBits::XC2C384 {ref ivoltage, ref ovoltage, ref data_gate, ref use_vref, ..}  => {
                linebreaks.add(209347);
                jed.f[209347] = !data_gate;

                linebreaks.add(209348);
                jed.f[209348] = !ivoltage[0];
                jed.f[209349] = !ivoltage[1];
                jed.f[209350] = !ivoltage[2];
                jed.f[209351] = !ivoltage[3];

                linebreaks.add(209352);
                jed.f[209352] = !ovoltage[0];
                jed.f[209353] = !ovoltage[1];
                jed.f[209354] = !ovoltage[2];
                jed.f[209355] = !ovoltage[3];

                linebreaks.add(209356);
                jed.f[209356] = !use_vref;
            }
            &XC2BitstreamBits::XC2C512 {ref ivoltage, ref ovoltage, ref data_gate, ref use_vref, ..}  => {
                linebreaks.add(296393);
                jed.f[296393] = !data_gate;

                linebreaks.add(296394);
                jed.f[296394] = ivoltage[0];
                jed.f[296395] = ivoltage[1];
                jed.f[296396] = ivoltage[2];
                jed.f[296397] = ivoltage[3];

                linebreaks.add(296398);
                jed.f[296398] = ovoltage[0];
                jed.f[296399] = ovoltage[1];
                jed.f[296400] = ovoltage[2];
                jed.f[296401] = ovoltage[3];

                linebreaks.add(296402);
                jed.f[296402] = !use_vref;
            }
        }

        // A-variant bank voltages
        match self {
            &XC2BitstreamBits::XC2C32A {ref ivoltage, ref ovoltage, ..} => {
                linebreaks.add(12274);
                jed.f[12274] = !ivoltage[0];
                linebreaks.add(12275);
                jed.f[12275] = !ovoltage[0];
                linebreaks.add(12276);
                jed.f[12276] = !ivoltage[1];
                linebreaks.add(12277);
                jed.f[12277] = !ovoltage[1];
            },
            &XC2BitstreamBits::XC2C64A {ref ivoltage, ref ovoltage, ..} => {
                linebreaks.add(25808);
                jed.f[25808] = !ivoltage[0];
                linebreaks.add(25809);
                jed.f[25809] = !ovoltage[0];
                linebreaks.add(25810);
                jed.f[25810] = !ivoltage[1];
                linebreaks.add(25811);
                jed.f[25811] = !ovoltage[1];
            },
            _ => {}
        }
    }
}

/// Common logic for reading bitstreams on "small" devices
fn read_bitstream_logical_common_small(fuses: &[bool], device: XC2Device,
    fb: &mut [XC2BitstreamFB], iobs: &mut [XC2MCSmallIOB]) -> Result<(), XC2BitError> {

    for i in 0..fb.len() {
        let base_fuse = fb_fuse_idx(device, i as u32);
        let res = XC2BitstreamFB::from_jed(device, fuses, i as u32, base_fuse)?;
        fb[i] = res;

        let zia_row_width = zia_get_row_width(device);
        let size_of_zia = zia_row_width * INPUTS_PER_ANDTERM;
        let size_of_and = INPUTS_PER_ANDTERM * 2 * ANDTERMS_PER_FB;
        let size_of_or = ANDTERMS_PER_FB * MCS_PER_FB;
        let mut iob_fuse = base_fuse + size_of_zia + size_of_and + size_of_or;
        for mc in 0..MCS_PER_FB {
            let iob = fb_mc_num_to_iob_num(device, i as u32, mc as u32);
            let res = XC2MCSmallIOB::from_jed(fuses, iob_fuse)?;
            iobs[iob.unwrap() as usize] = res;
            iob_fuse += 27;
        }
    };

    Ok(())
}

/// Common logic for reading bitstreams on "large" devices
fn read_bitstream_logical_common_large(fuses: &[bool], device: XC2Device,
    fb: &mut [XC2BitstreamFB], iobs: &mut [XC2MCLargeIOB]) -> Result<(), XC2BitError> {

    for i in 0..fb.len() {
        let base_fuse = fb_fuse_idx(device, i as u32);
        let res = XC2BitstreamFB::from_jed(device, fuses, i as u32, base_fuse)?;
        fb[i] = res;

        let zia_row_width = zia_get_row_width(device);
        let size_of_zia = zia_row_width * INPUTS_PER_ANDTERM;
        let size_of_and = INPUTS_PER_ANDTERM * 2 * ANDTERMS_PER_FB;
        let size_of_or = ANDTERMS_PER_FB * MCS_PER_FB;
        let mut iob_fuse = base_fuse + size_of_zia + size_of_and + size_of_or;
        for mc in 0..MCS_PER_FB {
            let iob = fb_mc_num_to_iob_num(device, i as u32, mc as u32);
            if iob.is_some() {
                let res = XC2MCLargeIOB::from_jed(fuses, iob_fuse)?;
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
fn read_32_bitstream_logical(fuses: &[bool]) -> Result<XC2BitstreamBits, XC2BitError> {
    let mut fb = [XC2BitstreamFB::default(); 2];
    let mut iobs = [XC2MCSmallIOB::default(); 32];
    
    read_bitstream_logical_common_small(fuses, XC2Device::XC2C32, &mut fb, &mut iobs)?;

    let inpin = XC2ExtraIBuf::decode_jed(fuses);

    let global_nets = XC2GlobalNets::from_jed(XC2Device::XC2C32, fuses);

    Ok(XC2BitstreamBits::XC2C32 {
        fb,
        iobs,
        inpin,
        global_nets,
        ovoltage: !fuses[12270],
        ivoltage: !fuses[12271],
    })
}

/// Internal function for parsing an XC2C32A bitstream
fn read_32a_bitstream_logical(fuses: &[bool]) -> Result<XC2BitstreamBits, XC2BitError> {
    let mut fb = [XC2BitstreamFB::default(); 2];
    let mut iobs = [XC2MCSmallIOB::default(); 32];
    
    read_bitstream_logical_common_small(fuses, XC2Device::XC2C32A, &mut fb, &mut iobs)?;

    let inpin = XC2ExtraIBuf::decode_jed(fuses);

    let global_nets = XC2GlobalNets::from_jed(XC2Device::XC2C32A, fuses);

    Ok(XC2BitstreamBits::XC2C32A {
        fb,
        iobs,
        inpin,
        global_nets,
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
fn read_64_bitstream_logical(fuses: &[bool]) -> Result<XC2BitstreamBits, XC2BitError> {
    let mut fb = [XC2BitstreamFB::default(); 4];
    let mut iobs = [XC2MCSmallIOB::default(); 64];
    
    read_bitstream_logical_common_small(fuses, XC2Device::XC2C64, &mut fb, &mut iobs)?;

    let global_nets = XC2GlobalNets::from_jed(XC2Device::XC2C64, fuses);

    let mut iobs2 = [[XC2MCSmallIOB::default(); 32]; 2];
    for i in 0..iobs.len() {
        iobs2[i / 32][i % 32] = iobs[i];
    }

    Ok(XC2BitstreamBits::XC2C64 {
        fb,
        iobs: iobs2,
        global_nets,
        ovoltage: !fuses[25806],
        ivoltage: !fuses[25807],
    })
}

/// Internal function for parsing an XC2C64A bitstream
fn read_64a_bitstream_logical(fuses: &[bool]) -> Result<XC2BitstreamBits, XC2BitError> {
    let mut fb = [XC2BitstreamFB::default(); 4];
    let mut iobs = [XC2MCSmallIOB::default(); 64];
    
    read_bitstream_logical_common_small(fuses, XC2Device::XC2C64A, &mut fb, &mut iobs)?;

    let global_nets = XC2GlobalNets::from_jed(XC2Device::XC2C64A, fuses);

    let mut iobs2 = [[XC2MCSmallIOB::default(); 32]; 2];
    for i in 0..iobs.len() {
        iobs2[i / 32][i % 32] = iobs[i];
    }

    Ok(XC2BitstreamBits::XC2C64A {
        fb,
        iobs: iobs2,
        global_nets,
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
fn read_128_bitstream_logical(fuses: &[bool]) -> Result<XC2BitstreamBits, XC2BitError> {
    let mut fb = [XC2BitstreamFB::default(); 8];
    let mut iobs = [XC2MCLargeIOB::default(); 100];
    
    read_bitstream_logical_common_large(fuses, XC2Device::XC2C128, &mut fb, &mut iobs)?;

    let global_nets = XC2GlobalNets::from_jed(XC2Device::XC2C128, fuses);

    let mut iobs2 = [[XC2MCLargeIOB::default(); 25]; 4];
    for i in 0..iobs.len() {
        iobs2[i / 25][i % 25] = iobs[i];
    }

    Ok(XC2BitstreamBits::XC2C128 {
        fb,
        iobs: iobs2,
        global_nets,
        clock_div: XC2ClockDiv::from_jed(XC2Device::XC2C128, fuses),
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
fn read_256_bitstream_logical(fuses: &[bool]) -> Result<XC2BitstreamBits, XC2BitError> {
    let mut fb = [XC2BitstreamFB::default(); 16];
    let mut iobs = [XC2MCLargeIOB::default(); 184];
    
    read_bitstream_logical_common_large(fuses, XC2Device::XC2C256, &mut fb, &mut iobs)?;

    let global_nets = XC2GlobalNets::from_jed(XC2Device::XC2C256, fuses);

    let mut iobs2 = [[XC2MCLargeIOB::default(); 23]; 8];
    for i in 0..iobs.len() {
        iobs2[i / 23][i % 23] = iobs[i];
    }

    Ok(XC2BitstreamBits::XC2C256 {
        fb,
        iobs: iobs2,
        global_nets,
        clock_div: XC2ClockDiv::from_jed(XC2Device::XC2C256, fuses),
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
fn read_384_bitstream_logical(fuses: &[bool]) -> Result<XC2BitstreamBits, XC2BitError> {
    let mut fb = [XC2BitstreamFB::default(); 24];
    let mut iobs = [XC2MCLargeIOB::default(); 240];
    
    read_bitstream_logical_common_large(fuses, XC2Device::XC2C384, &mut fb, &mut iobs)?;

    let global_nets = XC2GlobalNets::from_jed(XC2Device::XC2C384, fuses);

    let mut iobs2 = [[XC2MCLargeIOB::default(); 24]; 10];
    for i in 0..iobs.len() {
        iobs2[i / 24][i % 24] = iobs[i];
    }

    Ok(XC2BitstreamBits::XC2C384 {
        fb,
        iobs: iobs2,
        global_nets,
        clock_div: XC2ClockDiv::from_jed(XC2Device::XC2C384, fuses),
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
fn read_512_bitstream_logical(fuses: &[bool]) -> Result<XC2BitstreamBits, XC2BitError> {
    let mut fb = [XC2BitstreamFB::default(); 32];
    let mut iobs = [XC2MCLargeIOB::default(); 270];
    
    read_bitstream_logical_common_large(fuses, XC2Device::XC2C512, &mut fb, &mut iobs)?;

    let global_nets = XC2GlobalNets::from_jed(XC2Device::XC2C512, fuses);

    let mut iobs2 = [[XC2MCLargeIOB::default(); 27]; 10];
    for i in 0..iobs.len() {
        iobs2[i / 27][i % 27] = iobs[i];
    }

    Ok(XC2BitstreamBits::XC2C512 {
        fb,
        iobs: iobs2,
        global_nets,
        clock_div: XC2ClockDiv::from_jed(XC2Device::XC2C512, fuses),
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
fn read_bitstream_physical_common_small(fuse_array: &FuseArray, device: XC2Device,
    fb: &mut [XC2BitstreamFB], iobs: &mut [XC2MCSmallIOB]) -> Result<(), XC2BitError> {

    for i in 0..fb.len() {
        fb[i] = XC2BitstreamFB::from_crbit(device, i as u32, fuse_array)?;
    };

    for i in 0..iobs.len() {
        iobs[i] = XC2MCSmallIOB::from_crbit(device, i as u32, fuse_array)?;
    }

    Ok(())
}

/// Common logic for reading bitstreams on "large" devices
fn read_bitstream_physical_common_large(fuse_array: &FuseArray, device: XC2Device,
    fb: &mut [XC2BitstreamFB], iobs: &mut [XC2MCLargeIOB]) -> Result<(), XC2BitError> {

    for i in 0..fb.len() {
        fb[i] = XC2BitstreamFB::from_crbit(device, i as u32, fuse_array)?;
    };

    for i in 0..iobs.len() {
        iobs[i] = XC2MCLargeIOB::from_crbit(device, i as u32, fuse_array)?;
    }

    Ok(())
}

/// Internal function for parsing an XC2C32 bitstream
fn read_32_bitstream_physical(fuse_array: &FuseArray) -> Result<XC2BitstreamBits, XC2BitError> {
    let mut fb = [XC2BitstreamFB::default(); 2];
    let mut iobs = [XC2MCSmallIOB::default(); 32];
    
    read_bitstream_physical_common_small(fuse_array, XC2Device::XC2C32, &mut fb, &mut iobs)?;

    let inpin = XC2ExtraIBuf::decode_crbit(fuse_array);

    let global_nets = XC2GlobalNets::from_crbit(XC2Device::XC2C32, fuse_array);

    Ok(XC2BitstreamBits::XC2C32 {
        fb,
        iobs,
        inpin,
        global_nets,
        ovoltage: !fuse_array.get(130, 24),
        ivoltage: !fuse_array.get(130, 25),
    })
}

/// Internal function for parsing an XC2C32A bitstream
fn read_32a_bitstream_physical(fuse_array: &FuseArray) -> Result<XC2BitstreamBits, XC2BitError> {
    let mut fb = [XC2BitstreamFB::default(); 2];
    let mut iobs = [XC2MCSmallIOB::default(); 32];
    
    read_bitstream_physical_common_small(fuse_array, XC2Device::XC2C32A, &mut fb, &mut iobs)?;

    let inpin = XC2ExtraIBuf::decode_crbit(fuse_array);

    let global_nets = XC2GlobalNets::from_crbit(XC2Device::XC2C32A, fuse_array);

    Ok(XC2BitstreamBits::XC2C32A {
        fb,
        iobs,
        inpin,
        global_nets,
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

/// Internal function for parsing an XC2C64 bitstream
fn read_64_bitstream_physical(fuse_array: &FuseArray) -> Result<XC2BitstreamBits, XC2BitError> {
    let mut fb = [XC2BitstreamFB::default(); 4];
    let mut iobs = [XC2MCSmallIOB::default(); 64];
    
    read_bitstream_physical_common_small(fuse_array, XC2Device::XC2C64, &mut fb, &mut iobs)?;

    let global_nets = XC2GlobalNets::from_crbit(XC2Device::XC2C64, fuse_array);

    let mut iobs2 = [[XC2MCSmallIOB::default(); 32]; 2];
    for i in 0..iobs.len() {
        iobs2[i / 32][i % 32] = iobs[i];
    }

    Ok(XC2BitstreamBits::XC2C64 {
        fb,
        iobs: iobs2,
        global_nets,
        ovoltage: !fuse_array.get(137, 23),
        ivoltage: !fuse_array.get(138, 23),
    })
}

/// Internal function for parsing an XC2C64A bitstream
fn read_64a_bitstream_physical(fuse_array: &FuseArray) -> Result<XC2BitstreamBits, XC2BitError> {
    let mut fb = [XC2BitstreamFB::default(); 4];
    let mut iobs = [XC2MCSmallIOB::default(); 64];
    
    read_bitstream_physical_common_small(fuse_array, XC2Device::XC2C64A, &mut fb, &mut iobs)?;

    let global_nets = XC2GlobalNets::from_crbit(XC2Device::XC2C64A, fuse_array);

    let mut iobs2 = [[XC2MCSmallIOB::default(); 32]; 2];
    for i in 0..iobs.len() {
        iobs2[i / 32][i % 32] = iobs[i];
    }

    Ok(XC2BitstreamBits::XC2C64A {
        fb,
        iobs: iobs2,
        global_nets,
        legacy_ovoltage: !fuse_array.get(137, 23),
        legacy_ivoltage: !fuse_array.get(138, 23),
        ivoltage: [
            !fuse_array.get(139, 23),
            !fuse_array.get(141, 23),
        ],
        ovoltage: [
            !fuse_array.get(140, 23),
            !fuse_array.get(142, 23),
        ]
    })
}

/// Internal function for parsing an XC2C128 bitstream
fn read_128_bitstream_physical(fuse_array: &FuseArray) -> Result<XC2BitstreamBits, XC2BitError> {
    let mut fb = [XC2BitstreamFB::default(); 8];
    let mut iobs = [XC2MCLargeIOB::default(); 100];
    
    read_bitstream_physical_common_large(fuse_array, XC2Device::XC2C128, &mut fb, &mut iobs)?;

    let global_nets = XC2GlobalNets::from_crbit(XC2Device::XC2C128, fuse_array);

    let mut iobs2 = [[XC2MCLargeIOB::default(); 25]; 4];
    for i in 0..iobs.len() {
        iobs2[i / 25][i % 25] = iobs[i];
    }

    Ok(XC2BitstreamBits::XC2C128 {
        fb,
        iobs: iobs2,
        global_nets,
        clock_div: XC2ClockDiv::from_crbit(XC2Device::XC2C128, fuse_array),
        data_gate: !fuse_array.get(371, 67),
        use_vref: !fuse_array.get(10, 67),
        ivoltage: [
            !fuse_array.get(8, 67),
            !fuse_array.get(368, 67),
        ],
        ovoltage: [
            !fuse_array.get(9, 67),
            !fuse_array.get(369, 67),
        ]
    })
}

/// Internal function for parsing an XC2C256 bitstream
fn read_256_bitstream_physical(fuse_array: &FuseArray) -> Result<XC2BitstreamBits, XC2BitError> {
    let mut fb = [XC2BitstreamFB::default(); 16];
    let mut iobs = [XC2MCLargeIOB::default(); 184];
    
    read_bitstream_physical_common_large(fuse_array, XC2Device::XC2C256, &mut fb, &mut iobs)?;

    let global_nets = XC2GlobalNets::from_crbit(XC2Device::XC2C256, fuse_array);

    let mut iobs2 = [[XC2MCLargeIOB::default(); 23]; 8];
    for i in 0..iobs.len() {
        iobs2[i / 23][i % 23] = iobs[i];
    }

    Ok(XC2BitstreamBits::XC2C256 {
        fb,
        iobs: iobs2,
        global_nets,
        clock_div: XC2ClockDiv::from_crbit(XC2Device::XC2C256, fuse_array),
        data_gate: !fuse_array.get(518, 23),
        use_vref: !fuse_array.get(177, 23),
        ivoltage: [
            !fuse_array.get(175, 23),
            !fuse_array.get(515, 23),
        ],
        ovoltage: [
            !fuse_array.get(176, 23),
            !fuse_array.get(516, 23),
        ]
    })
}

/// Internal function for parsing an XC2C384 bitstream
fn read_384_bitstream_physical(fuse_array: &FuseArray) -> Result<XC2BitstreamBits, XC2BitError> {
    let mut fb = [XC2BitstreamFB::default(); 24];
    let mut iobs = [XC2MCLargeIOB::default(); 240];
    
    read_bitstream_physical_common_large(fuse_array, XC2Device::XC2C384, &mut fb, &mut iobs)?;

    let global_nets = XC2GlobalNets::from_crbit(XC2Device::XC2C384, fuse_array);

    let mut iobs2 = [[XC2MCLargeIOB::default(); 24]; 10];
    for i in 0..iobs.len() {
        iobs2[i / 24][i % 24] = iobs[i];
    }

    Ok(XC2BitstreamBits::XC2C384 {
        fb,
        iobs: iobs2,
        global_nets,
        clock_div: XC2ClockDiv::from_crbit(XC2Device::XC2C384, fuse_array),
        data_gate: !fuse_array.get(932, 17),
        use_vref: !fuse_array.get(3, 17),
        ivoltage: [
            !fuse_array.get(936, 17),
            !fuse_array.get(1864, 17),
            !fuse_array.get(1, 17),
            !fuse_array.get(929, 17),
        ],
        ovoltage: [
            !fuse_array.get(937, 17),
            !fuse_array.get(1865, 17),
            !fuse_array.get(2, 17),
            !fuse_array.get(930, 17),
        ]
    })
}

/// Internal function for parsing an XC2C512 bitstream
fn read_512_bitstream_physical(fuse_array: &FuseArray) -> Result<XC2BitstreamBits, XC2BitError> {
    let mut fb = [XC2BitstreamFB::default(); 32];
    let mut iobs = [XC2MCLargeIOB::default(); 270];
    
    read_bitstream_physical_common_large(fuse_array, XC2Device::XC2C512, &mut fb, &mut iobs)?;

    let global_nets = XC2GlobalNets::from_crbit(XC2Device::XC2C512, fuse_array);

    let mut iobs2 = [[XC2MCLargeIOB::default(); 27]; 10];
    for i in 0..iobs.len() {
        iobs2[i / 27][i % 27] = iobs[i];
    }

    Ok(XC2BitstreamBits::XC2C512 {
        fb,
        iobs: iobs2,
        global_nets,
        clock_div: XC2ClockDiv::from_crbit(XC2Device::XC2C512, fuse_array),
        data_gate: !fuse_array.get(982, 147),
        use_vref: !fuse_array.get(1, 147),
        ivoltage: [
            fuse_array.get(992, 147),
            fuse_array.get(1965, 147),
            fuse_array.get(3, 147),
            fuse_array.get(985, 147),
        ],
        ovoltage: [
            fuse_array.get(991, 147),
            fuse_array.get(1964, 147),
            fuse_array.get(2, 147),
            fuse_array.get(984, 147),
        ]
    })
}
