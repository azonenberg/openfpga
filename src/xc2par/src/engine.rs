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

use std::collections::{HashSet};
use std::iter::FromIterator;

extern crate xc2bit;
use self::xc2bit::*;

use *;
use objpool::*;

pub fn greedy_initial_placement(mcs: &[NetlistMacrocell]) -> Vec<[isize; MCS_PER_FB]> {
    let mut ret = Vec::new();

    // TODO: Number of FBs
    // FIXME: Hack for dedicated input
    for _ in 0..2 {
        ret.push([-1; MCS_PER_FB]);
    }
    if true {
        let x = ret.len();
        ret.push([-2; MCS_PER_FB]);
        ret[x][0] = -1;
    }

    // TODO: Handle LOCs
    let mut candidate_sites = Vec::new();
    if true {
        candidate_sites.push((2, 0));
    }
    for i in (0..2).rev() {
        for j in (0..MCS_PER_FB).rev() {
            candidate_sites.push((i, j));
        }
    }

    // Do the actual greedy assignment
    for i in 0..mcs.len() {
        if candidate_sites.len() == 0 {
            panic!("no more sites");
        }

        if let NetlistMacrocell::PinInputUnreg{..} = mcs[i] {} else {
            // Not an unregistered pin input. Special-case check for dedicated input
            if true && candidate_sites.len() == 1 {
                panic!("no more sites!");
            }
        }

        let (fb, mc) = candidate_sites.pop().unwrap();
        ret[fb][mc] = i as isize;
    }

    ret
}

fn compare_andterms(g: &NetlistGraph, a: ObjPoolIndex<NetlistGraphNode>, b: ObjPoolIndex<NetlistGraphNode>) -> bool {
    let a_ = g.nodes.get(a);
    let b_ = g.nodes.get(b);
    if let NetlistGraphNodeVariant::AndTerm{
        inputs_true: ref a_inp_true, inputs_comp: ref a_inp_comp, ..} = a_.variant {

        if let NetlistGraphNodeVariant::AndTerm{
            inputs_true: ref b_inp_true, inputs_comp: ref b_inp_comp, ..} = b_.variant {

            let mut a_inp_true_h: HashSet<ObjPoolIndex<NetlistGraphNode>> = HashSet::new();
            let mut a_inp_comp_h: HashSet<ObjPoolIndex<NetlistGraphNode>> = HashSet::new();
            let mut b_inp_true_h: HashSet<ObjPoolIndex<NetlistGraphNode>> = HashSet::new();
            let mut b_inp_comp_h: HashSet<ObjPoolIndex<NetlistGraphNode>> = HashSet::new();

            for &x in a_inp_true {
                let inp_net = g.nets.get(x);
                let src_node_idx = inp_net.source.unwrap().0;
                let src_node = g.nodes.get(src_node_idx);
                if let NetlistGraphNodeVariant::ZIADummyBuf{input, ..} = src_node.variant {
                    let zia_inp_net = g.nets.get(input);
                    let zia_src_node_idx = zia_inp_net.source.unwrap().0;
                    a_inp_true_h.insert(zia_src_node_idx);
                } else {
                    panic!("mismatched node types");
                }
            }

            for &x in a_inp_comp {
                let inp_net = g.nets.get(x);
                let src_node_idx = inp_net.source.unwrap().0;
                let src_node = g.nodes.get(src_node_idx);
                if let NetlistGraphNodeVariant::ZIADummyBuf{input, ..} = src_node.variant {
                    let zia_inp_net = g.nets.get(input);
                    let zia_src_node_idx = zia_inp_net.source.unwrap().0;
                    a_inp_comp_h.insert(zia_src_node_idx);
                } else {
                    panic!("mismatched node types");
                }
            }

            for &x in b_inp_true {
                let inp_net = g.nets.get(x);
                let src_node_idx = inp_net.source.unwrap().0;
                let src_node = g.nodes.get(src_node_idx);
                if let NetlistGraphNodeVariant::ZIADummyBuf{input, ..} = src_node.variant {
                    let zia_inp_net = g.nets.get(input);
                    let zia_src_node_idx = zia_inp_net.source.unwrap().0;
                    b_inp_true_h.insert(zia_src_node_idx);
                } else {
                    panic!("mismatched node types");
                }
            }

            for &x in b_inp_comp {
                let inp_net = g.nets.get(x);
                let src_node_idx = inp_net.source.unwrap().0;
                let src_node = g.nodes.get(src_node_idx);
                if let NetlistGraphNodeVariant::ZIADummyBuf{input, ..} = src_node.variant {
                    let zia_inp_net = g.nets.get(input);
                    let zia_src_node_idx = zia_inp_net.source.unwrap().0;
                    b_inp_comp_h.insert(zia_src_node_idx);
                } else {
                    panic!("mismatched node types");
                }
            }

            a_inp_true_h == b_inp_true_h && a_inp_comp_h == b_inp_comp_h
        } else {
            panic!("not an and term");
        }
    } else {
        panic!("not an and term");
    }
}

pub enum AndTermAssignmentResult {
    Success([Option<ObjPoolIndex<NetlistGraphNode>>; ANDTERMS_PER_FB]),
    Failure(Vec<(usize, u32)>),
}

pub fn try_assign_andterms(g: &NetlistGraph, mcs: &[NetlistMacrocell], mc_assignment: &[[isize; MCS_PER_FB]])
    -> AndTermAssignmentResult {

    let mut ret = [None; ANDTERMS_PER_FB];

    AndTermAssignmentResult::Success(ret)
}
