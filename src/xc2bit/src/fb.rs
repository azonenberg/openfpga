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
use pla::{read_and_term_logical, read_or_term_logical};
use mc::{read_small_ff_logical, read_large_ff_logical, read_large_buried_ff_logical};
use zia::{encode_32_zia_choice, encode_64_zia_choice, encode_128_zia_choice,
          read_32_zia_fb_row_logical, read_64_zia_fb_row_logical, read_128_zia_fb_row_logical, zia_get_row_width};

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
                        write!(writer, "{}", if zia_choice_bits[zia_choice_bits.len() - 1 - j] {"1"} else {"0"})?;
                    }
                },
                XC2Device::XC2C64 | XC2Device::XC2C64A => {
                    let zia_choice_bits = encode_64_zia_choice(i as u32, self.zia_bits[i].selected)
                        // FIXME: Fold this into the error system??
                        .expect("invalid ZIA input");
                    for j in 0..zia_choice_bits.len() {
                        write!(writer, "{}", if zia_choice_bits[zia_choice_bits.len() - 1 - j] {"1"} else {"0"})?;
                    }
                },
                XC2Device::XC2C128 => {
                    let zia_choice_bits = encode_128_zia_choice(i as u32, self.zia_bits[i].selected)
                        // FIXME: Fold this into the error system??
                        .expect("invalid ZIA input");
                    for j in 0..zia_choice_bits.len() {
                        write!(writer, "{}", if zia_choice_bits[zia_choice_bits.len() - 1 - j] {"1"} else {"0"})?;
                    }
                },
                _ => unreachable!(),
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
        _ => unreachable!(),
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
        let result = zia_row_read_function(fuses, zia_block_idx, i);
        if let Err(err) = result {
            return Err(err);
        }
        zia_bits[i] = result.unwrap();
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
