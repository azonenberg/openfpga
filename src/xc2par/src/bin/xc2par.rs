/*
Copyright (c) 2018, Robert Ou <rqou@robertou.com> and contributors
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

use std::fs::File;
use std::path::{Path};

extern crate clap;
use clap::{App, Arg};

extern crate slog_term;
#[macro_use]
extern crate slog;
use slog::Drain;

extern crate xc2bit;
use xc2bit::*;

extern crate xc2par;
use xc2par::*;

fn main() -> Result<(), Box<std::error::Error>> {
    let matches = App::new("xc2par")
        .author("Robert Ou <rqou@robertou.com>")
        .about("Unofficial place-and-route tool for Xilinx Coolrunner-II CPLDs")
        .arg(Arg::with_name("INPUT")
            .help("Input file name (Yosys JSON file)")
            .required(true)
            .index(1))
        .arg(Arg::with_name("OUTPUT")
            .help("Output file name")
            .required(false)
            .index(2))
        .get_matches();

    let in_fn = Path::new(matches.value_of_os("INPUT").unwrap());

    let out_fn = if let Some(out_fn_str) = matches.value_of_os("OUTPUT") {
        Path::new(out_fn_str).to_owned()
    } else {
        let mut out_fn = in_fn.to_owned();
        out_fn.set_extension("jed");
        out_fn
    };

    let decorator = slog_term::TermDecorator::new().build();
    let drain = slog_term::FullFormat::new(decorator).build().fuse();
    let drain = std::sync::Mutex::new(drain).fuse();
    let log = slog::Logger::root(drain, o!());

    let in_f = File::open(in_fn)?;
    let out_f  = File::create(out_fn)?;

    let options = XC2ParOptions::new();
    // TODO
    let device_type = XC2DeviceSpeedPackage::from_str("xc2c32a-4-vq44").expect("invalid device name");
    xc2par_complete_flow(&options, device_type, in_f, out_f, log)?;

    Ok(())
}
