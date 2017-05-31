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

// Xilinx JED file I/O

use std::num::Wrapping;
use std::str;

use *;
use bitstream::{read_32_bitstream_logical};

#[derive(Eq, PartialEq, Copy, Clone)]
enum Ternary {
    Zero,
    One,
    Undef,
}

const STX: u8 = 0x02;
const ETX: u8 = 0x03;

// Reads .jed file and outputs the fuses as an array of booleans and optional device name
pub fn read_jed(in_bytes: &[u8]) -> Result<(Vec<bool>, Option<String>), &'static str> {
    let mut fuse_csum = Wrapping(0u16);
    let mut fuse_expected_csum = None;
    let mut file_csum = Wrapping(0u16);
    let mut num_fuses: u32 = 0;
    let mut device = None;
    let mut jed_stx: usize = 0;
    let mut jed_etx: usize;
    let mut fuses_ternary = vec![];
    let mut default_fuse = Ternary::Undef;

    // Find STX
    while in_bytes[jed_stx] != STX {
        jed_stx += 1;
        if jed_stx >= in_bytes.len() {
            return Err("STX not found");
        }
    }

    // Checksum and find ETX
    jed_etx = jed_stx;
    while in_bytes[jed_etx] != ETX {
        file_csum += Wrapping(in_bytes[jed_etx] as u16);
        jed_etx += 1;
        if jed_etx >= in_bytes.len() {
            return Err("ETX not found");
        }
    }
    // Add the ETX to the checksum too
    file_csum += Wrapping(ETX as u16);

    // Check the checksum
    if jed_etx + 4 >= in_bytes.len() {
        return Err("unexpected end of file - checksum");
    }
    let csum_expected = &in_bytes[jed_etx + 1..jed_etx + 5];
    let csum_expected = str::from_utf8(csum_expected);
    if csum_expected.is_err() {
        return Err("invalid character encountered - file checksum");
    }
    let csum_expected = u16::from_str_radix(csum_expected.unwrap(), 16);
    if csum_expected.is_err() {
        return Err("invalid character encountered - file checksum");
    }
    let csum_expected = csum_expected.unwrap();
    if csum_expected != 0 && csum_expected != file_csum.0 {
        return Err("invalid file checksum");
    }

    // Make a str object out of the body
    let jed_body = str::from_utf8(&in_bytes[jed_stx + 1..jed_etx]);
    if jed_body.is_err() {
        return Err("invalid character encountered - non-ASCII");
    }
    let jed_body = jed_body.unwrap();

    // Ready to parse each line
    for l in jed_body.split('*') {
        let l = l.trim_matches(|c| c == ' ' || c == '\r' || c == '\n');
        if l.len() == 0 {
            // FIXME: Should we do something else here?
            // ignore empty fields
            continue;
        }

        // Now we can look at the first byte to figure out what we have
        match l.chars().next().unwrap() {
            'X' | 'J' => {}, // Do nothing, don't care about these
            'F' => {
                // Default state
                let (_, default_state_str) = l.split_at(1);
                default_fuse = match default_state_str {
                    "0" => Ternary::Zero,
                    "1" => Ternary::One,
                    _ => return Err("invalid character encountered - F field")
                }
            },
            'N' => {
                // Notes; we want to extract N DEVICE but otherwise ignore it
                let note_pieces = l.split(|c| c == ' ' || c == '\r' || c == '\n').collect::<Vec<_>>();
                if note_pieces.len() == 3 && note_pieces[1] == "DEVICE" {
                    device = Some(note_pieces[2].to_owned());
                }
            },
            'Q' => {
                // Look for QF
                if l.starts_with("QF") {
                    let (_, num_fuses_str) = l.split_at(2);
                    let num_fuses_maybe = u32::from_str_radix(num_fuses_str, 10);
                    if num_fuses_maybe.is_err() {
                        return Err("invalid character encountered - QF field");
                    }
                    num_fuses = num_fuses_maybe.unwrap();
                    fuses_ternary.reserve(num_fuses as usize);
                    for _ in 0..num_fuses {
                        fuses_ternary.push(Ternary::Undef);
                    }
                }
            },
            'L' => {
                // A set of fuses
                if num_fuses == 0 {
                    return Err("missing QF field");
                }

                let mut fuse_field_splitter = l.split(|c| c == ' ' || c == '\r' || c == '\n');
                let fuse_idx_str = fuse_field_splitter.next();
                let (_, fuse_idx_str) = fuse_idx_str.unwrap().split_at(1);
                let fuse_idx_maybe = u32::from_str_radix(fuse_idx_str, 10);
                if fuse_idx_maybe.is_err() {
                    return Err("invalid character encountered - fuse number");
                }
                let mut fuse_idx = fuse_idx_maybe.unwrap();

                let fuse_bits_part = fuse_field_splitter.next();
                if fuse_bits_part.is_none() {
                    return Err("malformed L field");
                }
                let fuse_bits_part = fuse_bits_part.unwrap();
                for fuse in fuse_bits_part.chars() {
                    match fuse {
                        '0' => {
                            if fuse_idx >= num_fuses {
                                return Err("invalid fuse index out of range");
                            }
                            fuses_ternary[fuse_idx as usize] = Ternary::Zero;
                            fuse_idx += 1;
                        },
                        '1' => {
                            if fuse_idx >= num_fuses {
                                return Err("invalid fuse index out of range");
                            }
                            fuses_ternary[fuse_idx as usize] = Ternary::One;
                            fuse_idx += 1;
                        },
                        ' ' | '\r' | '\n' => {}, // Do nothing
                        _ => return Err("invalid character encountered - fuse value"),
                    }
                }
            },
            'C' => {
                // Checksum
                let (_, csum_str) = l.split_at(1);
                if csum_str.len() != 4 {
                    return Err("malformed fuse checksum");
                }
                let csum_maybe = u16::from_str_radix(csum_str, 16);
                if csum_maybe.is_err() {
                    return Err("invalid character encountered - C");
                }
                fuse_expected_csum = Some(csum_maybe.unwrap());
            }
            _ => return Err("unrecognized field"),
        }
    }

    if default_fuse == Ternary::Undef {
        return Err("missing F field")
    }

    // Fill in the default values
    for x in &mut fuses_ternary {
        if *x == Ternary::Undef {
            *x = default_fuse;
        }
    }

    // Un-ternary it
    let fuses = fuses_ternary.iter().map(|&x| match x {
        Ternary::Zero => false,
        Ternary::One => true,
        _ => unreachable!(),
    }).collect::<Vec<_>>();

    // Fuse checksum
    if let Some(fuse_expected_csum) = fuse_expected_csum {
        for i in 0..num_fuses {
            if fuses[i as usize] {
                // Fuse is a 1 and contributes to the sum
                fuse_csum += Wrapping(1u16 << (i % 8));
            }
        }

        if fuse_expected_csum != fuse_csum.0 {
            return Err("invalid fuse checksum");
        }
    }

    Ok((fuses, device))
}

// Processes a fuse array into a bitstream object
pub fn process_jed(fuses: &[bool], device: &str) -> Result<XC2Bitstream, &'static str> {
    let device_split = device.split('-').collect::<Vec<_>>();

    if device_split.len() != 3 {
        return Err("malformed device name");
    }

    // TODO: Validate these
    let device_speed = device_split[1];
    let device_package = device_split[2];

    // Part name
    match device_split[0] {
        "XC2C32A" => {
            if fuses.len() != 12278 {
                return Err("wrong number of fuses");
            }
            let bits = read_32_bitstream_logical(fuses);
            if let Err(err) = bits {
                return Err(err);
            }
            Ok(XC2Bitstream {
                speed_grade: device_speed.to_owned(),
                package: device_package.to_owned(),
                bits: bits.unwrap(),
            })
        },
        _ => Err("unsupported part"),
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn read_no_stx() {
        let ret = read_jed(b"asdf");

        assert_eq!(ret, Err("STX not found"));
    }

    #[test]
    fn read_no_etx() {
        let ret = read_jed(b"asdf\x02fdsa");

        assert_eq!(ret, Err("ETX not found"));
    }

    #[test]
    fn read_no_csum() {
        let ret = read_jed(b"asdf\x02fdsa\x03");
        assert_eq!(ret, Err("unexpected end of file - checksum"));

        let ret = read_jed(b"asdf\x02fdsa\x03AAA");
        assert_eq!(ret, Err("unexpected end of file - checksum"));
    }

    #[test]
    fn read_bad_csum() {
        let ret = read_jed(b"asdf\x02fdsa\x03AAAA");

        assert_eq!(ret, Err("invalid file checksum"));
    }

    #[test]
    fn read_malformed_csum() {
        let ret = read_jed(b"asdf\x02fdsa\x03AAAZ");

        assert_eq!(ret, Err("invalid character encountered - file checksum"));
    }

    #[test]
    fn read_no_qf_no_f() {
        let ret = read_jed(b"\x02\x030000");

        assert_eq!(ret, Err("missing F field"));
    }

    #[test]
    fn read_empty_no_fuses() {
        let ret = read_jed(b"\x02F0*\x030000");

        assert_eq!(ret, Ok((vec![], None)));
    }

    #[test]
    fn read_bogus_f_command() {
        let ret = read_jed(b"\x02F2*\x030000");

        assert_eq!(ret, Err("invalid character encountered - F field"));
    }

    #[test]
    fn read_empty_with_device() {
        let ret = read_jed(b"\x02F0*N DEVICE asdf*\x030000");

        assert_eq!(ret, Ok((vec![], Some(String::from("asdf")))));
    }

    #[test]
    fn read_l_without_qf() {
        let ret = read_jed(b"\x02F0*L0 0*\x030000");

        assert_eq!(ret, Err("missing QF field"));
    }

    #[test]
    fn read_one_fuse() {
        let ret = read_jed(b"\x02F0*QF1*L0 1*\x030000");

        assert_eq!(ret, Ok((vec![true], None)));
    }

    #[test]
    fn read_one_fuse_csum_good() {
        let ret = read_jed(b"\x02F0*QF1*L0 1*C0001*\x030000");

        assert_eq!(ret, Ok((vec![true], None)));
    }

    #[test]
    fn read_one_fuse_csum_bad() {
        let ret = read_jed(b"\x02F0*QF1*L0 1*C0002*\x030000");

        assert_eq!(ret, Err("invalid fuse checksum"));
    }
}
