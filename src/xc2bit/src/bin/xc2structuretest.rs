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

use std::cell::RefCell;

fn main() {
    let args = ::std::env::args().collect::<Vec<_>>();

    if args.len() != 2 {
        println!("Usage: {} <device>-<speed>-<package>", args[0]);
        ::std::process::exit(1);
    }

    let device_combination = &args[1];
    let XC2DeviceSpeedPackage {
        dev: device, spd: _, pkg: _
    } = XC2DeviceSpeedPackage::from_str(device_combination).expect("invalid device name");

    let node_vec = RefCell::new(Vec::new());
    let wire_vec = RefCell::new(Vec::new());

    get_device_structure(device,
        |node_name: &str, node_type: &str, fb: u32, idx: u32| {
            let mut node_vec = node_vec.borrow_mut();

            println!("Node: {} {} {} {}", node_name, node_type, fb, idx);
            let i = node_vec.len();
            node_vec.push((node_name.to_owned(), node_type.to_owned(), fb, idx));
            i
        },
        |wire_name: &str| {
            let mut wire_vec = wire_vec.borrow_mut();

            println!("Wire: {}", wire_name);
            let i = wire_vec.len();
            wire_vec.push(wire_name.to_owned());
            i + 1000000
        },
        |node_ref: usize, wire_ref: usize, port_name: &str, port_idx: u32, extra_data: (u32, u32)| {
            if node_ref >= 1000000 {
                panic!("wire instead of node");
            }

            if wire_ref < 1000000 {
                panic!("node instead of wire");
            }
            let wire_ref = wire_ref - 1000000;

            let node_vec = node_vec.borrow();
            let wire_vec = wire_vec.borrow();

            println!("Node connection: {} {} {} {} {} {} {} {} {}",
                node_vec[node_ref].0, node_vec[node_ref].1, node_vec[node_ref].2, node_vec[node_ref].3,
                wire_vec[wire_ref], port_name, port_idx, extra_data.0, extra_data.1);
        });
}
