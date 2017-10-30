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

// First element of tuple is anything, second element can only be pin input
pub fn greedy_initial_placement(mcs: &[NetlistMacrocell]) -> Vec<[(isize, isize); MCS_PER_FB]> {
    let mut ret = Vec::new();

    // TODO: Number of FBs
    // FIXME: Hack for dedicated input
    for _ in 0..2 {
        ret.push([(-1, -1); MCS_PER_FB]);
    }
    if true {
        let x = ret.len();
        ret.push([(-2, -2); MCS_PER_FB]);
        ret[x][0] = (-2, -1);
    }

    // TODO: Handle LOCs

    let mut candidate_sites_anything = Vec::new();
    let mut candidate_sites_pininput_conservative = Vec::new();
    let mut not_available_for_pininput_reg = HashSet::new();
    let mut not_available_for_pininput_unreg = HashSet::new();
    if true {
        candidate_sites_pininput_conservative.push((2, 0));
        not_available_for_pininput_reg.insert((2, 0));
    }
    for i in (0..2).rev() {
        for j in (0..MCS_PER_FB).rev() {
            candidate_sites_anything.push((i, j));
            candidate_sites_pininput_conservative.push((i, j));
        }
    }

    // Do the actual greedy assignment
    for i in 0..mcs.len() {
        match mcs[i] {
            NetlistMacrocell::PinOutput{..} => {
                if candidate_sites_anything.len() == 0 {
                    panic!("no more sites");
                }

                let (fb, mc) = candidate_sites_anything.pop().unwrap();
                // Remove from the pininput candidates as well. This requires that all PinOutput entries come first.
                candidate_sites_pininput_conservative.pop().unwrap();
                ret[fb][mc].0 = i as isize;
            },
            NetlistMacrocell::BuriedComb{..} => {
                if candidate_sites_anything.len() == 0 {
                    panic!("no more sites");
                }

                let (fb, mc) = candidate_sites_anything.pop().unwrap();
                // This is compatible with all input pin types
                ret[fb][mc].0 = i as isize;
            },
            NetlistMacrocell::BuriedReg{has_comb_fb, ..} => {
                if candidate_sites_anything.len() == 0 {
                    panic!("no more sites");
                }

                let (fb, mc) = candidate_sites_anything.pop().unwrap();
                // Cannot share with registered pin input
                not_available_for_pininput_reg.insert((fb, mc));
                if has_comb_fb {
                    // Not available to be shared at all
                    not_available_for_pininput_unreg.insert((fb, mc));
                }
                ret[fb][mc].0 = i as isize;
            },
            NetlistMacrocell::PinInputReg{..} => {
                // XXX This is not particularly efficient
                let mut idx = None;
                for j in (0..candidate_sites_pininput_conservative.len()).rev() {
                    if !not_available_for_pininput_reg.contains(&candidate_sites_pininput_conservative[j]) {
                        idx = Some(j);
                        break;
                    }
                }

                if idx.is_none() {
                    panic!("no more sites");
                }

                let (fb, mc) = candidate_sites_pininput_conservative.remove(idx.unwrap());
                ret[fb][mc].1 = i as isize;
            },
            NetlistMacrocell::PinInputUnreg{..} => {
                // XXX This is not particularly efficient
                let mut idx = None;
                for j in (0..candidate_sites_pininput_conservative.len()).rev() {
                    if !not_available_for_pininput_unreg.contains(&candidate_sites_pininput_conservative[j]) {
                        idx = Some(j);
                        break;
                    }
                }

                if idx.is_none() {
                    panic!("no more sites");
                }

                let (fb, mc) = candidate_sites_pininput_conservative.remove(idx.unwrap());
                ret[fb][mc].1 = i as isize;
            }
        }
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
    FailurePTCNeverSatisfiable,
    FailurePtermConflict(u32),
    FailurePtermExceeded(u32),
}

pub fn try_assign_andterms(g: &NetlistGraph, mcs: &[NetlistMacrocell], mc_assignment: &[(isize, isize); MCS_PER_FB])
    -> AndTermAssignmentResult {

    let mut ret = [None; ANDTERMS_PER_FB];

    // Place all the special product terms
    let mut pterm_conflicts = 0;
    // TODO: Implement using special product terms
    for mc_i in 0..MCS_PER_FB {
        if mc_assignment[mc_i].0 < 0 {
            continue;
        }

        // FIXME: Ugly code duplication
        let this_mc = &mcs[mc_assignment[mc_i].0 as usize];
        match *this_mc {
            NetlistMacrocell::PinOutput{i} => {
                let iobufe = g.nodes.get(i);
                if let NetlistGraphNodeVariant::IOBuf{input, oe, ..} = iobufe.variant {
                    if oe.is_some() {
                        // The output enable is being used

                        // but if the output pin is a constant zero, it isn't "really" being used
                        if input.unwrap() != g.vss_net {
                            let oe_idx = g.nets.get(input.unwrap()).source.unwrap().0;
                            // This goes into PTB
                            let ptb_idx = get_ptb(mc_i as u32) as usize;
                            if ret[ptb_idx].is_none() {
                                ret[ptb_idx] = Some(oe_idx);
                            } else {
                                if !compare_andterms(g, ret[ptb_idx].unwrap(), oe_idx) {
                                    pterm_conflicts += 1;
                                }
                            }
                        }
                    }

                    // Now need to look at input to this
                    if input.is_some() {
                        let input_node = g.nodes.get(g.nets.get(input.unwrap()).source.unwrap().0);
                        if let NetlistGraphNodeVariant::Xor{andterm_input, ..} = input_node.variant {
                            if andterm_input.is_some() {
                                let ptc_node_idx = g.nets.get(andterm_input.unwrap()).source.unwrap().0;
                                // This goes into PTC
                                let ptc_idx = get_ptc(mc_i as u32) as usize;
                                if ret[ptc_idx].is_none() {
                                    ret[ptc_idx] = Some(ptc_node_idx);
                                } else {
                                    if !compare_andterms(g, ret[ptc_idx].unwrap(), ptc_node_idx) {
                                        pterm_conflicts += 1;
                                    }
                                }
                            }
                        } else if let NetlistGraphNodeVariant::Reg{
                            set_input, reset_input, ce_input, clk_input, dt_input, ..} = input_node.variant {

                            let dt_input_node = g.nodes.get(g.nets.get(dt_input).source.unwrap().0);
                            if let NetlistGraphNodeVariant::Xor{andterm_input, ..} = dt_input_node.variant {
                                // XOR feeding register
                                if andterm_input.is_some() {
                                    let ptc_node_idx = g.nets.get(andterm_input.unwrap()).source.unwrap().0;
                                    // This goes into PTC
                                    let ptc_idx = get_ptc(mc_i as u32) as usize;
                                    if ret[ptc_idx].is_none() {
                                        ret[ptc_idx] = Some(ptc_node_idx);
                                    } else {
                                        if !compare_andterms(g, ret[ptc_idx].unwrap(), ptc_node_idx) {
                                            pterm_conflicts += 1;
                                        }
                                    }

                                    // Extra check for unsatisfiable PTC usage
                                    if ce_input.is_some() {
                                        let ce_input_node = g.nets.get(ce_input.unwrap()).source.unwrap().0;

                                        // If a node is using both the PTC fast path into the XOR gate as well as
                                        // clock enables, these must be identical. Both of these do not have any
                                        // choices for using global or control terms.
                                        if !compare_andterms(g, ptc_node_idx, ce_input_node) {
                                            return AndTermAssignmentResult::FailurePTCNeverSatisfiable;
                                        }
                                    }
                                }
                            }

                            if ce_input.is_some() {
                                let ptc_node_idx = g.nets.get(ce_input.unwrap()).source.unwrap().0;
                                // This goes into PTC
                                let ptc_idx = get_ptc(mc_i as u32) as usize;
                                if ret[ptc_idx].is_none() {
                                    ret[ptc_idx] = Some(ptc_node_idx);
                                } else {
                                    if !compare_andterms(g, ret[ptc_idx].unwrap(), ptc_node_idx) {
                                        pterm_conflicts += 1;
                                    }
                                }
                            }

                            let clk_node_idx = g.nets.get(clk_input).source.unwrap().0;
                            let clk_node = g.nodes.get(clk_node_idx);
                            if let NetlistGraphNodeVariant::AndTerm{..} = clk_node.variant {
                                // This goes into PTC
                                let ptc_idx = get_ptc(mc_i as u32) as usize;
                                if ret[ptc_idx].is_none() {
                                    ret[ptc_idx] = Some(clk_node_idx);
                                } else {
                                    if !compare_andterms(g, ret[ptc_idx].unwrap(), clk_node_idx) {
                                        pterm_conflicts += 1;
                                    }
                                }
                            }

                            if set_input.is_some() {
                                let set_node_idx = g.nets.get(set_input.unwrap()).source.unwrap().0;
                                let set_node = g.nodes.get(set_node_idx);
                                if let NetlistGraphNodeVariant::AndTerm{..} = set_node.variant {
                                    // This goes into PTA
                                    let pta_idx = get_pta(mc_i as u32) as usize;
                                    if ret[pta_idx].is_none() {
                                        ret[pta_idx] = Some(set_node_idx);
                                    } else {
                                        if !compare_andterms(g, ret[pta_idx].unwrap(), set_node_idx) {
                                            pterm_conflicts += 1;
                                        }
                                    }
                                }
                            }

                            if reset_input.is_some() {
                                let rst_node_idx = g.nets.get(reset_input.unwrap()).source.unwrap().0;
                                let rst_node = g.nodes.get(rst_node_idx);
                                if let NetlistGraphNodeVariant::AndTerm{..} = rst_node.variant {
                                    // This goes into PTA
                                    let pta_idx = get_pta(mc_i as u32) as usize;
                                    if ret[pta_idx].is_none() {
                                        ret[pta_idx] = Some(rst_node_idx);
                                    } else {
                                        if !compare_andterms(g, ret[pta_idx].unwrap(), rst_node_idx) {
                                            pterm_conflicts += 1;
                                        }
                                    }
                                }
                            }
                        } else {
                            panic!("not an xor or reg");
                        }
                    }
                } else {
                    panic!("not an iobufe");
                }
            },
            NetlistMacrocell::BuriedComb{i} => {
                let xornode = g.nodes.get(i);
                if let NetlistGraphNodeVariant::Xor{andterm_input, ..} = xornode.variant {
                    if andterm_input.is_some() {
                        let ptc_node_idx = g.nets.get(andterm_input.unwrap()).source.unwrap().0;
                        // This goes into PTC
                        let ptc_idx = get_ptc(mc_i as u32) as usize;
                        if ret[ptc_idx].is_none() {
                            ret[ptc_idx] = Some(ptc_node_idx);
                        } else {
                            // This should be impossible. We assign all the special product terms first. Nothing else
                            // can get assigned to this site
                            panic!("unexpected p-term in use")
                        }
                    }
                } else {
                    panic!("not an xor");
                }
            },
            NetlistMacrocell::BuriedReg{i, ..} => {
                let regnode = g.nodes.get(i);

                if let NetlistGraphNodeVariant::Reg{
                    set_input, reset_input, ce_input, clk_input, dt_input, ..} = regnode.variant {

                    let dt_input_node = g.nodes.get(g.nets.get(dt_input).source.unwrap().0);
                    if let NetlistGraphNodeVariant::Xor{andterm_input, ..} = dt_input_node.variant {
                        // XOR feeding register
                        if andterm_input.is_some() {
                            let ptc_node_idx = g.nets.get(andterm_input.unwrap()).source.unwrap().0;
                            // This goes into PTC
                            let ptc_idx = get_ptc(mc_i as u32) as usize;
                            if ret[ptc_idx].is_none() {
                                ret[ptc_idx] = Some(ptc_node_idx);
                            } else {
                                if !compare_andterms(g, ret[ptc_idx].unwrap(), ptc_node_idx) {
                                    pterm_conflicts += 1;
                                }
                            }

                            // Extra check for unsatisfiable PTC usage
                            if ce_input.is_some() {
                                let ce_input_node = g.nets.get(ce_input.unwrap()).source.unwrap().0;

                                // If a node is using both the PTC fast path into the XOR gate as well as
                                // clock enables, these must be identical. Both of these do not have any
                                // choices for using global or control terms.
                                if !compare_andterms(g, ptc_node_idx, ce_input_node) {
                                    return AndTermAssignmentResult::FailurePTCNeverSatisfiable;
                                }
                            }
                        }
                    } else {
                        // For a buried node, this must be an xor. The other choice is a direct input which is
                        // not possible.
                        panic!("not an xor")
                    }

                    if ce_input.is_some() {
                        let ptc_node_idx = g.nets.get(ce_input.unwrap()).source.unwrap().0;
                        // This goes into PTC
                        let ptc_idx = get_ptc(mc_i as u32) as usize;
                        if ret[ptc_idx].is_none() {
                            ret[ptc_idx] = Some(ptc_node_idx);
                        } else {
                            if !compare_andterms(g, ret[ptc_idx].unwrap(), ptc_node_idx) {
                                pterm_conflicts += 1;
                            }
                        }
                    }

                    let clk_node_idx = g.nets.get(clk_input).source.unwrap().0;
                    let clk_node = g.nodes.get(clk_node_idx);
                    if let NetlistGraphNodeVariant::AndTerm{..} = clk_node.variant {
                        // This goes into PTC
                        let ptc_idx = get_ptc(mc_i as u32) as usize;
                        if ret[ptc_idx].is_none() {
                            ret[ptc_idx] = Some(clk_node_idx);
                        } else {
                            if !compare_andterms(g, ret[ptc_idx].unwrap(), clk_node_idx) {
                                pterm_conflicts += 1;
                            }
                        }
                    }

                    if set_input.is_some() {
                        let set_node_idx = g.nets.get(set_input.unwrap()).source.unwrap().0;
                        let set_node = g.nodes.get(set_node_idx);
                        if let NetlistGraphNodeVariant::AndTerm{..} = set_node.variant {
                            // This goes into PTA
                            let pta_idx = get_pta(mc_i as u32) as usize;
                            if ret[pta_idx].is_none() {
                                ret[pta_idx] = Some(set_node_idx);
                            } else {
                                if !compare_andterms(g, ret[pta_idx].unwrap(), set_node_idx) {
                                    pterm_conflicts += 1;
                                }
                            }
                        }
                    }

                    if reset_input.is_some() {
                        let rst_node_idx = g.nets.get(reset_input.unwrap()).source.unwrap().0;
                        let rst_node = g.nodes.get(rst_node_idx);
                        if let NetlistGraphNodeVariant::AndTerm{..} = rst_node.variant {
                            // This goes into PTA
                            let pta_idx = get_pta(mc_i as u32) as usize;
                            if ret[pta_idx].is_none() {
                                ret[pta_idx] = Some(rst_node_idx);
                            } else {
                                if !compare_andterms(g, ret[pta_idx].unwrap(), rst_node_idx) {
                                    pterm_conflicts += 1;
                                }
                            }
                        }
                    }
                } else {
                    panic!("not a reg");
                }
            },
            // These must use the second index, not the first index
            NetlistMacrocell::PinInputUnreg{..} | NetlistMacrocell::PinInputReg{..} => unreachable!(),
        }
    }

    if pterm_conflicts > 0 {
        return AndTermAssignmentResult::FailurePtermConflict(pterm_conflicts);
    }

    AndTermAssignmentResult::Success(ret)
}
