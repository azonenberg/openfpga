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

//! Miscellaneous stuff related to possible part combinations

#[allow(unused_imports)]
use std::ascii::AsciiExt;
use std::fmt;

/// Coolrunner-II devices
#[derive(Copy, Clone, Eq, PartialEq, Debug, Serialize, Deserialize)]
pub enum XC2Device {
    XC2C32,
    XC2C32A,
    XC2C64,
    XC2C64A,
    XC2C128,
    XC2C256,
    XC2C384,
    XC2C512,
}

impl fmt::Display for XC2Device {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "{:?}", self)
    }
}

impl XC2Device {
    /// Returns the number of function blocks for the device type
    pub fn num_fbs(&self) -> usize {
        match *self {
            XC2Device::XC2C32 | XC2Device::XC2C32A => 2,
            XC2Device::XC2C64 | XC2Device::XC2C64A => 4,
            XC2Device::XC2C128 => 8,
            XC2Device::XC2C256 => 16,
            XC2Device::XC2C384 => 24,
            XC2Device::XC2C512 => 32,
        }
    }

    /// Returns the number of I/O pins for the device type
    pub fn num_iobs(&self) -> usize {
        match *self {
            XC2Device::XC2C32 | XC2Device::XC2C32A => 32,
            XC2Device::XC2C64 | XC2Device::XC2C64A => 64,
            XC2Device::XC2C128 => 100,
            XC2Device::XC2C256 => 184,
            XC2Device::XC2C384 => 240,
            XC2Device::XC2C512 => 270,
        }
    }

    pub fn is_small_iob(&self) -> bool {
        match *self {
            XC2Device::XC2C32 | XC2Device::XC2C32A |
            XC2Device::XC2C64 | XC2Device::XC2C64A => true,
            _ => false,
        }
    }

    pub fn is_large_iob(&self) -> bool {
        match *self {
            XC2Device::XC2C128 | XC2Device::XC2C256 |
            XC2Device::XC2C384 | XC2Device::XC2C512 => true,
             _ => false,
        }
    }
}

/// Possible speed grades
#[derive(Copy, Clone, Eq, PartialEq, Debug, Serialize, Deserialize)]
pub enum XC2Speed {
    Speed4,
    Speed5,
    Speed6,
    Speed7,
    Speed10,
}

impl fmt::Display for XC2Speed {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "{}", match *self {
            XC2Speed::Speed4 => "4",
            XC2Speed::Speed5 => "5",
            XC2Speed::Speed6 => "6",
            XC2Speed::Speed7 => "7",
            XC2Speed::Speed10 => "10",
        })
    }
}

/// Possible physical packages
#[derive(Copy, Clone, Eq, PartialEq, Debug, Serialize, Deserialize)]
pub enum XC2Package {
    PC44,
    QFG32,
    VQ44,
    QFG48,
    CP56,
    VQ100,
    CP132,
    TQ144,
    PQ208,
    FT256,
    FG324,
}

impl fmt::Display for XC2Package {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "{:?}", self)
    }
}

/// Determine if the given combination of device, speed, and package is a legal combination or not.
fn is_valid_part_combination(device: XC2Device, speed: XC2Speed, package: XC2Package) -> bool {
    match device {
        XC2Device::XC2C32 => {
            if speed == XC2Speed::Speed4 || speed == XC2Speed::Speed6 {
                if package == XC2Package::PC44 || package == XC2Package::VQ44 || package == XC2Package::CP56 {
                    true
                } else {
                    false
                }
            } else {
                false
            }
        },
        XC2Device::XC2C32A => {
            if speed == XC2Speed::Speed4 || speed == XC2Speed::Speed6 {
                if package == XC2Package::PC44 || package == XC2Package::VQ44 || package == XC2Package::CP56 || 
                   package == XC2Package::QFG32 {
                    true
                } else {
                    false
                }
            } else {
                false
            }
        },
        XC2Device::XC2C64 => {
            if speed == XC2Speed::Speed5 || speed == XC2Speed::Speed7 {
                if package == XC2Package::PC44 || package == XC2Package::VQ44 || package == XC2Package::CP56 ||
                    package == XC2Package::VQ100 {
                    true
                } else {
                    false
                }
            } else {
                false
            }
        },
        XC2Device::XC2C64A => {
            if speed == XC2Speed::Speed5 || speed == XC2Speed::Speed7 {
                if package == XC2Package::PC44 || package == XC2Package::VQ44 || package == XC2Package::CP56 ||
                    package == XC2Package::VQ100 || package == XC2Package::QFG48 {
                    true
                } else {
                    false
                }
            } else {
                false
            }
        },
        XC2Device::XC2C128 => {
            if speed == XC2Speed::Speed6 || speed == XC2Speed::Speed7 {
                if package == XC2Package::VQ100 || package == XC2Package::CP132 || package == XC2Package::TQ144 {
                    true
                } else {
                    false
                }
            } else {
                false
            }
        },
        XC2Device::XC2C256 => {
            if speed == XC2Speed::Speed6 || speed == XC2Speed::Speed7 {
                if package == XC2Package::VQ100 || package == XC2Package::CP132 || package == XC2Package::TQ144 ||
                   package == XC2Package::PQ208 || package == XC2Package::FT256 {
                    true
                } else {
                    false
                }
            } else {
                false
            }
        },
        XC2Device::XC2C384 => {
            if speed == XC2Speed::Speed7 || speed == XC2Speed::Speed10 {
                if package == XC2Package::TQ144 || package == XC2Package::PQ208 || package == XC2Package::FT256 ||
                   package == XC2Package::FG324 {
                    true
                } else {
                    false
                }
            } else {
                false
            }
        },
        XC2Device::XC2C512 => {
            if speed == XC2Speed::Speed7 || speed == XC2Speed::Speed10 {
                if package == XC2Package::PQ208 || package == XC2Package::FT256 || package == XC2Package::FG324 {
                    true
                } else {
                    false
                }
            } else {
                false
            }
        },
    }
}

/// Parses the given string in <device>-<speed>-<package> format and returns the parsed result if it is a legal
/// combination. Returns `None` if the part name string does not represent a valid device.
pub fn parse_part_name_string(part_name: &str) -> Option<(XC2Device, XC2Speed, XC2Package)> {
    let dev;
    let spd;
    let pkg;

    let name_split = part_name.split('-').collect::<Vec<_>>();
    if name_split.len() != 3 {
        return None;
    }

    if name_split[0].eq_ignore_ascii_case("xc2c32") {
        dev = XC2Device::XC2C32;
    } else if name_split[0].eq_ignore_ascii_case("xc2c32a") {
        dev = XC2Device::XC2C32A;
    } else if name_split[0].eq_ignore_ascii_case("xc2c64") {
        dev = XC2Device::XC2C64;
    } else if name_split[0].eq_ignore_ascii_case("xc2c64a") {
        dev = XC2Device::XC2C64A;
    } else if name_split[0].eq_ignore_ascii_case("xc2c128") {
        dev = XC2Device::XC2C128;
    } else if name_split[0].eq_ignore_ascii_case("xc2c256") {
        dev = XC2Device::XC2C256;
    } else if name_split[0].eq_ignore_ascii_case("xc2c384") {
        dev = XC2Device::XC2C384;
    } else if name_split[0].eq_ignore_ascii_case("xc2c512") {
        dev = XC2Device::XC2C512;
    } else {
        return None;
    }

    if name_split[1] == "4" {
        spd = XC2Speed::Speed4;
    } else if name_split[1] == "5" {
        spd = XC2Speed::Speed5;
    } else if name_split[1] == "6" {
        spd = XC2Speed::Speed6;
    } else if name_split[1] == "7" {
        spd = XC2Speed::Speed7;
    } else if name_split[1] == "10" {
        spd = XC2Speed::Speed10;
    } else {
        return None;
    }

    if name_split[2].eq_ignore_ascii_case("pc44") {
        pkg = XC2Package::PC44;
    } else if name_split[2].eq_ignore_ascii_case("qfg32") {
        pkg = XC2Package::QFG32;
    } else if name_split[2].eq_ignore_ascii_case("vq44") {
        pkg = XC2Package::VQ44;
    } else if name_split[2].eq_ignore_ascii_case("qfg48") {
        pkg = XC2Package::QFG48;
    } else if name_split[2].eq_ignore_ascii_case("cp56") {
        pkg = XC2Package::CP56;
    } else if name_split[2].eq_ignore_ascii_case("vq100") {
        pkg = XC2Package::VQ100;
    } else if name_split[2].eq_ignore_ascii_case("cp132") {
        pkg = XC2Package::CP132;
    } else if name_split[2].eq_ignore_ascii_case("tq144") {
        pkg = XC2Package::TQ144;
    } else if name_split[2].eq_ignore_ascii_case("pq208") {
        pkg = XC2Package::PQ208;
    } else if name_split[2].eq_ignore_ascii_case("ft256") {
        pkg = XC2Package::FT256;
    } else if name_split[2].eq_ignore_ascii_case("fg324") {
        pkg = XC2Package::FG324;
    } else {
        return None;
    }

    if !is_valid_part_combination(dev, spd, pkg) {
        return None;
    }

    Some((dev, spd, pkg))
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn spot_test_valid_part() {
        assert_eq!(parse_part_name_string("xc2c32a-6-vq44"),
            Some((XC2Device::XC2C32A, XC2Speed::Speed6, XC2Package::VQ44)));
        assert_eq!(parse_part_name_string("XC2C128-7-VQ100"),
            Some((XC2Device::XC2C128, XC2Speed::Speed7, XC2Package::VQ100)));
    }

    #[test]
    fn spot_test_invalid_part() {
        assert_eq!(parse_part_name_string("xc2c32a-1-vq44"), None);
        assert_eq!(parse_part_name_string("xc2c32a-5-vq100"), None);
    }

    #[test]
    fn malformed_part_names() {
        assert_eq!(parse_part_name_string("asdf"), None);
        assert_eq!(parse_part_name_string("xc2c32a-5-vq44-asdf"), None);
    }
}
