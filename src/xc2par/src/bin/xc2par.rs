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

use clap::{App, Arg};
#[macro_use]
extern crate slog;
use slog::Drain;
use xc2bit::*;
use xc2par::*;

fn main() -> Result<(), Box<dyn std::error::Error>> {
    let matches = App::new("xc2par")
        .author("Robert Ou <rqou@robertou.com>")
        .about("Unofficial place-and-route tool for Xilinx Coolrunner-II CPLDs")

        .arg(Arg::with_name("crbit")
            .help("Output in .crbit format")
            .long("crbit")
            .overrides_with("jed"))
        .arg(Arg::with_name("jed")
            .help("Output in .jed format (default)")
            .long("jed")
            .overrides_with("crbit"))

        .arg(Arg::with_name("max-iter")
            .help("Maximum iteration count")
            .long("max-iter")
            .takes_value(true))
        .arg(Arg::with_name("rng-seed")
            .help("Seed for internal random number generator (128-bit hex)")
            .long("rng-seed")
            .takes_value(true))

        .arg(Arg::with_name("part-name")
            .help("Part name (<device>-<speed>-<package>)")
            .short("p")
            .long("part")
            .takes_value(true)
            .required(true))

        .arg(Arg::with_name("verbose")
            .help("Increase logging verbosity")
            .short("v")
            .multiple(true))

        .arg(Arg::with_name("INPUT")
            .help("Input file name (Yosys JSON file)")
            .required(true)
            .index(1))
        .arg(Arg::with_name("OUTPUT")
            .help("Output file name")
            .required(false)
            .index(2))

        .get_matches();

    // Logging
    let loglevel = matches.occurrences_of("verbose");
    let decorator = slog_term::TermDecorator::new().build();
    let drain = slog_term::FullFormat::new(decorator).build().fuse();
    let log = if loglevel == 0 {
        let drain = drain.filter(|record| {record.level().is_at_least(slog::Level::Warning)});
        let drain = std::sync::Mutex::new(drain).fuse();
        slog::Logger::root(drain, o!())
    } else if loglevel == 1 {
        let drain = drain.filter(|record| record.level().is_at_least(slog::Level::Info));
        let drain = std::sync::Mutex::new(drain).fuse();
        slog::Logger::root(drain, o!())
    } else {
        let drain = std::sync::Mutex::new(drain).fuse();
        slog::Logger::root(drain, o!())
    };

    // Handling of options
    let mut options = XC2ParOptions::new();

    let is_crbit = matches.is_present("crbit");
    if is_crbit {
        options.output_format(ParOutputFormat::Crbit);
    } else {
        options.output_format(ParOutputFormat::Jed);
    }

    if let Some(iter_count_str) = matches.value_of_os("max-iter") {
        if let Some(iter_count_str) = iter_count_str.to_str() {
            if let Ok(iter_count) = iter_count_str.parse::<u32>() {
                options.max_iter(iter_count);
            } else {
                warn!(log, "Illegal value for max-iter"; "value" => iter_count_str);
            }
        } else {
            warn!(log, "Illegal value for max-iter"; "value" => iter_count_str.to_string_lossy().into_owned());
        }
    }

    if let Some(rng_seed_str) = matches.value_of_os("rng-seed") {
        if let Some(rng_seed_str) = rng_seed_str.to_str() {
            let mut rng_seed = [0u32; 4];
            let mut is_valid = true;
            let mut printed_trunc_warning = false;
            for (i, c) in rng_seed_str.chars().enumerate() {
                let shift = (rng_seed_str.len() - 1 - i) * 4;
                let nybble = match c {
                    '0'..='9' => {
                        c as u32 - '0' as u32
                    },
                    'a'..='f' => {
                        c as u32 - 'a' as u32 + 10
                    },
                    'A'..='F' => {
                        c as u32 - 'A' as u32 + 10
                    },
                    _ => {
                        warn!(log, "Illegal value for rng-seed"; "value" => rng_seed_str);
                        is_valid = false;
                        break;
                    }
                };

                let idx = 3 - (shift / 32);
                let shift = shift % 32;
                if idx >= 4 {
                    if !printed_trunc_warning {
                        warn!(log, "RNG seed is too long, truncating");
                    }
                    printed_trunc_warning = true;
                    continue;
                }
                rng_seed[idx] |= nybble << shift;
            }

            if is_valid {
                if rng_seed == [0, 0, 0, 0] {
                    warn!(log, "RNG seed cannot be all zeros");
                } else {
                    options.with_prng_seed(rng_seed);
                }
            }
        } else {
            warn!(log, "Illegal value for rng-seed"; "value" => rng_seed_str.to_string_lossy().into_owned());
        }
    }

    // Filenames
    let in_fn = Path::new(matches.value_of_os("INPUT").unwrap());
    let out_fn = if let Some(out_fn_str) = matches.value_of_os("OUTPUT") {
        Path::new(out_fn_str).to_owned()
    } else {
        let mut out_fn = in_fn.to_owned();
        if is_crbit {
            out_fn.set_extension("crbit");
        } else {
            out_fn.set_extension("jed");
        }
        out_fn
    };

    // Actual work
    let in_f = File::open(in_fn)?;
    let out_f  = File::create(out_fn)?;
    let part_name_str = matches.value_of_lossy("part-name").unwrap();
    let device_type = if let Some(x) = XC2DeviceSpeedPackage::from_str(&part_name_str) { x } else {
        error!(log, "Invalid part name"; "name" => part_name_str.into_owned());
        return Err(From::from("invalid part name".to_owned()));
    };
    xc2par_complete_flow(&options, device_type, in_f, out_f, log)?;

    Ok(())
}
