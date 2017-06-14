/*
Copyright (c) 2017, Robert Ou <rqou@robertou.com> and contributors
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

//! Contains routines for dealing with xc2bit's "native" crbit format. TODO: Document this format.

use std::io;
use std::io::Write;
use std::str;

/// Struct representing a 2-dimensional fuse array and handles converting xy-coordinates into a single linear index.
/// The x-axis is horizontal and the y-axis is vertical. The origin is at the top-left corner. (This is the standard
/// "computer graphics" coordinate scheme.)
pub struct FuseArray {
    /// Internal 1-dimensional storage
    v: Vec<bool>,
    /// Width of the array
    w: usize,
    /// Possibly contains a device name
    pub dev_name_str: Option<String>,
}

impl FuseArray {
    /// Get a fuse value at the particular xy coordinate
    pub fn get(&self, x: usize, y: usize) -> bool {
        self.v[y * self.w + x]
    }

    /// Set the fuse value at the particular xy coordinate
    pub fn set(&mut self, x: usize, y: usize, val: bool) {
        self.v[y * self.w + x] = val;
    }

    /// Returns the dimensions of this array as (width, height)
    pub fn dim(&self) -> (usize, usize) {
        (self.w, self.v.len() / self.w)
    }

    /// Processes the given data and converts it into a `FuseArray` struct.
    pub fn from_file_contents(in_bytes: &[u8]) -> Result<FuseArray, &'static str> {
        // let w = None;

        let in_str = str::from_utf8(in_bytes);
        if in_str.is_err() {
            return Err("invalid characters in crbit");
        }

        for l in in_str.unwrap().split('\n') {
            let l = l.trim_matches(|c| c == ' ' || c == '\r' || c == '\n');
            if l.len() == 0 {
                // ignore empty fields
                continue;
            }

            println!("{}", l);
        }

        unimplemented!();
    }

    pub fn from_dim(w: usize, h: usize) -> FuseArray {
        FuseArray {
            w,
            v: vec![false; w*h],
            dev_name_str: None,
        }
    }

    pub fn write_to_writer(&self, writer: &mut Write) -> Result<(), io::Error> {
        write!(writer, "// crbit native bitstream file written by xc2bit\n")?;
        write!(writer, "// https://github.com/azonenberg/openfpga\n\n")?;

        if let Some(ref dev_name) = self.dev_name_str {
            write!(writer, "// DEVICE {}\n\n", dev_name)?;
        }

        let (w, h) = self.dim();
        for y in 0..h {
            for x in 0..w {
                write!(writer, "{}", if self.get(x, y) {"1"} else {"0"})?;
            }
            write!(writer, "\n")?;
        }
        write!(writer, "\n")?;

        Ok(())
    }
}
