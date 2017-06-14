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

pub fn fuse_array_dims(device: XC2Device) -> (usize, usize) {
    match device {
        XC2Device::XC2C32 | XC2Device::XC2C32A => (260, 50),
        XC2Device::XC2C64 | XC2Device::XC2C64A => (274, 98),
        XC2Device::XC2C128 => (752, 82),
        XC2Device::XC2C256 => (1364, 98),
        XC2Device::XC2C384 => (1868, 122),
        XC2Device::XC2C512 => (1980, 162),
    }
}

pub fn and_block_loc(device: XC2Device, fb: u32) -> (usize, usize, bool) {
    match device {
        // "Type 1" blocks (OR array is in the middle)
        XC2Device::XC2C32 | XC2Device::XC2C32A => {
            match fb {
                // left
                0 => (10, 0, false),
                // right
                1 => (249, 0, true),
                _ => unreachable!(),
            }
        },
        XC2Device::XC2C64 | XC2Device::XC2C64A => {
            match fb {
                0 => (9, 0, false),                 1 => (264, 0, true),
                2 => (9, 48, false),                3 => (264, 48, true),
                _ => unreachable!(),
            }
        },
        XC2Device::XC2C256 => {
            match fb {
                // group 0
                0 => (11, 0 , false),               1 => (330, 0 , true),
                2 => (11, 48 , false),              3 => (330, 48 , true),
                // group 1
                4 => (351, 0 , false),              5 => (670, 0 , true),
                6 => (351, 48 , false),             7 => (670, 48, true),
                // group 2
                8 => (693, 0 , false),              9 => (1012, 0 , true),
                10 => (693, 48 , false),            11 => (1012, 48 , true),
                // group 3
                12 => (1033, 0 , false),            13 => (1352, 0 , true),
                14 => (1033, 48 , false),           15 => (1352, 48 , true),
                _ => unreachable!(),
            }
        },
        // "Type 2" blocks (OR array is on the sides)
        XC2Device::XC2C128 => {
            match fb {
                // group 0
                0 => (48, 0, false),                1 => (327, 0, true),
                2 => (48, 40, false),               3 => (327, 40, true),
                // group 1
                4 => (424, 0, false),               5 => (703, 0, true),
                6 => (424, 40, false),              7 => (703, 40, true),
                _ => unreachable!(),
            }
        },
        XC2Device::XC2C384 => {
            match fb {
                // group 0
                0 => (48, 0, false),                1 => (419, 0, true),
                2 => (48, 40, false),               3 => (419, 40, true),
                4 => (48, 80, false),               5 => (419, 80, true),
                // group 1
                6 => (514, 0, false),               7 => (885, 0, true),
                8 => (514, 40, false),              9 => (885, 40, true),
                10 => (514, 80, false),             11 => (885, 80, true),
                // group 2
                12 => (982, 0, false),              13 => (1353, 0, true),
                14 => (982, 40, false),             15 => (1353, 40, true),
                16 => (982, 80, false),             17 => (1353, 80, true),
                // group 3
                18 => (1448, 0, false),             19 => (1819, 0, true),
                20 => (1448, 40, false),            21 => (1819, 40, true),
                22 => (1448, 80, false),            23 => (1819, 80, true),
                _ => unreachable!(),
            }
        },
        XC2Device::XC2C512 => {
            match fb {
                // group 0
                0 => (48, 0, false),                1 => (447, 0, true),
                2 => (48, 40, false),               3 => (447, 40, true),
                4 => (48, 80, false),               5 => (447, 80, true),
                6 => (48, 120, false),              7 => (447, 120, true),
                // group 1
                8 => (542, 0, false),               9 => (941, 0, true),
                10 => (542, 40, false),             11 => (941, 40, true),
                12 => (542, 80, false),             13 => (941, 80, true),
                14 => (542, 120, false),            15 => (941, 120, true),
                // group 2
                16 => (1038, 0, false),             17 => (1437, 0, true),
                18 => (1038, 40, false),            19 => (1437, 40, true),
                20 => (1038, 80, false),            21 => (1437, 80, true),
                22 => (1038, 120, false),           23 => (1437, 120, true),
                // group 3
                24 => (1532, 0, false),             25 => (1931, 0, true),
                26 => (1532, 40, false),            27 => (1931, 40, true),
                28 => (1532, 80, false),            29 => (1931, 80, true),
                30 => (1532, 120, false),           31 => (1931, 120, true),
                _ => unreachable!(),
            }
        },
    }
}

pub fn or_block_loc(device: XC2Device, fb: u32) -> (usize, usize, bool) {
    match device {
        // "Type 1" blocks (OR array is in the middle)
        XC2Device::XC2C32 | XC2Device::XC2C32A | XC2Device::XC2C64 | XC2Device::XC2C64A | XC2Device::XC2C256 => {
            // We know we are in the middle of the AND array
            let (and_x, and_y, mirror) = and_block_loc(device, fb);
            (and_x, and_y + 20, mirror)
        },
        // "Type 2" blocks (OR array is on the sides)
        XC2Device::XC2C128 | XC2Device::XC2C384 | XC2Device::XC2C512 => {
            // "left-hand" non-mirrored blocks need to shift left 32
            // "right-hand" mirrored blocks need to shift right 32
            let (and_x, and_y, mirror) = and_block_loc(device, fb);
            if !mirror {
                (and_x - 32, and_y, mirror)
            } else {
                (and_x + 32, and_y, mirror)
            }
        },
    }
}

pub fn zia_block_loc(device: XC2Device, fb: u32) -> (usize, usize) {
    // The ZIA is always to the right of the "left-hand" even-numbered FB. If the FB is odd-numbered, it shifts
    // right by 1.

    let (and_x, and_y, _) = and_block_loc(device, (fb / 2) * 2);

    if fb % 2 == 0 {
        // "left-hand"
        (and_x + 112, and_y)
    } else {
        // "right-hand"
        (and_x + 113, and_y)
    }
}
