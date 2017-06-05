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

use std::fs::File;
use std::io::Read;

extern crate xbpar_rs;
use xbpar_rs::*;

extern crate xc2par;
use xc2par::*;

fn main() {
    let args = ::std::env::args().collect::<Vec<_>>();

    if args.len() != 2 {
        println!("Usage: {} file.json", args[0]);
        ::std::process::exit(1);
    }

    // Read the entire input file
    let mut f = File::open(&args[1]).expect("failed to open file");
    let mut data = Vec::new();
    f.read_to_end(&mut data).expect("failed to read data");

    // de-serialize the yosys netlist
    let yosys_netlist = read_yosys_netlist(&data).unwrap();
    println!("{:?}", yosys_netlist);

    // The graphs for the PAR engine
    let mut par_graphs = PARGraphPair::<_, ()>::new_pair();

    let (dgraph_rs, lmap) = DeviceGraph::new("xc2c32a", &mut par_graphs);
    println!("{:?}", lmap);
    println!("{:?}", dgraph_rs);
}
