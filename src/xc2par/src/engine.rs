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

extern crate rand;
use self::rand::{Rng, SeedableRng, XorShiftRng};

extern crate xc2bit;
use self::xc2bit::*;

use *;
use objpool::*;

#[derive(Copy, Clone, PartialEq, Eq, Debug)]
pub enum PARMCAssignment {
    MC(ObjPoolIndex<InputGraphMacrocell>),
    None,
    Banned,
}

impl PARMCAssignment {
    pub fn used(&self) -> bool {
        match self {
            &PARMCAssignment::MC(_) => true,
            _ => false,
        }
    }
}

// First element of tuple is anything, second element can only be pin input
pub fn greedy_initial_placement(g: &InputGraph) -> Vec<[(PARMCAssignment, PARMCAssignment); MCS_PER_FB]> {
    let mut ret = Vec::new();

    // TODO: Number of FBs
    // FIXME: Hack for dedicated input
    for _ in 0..2 {
        ret.push([(PARMCAssignment::None, PARMCAssignment::None); MCS_PER_FB]);
    }
    if true {
        let x = ret.len();
        ret.push([(PARMCAssignment::Banned, PARMCAssignment::Banned); MCS_PER_FB]);
        ret[x][0] = (PARMCAssignment::Banned, PARMCAssignment::None);
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
    for i_ in g.mcs.iter_idx() {
        let i = i_.get_raw_i();
        let mc = g.mcs.get(i_);
        match mc.get_type() {
            InputGraphMacrocellType::PinOutput => {
                if candidate_sites_anything.len() == 0 {
                    panic!("no more sites");
                }

                let (fb, mc) = candidate_sites_anything.pop().unwrap();
                // Remove from the pininput candidates as well. This requires that all PinOutput entries come first.
                candidate_sites_pininput_conservative.pop().unwrap();
                ret[fb][mc].0 = PARMCAssignment::MC(i_);
            },
            InputGraphMacrocellType::BuriedComb => {
                if candidate_sites_anything.len() == 0 {
                    panic!("no more sites");
                }

                let (fb, mc) = candidate_sites_anything.pop().unwrap();
                // This is compatible with all input pin types
                ret[fb][mc].0 = PARMCAssignment::MC(i_);
            },
            InputGraphMacrocellType::BuriedReg => {
                let has_comb_fb = mc.xor_feedback_used;

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
                ret[fb][mc].0 = PARMCAssignment::MC(i_);
            },
            InputGraphMacrocellType::PinInputReg => {
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
                ret[fb][mc].1 = PARMCAssignment::MC(i_);
            },
            InputGraphMacrocellType::PinInputUnreg => {
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
                ret[fb][mc].1 = PARMCAssignment::MC(i_);
            }
        }
    }

    ret
}

fn compare_andterms(g: &IntermediateGraph, a: ObjPoolIndex<IntermediateGraphNode>, b: ObjPoolIndex<IntermediateGraphNode>) -> bool {
    let a_ = g.nodes.get(a);
    let b_ = g.nodes.get(b);
    if let IntermediateGraphNodeVariant::AndTerm{
        inputs_true: ref a_inp_true, inputs_comp: ref a_inp_comp, ..} = a_.variant {

        if let IntermediateGraphNodeVariant::AndTerm{
            inputs_true: ref b_inp_true, inputs_comp: ref b_inp_comp, ..} = b_.variant {

            let mut a_inp_true_h: HashSet<ObjPoolIndex<IntermediateGraphNode>> = HashSet::new();
            let mut a_inp_comp_h: HashSet<ObjPoolIndex<IntermediateGraphNode>> = HashSet::new();
            let mut b_inp_true_h: HashSet<ObjPoolIndex<IntermediateGraphNode>> = HashSet::new();
            let mut b_inp_comp_h: HashSet<ObjPoolIndex<IntermediateGraphNode>> = HashSet::new();

            for &x in a_inp_true {
                let inp_net = g.nets.get(x);
                let src_node_idx = inp_net.source.unwrap().0;
                a_inp_true_h.insert(src_node_idx);
            }

            for &x in a_inp_comp {
                let inp_net = g.nets.get(x);
                let src_node_idx = inp_net.source.unwrap().0;
                a_inp_comp_h.insert(src_node_idx);
            }

            for &x in b_inp_true {
                let inp_net = g.nets.get(x);
                let src_node_idx = inp_net.source.unwrap().0;
                b_inp_true_h.insert(src_node_idx);
            }

            for &x in b_inp_comp {
                let inp_net = g.nets.get(x);
                let src_node_idx = inp_net.source.unwrap().0;
                b_inp_comp_h.insert(src_node_idx);
            }

            a_inp_true_h == b_inp_true_h && a_inp_comp_h == b_inp_comp_h
        } else {
            panic!("not an and term");
        }
    } else {
        panic!("not an and term");
    }
}

fn compare_andterms2(g: &InputGraph, a: ObjPoolIndex<InputGraphPTerm>, b: ObjPoolIndex<InputGraphPTerm>) -> bool {
    let a_ = g.pterms.get(a);
    let b_ = g.pterms.get(b);

    let mut a_inp_true_h: HashSet<(InputGraphPTermInputType, ObjPoolIndex<InputGraphMacrocell>)> =
        HashSet::from_iter(a_.inputs_true.iter().cloned());
    let mut a_inp_comp_h: HashSet<(InputGraphPTermInputType, ObjPoolIndex<InputGraphMacrocell>)> =
        HashSet::from_iter(a_.inputs_comp.iter().cloned());
    let mut b_inp_true_h: HashSet<(InputGraphPTermInputType, ObjPoolIndex<InputGraphMacrocell>)> =
        HashSet::from_iter(b_.inputs_true.iter().cloned());
    let mut b_inp_comp_h: HashSet<(InputGraphPTermInputType, ObjPoolIndex<InputGraphMacrocell>)> =
        HashSet::from_iter(b_.inputs_comp.iter().cloned());

    a_inp_true_h == b_inp_true_h && a_inp_comp_h == b_inp_comp_h
}

pub enum AndTermAssignmentResult {
    Success([Option<ObjPoolIndex<InputGraphPTerm>>; ANDTERMS_PER_FB]),
    FailurePTCNeverSatisfiable,
    FailurePtermConflict(u32),
    FailurePtermExceeded(u32),
}

pub fn try_assign_andterms(g: &IntermediateGraph, g2: &InputGraph, mcs: &[NetlistMacrocell], mc_assignment: &[(PARMCAssignment, PARMCAssignment); MCS_PER_FB])
    -> AndTermAssignmentResult {

    let mut ret = [None; ANDTERMS_PER_FB];

    // Place all the special product terms
    let mut pterm_conflicts = 0;
    // TODO: Implement using special control product terms
    for mc_i in 0..MCS_PER_FB {
        if let PARMCAssignment::MC(mc_g_idx) = mc_assignment[mc_i].0 {
            // FIXME: Ugly code duplication
            let this_mc = &g2.mcs.get(mc_g_idx);

            if let Some(io_bits) = this_mc.io_bits {
                if let Some(InputGraphIOOEType::PTerm(oe_idx)) = io_bits.oe {
                    // This goes into PTB
                    let ptb_idx = get_ptb(mc_i as u32) as usize;
                    if ret[ptb_idx].is_none() {
                        ret[ptb_idx] = Some(oe_idx);
                    } else {
                        if !compare_andterms2(g2, ret[ptb_idx].unwrap(), oe_idx) {
                            pterm_conflicts += 1;
                        }
                    }
                }
            }

            if let Some(xor_bits) = this_mc.xor_bits {
                if let Some(ptc_node_idx) = xor_bits.andterm_input {
                    // This goes into PTC
                    let ptc_idx = get_ptc(mc_i as u32) as usize;
                    if ret[ptc_idx].is_none() {
                        ret[ptc_idx] = Some(ptc_node_idx);
                    } else {
                        if !compare_andterms2(g2, ret[ptc_idx].unwrap(), ptc_node_idx) {
                            pterm_conflicts += 1;
                        }
                    }
                }
            }

            if let Some(reg_bits) = this_mc.reg_bits {
                if let Some(ptc_node_idx) = reg_bits.ce_input {
                    // This goes into PTC
                    let ptc_idx = get_ptc(mc_i as u32) as usize;
                    if ret[ptc_idx].is_none() {
                        ret[ptc_idx] = Some(ptc_node_idx);
                    } else {
                        if !compare_andterms2(g2, ret[ptc_idx].unwrap(), ptc_node_idx) {
                            pterm_conflicts += 1;
                        }
                    }

                    // Extra check for unsatisfiable PTC usage
                    if this_mc.xor_bits.is_some() && this_mc.xor_bits.unwrap().andterm_input.is_some() {
                        if !compare_andterms2(g2, ptc_node_idx, this_mc.xor_bits.unwrap().andterm_input.unwrap()) {
                            return AndTermAssignmentResult::FailurePTCNeverSatisfiable;
                        }
                    }
                }

                if let InputGraphRegClockType::PTerm(clk_node_idx) = reg_bits.clk_input {
                    // This goes into PTC
                    let ptc_idx = get_ptc(mc_i as u32) as usize;
                    if ret[ptc_idx].is_none() {
                        ret[ptc_idx] = Some(clk_node_idx);
                    } else {
                        if !compare_andterms2(g2, ret[ptc_idx].unwrap(), clk_node_idx) {
                            pterm_conflicts += 1;
                        }
                    }
                }

                if let Some(InputGraphRegRSType::PTerm(set_node_idx)) = reg_bits.set_input {
                    // This goes into PTA
                    let pta_idx = get_pta(mc_i as u32) as usize;
                    if ret[pta_idx].is_none() {
                        ret[pta_idx] = Some(set_node_idx);
                    } else {
                        if !compare_andterms2(g2, ret[pta_idx].unwrap(), set_node_idx) {
                            pterm_conflicts += 1;
                        }
                    }
                }

                if let Some(InputGraphRegRSType::PTerm(reset_node_idx)) = reg_bits.reset_input {
                    // This goes into PTA
                    let pta_idx = get_pta(mc_i as u32) as usize;
                    if ret[pta_idx].is_none() {
                        ret[pta_idx] = Some(reset_node_idx);
                    } else {
                        if !compare_andterms2(g2, ret[pta_idx].unwrap(), reset_node_idx) {
                            pterm_conflicts += 1;
                        }
                    }
                }
            }
        }
    }

    if pterm_conflicts > 0 {
        return AndTermAssignmentResult::FailurePtermConflict(pterm_conflicts);
    }

    // Finally, place all of the remaining p-terms
    let mut unfitted_pterms = 0;
    for mc_i in 0..MCS_PER_FB {
        if let PARMCAssignment::MC(mc_g_idx) = mc_assignment[mc_i].0 {
            let this_mc = &g2.mcs.get(mc_g_idx);

            if let Some(xor_bits) = this_mc.xor_bits {
                for andterm_node_idx in xor_bits.orterm_inputs {
                    let mut idx = None;
                    // FIXME: This code is super inefficient
                    // Is it equal to anything already assigned?
                    for pterm_i in 0..ANDTERMS_PER_FB {
                        if ret[pterm_i].is_some() && compare_andterms2(g2, ret[pterm_i].unwrap(), andterm_node_idx) {
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

pub fn try_assign_zia(g: &IntermediateGraph, g2: &InputGraph, mcs: &[NetlistMacrocell], mc_assignments: &[[(PARMCAssignment, PARMCAssignment); MCS_PER_FB]],
    pterm_assignment: &[Option<ObjPoolIndex<InputGraphPTerm>>; ANDTERMS_PER_FB])
    -> ZIAAssignmentResult {

    let mut ret = [XC2ZIAInput::One; INPUTS_PER_ANDTERM];

    // Collect the inputs that need to go into this FB
    let mut collected_inputs_vec = Vec::new();
    let mut collected_inputs_set = HashSet::new();
    for pt_i in 0..ANDTERMS_PER_FB {
        if pterm_assignment[pt_i].is_some() {
            let andterm_node = g2.pterms.get(pterm_assignment[pt_i].unwrap());
            for input_net in andterm_node.inputs_true {
                if !collected_inputs_set.contains(&input_net) {
                    collected_inputs_set.insert(input_net);
                    collected_inputs_vec.push(input_net);
                }
            }
            for input_net in andterm_node.inputs_comp {
                if !collected_inputs_set.contains(&input_net) {
                    collected_inputs_set.insert(input_net);
                    collected_inputs_vec.push(input_net);
                }
            }
        }
    }

    // Must have few enough results
    if collected_inputs_vec.len() > 40 {
        return ZIAAssignmentResult::FailureTooManyInputs(collected_inputs_vec.len() as u32 - 40)
    }

    // Find candidate sites
    let candidate_sites = collected_inputs_vec.iter().map(|input| {
        let input_obj = g2.mcs.get(input.1);
        let fb = input_obj.loc.unwrap().fb;
        let mc = input_obj.loc.unwrap().i;
        let need_to_use_ibuf_zia_path = input.0 == InputGraphPTermInputType::Reg && input_obj.xor_feedback_used;

        // What input do we actually want?
        let choice = match input.0 {
            InputGraphPTermInputType::Pin => {
                // FIXME: Hack
                if true && fb == 2 && mc == 0 {
                    XC2ZIAInput::DedicatedInput
                } else {
                    XC2ZIAInput::IBuf{ibuf: fb_mc_num_to_iob_num(XC2Device::XC2C32A, fb as u32, mc as u32).unwrap()}
                }
            },
            InputGraphPTermInputType::Xor => {
                XC2ZIAInput::Macrocell{fb: fb as u32, mc: mc as u32}
            },
            InputGraphPTermInputType::Reg => {
                if need_to_use_ibuf_zia_path {
                    XC2ZIAInput::IBuf{ibuf: fb_mc_num_to_iob_num(XC2Device::XC2C32A, fb as u32, mc as u32).unwrap()}
                } else {
                    XC2ZIAInput::Macrocell{fb: fb as u32, mc: mc as u32}
                }
            },
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
    Success(([Option<ObjPoolIndex<InputGraphPTerm>>; ANDTERMS_PER_FB], [XC2ZIAInput; INPUTS_PER_ANDTERM])),
    // macrocell assignment mc, score
    Failure(Vec<(u32, u32)>),
    // FIXME: This should report which one?
    FailurePTCNeverSatisfiable,
}

// FIXME: mutable assignments is a hack
pub fn try_assign_fb(g: &IntermediateGraph, g2: &InputGraph, mcs: &[NetlistMacrocell], mc_assignments: &mut [[(PARMCAssignment, PARMCAssignment); MCS_PER_FB]],
    fb_i: u32) -> FBAssignmentResult {

    let mut base_failing_score = 0;
    // TODO: Weight factors?

    // Can we even assign p-terms?
    let pterm_assign_result = try_assign_andterms(g, g2, mcs, &mc_assignments[fb_i as usize]);
    match pterm_assign_result {
        AndTermAssignmentResult::Success(andterm_assignment) => {
            // Can we assign the ZIA?
            let zia_assign_result = try_assign_zia(g, g2, mcs, mc_assignments, &andterm_assignment);
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
            match try_assign_andterms(g, g2, mcs, &mc_assignments[fb_i as usize]) {
                AndTermAssignmentResult::Success(andterm_assignment) => {
                    // Can we assign the ZIA?
                    match try_assign_zia(g, g2, mcs, mc_assignments, &andterm_assignment) {
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
    Success((Vec<([(PARMCAssignment, PARMCAssignment); MCS_PER_FB],
        [Option<ObjPoolIndex<InputGraphPTerm>>; ANDTERMS_PER_FB],
        [XC2ZIAInput; INPUTS_PER_ANDTERM])>, Vec<NetlistMacrocell>)),
    FailurePTCNeverSatisfiable,
    FailureIterationsExceeded,
}

pub fn do_par(g: &IntermediateGraph, g2: &InputGraph) -> PARResult {
    let mut prng: XorShiftRng = SeedableRng::from_seed([0, 0, 0, 1]);

    let ngraph_collected_mc = g.gather_macrocells();
    println!("{:?}", ngraph_collected_mc);

    let mut macrocell_placement = greedy_initial_placement(g2);
    println!("{:?}", macrocell_placement);

    let mut par_results_per_fb = Vec::with_capacity(2);
    for fb_i in 0..2 {
        par_results_per_fb.push(None);
    }

    for i in 0..1000 {
        let mut bad_candidates = Vec::new();
        let mut bad_score_sum = 0;
        for fb_i in 0..2 {
            let fb_assign_result = try_assign_fb(g, g2, &ngraph_collected_mc, &mut macrocell_placement, fb_i as u32);
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
                if macrocell_placement[cand_fb_i][cand_mc_i].0 == PARMCAssignment::Banned {
                    continue;
                }

                let mut new_badness = 0;
                // Does this violate a pairing constraint?
                if let PARMCAssignment::MC(mc_cand_idx) = macrocell_placement[cand_fb_i][cand_mc_i].1 {
                    let mc_cand = g2.mcs.get(mc_cand_idx);
                    let mc_move =
                        if let PARMCAssignment::MC(x) = macrocell_placement[move_fb as usize][move_mc as usize].0 {
                            g2.mcs.get(x)
                        } else {
                            unreachable!();
                        };

                    match mc_move.get_type() {
                        InputGraphMacrocellType::PinOutput => {
                            new_badness = 1;
                        },
                        InputGraphMacrocellType::BuriedComb => {
                            // Ok, do nothing
                        },
                        InputGraphMacrocellType::BuriedReg => {
                            let has_comb_fb = mc_move.xor_feedback_used;
                            match mc_cand.get_type() {
                                InputGraphMacrocellType::PinInputReg => {
                                    new_badness = 1;
                                },
                                InputGraphMacrocellType::PinInputUnreg => {
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
                match try_assign_fb(g, g2, &ngraph_collected_mc, &mut macrocell_placement, cand_fb_i as u32) {
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
