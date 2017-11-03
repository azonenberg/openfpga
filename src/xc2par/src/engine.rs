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

extern crate rand;
use self::rand::{Rng, SeedableRng, XorShiftRng};

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

    // Finally, place all of the remaining p-terms
    let mut unfitted_pterms = 0;
    for mc_i in 0..MCS_PER_FB {
        if mc_assignment[mc_i].0 < 0 {
            continue;
        }

        let mut orterm_node = None;
        let this_mc = &mcs[mc_assignment[mc_i].0 as usize];
        match *this_mc {
            NetlistMacrocell::PinOutput{i} => {
                let iobufe = g.nodes.get(i);
                if let NetlistGraphNodeVariant::IOBuf{input, ..} = iobufe.variant {
                    if input.is_some() {
                        let input_node = g.nodes.get(g.nets.get(input.unwrap()).source.unwrap().0);
                        if let NetlistGraphNodeVariant::Xor{orterm_input, ..} = input_node.variant {
                            if orterm_input.is_some() {
                                orterm_node = Some(g.nets.get(orterm_input.unwrap()).source.unwrap().0);
                            }
                        } else if let NetlistGraphNodeVariant::Reg{dt_input, ..} = input_node.variant {
                            let dt_input_node = g.nodes.get(g.nets.get(dt_input).source.unwrap().0);
                            if let NetlistGraphNodeVariant::Xor{orterm_input, ..} = dt_input_node.variant {
                                if orterm_input.is_some() {
                                    orterm_node = Some(g.nets.get(orterm_input.unwrap()).source.unwrap().0);
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
                if let NetlistGraphNodeVariant::Xor{orterm_input, ..} = xornode.variant {
                    if orterm_input.is_some() {
                        orterm_node = Some(g.nets.get(orterm_input.unwrap()).source.unwrap().0);
                    }
                } else {
                    panic!("not an xor");
                }
            },
            NetlistMacrocell::BuriedReg{i, ..} => {
                let regnode = g.nodes.get(i);

                if let NetlistGraphNodeVariant::Reg{dt_input, ..} = regnode.variant {
                    let dt_input_node = g.nodes.get(g.nets.get(dt_input).source.unwrap().0);
                    if let NetlistGraphNodeVariant::Xor{orterm_input, ..} = dt_input_node.variant {
                        if orterm_input.is_some() {
                            orterm_node = Some(g.nets.get(orterm_input.unwrap()).source.unwrap().0);
                        }
                    } else {
                        // For a buried node, this must be an xor. The other choice is a direct input which is
                        // not possible.
                        panic!("not an xor")
                    }
                } else {
                    panic!("not a reg");
                }
            },
            // These must use the second index, not the first index
            NetlistMacrocell::PinInputUnreg{..} | NetlistMacrocell::PinInputReg{..} => unreachable!(),
        }

        if orterm_node.is_some() {
            let orterm_obj = g.nodes.get(orterm_node.unwrap());
            if let NetlistGraphNodeVariant::OrTerm{ref inputs, ..} = orterm_obj.variant {
                for andterm_net_idx in inputs {
                    let andterm_node_idx = g.nets.get(*andterm_net_idx).source.unwrap().0;
                    let mut idx = None;
                    // FIXME: This code is super inefficient
                    // Is it equal to anything already assigned?
                    for pterm_i in 0..ANDTERMS_PER_FB {
                        if ret[pterm_i].is_some() && compare_andterms(g, ret[pterm_i].unwrap(), andterm_node_idx) {
                            idx = Some(pterm_i);
                            break;
                        }
                    }

                    if idx.is_none() {
                        // Need to find an unused spot
                        for pterm_i in 0..ANDTERMS_PER_FB {
                            if ret[pterm_i].is_none() {
                                idx = Some(pterm_i);
                                break;
                            }
                        }
                    }

                    if idx.is_none() {
                        unfitted_pterms += 1;
                    } else {
                        // Put it here
                        ret[idx.unwrap()] = Some(andterm_node_idx);
                    }
                }
            } else {
                panic!("not an or");
            }
        }
    }

    if unfitted_pterms > 0 {
        return AndTermAssignmentResult::FailurePtermExceeded(unfitted_pterms);
    }

    AndTermAssignmentResult::Success(ret)
}

pub enum ZIAAssignmentResult {
    Success([XC2ZIAInput; INPUTS_PER_ANDTERM]),
    FailureTooManyInputs(u32),
    FailureUnroutable(u32),
}

pub fn try_assign_zia(g: &NetlistGraph, mcs: &[NetlistMacrocell], mc_assignments: &[[(isize, isize); MCS_PER_FB]],
    pterm_assignment: &[Option<ObjPoolIndex<NetlistGraphNode>>; ANDTERMS_PER_FB])
    -> ZIAAssignmentResult {

    let mut ret = [XC2ZIAInput::One; INPUTS_PER_ANDTERM];

    // Collect the inputs that need to go into this FB
    let mut collected_inputs_vec = Vec::new();
    let mut collected_inputs_set = HashSet::new();
    for pt_i in 0..ANDTERMS_PER_FB {
        if pterm_assignment[pt_i].is_some() {
            let andterm_node = g.nodes.get(pterm_assignment[pt_i].unwrap());
            if let NetlistGraphNodeVariant::AndTerm{ref inputs_true, ref inputs_comp, ..} = andterm_node.variant {
                for &input_net in inputs_true {
                    let input_node = g.nodes.get(g.nets.get(input_net).source.unwrap().0);
                    if let NetlistGraphNodeVariant::ZIADummyBuf{input, ..} = input_node.variant {
                        let input_real_node_idx = g.nets.get(input).source.unwrap().0;
                        if !collected_inputs_set.contains(&input_real_node_idx) {
                            collected_inputs_set.insert(input_real_node_idx);
                            collected_inputs_vec.push(input_real_node_idx);
                        }
                    } else {
                        panic!("not a zia buf");
                    }
                }
                for &input_net in inputs_comp {
                    let input_node = g.nodes.get(g.nets.get(input_net).source.unwrap().0);
                    if let NetlistGraphNodeVariant::ZIADummyBuf{input, ..} = input_node.variant {
                        let input_real_node_idx = g.nets.get(input).source.unwrap().0;
                        if !collected_inputs_set.contains(&input_real_node_idx) {
                            collected_inputs_set.insert(input_real_node_idx);
                            collected_inputs_vec.push(input_real_node_idx);
                        }
                    } else {
                        panic!("not a zia buf");
                    }
                }
            } else {
                panic!("not an and");
            }
        }
    }

    // Must have few enough results
    if collected_inputs_vec.len() > 40 {
        return ZIAAssignmentResult::FailureTooManyInputs(collected_inputs_vec.len() as u32 - 40)
    }

    // Find candidate sites
    let candidate_sites = collected_inputs_vec.iter().map(|input| {
        // XXX FIXME
        let mut fbmc = None;
        for fb_i in 0..mc_assignments.len() {
            for mc_i in 0..MCS_PER_FB {
                if mc_assignments[fb_i][mc_i].0 >= 0 {
                    let netlist_mc = &mcs[mc_assignments[fb_i][mc_i].0 as usize];
                    let (i, need_to_use_ibuf_zia_path) = match netlist_mc {
                        &NetlistMacrocell::PinOutput{i} => (i, false),
                        &NetlistMacrocell::BuriedComb{i} => (i, false),
                        &NetlistMacrocell::BuriedReg{i, has_comb_fb} => (i, has_comb_fb),
                        _ => unreachable!(),
                    };
                    if i == (*input) {
                        fbmc = Some((fb_i, mc_i, need_to_use_ibuf_zia_path));
                        break;
                    }
                }
                if mc_assignments[fb_i][mc_i].1 >= 0 {
                    let netlist_mc = &mcs[mc_assignments[fb_i][mc_i].1 as usize];
                    let i = match netlist_mc {
                        &NetlistMacrocell::PinInputUnreg{i} => i,
                        &NetlistMacrocell::PinInputReg{i} => i,
                        _ => unreachable!(),
                    };
                    if i == (*input) {
                        fbmc = Some((fb_i, mc_i, false));
                        break;
                    }
                }
            }
        }
        let (fb, mc, need_to_use_ibuf_zia_path) = fbmc.expect("macrocell not assigned");

        // What input do we actually want?
        let input_obj = g.nodes.get(*input);
        let choice = match input_obj.variant {
            NetlistGraphNodeVariant::InBuf{..} => {
                // FIXME: Hack
                if true && fb == 2 && mc == 0 {
                    XC2ZIAInput::DedicatedInput
                } else {
                    XC2ZIAInput::IBuf{ibuf: fb_mc_num_to_iob_num(XC2Device::XC2C32A, fb as u32, mc as u32).unwrap()}
                }
            },
            NetlistGraphNodeVariant::IOBuf{..} => {
                XC2ZIAInput::IBuf{ibuf: fb_mc_num_to_iob_num(XC2Device::XC2C32A, fb as u32, mc as u32).unwrap()}
            },
            NetlistGraphNodeVariant::Xor{..} => {
                XC2ZIAInput::Macrocell{fb: fb as u32, mc: mc as u32}
            },
            NetlistGraphNodeVariant::Reg{..} => {
                if need_to_use_ibuf_zia_path {
                    XC2ZIAInput::IBuf{ibuf: fb_mc_num_to_iob_num(XC2Device::XC2C32A, fb as u32, mc as u32).unwrap()}
                } else {
                    XC2ZIAInput::Macrocell{fb: fb as u32, mc: mc as u32}
                }
            }
            _ => panic!("not a valid input"),
        };

        // Actually search the ZIA table for it
        let mut candidate_sites_for_this_input = Vec::new();
        for zia_i in 0..INPUTS_PER_ANDTERM {
            let row = zia_table_get_row(XC2Device::XC2C32A, zia_i);
            for &x in row {
                if x == choice {
                    candidate_sites_for_this_input.push(zia_i);
                }
            }
        }

        (choice, candidate_sites_for_this_input)
    }).collect::<Vec<_>>();

    // Actually do the search to assign ZIA rows
    let mut most_routed = 0;
    fn backtrack_inner(most_routed: &mut u32, ret: &mut [XC2ZIAInput; INPUTS_PER_ANDTERM],
        candidate_sites: &[(XC2ZIAInput, Vec<usize>)],
        working_on_idx: usize) -> bool {

        if working_on_idx == candidate_sites.len() {
            // Complete assignment, we are done
            return true;
        }
        let (choice, ref candidate_sites_for_this_input) = candidate_sites[working_on_idx];
        for &candidate_zia_row in candidate_sites_for_this_input {
            if ret[candidate_zia_row] == XC2ZIAInput::One {
                // It is possible to assign to this site
                ret[candidate_zia_row] = choice;
                *most_routed = working_on_idx as u32 + 1;
                if backtrack_inner(most_routed, ret, candidate_sites, working_on_idx + 1) {
                    return true;
                }
                ret[candidate_zia_row] = XC2ZIAInput::One;
            }
        }
        return false;
    };

    if !backtrack_inner(&mut most_routed, &mut ret, &candidate_sites, 0) {
        return ZIAAssignmentResult::FailureUnroutable(candidate_sites.len() as u32 - most_routed);
    }

    ZIAAssignmentResult::Success(ret)
}

pub enum FBAssignmentResult {
    Success(([Option<ObjPoolIndex<NetlistGraphNode>>; ANDTERMS_PER_FB], [XC2ZIAInput; INPUTS_PER_ANDTERM])),
    // macrocell assignment mc, score
    Failure(Vec<(u32, u32)>),
    // FIXME: This should report which one?
    FailurePTCNeverSatisfiable,
}

// FIXME: mutable assignments is a hack
pub fn try_assign_fb(g: &NetlistGraph, mcs: &[NetlistMacrocell], mc_assignments: &mut [[(isize, isize); MCS_PER_FB]],
    fb_i: u32) -> FBAssignmentResult {

    let mut base_failing_score = 0;
    // TODO: Weight factors?

    // Can we even assign p-terms?
    let pterm_assign_result = try_assign_andterms(g, mcs, &mc_assignments[fb_i as usize]);
    match pterm_assign_result {
        AndTermAssignmentResult::Success(andterm_assignment) => {
            // Can we assign the ZIA?
            let zia_assign_result = try_assign_zia(g, mcs, mc_assignments, &andterm_assignment);
            match zia_assign_result {
                ZIAAssignmentResult::Success(zia_assignment) =>
                    return FBAssignmentResult::Success((andterm_assignment, zia_assignment)),
                ZIAAssignmentResult::FailureTooManyInputs(x) => {
                    base_failing_score = x;
                },
                ZIAAssignmentResult::FailureUnroutable(x) => {
                    base_failing_score = x;
                },
            }
        },
        AndTermAssignmentResult::FailurePTCNeverSatisfiable =>
            return FBAssignmentResult::FailurePTCNeverSatisfiable,
        AndTermAssignmentResult::FailurePtermConflict(x) => {
            base_failing_score = x;
        },
        AndTermAssignmentResult::FailurePtermExceeded(x) => {
            base_failing_score = x;
        },
    }

    // If we got here, there was a failure. Delete one macrocell at a time and see what happens.
    let mut failure_scores = Vec::new();
    for mc_i in 0..MCS_PER_FB {
        let old_assign = mc_assignments[fb_i as usize][mc_i].0;
        if old_assign >= 0 {
            mc_assignments[fb_i as usize][mc_i].0 = -3;
            let mut new_failing_score = 0;
            match try_assign_andterms(g, mcs, &mc_assignments[fb_i as usize]) {
                AndTermAssignmentResult::Success(andterm_assignment) => {
                    // Can we assign the ZIA?
                    match try_assign_zia(g, mcs, mc_assignments, &andterm_assignment) {
                        ZIAAssignmentResult::Success(..) => {
                            new_failing_score = 0;
                        },
                        ZIAAssignmentResult::FailureTooManyInputs(x) => {
                            new_failing_score = x;
                        },
                        ZIAAssignmentResult::FailureUnroutable(x) => {
                            new_failing_score = x;
                        },
                    }
                },
                // This cannot happen here because it must have bombed out earlier in the initial attempt
                AndTermAssignmentResult::FailurePTCNeverSatisfiable => unreachable!(),
                AndTermAssignmentResult::FailurePtermConflict(x) => {
                    new_failing_score = x;
                },
                AndTermAssignmentResult::FailurePtermExceeded(x) => {
                    new_failing_score = x;
                },
            }

            if new_failing_score > base_failing_score {
                panic!("scores are borked");
            }

            if base_failing_score - new_failing_score > 0 {
                failure_scores.push((mc_i as u32, (base_failing_score - new_failing_score) as u32));
                mc_assignments[fb_i as usize][mc_i].0 = old_assign;
            } else {
                // XXX
            }
        }
    }

    FBAssignmentResult::Failure(failure_scores)
}

pub enum PARResult {
    Success((Vec<([(isize, isize); MCS_PER_FB],
        [Option<ObjPoolIndex<NetlistGraphNode>>; ANDTERMS_PER_FB],
        [XC2ZIAInput; INPUTS_PER_ANDTERM])>, Vec<NetlistMacrocell>)),
    FailurePTCNeverSatisfiable,
    FailureIterationsExceeded,
}

pub fn do_par(g: &NetlistGraph) -> PARResult {
    let mut prng: XorShiftRng = SeedableRng::from_seed([0, 0, 0, 1]);

    let ngraph_collected_mc = g.gather_macrocells();
    println!("{:?}", ngraph_collected_mc);

    let mut macrocell_placement = greedy_initial_placement(&ngraph_collected_mc);
    println!("{:?}", macrocell_placement);

    let mut par_results_per_fb = Vec::with_capacity(2);
    for fb_i in 0..2 {
        par_results_per_fb.push(None);
    }

    for i in 0..1000 {
        let mut bad_candidates = Vec::new();
        let mut bad_score_sum = 0;
        for fb_i in 0..2 {
            let fb_assign_result = try_assign_fb(g, &ngraph_collected_mc, &mut macrocell_placement, fb_i as u32);
            match fb_assign_result {
                FBAssignmentResult::Success((pterm, zia)) => {
                    par_results_per_fb[fb_i] = Some((pterm, zia));
                },
                FBAssignmentResult::FailurePTCNeverSatisfiable =>
                    return PARResult::FailurePTCNeverSatisfiable,
                FBAssignmentResult::Failure(fail_vec) => {
                    for (mc, score) in fail_vec {
                        bad_score_sum += bad_score_sum;
                        bad_candidates.push((fb_i as u32, mc, score));
                    }
                }
            }
        }

        if bad_candidates.len() == 0 {
            // It worked!
            let mut ret = Vec::new();
            for i in 0..2 {
                ret.push((macrocell_placement[i], par_results_per_fb[i].unwrap().0, par_results_per_fb[i].unwrap().1));
            }

            return PARResult::Success((ret, ngraph_collected_mc));
        }

        // Here, we need to swap some stuff around
        let mut move_cand_rand = prng.gen_range(0, bad_score_sum);
        let mut move_cand_idx = 0;
        while move_cand_rand >= bad_candidates[move_cand_idx].2 {
            move_cand_rand -= bad_candidates[move_cand_idx].2;
            move_cand_idx += 1;
        }
        let (move_fb, move_mc, _) = bad_candidates[move_cand_idx];

        // Find min-conflicts site
        let mut move_to_badness_vec = Vec::new();
        let mut best_badness = None;
        for cand_fb_i in 0..2 {
            for cand_mc_i in 0..MCS_PER_FB {
                // This site is not usable
                if macrocell_placement[cand_fb_i][cand_mc_i].0 < -1 {
                    continue;
                }

                let mut new_badness = 0;
                // Does this violate a pairing constraint?
                if macrocell_placement[cand_fb_i][cand_mc_i].1 >= 0 {
                    match ngraph_collected_mc[macrocell_placement[move_fb as usize][move_mc as usize].0 as usize] {
                        NetlistMacrocell::PinOutput{..} => {
                            new_badness = 1;
                        },
                        NetlistMacrocell::BuriedComb{..} => {
                            // Ok, do nothing
                        },
                        NetlistMacrocell::BuriedReg{has_comb_fb, ..} => {
                            match ngraph_collected_mc[macrocell_placement[cand_fb_i][cand_mc_i].1 as usize] {
                                NetlistMacrocell::PinInputReg{..} => {
                                    new_badness = 1;
                                },
                                NetlistMacrocell::PinInputUnreg{..} => {
                                    if has_comb_fb {
                                        new_badness = 1;
                                    }
                                },
                                _ => unreachable!(),
                            }
                        }
                        _ => unreachable!(),
                    }
                }

                // The new site is possibly ok, so let's swap it there for now
                let orig_move_assignment = macrocell_placement[move_fb as usize][move_mc as usize].0;
                let orig_cand_assignment = macrocell_placement[cand_fb_i][cand_mc_i].0;
                macrocell_placement[cand_fb_i][cand_mc_i].0 = orig_move_assignment;
                macrocell_placement[move_fb as usize][move_mc as usize].0 = orig_cand_assignment;

                // Now score
                match try_assign_fb(g, &ngraph_collected_mc, &mut macrocell_placement, cand_fb_i as u32) {
                    FBAssignmentResult::Success(..) => {
                        // Do nothing, don't need to change badness (will be 0 if was 0)
                    },
                    FBAssignmentResult::FailurePTCNeverSatisfiable => unreachable!(),
                    FBAssignmentResult::Failure(fail_vec) => {
                        for (mc, score) in fail_vec {
                            if mc == cand_mc_i as u32 {
                                new_badness += score;
                            }
                            // XXX what happens if this condition never triggers?
                        }
                    }
                }

                // Remember it
                move_to_badness_vec.push((cand_fb_i, cand_mc_i, new_badness));
                if best_badness.is_none() || best_badness.unwrap() > new_badness {
                    best_badness = Some(new_badness);
                }

                macrocell_placement[cand_fb_i][cand_mc_i].0 = orig_cand_assignment;
                macrocell_placement[move_fb as usize][move_mc as usize].0 = orig_move_assignment;
            }
        }

        // Now we need to get the candidates matching the lowest score
        let mut move_final_candidates = Vec::new();
        for x in move_to_badness_vec {
            if x.2 == best_badness.unwrap() {
                move_final_candidates.push((x.0, x.1));
            }
        }

        // Do the actual swap now
        let move_final_candidates_rand_idx = prng.gen_range(0, move_final_candidates.len());
        let (move_final_fb, move_final_mc) = move_final_candidates[move_final_candidates_rand_idx];
        let final_orig_move_assignment = macrocell_placement[move_fb as usize][move_mc as usize].0;
        let final_orig_cand_assignment = macrocell_placement[move_final_fb][move_final_mc].0;
        macrocell_placement[move_final_fb][move_final_mc].0 = final_orig_move_assignment;
        macrocell_placement[move_fb as usize][move_mc as usize].0 = final_orig_cand_assignment;
    }

    PARResult::FailureIterationsExceeded
}
