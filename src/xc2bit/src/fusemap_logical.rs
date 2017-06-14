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

/// Helper function that returns the first fuse for a given function block. This is made more complicated by buried
/// macrocells in the larger devices
pub fn fb_fuse_idx(device: XC2Device, fb: u32) -> usize {
    match device {
        XC2Device::XC2C32 | XC2Device::XC2C32A => {
            match fb {
                0 => 0,
                1 => 6128,
                _ => unreachable!(),
            }
        },
        XC2Device::XC2C64 | XC2Device::XC2C64A => {
            match fb {
                0 => 0,
                1 => 6448,
                2 => 12896,
                3 => 19344,
                _ => unreachable!(),
            }
        },
        XC2Device::XC2C128 => {
            match fb {
                0 => 0,
                1 => 6908,
                2 => 13816,
                3 => 20737,
                4 => 27658,
                5 => 34579,
                6 => 41487,
                7 => 48408,
                _ => unreachable!(),
            }
        },
        XC2Device::XC2C256 => {
            match fb {
                0 => 0,
                1 => 7695,
                2 => 15390,
                3 => 23085,
                4 => 30780,
                5 => 38475,
                6 => 46170,
                7 => 53878,
                8 => 61586,
                9 => 69294,
                10 => 77002,
                11 => 84710,
                12 => 92418,
                13 => 100113,
                14 => 107808,
                15 => 115516,
                _ => unreachable!(),
            }
        },
        XC2Device::XC2C384 => {
            match fb {
                0 => 0,
                1 => 8722,
                2 => 17444,
                3 => 26166,
                4 => 34888,
                5 => 43610,
                6 => 52332,
                7 => 61054,
                8 => 69776,
                9 => 78498,
                10 => 87220,
                11 => 95942,
                12 => 104664,
                13 => 113386,
                14 => 122108,
                15 => 130830,
                16 => 139552,
                17 => 148274,
                18 => 156996,
                19 => 165718,
                20 => 174440,
                21 => 183162,
                22 => 191884,
                23 => 200606,
                _ => unreachable!(),
            }
        },
        XC2Device::XC2C512 => {
            match fb {
                0 => 0,
                1 => 9256,
                2 => 18512,
                3 => 27781,
                4 => 37037,
                5 => 46306,
                6 => 55562,
                7 => 64831,
                8 => 74087,
                9 => 83343,
                10 => 92599,
                11 => 101855,
                12 => 111124,
                13 => 120380,
                14 => 129649,
                15 => 138905,
                16 => 148174,
                17 => 157443,
                18 => 166699,
                19 => 175968,
                20 => 185224,
                21 => 194493,
                22 => 203749,
                23 => 213018,
                24 => 222274,
                25 => 231530,
                26 => 240799,
                27 => 250055,
                28 => 259324,
                29 => 268580,
                30 => 277849,
                31 => 287105,
                _ => unreachable!(),
            }
        }
    }
}

pub fn gck_fuse_idx(device: XC2Device) -> usize {
    match device {
        XC2Device::XC2C32 | XC2Device::XC2C32A => 12256,
        XC2Device::XC2C64 | XC2Device::XC2C64A => 25792,
        XC2Device::XC2C128 => 55316,
        XC2Device::XC2C256 => 123224,
        XC2Device::XC2C384 => 209328,
        XC2Device::XC2C512 => 296374,
    }
}

pub fn gsr_fuse_idx(device: XC2Device) -> usize {
    match device {
        XC2Device::XC2C32 | XC2Device::XC2C32A => 12259,
        XC2Device::XC2C64 | XC2Device::XC2C64A => 25795,
        XC2Device::XC2C128 => 55324,
        XC2Device::XC2C256 => 123232,
        XC2Device::XC2C384 => 209336,
        XC2Device::XC2C512 => 296382,
    }
}

pub fn gts_fuse_idx(device: XC2Device) -> usize {
    match device {
        XC2Device::XC2C32 | XC2Device::XC2C32A => 12261,
        XC2Device::XC2C64 | XC2Device::XC2C64A => 25797,
        XC2Device::XC2C128 => 55326,
        XC2Device::XC2C256 => 123234,
        XC2Device::XC2C384 => 209338,
        XC2Device::XC2C512 => 296384,
    }
}

pub fn global_term_fuse_idx(device: XC2Device) -> usize {
    match device {
        XC2Device::XC2C32 | XC2Device::XC2C32A => 12269,
        XC2Device::XC2C64 | XC2Device::XC2C64A => 25805,
        XC2Device::XC2C128 => 55334,
        XC2Device::XC2C256 => 123242,
        XC2Device::XC2C384 => 209346,
        XC2Device::XC2C512 => 296392,
    }
}

pub fn total_logical_fuse_count(device: XC2Device) -> usize {
    match device {
        XC2Device::XC2C32 => 12274,
        XC2Device::XC2C32A => 12278,
        XC2Device::XC2C64 => 25808,
        XC2Device::XC2C64A => 25812,
        XC2Device::XC2C128 => 55341,
        XC2Device::XC2C256 => 123249,
        XC2Device::XC2C384 => 209357,
        XC2Device::XC2C512 => 296403,
    }
}

pub fn clock_div_fuse_idx(device: XC2Device) -> usize {
    match device {
        XC2Device::XC2C32 | XC2Device::XC2C32A | XC2Device::XC2C64 | XC2Device::XC2C64A => unreachable!(),
        XC2Device::XC2C128 => 55319,
        XC2Device::XC2C256 => 123227,
        XC2Device::XC2C384 => 209331,
        XC2Device::XC2C512 => 296377,
    }
}
