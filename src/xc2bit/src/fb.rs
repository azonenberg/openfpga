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

//! Contains functions pertaining to function blocks

use std::io;
use std::io::Write;

use *;
use fusemap_physical::{zia_block_loc, and_block_loc, or_block_loc};
use pla::{read_and_term_logical, read_or_term_logical};
use mc::{read_small_ff_logical, read_large_ff_logical, read_large_buried_ff_logical};
use zia::{encode_32_zia_choice, encode_64_zia_choice, encode_128_zia_choice, encode_256_zia_choice,
          encode_384_zia_choice, encode_512_zia_choice, read_32_zia_fb_row_logical, read_64_zia_fb_row_logical,
          read_128_zia_fb_row_logical, read_256_zia_fb_row_logical, read_384_zia_fb_row_logical,
          read_512_zia_fb_row_logical, zia_get_row_width};

/// Represents a collection of all the parts that make up one function block
#[derive(Copy)]
pub struct XC2BitstreamFB {
    /// The AND terms of the PLA part of the function block
    pub and_terms: [XC2PLAAndTerm; ANDTERMS_PER_FB],
    /// The OR terms of the PLA part of the function block
    pub or_terms: [XC2PLAOrTerm; MCS_PER_FB],
    /// The inputs to the function block from the ZIA
    pub zia_bits: [XC2ZIARowPiece; INPUTS_PER_ANDTERM],
    /// The macrocells of the function block
    pub ffs: [XC2Macrocell; MCS_PER_FB],
}

impl Clone for XC2BitstreamFB {
    fn clone(&self) -> XC2BitstreamFB {*self}
}

impl Default for XC2BitstreamFB {
    fn default() -> XC2BitstreamFB {
        XC2BitstreamFB {
            and_terms: [XC2PLAAndTerm::default(); ANDTERMS_PER_FB],
            or_terms: [XC2PLAOrTerm::default(); MCS_PER_FB],
            zia_bits: [XC2ZIARowPiece::default(); INPUTS_PER_ANDTERM],
            ffs: [XC2Macrocell::default(); MCS_PER_FB],
        }
    }
}

/// Internal helper that writes a ZIA row to the fuse array
fn zia_row_crbit_helper(x: usize, y: usize, zia_row: usize, zia_bits: &[bool], has_gap: bool,
    fuse_array: &mut FuseArray) {

    for zia_bit in 0..zia_bits.len() {
        let mut out_y = y + zia_row;
        if has_gap && zia_row >= 20 {
            // There is an OR array in the middle, 8 rows high
            out_y += 8;
        }

        let out_x = x + zia_bit * 2;

        fuse_array.set(out_x, out_y, zia_bits[zia_bits.len() - 1 - zia_bit]);
    }
}

// Weird mapping here in (mostly) groups of 3
// TODO: Explain better
static AND_BLOCK_TYPE2_P2L_MAP: [usize; ANDTERMS_PER_FB] = [
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
    55, 54, 53,
    11, 12, 13,
    52, 51, 50,
    14, 15, 16,
    49, 48, 47,
    17, 18, 19,
    46, 45, 44,
    20, 21, 22,
    43, 42, 41,
    23, 24, 25,
    40, 39, 38,
    26, 27, 28,
    37, 36, 35,
    29, 30, 31,
    34, 33, 32];

static OR_BLOCK_TYPE2_ROW_MAP: [usize; ANDTERMS_PER_FB / 2] =
    [17, 19, 22, 20, 0, 1, 3, 4, 5, 7, 8, 11, 12, 13, 15, 16, 23, 24, 26, 27, 28, 31, 32, 34, 35, 36, 38, 39];

impl XC2BitstreamFB {
    /// Dump a human-readable explanation of the settings for this FB to the given `writer` object.
    /// `device` must be the device type this FB was extracted from and is needed to decode I/O pin numbers.
    /// `fb` must be the index of this function block.
    pub fn dump_human_readable(&self, device: XC2Device, fb: u32, writer: &mut Write) -> Result<(), io::Error> {
        for i in 0..MCS_PER_FB {
            self.ffs[i].dump_human_readable(fb, i as u32, writer)?;
        }

        write!(writer, "\n")?;
        write!(writer, "ZIA inputs for FB{}\n", fb + 1)?;
        for i in 0..INPUTS_PER_ANDTERM {
            write!(writer, "{:2}: ", i)?;
            match self.zia_bits[i].selected {
                XC2ZIAInput::Zero => write!(writer, "0\n")?,
                XC2ZIAInput::One => write!(writer, "1\n")?,
                XC2ZIAInput::Macrocell{fb, ff} =>
                    write!(writer, "FB{}_{} FF\n", fb + 1, ff + 1)?,
                XC2ZIAInput::IBuf{ibuf} => {
                    let (fb, ff) = iob_num_to_fb_ff_num(device, ibuf).unwrap();
                    write!(writer, "FB{}_{} pad\n", fb + 1, ff + 1)?;
                },
                XC2ZIAInput::DedicatedInput => write!(writer, "dedicated input\n")?,
            }
        }

        write!(writer, "\n")?;
        write!(writer, "AND terms for FB{}\n", fb + 1)?;
        write!(writer, "   |  0| ~0|  1| ~1|  2| ~2|  3| ~3|  4| ~4|  5| ~5|  6| ~6|  7| ~7|  8| ~8|  9| ~9| 10|~10| \
                                     11|~11| 12|~12| 13|~13| 14|~14| 15|~15| 16|~16| 17|~17| 18|~18| 19|~19| 20|~20| \
                                     21|~21| 22|~22| 23|~23| 24|~24| 25|~25| 26|~26| 27|~27| 28|~28| 29|~29| 30|~30| \
                                     31|~31| 32|~32| 33|~33| 34|~34| 35|~35| 36|~36| 37|~37| 38|~38| 39|~39\
                                     \n")?;
        for i in 0..ANDTERMS_PER_FB {
            write!(writer, "{:2}:", i)?;
            for j in 0..INPUTS_PER_ANDTERM {
                if self.and_terms[i].input[j] {
                    write!(writer, "|XXX")?;
                } else {
                    write!(writer, "|   ")?;
                }

                if self.and_terms[i].input_b[j] {
                    write!(writer, "|XXX")?;
                } else {
                    write!(writer, "|   ")?;
                }
            }
            write!(writer, "\n")?;
        }

        write!(writer, "\n")?;
        write!(writer, "OR terms for FB{}\n", fb + 1)?;
        write!(writer, "   | 0| 1| 2| 3| 4| 5| 6| 7| 8| 9|10|11|12|13|14|15|16|17|18|19|20|\
                               21|22|23|24|25|26|27|28|29|30|31|32|33|34|35|36|37|38|39|40|\
                               41|42|43|44|45|46|47|48|49|50|51|52|53|54|55\n")?;
        for i in 0..MCS_PER_FB {
            write!(writer, "{:2}:", i)?;
            for j in 0..ANDTERMS_PER_FB {
                if self.or_terms[i].input[j] {
                    write!(writer, "|XX")?;
                } else {
                    write!(writer, "|  ")?;
                }
            }
            write!(writer, "\n")?;
        }

        Ok(())
    }

    /// Write the crbit representation of the settings for this FB to the given `fuse_array`.
    /// `device` must be the device type this FB was extracted from.
    /// `fb` must be the index of this function block.
    pub fn to_crbit(&self, device: XC2Device, fb: u32, fuse_array: &mut FuseArray) {
        // ZIA
        let (x, y) = zia_block_loc(device, fb);
        for zia_row in 0..INPUTS_PER_ANDTERM {
            match device {
                XC2Device::XC2C32 | XC2Device::XC2C32A => {
                    let zia_choice_bits = encode_32_zia_choice(zia_row as u32, self.zia_bits[zia_row].selected)
                        // FIXME: Fold this into the error system??
                        .expect("invalid ZIA input");

                    zia_row_crbit_helper(x, y, zia_row, &zia_choice_bits, true, fuse_array);
                },
                XC2Device::XC2C64 | XC2Device::XC2C64A => {
                    let zia_choice_bits = encode_64_zia_choice(zia_row as u32, self.zia_bits[zia_row].selected)
                        // FIXME: Fold this into the error system??
                        .expect("invalid ZIA input");

                    zia_row_crbit_helper(x, y, zia_row, &zia_choice_bits, true, fuse_array);
                },
                XC2Device::XC2C128 => {
                    let zia_choice_bits = encode_128_zia_choice(zia_row as u32, self.zia_bits[zia_row].selected)
                        // FIXME: Fold this into the error system??
                        .expect("invalid ZIA input");

                    zia_row_crbit_helper(x, y, zia_row, &zia_choice_bits, false, fuse_array);
                },
                XC2Device::XC2C256 => {
                    let zia_choice_bits = encode_256_zia_choice(zia_row as u32, self.zia_bits[zia_row].selected)
                        // FIXME: Fold this into the error system??
                        .expect("invalid ZIA input");

                    zia_row_crbit_helper(x, y, zia_row, &zia_choice_bits, true, fuse_array);
                },
                XC2Device::XC2C384 => {
                    let zia_choice_bits = encode_384_zia_choice(zia_row as u32, self.zia_bits[zia_row].selected)
                        // FIXME: Fold this into the error system??
                        .expect("invalid ZIA input");

                    zia_row_crbit_helper(x, y, zia_row, &zia_choice_bits, false, fuse_array);
                },
                XC2Device::XC2C512 => {
                    let zia_choice_bits = encode_512_zia_choice(zia_row as u32, self.zia_bits[zia_row].selected)
                        // FIXME: Fold this into the error system??
                        .expect("invalid ZIA input");

                    zia_row_crbit_helper(x, y, zia_row, &zia_choice_bits, false, fuse_array);
                },
            };
        }

        // AND block
        let (x, y, mirror) = and_block_loc(device, fb);
        match device {
            // "Type 1" blocks (OR array is in the middle)
            XC2Device::XC2C32 | XC2Device::XC2C32A | XC2Device::XC2C64 | XC2Device::XC2C64A | XC2Device::XC2C256 => {
                for term_idx in 0..ANDTERMS_PER_FB {
                    for input_idx in 0..INPUTS_PER_ANDTERM {
                        let mut out_y = y + input_idx;
                        if input_idx >= 20 {
                            // There is an OR array in the middle, 8 rows high
                            out_y += 8;
                        }

                        if !mirror {
                            // true input
                            fuse_array.set(x + term_idx * 2 + 1, out_y, !self.and_terms[term_idx].input[input_idx]);
                            // complement input
                            fuse_array.set(x + term_idx * 2 + 0, out_y, !self.and_terms[term_idx].input_b[input_idx]);
                        } else {
                            // true input
                            fuse_array.set(x - term_idx * 2 - 1, out_y, !self.and_terms[term_idx].input[input_idx]);
                            // complement input
                            fuse_array.set(x - term_idx * 2 - 0, out_y, !self.and_terms[term_idx].input_b[input_idx]);
                        }
                    }
                }
            },
            // "Type 2" blocks (OR array is on the sides)
            XC2Device::XC2C128 | XC2Device::XC2C384 | XC2Device::XC2C512 => {
                for term_idx in 0..ANDTERMS_PER_FB {
                    for input_idx in 0..INPUTS_PER_ANDTERM {
                        let phys_term_idx = AND_BLOCK_TYPE2_P2L_MAP[term_idx];
                        if !mirror {
                            // true input
                            fuse_array.set(x + term_idx * 2 + 1, y + input_idx,
                                !self.and_terms[phys_term_idx].input[input_idx]);
                            // complement input
                            fuse_array.set(x + term_idx * 2 + 0, y + input_idx,
                                !self.and_terms[phys_term_idx].input_b[input_idx]);
                        } else {
                            // true input
                            fuse_array.set(x - term_idx * 2 - 1, y + input_idx,
                                !self.and_terms[phys_term_idx].input[input_idx]);
                            // complement input
                            fuse_array.set(x - term_idx * 2 - 0, y + input_idx,
                                !self.and_terms[phys_term_idx].input_b[input_idx]);
                        }
                    }
                }
            },
        }

        // OR block
        let (x, y, mirror) = or_block_loc(device, fb);
        match device {
            // "Type 1" blocks (OR array is in the middle)
            XC2Device::XC2C32 | XC2Device::XC2C32A | XC2Device::XC2C64 | XC2Device::XC2C64A | XC2Device::XC2C256 => {
                for or_term_idx in 0..MCS_PER_FB {
                    for and_term_idx in 0..ANDTERMS_PER_FB {
                        let out_y = y + (or_term_idx / 2);
                        let off_x = and_term_idx * 2 + (or_term_idx % 2);
                        let out_x = if !mirror {
                            x + off_x
                        } else {
                            x - off_x
                        };

                        fuse_array.set(out_x, out_y, !self.or_terms[or_term_idx].input[and_term_idx]);
                    }
                }
            },
            // "Type 2" blocks (OR array is on the sides)
            XC2Device::XC2C128 | XC2Device::XC2C384 | XC2Device::XC2C512 => {
                for or_term_idx in 0..MCS_PER_FB {
                    for and_term_idx in 0..ANDTERMS_PER_FB {
                        let out_y = y + OR_BLOCK_TYPE2_ROW_MAP[and_term_idx / 2];
                        let mut out_x = or_term_idx * 2;
                        // TODO: Explain wtf is happening here
                        if OR_BLOCK_TYPE2_ROW_MAP[and_term_idx / 2] >= 23 {
                            // "Reverse"
                            if and_term_idx % 2 == 0 {
                                out_x += 1;
                            }
                        } else {
                            if and_term_idx % 2 == 1 {
                                out_x += 1;
                            }
                        }

                        let out_x = if !mirror {
                            x + out_x
                        } else {
                            x - out_x
                        };

                        fuse_array.set(out_x, out_y, !self.or_terms[or_term_idx].input[and_term_idx]);
                    }
                }
            },
        }
    }

    /// Write the .JED representation of the settings for this FB to the given `writer` object.
    /// `device` must be the device type this FB was extracted from and is needed to encode the ZIA.
    /// `fuse_base` must be the starting fuse number of this function block.
    pub fn write_to_jed(&self, device: XC2Device, fuse_base: usize, writer: &mut Write) -> Result<(), io::Error> {
        // ZIA
        let zia_row_width = zia_get_row_width(device);
        for i in 0..INPUTS_PER_ANDTERM {
            write!(writer, "L{:06} ", fuse_base + i * zia_row_width)?;
            match device {
                XC2Device::XC2C32 | XC2Device::XC2C32A => {
                    let zia_choice_bits = encode_32_zia_choice(i as u32, self.zia_bits[i].selected)
                        // FIXME: Fold this into the error system??
                        .expect("invalid ZIA input");
                    for j in 0..zia_choice_bits.len() {
                        write!(writer, "{}", if zia_choice_bits[j] {"1"} else {"0"})?;
                    }
                },
                XC2Device::XC2C64 | XC2Device::XC2C64A => {
                    let zia_choice_bits = encode_64_zia_choice(i as u32, self.zia_bits[i].selected)
                        // FIXME: Fold this into the error system??
                        .expect("invalid ZIA input");
                    for j in 0..zia_choice_bits.len() {
                        write!(writer, "{}", if zia_choice_bits[j] {"1"} else {"0"})?;
                    }
                },
                XC2Device::XC2C128 => {
                    let zia_choice_bits = encode_128_zia_choice(i as u32, self.zia_bits[i].selected)
                        // FIXME: Fold this into the error system??
                        .expect("invalid ZIA input");
                    for j in 0..zia_choice_bits.len() {
                        write!(writer, "{}", if zia_choice_bits[j] {"1"} else {"0"})?;
                    }
                },
                XC2Device::XC2C256 => {
                    let zia_choice_bits = encode_256_zia_choice(i as u32, self.zia_bits[i].selected)
                        // FIXME: Fold this into the error system??
                        .expect("invalid ZIA input");
                    for j in 0..zia_choice_bits.len() {
                        write!(writer, "{}", if zia_choice_bits[j] {"1"} else {"0"})?;
                    }
                },
                XC2Device::XC2C384 => {
                    let zia_choice_bits = encode_384_zia_choice(i as u32, self.zia_bits[i].selected)
                        // FIXME: Fold this into the error system??
                        .expect("invalid ZIA input");
                    for j in 0..zia_choice_bits.len() {
                        write!(writer, "{}", if zia_choice_bits[j] {"1"} else {"0"})?;
                    }
                },
                XC2Device::XC2C512 => {
                    let zia_choice_bits = encode_512_zia_choice(i as u32, self.zia_bits[i].selected)
                        // FIXME: Fold this into the error system??
                        .expect("invalid ZIA input");
                    for j in 0..zia_choice_bits.len() {
                        write!(writer, "{}", if zia_choice_bits[j] {"1"} else {"0"})?;
                    }
                },
            }
            write!(writer, "*\n")?;
        }
        write!(writer, "\n")?;

        // AND terms
        for i in 0..ANDTERMS_PER_FB {
            write!(writer, "L{:06} ",
                fuse_base + zia_row_width * INPUTS_PER_ANDTERM + i * INPUTS_PER_ANDTERM * 2)?;
            for j in 0..INPUTS_PER_ANDTERM {
                if self.and_terms[i].input[j] {
                    write!(writer, "0")?;
                } else {
                    write!(writer, "1")?;
                }
                if self.and_terms[i].input_b[j] {
                    write!(writer, "0")?;
                } else {
                    write!(writer, "1")?;
                }
            }
            write!(writer, "*\n")?;
        }
        write!(writer, "\n")?;

        // OR terms
        for i in 0..ANDTERMS_PER_FB {
            write!(writer, "L{:06} ",
                fuse_base + zia_row_width * INPUTS_PER_ANDTERM +
                ANDTERMS_PER_FB * INPUTS_PER_ANDTERM * 2 + i * MCS_PER_FB)?;
            for j in 0..MCS_PER_FB {
                if self.or_terms[j].input[i] {
                    write!(writer, "0")?;
                } else {
                    write!(writer, "1")?;
                }
            }
            write!(writer, "*\n")?;
        }
        write!(writer, "\n")?;

        Ok(())
    }
}

/// Internal function that reads a function block
pub fn read_fb_logical(device: XC2Device, fuses: &[bool], fb: u32, fuse_base: usize)
    -> Result<XC2BitstreamFB, &'static str> {

    let zia_row_width = zia_get_row_width(device);
    let size_of_zia = zia_row_width * INPUTS_PER_ANDTERM;
    let size_of_and = INPUTS_PER_ANDTERM * 2 * ANDTERMS_PER_FB;
    let size_of_or = ANDTERMS_PER_FB * MCS_PER_FB;

    let device_is_large =  match device {
        XC2Device::XC2C32 | XC2Device::XC2C32A | XC2Device::XC2C64 | XC2Device::XC2C64A => false,
        _ => true,
    };

    let zia_row_read_function = match device {
        XC2Device::XC2C32 | XC2Device::XC2C32A => read_32_zia_fb_row_logical,
        XC2Device::XC2C64 | XC2Device::XC2C64A => read_64_zia_fb_row_logical,
        XC2Device::XC2C128 => read_128_zia_fb_row_logical,
        XC2Device::XC2C256 => read_256_zia_fb_row_logical,
        XC2Device::XC2C384 => read_384_zia_fb_row_logical,
        XC2Device::XC2C512 => read_512_zia_fb_row_logical,
    };

    let mut and_terms = [XC2PLAAndTerm::default(); ANDTERMS_PER_FB];
    let and_block_idx = fuse_base + size_of_zia;
    for i in 0..and_terms.len() {
        and_terms[i] = read_and_term_logical(fuses, and_block_idx, i);
    }

    let mut or_terms = [XC2PLAOrTerm::default(); MCS_PER_FB];
    let or_block_idx = fuse_base + size_of_zia + size_of_and;
    for i in 0..or_terms.len() {
        or_terms[i] = read_or_term_logical(fuses, or_block_idx, i);
    }

    let mut zia_bits = [XC2ZIARowPiece::default(); INPUTS_PER_ANDTERM];
    let zia_block_idx = fuse_base;
    for i in 0..zia_bits.len() {
        let result = zia_row_read_function(fuses, zia_block_idx, i)?;
        zia_bits[i] = result;
    }

    let mut ff_bits = [XC2Macrocell::default(); MCS_PER_FB];
    let ff_block_idx = fuse_base + size_of_zia + size_of_and + size_of_or;
    let mut cur_ff_idx = ff_block_idx;
    for i in 0..ff_bits.len() {
        if fb_ff_num_to_iob_num(device, fb, i as u32).is_none() {
            // Buried (must be large)
            ff_bits[i] = read_large_buried_ff_logical(fuses, cur_ff_idx);
            cur_ff_idx += 16;
        } else {
            // Not buried
            if device_is_large {
                ff_bits[i] = read_large_ff_logical(fuses, cur_ff_idx);
                cur_ff_idx += 29;
            } else {
                ff_bits[i] = read_small_ff_logical(fuses, ff_block_idx, i);
                cur_ff_idx += 27;
            }
        }
    }

    Ok(XC2BitstreamFB {
        and_terms: and_terms,
        or_terms: or_terms,
        zia_bits: zia_bits,
        ffs: ff_bits,
    })
}

// TODO: This is the same across all sizes, right?

/// The index of the special CTC product term
pub const CTC: u32 = 4;

/// The index of the special CTR product term
pub const CTR: u32 = 5;

/// The index of the special CTS product term
pub const CTS: u32 = 6;

/// The index of the special CTE product term
pub const CTE: u32 = 7;

/// Returns the special PTA product term given a macrocell index
pub fn get_pta(mc: u32) -> u32 {
    3 * mc + 8
}

/// Returns the special PTB product term given a macrocell index
pub fn get_ptb(mc: u32) -> u32 {
    3 * mc + 9
}

/// Returns the special PTC product term given a macrocell index
pub fn get_ptc(mc: u32) -> u32 {
    3 * mc + 10
}
