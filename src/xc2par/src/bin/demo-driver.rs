
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

extern crate xc2bit;
use xc2bit::*;

extern crate xc2par;
use xc2par::*;

extern crate yosys_netlist_json;

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
    let yosys_netlist = yosys_netlist_json::Netlist::from_slice(&data).unwrap();
    println!("{:?}", yosys_netlist);

    // Netlist graph (native part)
    let ngraph_rs = IntermediateGraph::from_yosys_netlist(&yosys_netlist).unwrap();
    // ngraph_rs.insert_into_par_graph(&mut par_graphs, &lmap);
    println!("{:?}", ngraph_rs);

    // New data structure
    let input_graph = InputGraph::from_intermed_graph(&ngraph_rs).unwrap();
    println!("{:?}", input_graph);

    // TODO
    let (device_type, _, _) = parse_part_name_string("xc2c32a-4-vq44").expect("invalid device name");

    let par_result = do_par(&input_graph);
    if let PARResult::Success(x) = par_result {
        // Get a bitstream result
        //let bitstream = produce_bitstream(device_type, &ngraph_rs, &mcs, &x);
        println!("********************************************************************************");
        //bitstream.to_jed(&mut ::std::io::stdout()).unwrap();
    } else {
        panic!("par failed!")
    }
}
