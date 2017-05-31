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

// Function block

use *;
use pla::{read_and_term_logical, read_or_term_logical};
use mc::{read_32_ff_logical};
use zia::{read_32_zia_fb_row_logical};

#[derive(Copy)]
pub struct XC2BistreamFB {
    pub and_terms: [XC2PLAAndTerm; 56],
    pub or_terms: [XC2PLAOrTerm; 16],
    pub zia_bits: [XC2ZIARowPiece; 40],
    pub ffs: [XC2MCFF; 16],
}

impl Clone for XC2BistreamFB {
    fn clone(&self) -> XC2BistreamFB {*self}
}

impl Default for XC2BistreamFB {
    fn default() -> XC2BistreamFB {
        XC2BistreamFB {
            and_terms: [XC2PLAAndTerm::default(); 56],
            or_terms: [XC2PLAOrTerm::default(); 16],
            zia_bits: [XC2ZIARowPiece::default(); 40],
            ffs: [XC2MCFF::default(); 16],
        }
    }
}

pub fn read_32_fb_logical(fuses: &[bool], block_idx: usize) -> Result<XC2BistreamFB, &'static str> {
    let mut and_terms = [XC2PLAAndTerm::default(); 56];
    let and_block_idx = match block_idx {
        0 => 320,
        1 => 6448,
        _ => return Err("invalid block_idx"),
    };
    for i in 0..and_terms.len() {
        and_terms[i] = read_and_term_logical(fuses, and_block_idx, i);
    }

    let mut or_terms = [XC2PLAOrTerm::default(); 16];
    let or_block_idx = match block_idx {
        0 => 4800,
        1 => 10928,
        _ => return Err("invalid block_idx"),
    };
    for i in 0..or_terms.len() {
        or_terms[i] = read_or_term_logical(fuses, or_block_idx, i);
    }

    let mut zia_bits = [XC2ZIARowPiece::default(); 40];
    let zia_block_idx = match block_idx {
        0 => 0,
        1 => 6128,
        _ => return Err("invalid block_idx"),
    };
    for i in 0..zia_bits.len() {
        let result = read_32_zia_fb_row_logical(fuses, zia_block_idx, i);
        if let Err(err) = result {
            return Err(err);
        }
        zia_bits[i] = result.unwrap();
    }

    let mut ff_bits = [XC2MCFF::default(); 16];
    let ff_block_idx = match block_idx {
        0 => 5696,
        1 => 11824,
        _ => return Err("invalid block_idx"),
    };
    for i in 0..ff_bits.len() {
        ff_bits[i] = read_32_ff_logical(fuses, ff_block_idx, i);
    }


    Ok(XC2BistreamFB {
        and_terms: and_terms,
        or_terms: or_terms,
        zia_bits: zia_bits,
        ffs: ff_bits,
    })
}