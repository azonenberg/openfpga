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

extern crate jedec;
use self::jedec::*;

use *;
use fusemap_physical::{zia_block_loc, and_block_loc, or_block_loc};
use util::{LinebreakSet};
use zia::{zia_get_row_width};

/// Represents a collection of all the parts that make up one function block
#[derive(Copy, Clone, Serialize)]
pub struct XC2BitstreamFB {
    /// The AND terms of the PLA part of the function block
    #[serde(serialize_with = "<[_]>::serialize")]
    pub and_terms: [XC2PLAAndTerm; ANDTERMS_PER_FB],
    /// The OR terms of the PLA part of the function block
    pub or_terms: [XC2PLAOrTerm; MCS_PER_FB],
    /// The inputs to the function block from the ZIA
    #[serde(serialize_with = "<[_]>::serialize")]
    pub zia_bits: [XC2ZIAInput; INPUTS_PER_ANDTERM],
    /// The macrocells of the function block
    pub mcs: [XC2Macrocell; MCS_PER_FB],
}

impl Default for XC2BitstreamFB {
    fn default() -> Self {
        XC2BitstreamFB {
            and_terms: [XC2PLAAndTerm::default(); ANDTERMS_PER_FB],
            or_terms: [XC2PLAOrTerm::default(); MCS_PER_FB],
            zia_bits: [XC2ZIAInput::default(); INPUTS_PER_ANDTERM],
            mcs: [XC2Macrocell::default(); MCS_PER_FB],
        }
    }
}

/// Internal helper that writes a ZIA row to the fuse array
fn zia_row_crbit_write_helper(x: usize, y: usize, zia_row: usize, zia_bits: &[bool], has_gap: bool,
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

/// Internal helper that reads a ZIA row from the fuse array
fn zia_row_crbit_read_helper(x: usize, y: usize, zia_row: usize, zia_bits: &mut [bool], has_gap: bool,
    fuse_array: &FuseArray) {

    let l = zia_bits.len();

    for zia_bit in 0..l {
        let mut out_y = y + zia_row;
        if has_gap && zia_row >= 20 {
            // There is an OR array in the middle, 8 rows high
            out_y += 8;
        }

        let out_x = x + zia_bit * 2;

        zia_bits[l - 1 - zia_bit] = fuse_array.get(out_x, out_y);
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
    pub fn dump_human_readable<W: Write>(&self, device: XC2Device, fb: u32, mut writer: W) -> Result<(), io::Error> {
        for i in 0..MCS_PER_FB {
            self.mcs[i].dump_human_readable(fb, i as u32, &mut writer)?;
        }

        write!(writer, "\n")?;
        write!(writer, "ZIA inputs for FB{}\n", fb + 1)?;
        for i in 0..INPUTS_PER_ANDTERM {
            write!(writer, "{:2}: ", i)?;
            match self.zia_bits[i] {
                XC2ZIAInput::Zero => write!(writer, "0\n")?,
                XC2ZIAInput::One => write!(writer, "1\n")?,
                XC2ZIAInput::Macrocell{fb, mc} =>
                    write!(writer, "FB{}_{} FF\n", fb + 1, mc + 1)?,
                XC2ZIAInput::IBuf{ibuf} => {
                    let (fb, mc) = iob_num_to_fb_mc_num(device, ibuf as u32).unwrap();
                    write!(writer, "FB{}_{} pad\n", fb + 1, mc + 1)?;
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
                if self.and_terms[i].get(j) {
                    write!(writer, "|XXX")?;
                } else {
                    write!(writer, "|   ")?;
                }

                if self.and_terms[i].get_b(j) {
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
                if self.or_terms[i].get(j) {
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
        // FFs
        for i in 0..MCS_PER_FB {
            self.mcs[i].to_crbit(device, fb, i as u32, fuse_array);
        }

        // ZIA
        let (x, y) = zia_block_loc(device, fb);
        for zia_row in 0..INPUTS_PER_ANDTERM {
            match device {
                XC2Device::XC2C32 | XC2Device::XC2C32A => {
                    let zia_choice_bits = XC2ZIAInput::encode_32_zia_choice(zia_row as u32, self.zia_bits[zia_row])
                        // FIXME: Fold this into the error system??
                        .expect("invalid ZIA input");

                    zia_row_crbit_write_helper(x, y, zia_row, &zia_choice_bits, true, fuse_array);
                },
                XC2Device::XC2C64 | XC2Device::XC2C64A => {
                    let zia_choice_bits = XC2ZIAInput::encode_64_zia_choice(zia_row as u32, self.zia_bits[zia_row])
                        // FIXME: Fold this into the error system??
                        .expect("invalid ZIA input");

                    zia_row_crbit_write_helper(x, y, zia_row, &zia_choice_bits, true, fuse_array);
                },
                XC2Device::XC2C128 => {
                    let zia_choice_bits = XC2ZIAInput::encode_128_zia_choice(zia_row as u32, self.zia_bits[zia_row])
                        // FIXME: Fold this into the error system??
                        .expect("invalid ZIA input");

                    zia_row_crbit_write_helper(x, y, zia_row, &zia_choice_bits, false, fuse_array);
                },
                XC2Device::XC2C256 => {
                    let zia_choice_bits = XC2ZIAInput::encode_256_zia_choice(zia_row as u32, self.zia_bits[zia_row])
                        // FIXME: Fold this into the error system??
                        .expect("invalid ZIA input");

                    zia_row_crbit_write_helper(x, y, zia_row, &zia_choice_bits, true, fuse_array);
                },
                XC2Device::XC2C384 => {
                    let zia_choice_bits = XC2ZIAInput::encode_384_zia_choice(zia_row as u32, self.zia_bits[zia_row])
                        // FIXME: Fold this into the error system??
                        .expect("invalid ZIA input");

                    zia_row_crbit_write_helper(x, y, zia_row, &zia_choice_bits, false, fuse_array);
                },
                XC2Device::XC2C512 => {
                    let zia_choice_bits = XC2ZIAInput::encode_512_zia_choice(zia_row as u32, self.zia_bits[zia_row])
                        // FIXME: Fold this into the error system??
                        .expect("invalid ZIA input");

                    zia_row_crbit_write_helper(x, y, zia_row, &zia_choice_bits, false, fuse_array);
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
                            fuse_array.set(x + term_idx * 2 + 1, out_y, !self.and_terms[term_idx].get(input_idx));
                            // complement input
                            fuse_array.set(x + term_idx * 2 + 0, out_y, !self.and_terms[term_idx].get_b(input_idx));
                        } else {
                            // true input
                            fuse_array.set(x - term_idx * 2 - 1, out_y, !self.and_terms[term_idx].get(input_idx));
                            // complement input
                            fuse_array.set(x - term_idx * 2 - 0, out_y, !self.and_terms[term_idx].get_b(input_idx));
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
                                !self.and_terms[phys_term_idx].get(input_idx));
                            // complement input
                            fuse_array.set(x + term_idx * 2 + 0, y + input_idx,
                                !self.and_terms[phys_term_idx].get_b(input_idx));
                        } else {
                            // true input
                            fuse_array.set(x - term_idx * 2 - 1, y + input_idx,
                                !self.and_terms[phys_term_idx].get(input_idx));
                            // complement input
                            fuse_array.set(x - term_idx * 2 - 0, y + input_idx,
                                !self.and_terms[phys_term_idx].get_b(input_idx));
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

                        fuse_array.set(out_x, out_y, !self.or_terms[or_term_idx].get(and_term_idx));
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

                        fuse_array.set(out_x, out_y, !self.or_terms[or_term_idx].get(and_term_idx));
                    }
                }
            },
        }
    }

    /// Reads the crbit representation of the settings for this FB from the given `fuse_array`.
    /// `device` must be the device type this FB was extracted from.
    /// `fb` must be the index of this function block.
    pub fn from_crbit(device: XC2Device, fb: u32, fuse_array: &FuseArray) -> Result<Self, XC2BitError> {
        // ZIA
        let mut zia_bits = [XC2ZIAInput::default(); INPUTS_PER_ANDTERM];
        let (x, y) = zia_block_loc(device, fb);
        for zia_row in 0..INPUTS_PER_ANDTERM {
            zia_bits[zia_row] = match device {
                XC2Device::XC2C32 | XC2Device::XC2C32A => {
                    let mut zia_bits = [false; 8];
                    zia_row_crbit_read_helper(x, y, zia_row, &mut zia_bits, true, fuse_array);
                    XC2ZIAInput::decode_32_zia_choice(zia_row, &zia_bits)?
                },
                XC2Device::XC2C64 | XC2Device::XC2C64A => {
                    let mut zia_bits = [false; 16];
                    zia_row_crbit_read_helper(x, y, zia_row, &mut zia_bits, true, fuse_array);
                    XC2ZIAInput::decode_64_zia_choice(zia_row, &zia_bits)?
                },
                XC2Device::XC2C128 => {
                    let mut zia_bits = [false; 28];
                    zia_row_crbit_read_helper(x, y, zia_row, &mut zia_bits, false, fuse_array);
                    XC2ZIAInput::decode_128_zia_choice(zia_row, &zia_bits)?
                },
                XC2Device::XC2C256 => {
                    let mut zia_bits = [false; 48];
                    zia_row_crbit_read_helper(x, y, zia_row, &mut zia_bits, true, fuse_array);
                    XC2ZIAInput::decode_256_zia_choice(zia_row, &zia_bits)?
                },
                XC2Device::XC2C384 => {
                    let mut zia_bits = [false; 74];
                    zia_row_crbit_read_helper(x, y, zia_row, &mut zia_bits, false, fuse_array);
                    XC2ZIAInput::decode_384_zia_choice(zia_row, &zia_bits)?
                },
                XC2Device::XC2C512 => {
                    let mut zia_bits = [false; 88];
                    zia_row_crbit_read_helper(x, y, zia_row, &mut zia_bits, false, fuse_array);
                    XC2ZIAInput::decode_512_zia_choice(zia_row, &zia_bits)?
                },
            };
        }

        // AND block
        let mut and_terms = [XC2PLAAndTerm::default(); ANDTERMS_PER_FB];
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
                            and_terms[term_idx].set(input_idx, !fuse_array.get(x + term_idx * 2 + 1, out_y));
                            // complement input
                            and_terms[term_idx].set_b(input_idx, !fuse_array.get(x + term_idx * 2 + 0, out_y));
                        } else {
                            // true input
                            and_terms[term_idx].set(input_idx, !fuse_array.get(x - term_idx * 2 - 1, out_y));
                            // complement input
                            and_terms[term_idx].set_b(input_idx, !fuse_array.get(x - term_idx * 2 - 0, out_y));
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
                            and_terms[phys_term_idx].set(input_idx,
                                !fuse_array.get(x + term_idx * 2 + 1, y + input_idx));
                            // complement input
                            and_terms[phys_term_idx].set_b(input_idx,
                                !fuse_array.get(x + term_idx * 2 + 0, y + input_idx));
                        } else {
                            // true input
                            and_terms[phys_term_idx].set(input_idx,
                                !fuse_array.get(x - term_idx * 2 - 1, y + input_idx));
                            // complement input
                            and_terms[phys_term_idx].set_b(input_idx,
                                !fuse_array.get(x - term_idx * 2 - 0, y + input_idx));
                        }
                    }
                }
            },
        }

        // OR block
        let mut or_terms = [XC2PLAOrTerm::default(); MCS_PER_FB];
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

                        or_terms[or_term_idx].set(and_term_idx, !fuse_array.get(out_x, out_y));
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

                        or_terms[or_term_idx].set(and_term_idx, !fuse_array.get(out_x, out_y));
                    }
                }
            },
        }

        // FFs
        let mut mcs = [XC2Macrocell::default(); MCS_PER_FB];
        for i in 0..MCS_PER_FB {
            mcs[i] = XC2Macrocell::from_crbit(device, fb, i as u32, fuse_array);
        }

        Ok(XC2BitstreamFB {
            and_terms,
            or_terms,
            zia_bits,
            mcs,
        })
    }

    /// Write the .JED representation of the settings for this FB to the given `jed` object.
    /// `device` must be the device type this FB was extracted from and is needed to encode the ZIA.
    /// `fuse_base` must be the starting fuse number of this function block.
    pub fn to_jed(&self, device: XC2Device, fuse_base: usize, jed: &mut JEDECFile, linebreaks: &mut LinebreakSet) {
        // ZIA
        let zia_row_width = zia_get_row_width(device);

        if fuse_base != 0 {
            linebreaks.add(fuse_base);
        }
        for i in 0..INPUTS_PER_ANDTERM {
            let mut zia_fuse_base = fuse_base + i * zia_row_width;
            if zia_fuse_base != 0 {
                linebreaks.add(zia_fuse_base);
            }
            match device {
                XC2Device::XC2C32 | XC2Device::XC2C32A => {
                    let zia_choice_bits = XC2ZIAInput::encode_32_zia_choice(i as u32, self.zia_bits[i])
                        // FIXME: Fold this into the error system??
                        .expect("invalid ZIA input");
                    for j in 0..zia_choice_bits.len() {
                        jed.f[zia_fuse_base] = zia_choice_bits[j];
                        zia_fuse_base += 1;
                    }
                },
                XC2Device::XC2C64 | XC2Device::XC2C64A => {
                    let zia_choice_bits = XC2ZIAInput::encode_64_zia_choice(i as u32, self.zia_bits[i])
                        // FIXME: Fold this into the error system??
                        .expect("invalid ZIA input");
                    for j in 0..zia_choice_bits.len() {
                        jed.f[zia_fuse_base] = zia_choice_bits[j];
                        zia_fuse_base += 1;
                    }
                },
                XC2Device::XC2C128 => {
                    let zia_choice_bits = XC2ZIAInput::encode_128_zia_choice(i as u32, self.zia_bits[i])
                        // FIXME: Fold this into the error system??
                        .expect("invalid ZIA input");
                    for j in 0..zia_choice_bits.len() {
                        jed.f[zia_fuse_base] = zia_choice_bits[j];
                        zia_fuse_base += 1;
                    }
                },
                XC2Device::XC2C256 => {
                    let zia_choice_bits = XC2ZIAInput::encode_256_zia_choice(i as u32, self.zia_bits[i])
                        // FIXME: Fold this into the error system??
                        .expect("invalid ZIA input");
                    for j in 0..zia_choice_bits.len() {
                        jed.f[zia_fuse_base] = zia_choice_bits[j];
                        zia_fuse_base += 1;
                    }
                },
                XC2Device::XC2C384 => {
                    let zia_choice_bits = XC2ZIAInput::encode_384_zia_choice(i as u32, self.zia_bits[i])
                        // FIXME: Fold this into the error system??
                        .expect("invalid ZIA input");
                    for j in 0..zia_choice_bits.len() {
                        jed.f[zia_fuse_base] = zia_choice_bits[j];
                        zia_fuse_base += 1;
                    }
                },
                XC2Device::XC2C512 => {
                    let zia_choice_bits = XC2ZIAInput::encode_512_zia_choice(i as u32, self.zia_bits[i])
                        // FIXME: Fold this into the error system??
                        .expect("invalid ZIA input");
                    for j in 0..zia_choice_bits.len() {
                        jed.f[zia_fuse_base] = zia_choice_bits[j];
                        zia_fuse_base += 1;
                    }
                },
            }
        }

        // AND terms
        linebreaks.add(fuse_base + zia_row_width * INPUTS_PER_ANDTERM);
        for i in 0..ANDTERMS_PER_FB {
            let and_fuse_base = fuse_base + zia_row_width * INPUTS_PER_ANDTERM + i * INPUTS_PER_ANDTERM * 2;
            linebreaks.add(and_fuse_base);
            for j in 0..INPUTS_PER_ANDTERM {
                jed.f[and_fuse_base + j * 2 + 0] = !self.and_terms[i].get(j);
                jed.f[and_fuse_base + j * 2 + 1] = !self.and_terms[i].get_b(j);
            }
        }

        // OR terms
        linebreaks.add(fuse_base + zia_row_width * INPUTS_PER_ANDTERM + ANDTERMS_PER_FB * INPUTS_PER_ANDTERM * 2);
        for i in 0..ANDTERMS_PER_FB {
            let or_fuse_base = fuse_base + zia_row_width * INPUTS_PER_ANDTERM +
                ANDTERMS_PER_FB * INPUTS_PER_ANDTERM * 2 + i * MCS_PER_FB;
            linebreaks.add(or_fuse_base);
            for j in 0..MCS_PER_FB {
                jed.f[or_fuse_base + j] = !self.or_terms[j].get(i);
            }
        }
    }

    /// Internal function that reads a function block
    pub fn from_jed(device: XC2Device, fuses: &[bool], fb: u32, fuse_base: usize)
        -> Result<XC2BitstreamFB, XC2BitError> {

        let zia_row_width = zia_get_row_width(device);
        let size_of_zia = zia_row_width * INPUTS_PER_ANDTERM;
        let size_of_and = INPUTS_PER_ANDTERM * 2 * ANDTERMS_PER_FB;
        let size_of_or = ANDTERMS_PER_FB * MCS_PER_FB;

        let device_is_large =  match device {
            XC2Device::XC2C32 | XC2Device::XC2C32A | XC2Device::XC2C64 | XC2Device::XC2C64A => false,
            _ => true,
        };

        let zia_row_decode_function = match device {
            XC2Device::XC2C32 | XC2Device::XC2C32A => XC2ZIAInput::decode_32_zia_choice,
            XC2Device::XC2C64 | XC2Device::XC2C64A => XC2ZIAInput::decode_64_zia_choice,
            XC2Device::XC2C128 => XC2ZIAInput::decode_128_zia_choice,
            XC2Device::XC2C256 => XC2ZIAInput::decode_256_zia_choice,
            XC2Device::XC2C384 => XC2ZIAInput::decode_384_zia_choice,
            XC2Device::XC2C512 => XC2ZIAInput::decode_512_zia_choice,
        };

        let mut and_terms = [XC2PLAAndTerm::default(); ANDTERMS_PER_FB];
        let and_block_idx = fuse_base + size_of_zia;
        for i in 0..and_terms.len() {
            and_terms[i] = XC2PLAAndTerm::from_jed(fuses, and_block_idx, i);
        }

        let mut or_terms = [XC2PLAOrTerm::default(); MCS_PER_FB];
        let or_block_idx = fuse_base + size_of_zia + size_of_and;
        for i in 0..or_terms.len() {
            or_terms[i] = XC2PLAOrTerm::from_jed(fuses, or_block_idx, i);
        }

        let mut zia_bits = [XC2ZIAInput::default(); INPUTS_PER_ANDTERM];
        let zia_block_idx = fuse_base;
        for i in 0..zia_bits.len() {
            let zia_row_fuses = &fuses[zia_block_idx + i * zia_row_width..zia_block_idx + (i + 1) * zia_row_width];
            let result = zia_row_decode_function(i, zia_row_fuses)?;
            zia_bits[i] = result;
        }

        let mut mcs = [XC2Macrocell::default(); MCS_PER_FB];
        let mc_block_idx = fuse_base + size_of_zia + size_of_and + size_of_or;
        let mut cur_mc_idx = mc_block_idx;
        for i in 0..mcs.len() {
            if fb_mc_num_to_iob_num(device, fb, i as u32).is_none() {
                // Buried (must be large)
                mcs[i] = XC2Macrocell::from_jed_large_buried(fuses, cur_mc_idx);
                cur_mc_idx += 16;
            } else {
                // Not buried
                if device_is_large {
                    mcs[i] = XC2Macrocell::from_jed_large(fuses, cur_mc_idx);
                    cur_mc_idx += 29;
                } else {
                    mcs[i] = XC2Macrocell::from_jed_small(fuses, mc_block_idx, i);
                    cur_mc_idx += 27;
                }
            }
        }

        Ok(XC2BitstreamFB {
            and_terms,
            or_terms,
            zia_bits,
            mcs,
        })
    }
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
