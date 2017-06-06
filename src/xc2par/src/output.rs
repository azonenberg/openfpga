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

extern crate xbpar_rs;
use self::xbpar_rs::*;

extern crate xc2bit;
use self::xc2bit::*;

use *;
use objpool::*;

pub fn produce_bitstream(par_graphs: &PARGraphPair<ObjPoolIndex<DeviceGraphNode>, ObjPoolIndex<NetlistGraphNode>>,
    dgraph_rs: &DeviceGraph, ngraph_rs: &NetlistGraph) -> XC2Bitstream {

    // FIXME: Don't hardcode
    let mut bitstream = XC2Bitstream::blank_bitstream("XC2C32A", "6", "VQ44").unwrap();

    let graphs = par_graphs.borrow();
    let dgraph = graphs.d;
    let ngraph = graphs.n;

    // Walk all device graph nodes
    for i in 0..dgraph.get_num_nodes() {
        let dgraph_node = dgraph.get_node_by_index(i);
        if dgraph_node.get_mate().is_none() {
            // Not being used by the netlist; skip this
            continue;
        }

        let dgraph_node_rs = dgraph_rs.nodes.get(*dgraph_node.get_associated_data());
        println!("{:?}", dgraph_node_rs);
        let ngraph_node_rs = ngraph_rs.nodes.get(*dgraph_node.get_mate().unwrap().get_associated_data());
        println!("{:?}", ngraph_node_rs);

        match dgraph_node_rs {
            &DeviceGraphNode::AndTerm{fb, i} => {

            },
            _ => unreachable!(),
        }
    }

    bitstream
}
