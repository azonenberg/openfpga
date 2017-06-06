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

// ZIA

pub struct XC2ZIARowPiece {
    selected: Option<XC2ZIAInput>,
}

pub enum XC2ZIAInput {
    Macrocell {
        fb: u32,
        ff: u32,
    },
    IBuf {
        ibuf: u32,
    },
    Zero,
    One,
}

static ZIA_BIT_TO_CHOICE_32: [[XC2ZIAInput; 6]; 40] = [
    // Row 0
    [XC2ZIAInput::IBuf{ibuf: 0},
     XC2ZIAInput::IBuf{ibuf: 10},
     XC2ZIAInput::IBuf{ibuf: 21},
     XC2ZIAInput::Macrocell{fb: 0, ff: 1},
     XC2ZIAInput::Macrocell{fb: 0, ff: 13},
     XC2ZIAInput::Macrocell{fb: 1, ff: 9}],

    [XC2ZIAInput::IBuf{ibuf: 1},
     XC2ZIAInput::IBuf{ibuf: 11},
     XC2ZIAInput::IBuf{ibuf: 22},
     XC2ZIAInput::Macrocell{fb: 0, ff: 8},
     XC2ZIAInput::Macrocell{fb: 0, ff: 15},
     XC2ZIAInput::Macrocell{fb: 1, ff: 12}],

    [XC2ZIAInput::IBuf{ibuf: 2},
     XC2ZIAInput::IBuf{ibuf: 12},
     XC2ZIAInput::IBuf{ibuf: 29},
     XC2ZIAInput::Macrocell{fb: 0, ff: 2},
     XC2ZIAInput::Macrocell{fb: 1, ff: 4},
     XC2ZIAInput::Macrocell{fb: 1, ff: 11}],

    [XC2ZIAInput::IBuf{ibuf: 3},
     XC2ZIAInput::IBuf{ibuf: 13},
     XC2ZIAInput::IBuf{ibuf: 25},
     XC2ZIAInput::Macrocell{fb: 0, ff: 9},
     XC2ZIAInput::Macrocell{fb: 0, ff: 14},
     XC2ZIAInput::Macrocell{fb: 1, ff: 6}],

    [XC2ZIAInput::IBuf{ibuf: 4},
     XC2ZIAInput::IBuf{ibuf: 14},
     XC2ZIAInput::IBuf{ibuf: 27},
     XC2ZIAInput::Macrocell{fb: 0, ff: 5},
     XC2ZIAInput::Macrocell{fb: 0, ff: 11},
     XC2ZIAInput::Macrocell{fb: 1, ff: 10}],
    // Row 5
    [XC2ZIAInput::IBuf{ibuf: 5},
     XC2ZIAInput::IBuf{ibuf: 15},
     XC2ZIAInput::IBuf{ibuf: 30},
     XC2ZIAInput::Macrocell{fb: 0, ff: 7},
     XC2ZIAInput::Macrocell{fb: 1, ff: 1},
     XC2ZIAInput::Macrocell{fb: 1, ff: 7}],

    [XC2ZIAInput::IBuf{ibuf: 6},
     XC2ZIAInput::IBuf{ibuf: 32},
     XC2ZIAInput::IBuf{ibuf: 20},
     XC2ZIAInput::Macrocell{fb: 0, ff: 0},
     XC2ZIAInput::Macrocell{fb: 1, ff: 3},
     XC2ZIAInput::Macrocell{fb: 1, ff: 13}],

    [XC2ZIAInput::IBuf{ibuf: 7},
     XC2ZIAInput::IBuf{ibuf: 16},
     XC2ZIAInput::IBuf{ibuf: 26},
     XC2ZIAInput::IBuf{ibuf: 31},
     XC2ZIAInput::Macrocell{fb: 0, ff: 12},
     XC2ZIAInput::Macrocell{fb: 1, ff: 15}],

    [XC2ZIAInput::IBuf{ibuf: 8},
     XC2ZIAInput::IBuf{ibuf: 17},
     XC2ZIAInput::IBuf{ibuf: 24},
     XC2ZIAInput::Macrocell{fb: 0, ff: 6},
     XC2ZIAInput::Macrocell{fb: 0, ff: 10},
     XC2ZIAInput::Macrocell{fb: 1, ff: 8}],

    [XC2ZIAInput::IBuf{ibuf: 9},
     XC2ZIAInput::IBuf{ibuf: 18},
     XC2ZIAInput::IBuf{ibuf: 23},
     XC2ZIAInput::Macrocell{fb: 0, ff: 4},
     XC2ZIAInput::Macrocell{fb: 1, ff: 2},
     XC2ZIAInput::Macrocell{fb: 1, ff: 5}],
    // Row 10
    [XC2ZIAInput::IBuf{ibuf: 7},
     XC2ZIAInput::IBuf{ibuf: 19},
     XC2ZIAInput::IBuf{ibuf: 28},
     XC2ZIAInput::Macrocell{fb: 0, ff: 3},
     XC2ZIAInput::Macrocell{fb: 1, ff: 0},
     XC2ZIAInput::Macrocell{fb: 1, ff: 14}],

    [XC2ZIAInput::IBuf{ibuf: 0},
     XC2ZIAInput::IBuf{ibuf: 11},
     XC2ZIAInput::IBuf{ibuf: 22},
     XC2ZIAInput::Macrocell{fb: 0, ff: 2},
     XC2ZIAInput::Macrocell{fb: 0, ff: 14},
     XC2ZIAInput::Macrocell{fb: 1, ff: 10}],

    [XC2ZIAInput::IBuf{ibuf: 1},
     XC2ZIAInput::IBuf{ibuf: 12},
     XC2ZIAInput::IBuf{ibuf: 28},
     XC2ZIAInput::Macrocell{fb: 0, ff: 4},
     XC2ZIAInput::Macrocell{fb: 1, ff: 1},
     XC2ZIAInput::Macrocell{fb: 1, ff: 15}],

    [XC2ZIAInput::IBuf{ibuf: 2},
     XC2ZIAInput::IBuf{ibuf: 18},
     XC2ZIAInput::IBuf{ibuf: 23},
     XC2ZIAInput::Macrocell{fb: 0, ff: 9},
     XC2ZIAInput::Macrocell{fb: 1, ff: 0},
     XC2ZIAInput::Macrocell{fb: 1, ff: 13}],

    [XC2ZIAInput::IBuf{ibuf: 3},
     XC2ZIAInput::IBuf{ibuf: 15},
     XC2ZIAInput::IBuf{ibuf: 30},
     XC2ZIAInput::Macrocell{fb: 0, ff: 3},
     XC2ZIAInput::Macrocell{fb: 0, ff: 11},
     XC2ZIAInput::Macrocell{fb: 1, ff: 12}],
    // Row 15
    [XC2ZIAInput::IBuf{ibuf: 4},
     XC2ZIAInput::IBuf{ibuf: 16},
     XC2ZIAInput::IBuf{ibuf: 21},
     XC2ZIAInput::Macrocell{fb: 0, ff: 0},
     XC2ZIAInput::Macrocell{fb: 0, ff: 15},
     XC2ZIAInput::Macrocell{fb: 1, ff: 7}],

    [XC2ZIAInput::IBuf{ibuf: 5},
     XC2ZIAInput::IBuf{ibuf: 19},
     XC2ZIAInput::IBuf{ibuf: 28},
     XC2ZIAInput::Macrocell{fb: 0, ff: 6},
     XC2ZIAInput::Macrocell{fb: 0, ff: 12},
     XC2ZIAInput::Macrocell{fb: 1, ff: 11}],

    [XC2ZIAInput::IBuf{ibuf: 6},
     XC2ZIAInput::IBuf{ibuf: 10},
     XC2ZIAInput::IBuf{ibuf: 21},
     XC2ZIAInput::Macrocell{fb: 0, ff: 8},
     XC2ZIAInput::Macrocell{fb: 1, ff: 2},
     XC2ZIAInput::Macrocell{fb: 1, ff: 8}],

    [XC2ZIAInput::IBuf{ibuf: 7},
     XC2ZIAInput::IBuf{ibuf: 32},
     XC2ZIAInput::IBuf{ibuf: 20},
     XC2ZIAInput::Macrocell{fb: 0, ff: 1},
     XC2ZIAInput::Macrocell{fb: 1, ff: 4},
     XC2ZIAInput::Macrocell{fb: 1, ff: 14}],

    [XC2ZIAInput::IBuf{ibuf: 8},
     XC2ZIAInput::IBuf{ibuf: 14},
     XC2ZIAInput::IBuf{ibuf: 27},
     XC2ZIAInput::IBuf{ibuf: 31},
     XC2ZIAInput::Macrocell{fb: 0, ff: 13},
     XC2ZIAInput::Macrocell{fb: 1, ff: 6}],
    // Row 20
    [XC2ZIAInput::IBuf{ibuf: 9},
     XC2ZIAInput::IBuf{ibuf: 13},
     XC2ZIAInput::IBuf{ibuf: 25},
     XC2ZIAInput::Macrocell{fb: 0, ff: 7},
     XC2ZIAInput::Macrocell{fb: 0, ff: 10},
     XC2ZIAInput::Macrocell{fb: 1, ff: 9}],

    [XC2ZIAInput::IBuf{ibuf: 8},
     XC2ZIAInput::IBuf{ibuf: 17},
     XC2ZIAInput::IBuf{ibuf: 24},
     XC2ZIAInput::Macrocell{fb: 0, ff: 5},
     XC2ZIAInput::Macrocell{fb: 1, ff: 3},
     XC2ZIAInput::Macrocell{fb: 1, ff: 5}],

    [XC2ZIAInput::IBuf{ibuf: 0},
     XC2ZIAInput::IBuf{ibuf: 12},
     XC2ZIAInput::IBuf{ibuf: 23},
     XC2ZIAInput::Macrocell{fb: 0, ff: 3},
     XC2ZIAInput::Macrocell{fb: 0, ff: 15},
     XC2ZIAInput::Macrocell{fb: 1, ff: 11}],

    [XC2ZIAInput::IBuf{ibuf: 1},
     XC2ZIAInput::IBuf{ibuf: 18},
     XC2ZIAInput::IBuf{ibuf: 25},
     XC2ZIAInput::Macrocell{fb: 0, ff: 6},
     XC2ZIAInput::Macrocell{fb: 1, ff: 4},
     XC2ZIAInput::Macrocell{fb: 1, ff: 5}],

    [XC2ZIAInput::IBuf{ibuf: 2},
     XC2ZIAInput::IBuf{ibuf: 13},
     XC2ZIAInput::IBuf{ibuf: 30},
     XC2ZIAInput::Macrocell{fb: 0, ff: 5},
     XC2ZIAInput::Macrocell{fb: 1, ff: 2},
     XC2ZIAInput::Macrocell{fb: 1, ff: 6}],
    // Row 25
    [XC2ZIAInput::IBuf{ibuf: 3},
     XC2ZIAInput::IBuf{ibuf: 19},
     XC2ZIAInput::IBuf{ibuf: 24},
     XC2ZIAInput::Macrocell{fb: 0, ff: 0},
     XC2ZIAInput::Macrocell{fb: 1, ff: 1},
     XC2ZIAInput::Macrocell{fb: 1, ff: 14}],

    [XC2ZIAInput::IBuf{ibuf: 4},
     XC2ZIAInput::IBuf{ibuf: 32},
     XC2ZIAInput::IBuf{ibuf: 21},
     XC2ZIAInput::Macrocell{fb: 0, ff: 4},
     XC2ZIAInput::Macrocell{fb: 0, ff: 12},
     XC2ZIAInput::Macrocell{fb: 1, ff: 13}],

    [XC2ZIAInput::IBuf{ibuf: 5},
     XC2ZIAInput::IBuf{ibuf: 17},
     XC2ZIAInput::IBuf{ibuf: 27},
     XC2ZIAInput::Macrocell{fb: 0, ff: 1},
     XC2ZIAInput::Macrocell{fb: 1, ff: 0},
     XC2ZIAInput::Macrocell{fb: 1, ff: 8}],

    [XC2ZIAInput::IBuf{ibuf: 6},
     XC2ZIAInput::IBuf{ibuf: 11},
     XC2ZIAInput::IBuf{ibuf: 29},
     XC2ZIAInput::Macrocell{fb: 0, ff: 7},
     XC2ZIAInput::Macrocell{fb: 0, ff: 13},
     XC2ZIAInput::Macrocell{fb: 1, ff: 12}],

    [XC2ZIAInput::IBuf{ibuf: 7},
     XC2ZIAInput::IBuf{ibuf: 10},
     XC2ZIAInput::IBuf{ibuf: 22},
     XC2ZIAInput::Macrocell{fb: 0, ff: 9},
     XC2ZIAInput::Macrocell{fb: 1, ff: 3},
     XC2ZIAInput::Macrocell{fb: 1, ff: 9}],
    // Row 30
    [XC2ZIAInput::IBuf{ibuf: 8},
     XC2ZIAInput::IBuf{ibuf: 16},
     XC2ZIAInput::IBuf{ibuf: 20},
     XC2ZIAInput::Macrocell{fb: 0, ff: 2},
     XC2ZIAInput::Macrocell{fb: 0, ff: 11},
     XC2ZIAInput::Macrocell{fb: 1, ff: 15}],

    [XC2ZIAInput::IBuf{ibuf: 9},
     XC2ZIAInput::IBuf{ibuf: 15},
     XC2ZIAInput::IBuf{ibuf: 28},
     XC2ZIAInput::IBuf{ibuf: 31},
     XC2ZIAInput::Macrocell{fb: 0, ff: 14},
     XC2ZIAInput::Macrocell{fb: 1, ff: 7}],

    [XC2ZIAInput::IBuf{ibuf: 9},
     XC2ZIAInput::IBuf{ibuf: 14},
     XC2ZIAInput::IBuf{ibuf: 21},
     XC2ZIAInput::Macrocell{fb: 0, ff: 8},
     XC2ZIAInput::Macrocell{fb: 0, ff: 10},
     XC2ZIAInput::Macrocell{fb: 1, ff: 10}],

    [XC2ZIAInput::IBuf{ibuf: 0},
     XC2ZIAInput::IBuf{ibuf: 13},
     XC2ZIAInput::IBuf{ibuf: 24},
     XC2ZIAInput::Macrocell{fb: 0, ff: 4},
     XC2ZIAInput::Macrocell{fb: 1, ff: 0},
     XC2ZIAInput::Macrocell{fb: 1, ff: 12}],

    [XC2ZIAInput::IBuf{ibuf: 1},
     XC2ZIAInput::IBuf{ibuf: 15},
     XC2ZIAInput::IBuf{ibuf: 27},
     XC2ZIAInput::Macrocell{fb: 0, ff: 9},
     XC2ZIAInput::Macrocell{fb: 0, ff: 10},
     XC2ZIAInput::Macrocell{fb: 1, ff: 11}],
    // Row 35
    [XC2ZIAInput::IBuf{ibuf: 2},
     XC2ZIAInput::IBuf{ibuf: 19},
     XC2ZIAInput::IBuf{ibuf: 21},
     XC2ZIAInput::Macrocell{fb: 0, ff: 7},
     XC2ZIAInput::Macrocell{fb: 0, ff: 11},
     XC2ZIAInput::Macrocell{fb: 1, ff: 5}],
     
    [XC2ZIAInput::IBuf{ibuf: 3},
     XC2ZIAInput::IBuf{ibuf: 14},
     XC2ZIAInput::IBuf{ibuf: 21},
     XC2ZIAInput::Macrocell{fb: 0, ff: 6},
     XC2ZIAInput::Macrocell{fb: 1, ff: 3},
     XC2ZIAInput::Macrocell{fb: 1, ff: 7}],
     
    [XC2ZIAInput::IBuf{ibuf: 4},
     XC2ZIAInput::IBuf{ibuf: 11},
     XC2ZIAInput::IBuf{ibuf: 25},
     XC2ZIAInput::Macrocell{fb: 0, ff: 1},
     XC2ZIAInput::Macrocell{fb: 1, ff: 2},
     XC2ZIAInput::Macrocell{fb: 1, ff: 15}],
     
    [XC2ZIAInput::IBuf{ibuf: 5},
     XC2ZIAInput::IBuf{ibuf: 16},
     XC2ZIAInput::IBuf{ibuf: 22},
     XC2ZIAInput::Macrocell{fb: 0, ff: 5},
     XC2ZIAInput::Macrocell{fb: 0, ff: 13},
     XC2ZIAInput::Macrocell{fb: 1, ff: 14}],
     
    [XC2ZIAInput::IBuf{ibuf: 6},
     XC2ZIAInput::IBuf{ibuf: 18},
     XC2ZIAInput::IBuf{ibuf: 28},
     XC2ZIAInput::Macrocell{fb: 0, ff: 2},
     XC2ZIAInput::Macrocell{fb: 1, ff: 1},
     XC2ZIAInput::Macrocell{fb: 1, ff: 9}],
];

// Read a piece of the ZIA corresponding to one FB and one row
pub fn read_32_zia_fb_row_logical(fuses: &[bool], block_idx: usize, row_idx: usize)
    -> Result<XC2ZIARowPiece, &'static str> {

    Err("err")
}
