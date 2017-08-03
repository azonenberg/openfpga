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

//! Testing tool that prints out the internal CPLD structure

extern crate xc2bit;
use xc2bit::*;

fn main() {
    let args = ::std::env::args().collect::<Vec<_>>();

    if args.len() != 2 {
        println!("Usage: {} <device>-<speed>-<package>", args[0]);
        ::std::process::exit(1);
    }

    let device_combination = &args[1];
    let (device, _, _) = parse_part_name_string(device_combination).expect("invalid device name");
    get_device_structure(device,
        |node_name: &str, node_type: &str, fb: u32, idx: u32| {
            println!("Node: {} {} {} {}", node_name, node_type, fb, idx);
        },
        |wire_name: &str| {
            println!("Wire: {}", wire_name);
        },
        |node_name: &str, node_type: &str, fb: u32, idx: u32, wire_name: &str, port_name: &str, port_idx: u32| {
            println!("Node connection: {} {} {} {} {} {} {}", node_name, node_type, fb, idx,
                wire_name, port_name, port_idx);
        });
}
