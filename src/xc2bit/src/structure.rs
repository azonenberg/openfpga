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

use *;

/// Returns the function block and macrocell index of the global clock signal GCKn for the given device
pub fn get_gck(device: XC2Device, idx: usize) -> Option<(u32, u32)> {
    if idx >= 3 {
        return None;
    }

    match device {
        XC2Device::XC2C32 | XC2Device::XC2C32A => match idx {
            0 => Some((1, 4)), 1 => Some((1, 5)), 2 => Some((1, 6)), _ => unreachable!()
        },
        XC2Device::XC2C64 | XC2Device::XC2C64A => match idx {
            0 => Some((1, 6)), 1 => Some((1, 7)), 2 => Some((1, 9)), _ => unreachable!()
        },
        XC2Device::XC2C128 => match idx {
            0 => Some((1, 12)), 1 => Some((1, 13)), 2 => Some((1, 15)), _ => unreachable!()
        },
        XC2Device::XC2C256 => match idx {
            0 => Some((4, 5)), 1 => Some((4, 3)), 2 => Some((5, 3)), _ => unreachable!()
        },
        XC2Device::XC2C384 => match idx {
            0 => Some((6, 14)), 1 => Some((6, 11)), 2 => Some((7, 1)), _ => unreachable!()
        },
        XC2Device::XC2C512 => match idx {
            0 => Some((10, 2)), 1 => Some((8, 15)), 2 => Some((9, 2)), _ => unreachable!()
        }
    }
}

/// Returns the function block and macrocell index of the global tristate signal GTSn for the given device
pub fn get_gts(device: XC2Device, idx: usize) -> Option<(u32, u32)> {
    if idx >= 4 {
        return None;
    }

    match device {
        XC2Device::XC2C32 | XC2Device::XC2C32A => match idx {
            0 => Some((0, 4)), 1 => Some((0, 3)), 2 => Some((0, 6)), 3 => Some((0, 5)), _ => unreachable!()
        },
        XC2Device::XC2C64 | XC2Device::XC2C64A => match idx {
            0 => Some((0, 9)), 1 => Some((0, 8)), 2 => Some((0, 11)), 3 => Some((0, 10)), _ => unreachable!()
        },
        XC2Device::XC2C128 => match idx {
            0 => Some((0, 15)), 1 => Some((0, 14)), 2 => Some((2, 2)), 3 => Some((2, 1)), _ => unreachable!()
        },
        XC2Device::XC2C256 => match idx {
            0 => Some((1, 4)), 1 => Some((1, 11)), 2 => Some((1, 0)), 3 => Some((1, 2)), _ => unreachable!()
        },
        XC2Device::XC2C384 => match idx {
            0 => Some((1, 4)), 1 => Some((1, 14)), 2 => Some((1, 0)), 3 => Some((1, 2)), _ => unreachable!()
        },
        XC2Device::XC2C512 => match idx {
            0 => Some((0, 0)), 1 => Some((1, 13)), 2 => Some((0, 12)), 3 => Some((0, 2)), _ => unreachable!()
        }
    }
}

/// Returns the function block and macrocell index of the global set/reset signal GSR for the given device
pub fn get_gsr(device: XC2Device) -> (u32, u32) {
    match device {
        XC2Device::XC2C32 | XC2Device::XC2C32A => (0, 7),
        XC2Device::XC2C64 | XC2Device::XC2C64A => (0, 12),
        XC2Device::XC2C128 => (2, 3),
        XC2Device::XC2C256 => (0, 2),
        XC2Device::XC2C384 => (0, 2),
        XC2Device::XC2C512 => (0, 15),
    }
}

/// Returns the function block and macrocell index of the global clock divider reset signal CDRST for the given device
pub fn get_cdrst(device: XC2Device) -> Option<(u32, u32)> {
    match device {
        XC2Device::XC2C32 | XC2Device::XC2C32A => None,
        XC2Device::XC2C64 | XC2Device::XC2C64A => None,
        XC2Device::XC2C128 => Some((1, 14)),
        XC2Device::XC2C256 => Some((5, 1)),
        XC2Device::XC2C384 => Some((6, 0)),
        XC2Device::XC2C512 => Some((9, 0)),
    }
}

/// Returns the function block and macrocell index of the global DataGATE enable signal DGE for the given device
pub fn get_dge(device: XC2Device) -> Option<(u32, u32)> {
    match device {
        XC2Device::XC2C32 | XC2Device::XC2C32A => None,
        XC2Device::XC2C64 | XC2Device::XC2C64A => None,
        XC2Device::XC2C128 => Some((3, 0)),
        XC2Device::XC2C256 => Some((5, 11)),
        XC2Device::XC2C384 => Some((7, 4)),
        XC2Device::XC2C512 => Some((9, 13)),
    }
}
