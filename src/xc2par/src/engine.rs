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

use std::collections::{HashSet, HashMap};
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

type PARFBAssignment = [(PARMCAssignment, PARMCAssignment); MCS_PER_FB];
// fb, mc, pininput?
type PARFBAssignLoc = (u32, u32, bool);
pub type PARZIAAssignment = [XC2ZIAInput; INPUTS_PER_ANDTERM];

// TODO: LOC constraints for not-macrocell stuff

// First element of tuple is anything, second element can only be pin input
pub fn greedy_initial_placement(g: &mut InputGraph) -> Option<Vec<PARFBAssignment>> {
    let mut ret = Vec::new();

    // First greedily assign all of the global nets
    // TODO: Replace with BitSet when it becomes stable
    let mut gck_used = HashSet::with_capacity(NUM_BUFG_CLK);
    let mut gts_used = HashSet::with_capacity(NUM_BUFG_GTS);
    let mut gsr_used = HashSet::with_capacity(NUM_BUFG_GSR);
    
    // Begin with assigning those that have a LOC constraint on the buffer. We know that these already have LOC
    // constraints on the pin as well.
    {
        for gck in g.bufg_clks.iter_mut() {
            if let Some(RequestedLocation{i: Some(idx), ..}) = gck.requested_loc {
                if gck_used.contains(&idx) {
                    return None;
                }
                gck_used.insert(idx);

                gck.loc = Some(AssignedLocationInner {
                    fb: 0,
                    i: idx,
                });
            }
        }
    }
    {
        for gts in g.bufg_gts.iter_mut() {
            if let Some(RequestedLocation{i: Some(idx), ..}) = gts.requested_loc {
                if gts_used.contains(&idx) {
                    return None;
                }
                gts_used.insert(idx);

                gts.loc = Some(AssignedLocationInner {
                    fb: 0,
                    i: idx,
                });
            }
        }
    }
    {
        for gsr in g.bufg_gsr.iter_mut() {
            if let Some(RequestedLocation{i: Some(idx), ..}) = gsr.requested_loc {
                if gsr_used.contains(&idx) {
                    return None;
                }
                gsr_used.insert(idx);

                gsr.loc = Some(AssignedLocationInner {
                    fb: 0,
                    i: idx,
                });
            }
        }
    }

    // Now we assign locations to all of the remaining global buffers. Note that we checked ahead of time that there
    // aren't too many of these in use. However, it is still possible to get an unsatisfiable assignment due to
    // FB constraints on the macrocell.
    {
        for gck in g.bufg_clks.iter_mut() {
            if gck.loc.is_some() {
                continue;
            }

            let mut idx = None;
            for i in 0..NUM_BUFG_CLK {
                if gck_used.contains(&(i as u32)) {
                    continue;
                }

                let mc_req_loc = g.mcs.get(gck.input).requested_loc;
                let actual_mc_loc = get_gck(XC2Device::XC2C32A, i).unwrap();
                assert!(mc_req_loc.is_none() || mc_req_loc.unwrap().i.is_none());
                if mc_req_loc.is_some() && mc_req_loc.unwrap().fb != actual_mc_loc.0 {
                    continue;
                }

                idx = Some(i as u32);
                // Now force the MC to have the full location
                // XXX: This can in very rare occasions cause a design that should in theory fit to no longer fit.
                // However, we consider this to be unimportant because global nets almost always need special treatment
                // by the HDL designer to work properly anyways.
                g.mcs.get_mut(gck.input).requested_loc = Some(RequestedLocation{
                    fb: actual_mc_loc.0, i: Some(actual_mc_loc.1)});
                break;
            }

            if idx.is_none() {
                return None;
            }

            gck_used.insert(idx.unwrap());
            gck.loc = Some(AssignedLocationInner {
                fb: 0,
                i: idx.unwrap(),
            });
        }
    }
    {
        for gts in g.bufg_clks.iter_mut() {
            if gts.loc.is_some() {
                continue;
            }

            let mut idx = None;
            for i in 0..NUM_BUFG_GTS {
                if gts_used.contains(&(i as u32)) {
                    continue;
                }

                let mc_req_loc = g.mcs.get(gts.input).requested_loc;
                let actual_mc_loc = get_gts(XC2Device::XC2C32A, i).unwrap();
                assert!(mc_req_loc.is_none() || mc_req_loc.unwrap().i.is_none());
                if mc_req_loc.is_some() && mc_req_loc.unwrap().fb != actual_mc_loc.0 {
                    continue;
                }

                idx = Some(i as u32);
                // Now force the MC to have the full location
                // XXX: This can in very rare occasions cause a design that should in theory fit to no longer fit.
                // However, we consider this to be unimportant because global nets almost always need special treatment
                // by the HDL designer to work properly anyways.
                g.mcs.get_mut(gts.input).requested_loc = Some(RequestedLocation{
                    fb: actual_mc_loc.0, i: Some(actual_mc_loc.1)});
                break;
            }

            if idx.is_none() {
                return None;
            }

            gts_used.insert(idx.unwrap());
            gts.loc = Some(AssignedLocationInner {
                fb: 0,
                i: idx.unwrap(),
            });
        }
    }
    {
        for gsr in g.bufg_clks.iter_mut() {
            if gsr.loc.is_some() {
                continue;
            }

            let mut idx = None;
            for i in 0..NUM_BUFG_GSR {
                if gsr_used.contains(&(i as u32)) {
                    continue;
                }

                let mc_req_loc = g.mcs.get(gsr.input).requested_loc;
                let actual_mc_loc = get_gsr(XC2Device::XC2C32A);
                assert!(mc_req_loc.is_none() || mc_req_loc.unwrap().i.is_none());
                if mc_req_loc.is_some() && mc_req_loc.unwrap().fb != actual_mc_loc.0 {
                    continue;
                }

                idx = Some(i as u32);
                // Now force the MC to have the full location
                // XXX: This can in very rare occasions cause a design that should in theory fit to no longer fit.
                // However, we consider this to be unimportant because global nets almost always need special treatment
                // by the HDL designer to work properly anyways.
                g.mcs.get_mut(gsr.input).requested_loc = Some(RequestedLocation{
                    fb: actual_mc_loc.0, i: Some(actual_mc_loc.1)});
                break;
            }

            if idx.is_none() {
                return None;
            }

            gsr_used.insert(idx.unwrap());
            gsr.loc = Some(AssignedLocationInner {
                fb: 0,
                i: idx.unwrap(),
            });
        }
    }

    println!("after global assign {:?}", g);

    // Now actually assign macrocell locations

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

    // Immediately place all fully LOC'd macrocells now
    for i in g.mcs.iter_idx() {
        let mc = g.mcs.get(i);
        let is_pininput = match mc.get_type() {
            InputGraphMacrocellType::PinOutput |
            InputGraphMacrocellType::BuriedComb |
            InputGraphMacrocellType::BuriedReg => false,
            InputGraphMacrocellType::PinInputReg |
            InputGraphMacrocellType::PinInputUnreg => true,
        };

        if let Some(RequestedLocation{fb, i: Some(mc_idx)}) = mc.requested_loc {
            if !is_pininput {
                if ret[fb as usize][mc_idx as usize].0 != PARMCAssignment::None {
                    return None;
                }

                ret[fb as usize][mc_idx as usize].0 = PARMCAssignment::MC(i);
            } else {
                if ret[fb as usize][mc_idx as usize].1 != PARMCAssignment::None {
                    return None;
                }

                ret[fb as usize][mc_idx as usize].1 = PARMCAssignment::MC(i);
            }
        }
    }

    // Check for pairing violations
    for fb_i in 0..3 {
        for mc_i in 0..MCS_PER_FB {
            if let PARMCAssignment::MC(mc_idx_0) = ret[fb_i as usize][mc_i].0 {
                if let PARMCAssignment::MC(mc_idx_1) = ret[fb_i as usize][mc_i].1 {
                    let type_0 = g.mcs.get(mc_idx_0).get_type();
                    let type_1 = g.mcs.get(mc_idx_1).get_type();
                    match (type_0, type_1) {
                        (InputGraphMacrocellType::PinInputUnreg, InputGraphMacrocellType::BuriedComb) |
                        (InputGraphMacrocellType::PinInputReg, InputGraphMacrocellType::BuriedComb) => {},
                        (InputGraphMacrocellType::PinInputUnreg, InputGraphMacrocellType::BuriedReg) => {
                            if g.mcs.get(mc_idx_0).xor_feedback_used {
                                return None;
                            }
                        }
                        _ => return None,
                    }
                }
            }
        }
    }

    // Now place macrocells that have a FB constraint but no MC constraint
    for i in g.mcs.iter_idx() {
        let mc = g.mcs.get(i);
        let is_pininput = match mc.get_type() {
            InputGraphMacrocellType::PinOutput |
            InputGraphMacrocellType::BuriedComb |
            InputGraphMacrocellType::BuriedReg => false,
            InputGraphMacrocellType::PinInputReg |
            InputGraphMacrocellType::PinInputUnreg => true,
        };

        if let Some(RequestedLocation{fb, i: None}) = mc.requested_loc {
            let mut mc_i = None;
            for i in 0..MCS_PER_FB {
                if !is_pininput {
                    if ret[fb as usize][i].0 != PARMCAssignment::None {
                        continue;
                    }
                } else {
                    if ret[fb as usize][i].1 != PARMCAssignment::None {
                        continue;
                    }

                    // If this is a pin input, check if pairing is ok
                    // This logic relies on the gather_macrocells sorting.
                    if let PARMCAssignment::MC(mc_idx_0) = ret[fb as usize][i].0 {
                        let type_0 = g.mcs.get(mc_idx_0).get_type();
                        let type_1 = mc.get_type();
                        match (type_0, type_1) {
                            (InputGraphMacrocellType::PinInputUnreg, InputGraphMacrocellType::BuriedComb) |
                            (InputGraphMacrocellType::PinInputReg, InputGraphMacrocellType::BuriedComb) => {},
                            (InputGraphMacrocellType::PinInputUnreg, InputGraphMacrocellType::BuriedReg) => {
                                if g.mcs.get(mc_idx_0).xor_feedback_used {
                                    continue;
                                }
                            }
                            _ => continue,
                        }
                    }
                }

                mc_i = Some(i as u32);
                break;
            }

            if mc_i.is_none() {
                return None;
            }
            let mc_idx = mc_i.unwrap();
            if !is_pininput {
                ret[fb as usize][mc_idx as usize].0 = PARMCAssignment::MC(i);
            } else {
                ret[fb as usize][mc_idx as usize].1 = PARMCAssignment::MC(i);
            }
        }
    }

    // Now place all the other macrocells
    // FIXME: Copypasta
    for i in g.mcs.iter_idx() {
        let mc = g.mcs.get(i);
        let is_pininput = match mc.get_type() {
            InputGraphMacrocellType::PinOutput |
            InputGraphMacrocellType::BuriedComb |
            InputGraphMacrocellType::BuriedReg => false,
            InputGraphMacrocellType::PinInputReg |
            InputGraphMacrocellType::PinInputUnreg => true,
        };

        if mc.requested_loc.is_none() {
            let mut fbmc_i = None;
            for fb in 0..3 {
                for i in 0..MCS_PER_FB {
                    if !is_pininput {
                        if ret[fb][i].0 != PARMCAssignment::None {
                            continue;
                        }
                    } else {
                        if ret[fb][i].1 != PARMCAssignment::None {
                            continue;
                        }

                        // If this is a pin input, check if pairing is ok
                        // This logic relies on the gather_macrocells sorting.
                        if let PARMCAssignment::MC(mc_idx_0) = ret[fb][i].0 {
                            let type_0 = g.mcs.get(mc_idx_0).get_type();
                            let type_1 = mc.get_type();
                            match (type_0, type_1) {
                                (InputGraphMacrocellType::PinInputUnreg, InputGraphMacrocellType::BuriedComb) |
                                (InputGraphMacrocellType::PinInputReg, InputGraphMacrocellType::BuriedComb) => {},
                                (InputGraphMacrocellType::PinInputUnreg, InputGraphMacrocellType::BuriedReg) => {
                                    if g.mcs.get(mc_idx_0).xor_feedback_used {
                                        continue;
                                    }
                                }
                                _ => continue,
                            }
                        }
                    }

                    fbmc_i = Some((fb as u32, i as u32));
                    break;
                }
            }

            if fbmc_i.is_none() {
                return None;
            }
            let (fb, mc_idx) = fbmc_i.unwrap();
            if !is_pininput {
                ret[fb as usize][mc_idx as usize].0 = PARMCAssignment::MC(i);
            } else {
                ret[fb as usize][mc_idx as usize].1 = PARMCAssignment::MC(i);
            }
        }
    }

    // Update the "reverse" pointers
    for fb_i in 0..3 {
        for mc_i in 0..MCS_PER_FB {
            if let PARMCAssignment::MC(mc_idx) = ret[fb_i as usize][mc_i].0 {
                let mc = g.mcs.get_mut(mc_idx);
                mc.loc = Some(AssignedLocationInner{
                    fb: fb_i,
                    i: mc_i as u32,
                });
            }
            if let PARMCAssignment::MC(mc_idx) = ret[fb_i as usize][mc_i].1 {
                let mc = g.mcs.get_mut(mc_idx);
                mc.loc = Some(AssignedLocationInner{
                    fb: fb_i,
                    i: mc_i as u32,
                });
            }
        }
    }

    Some(ret)
}

#[derive(Copy, Clone, PartialEq, Eq, Debug)]
pub enum AndTermAssignmentResult {
    Success,
    FailurePtermConflict(u32),
    FailurePtermExceeded(u32),
}

pub fn try_assign_andterms(g: &mut InputGraph, mc_assignment: &PARFBAssignment, fb_i: u32) -> AndTermAssignmentResult {
    let mut ret = [None; ANDTERMS_PER_FB];

    // Place all the special product terms
    let mut pterm_conflicts = 0;
    // TODO: Implement using special control product terms
    for mc_i in 0..MCS_PER_FB {
        if let PARMCAssignment::MC(mc_g_idx) = mc_assignment[mc_i].0 {
            // FIXME: Ugly code duplication
            let this_mc = &g.mcs.get(mc_g_idx);

            if let Some(ref io_bits) = this_mc.io_bits {
                if let Some(InputGraphIOOEType::PTerm(oe_idx)) = io_bits.oe {
                    // This goes into PTB
                    let ptb_idx = get_ptb(mc_i as u32) as usize;
                    if ret[ptb_idx].is_none() {
                        ret[ptb_idx] = Some(oe_idx);
                    } else {
                        if g.pterms.get(ret[ptb_idx].unwrap()) != g.pterms.get(oe_idx) {
                            pterm_conflicts += 1;
                        }
                    }
                }
            }

            if let Some(ref xor_bits) = this_mc.xor_bits {
                if let Some(ptc_node_idx) = xor_bits.andterm_input {
                    // This goes into PTC
                    let ptc_idx = get_ptc(mc_i as u32) as usize;
                    if ret[ptc_idx].is_none() {
                        ret[ptc_idx] = Some(ptc_node_idx);
                    } else {
                        if g.pterms.get(ret[ptc_idx].unwrap()) != g.pterms.get(ptc_node_idx) {
                            pterm_conflicts += 1;
                        }
                    }
                }
            }

            if let Some(ref reg_bits) = this_mc.reg_bits {
                if let Some(ptc_node_idx) = reg_bits.ce_input {
                    // This goes into PTC
                    let ptc_idx = get_ptc(mc_i as u32) as usize;
                    if ret[ptc_idx].is_none() {
                        ret[ptc_idx] = Some(ptc_node_idx);
                    } else {
                        if g.pterms.get(ret[ptc_idx].unwrap()) != g.pterms.get(ptc_node_idx) {
                            pterm_conflicts += 1;
                        }
                    }
                }

                if let InputGraphRegClockType::PTerm(clk_node_idx) = reg_bits.clk_input {
                    // This goes into PTC
                    let ptc_idx = get_ptc(mc_i as u32) as usize;
                    if ret[ptc_idx].is_none() {
                        ret[ptc_idx] = Some(clk_node_idx);
                    } else {
                        if g.pterms.get(ret[ptc_idx].unwrap()) != g.pterms.get(clk_node_idx) {
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
                        if g.pterms.get(ret[pta_idx].unwrap()) != g.pterms.get(set_node_idx) {
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
                        if g.pterms.get(ret[pta_idx].unwrap()) != g.pterms.get(reset_node_idx) {
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
    let mut existing_pterm_map = HashMap::new();
    for pterm_i in 0..ANDTERMS_PER_FB {
        if let Some(pterm_idx) = ret[pterm_i] {
            let pterm_obj = g.pterms.get(pterm_idx);
            existing_pterm_map.insert(pterm_obj.clone(), pterm_i);
        }
    }

    let mut unfitted_pterms = 0;
    for mc_i in 0..MCS_PER_FB {
        if let PARMCAssignment::MC(mc_g_idx) = mc_assignment[mc_i].0 {
            let this_mc = &g.mcs.get(mc_g_idx);

            if let Some(ref xor_bits) = this_mc.xor_bits {
                for &andterm_node_idx in &xor_bits.orterm_inputs {
                    let this_pterm_obj = g.pterms.get(andterm_node_idx);

                    if let Some(_) = existing_pterm_map.get(&this_pterm_obj) {
                        // It is equal to this existing term, so we don't have to do anything
                    } else {
                        // Need to find an unused spot
                        let mut idx = None;
                        for pterm_i in 0..ANDTERMS_PER_FB {
                            if ret[pterm_i].is_none() {
                                idx = Some(pterm_i);
                                break;
                            }
                        }

                        if idx.is_none() {
                            unfitted_pterms += 1;
                        } else {
                            // Put it here
                            ret[idx.unwrap()] = Some(andterm_node_idx);
                            existing_pterm_map.insert(this_pterm_obj.clone(), idx.unwrap());
                        }
                    }
                }
            }
        }
    }

    if unfitted_pterms > 0 {
        return AndTermAssignmentResult::FailurePtermExceeded(unfitted_pterms);
    }

    // Update the "reverse" pointers
    for pterm in g.pterms.iter_mut() {
        // Only do this update if this lookup succeeds. This lookup will fail for terms that are in other FBs
        if let Some(&mc_i) = existing_pterm_map.get(pterm) {
            pterm.loc = Some(AssignedLocationInner{
                fb: fb_i,
                i: mc_i as u32,
            });
        }
    }

    AndTermAssignmentResult::Success
}

pub enum ZIAAssignmentResult {
    Success(PARZIAAssignment),
    FailureTooManyInputs(u32),
    FailureUnroutable(u32),
}

pub fn try_assign_zia(g: &mut InputGraph, mc_assignment: &PARFBAssignment) -> ZIAAssignmentResult {
    let mut ret_zia = [XC2ZIAInput::One; INPUTS_PER_ANDTERM];
    let mut input_to_row_map = HashMap::new();

    // Collect the p-terms that will be used by this FB
    let mut collected_pterms = Vec::new();
    for mc_i in 0..MCS_PER_FB {
        if let PARMCAssignment::MC(mc_g_idx) = mc_assignment[mc_i].0 {
            // FIXME: Ugly code duplication
            let this_mc = &g.mcs.get(mc_g_idx);

            if let Some(ref io_bits) = this_mc.io_bits {
                if let Some(InputGraphIOOEType::PTerm(oe_idx)) = io_bits.oe {
                    collected_pterms.push(oe_idx);
                }
            }

            if let Some(ref xor_bits) = this_mc.xor_bits {
                if let Some(ptc_node_idx) = xor_bits.andterm_input {
                    collected_pterms.push(ptc_node_idx);
                }

                for &andterm_node_idx in &xor_bits.orterm_inputs {
                    collected_pterms.push(andterm_node_idx);
                }
            }

            if let Some(ref reg_bits) = this_mc.reg_bits {
                if let Some(ptc_node_idx) = reg_bits.ce_input {
                    collected_pterms.push(ptc_node_idx);
                }

                if let InputGraphRegClockType::PTerm(clk_node_idx) = reg_bits.clk_input {
                    collected_pterms.push(clk_node_idx);
                }

                if let Some(InputGraphRegRSType::PTerm(set_node_idx)) = reg_bits.set_input {
                    collected_pterms.push(set_node_idx);
                }

                if let Some(InputGraphRegRSType::PTerm(reset_node_idx)) = reg_bits.reset_input {
                    collected_pterms.push(reset_node_idx);
                }
            }
        }
    }

    // Collect the inputs that need to go into this FB
    let mut collected_inputs_vec = Vec::new();
    let mut collected_inputs_set = HashSet::new();
    for &pt_idx in &collected_pterms {
        let andterm_node = g.pterms.get(pt_idx);
        for &input_net in &andterm_node.inputs_true {
            if !collected_inputs_set.contains(&input_net) {
                collected_inputs_set.insert(input_net);
                collected_inputs_vec.push(input_net);
            }
        }
        for &input_net in &andterm_node.inputs_comp {
            if !collected_inputs_set.contains(&input_net) {
                collected_inputs_set.insert(input_net);
                collected_inputs_vec.push(input_net);
            }
        }
    }

    // Must have few enough results
    if collected_inputs_vec.len() > 40 {
        return ZIAAssignmentResult::FailureTooManyInputs(collected_inputs_vec.len() as u32 - 40)
    }

    // Find candidate sites
    let candidate_sites = collected_inputs_vec.iter().map(|input| {
        let input_obj = g.mcs.get(input.1);
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

        (*input, choice, candidate_sites_for_this_input)
    }).collect::<Vec<_>>();

    // Actually do the search to assign ZIA rows
    let mut most_routed = 0;
    fn backtrack_inner(most_routed: &mut u32, ret: &mut PARZIAAssignment,
        candidate_sites: &[(InputGraphPTermInput, XC2ZIAInput, Vec<usize>)],
        working_on_idx: usize,
        input_to_row_map: &mut HashMap<InputGraphPTermInput, u32>) -> bool {

        if working_on_idx == candidate_sites.len() {
            // Complete assignment, we are done
            return true;
        }
        let (input, choice, ref candidate_sites_for_this_input) = candidate_sites[working_on_idx];
        for &candidate_zia_row in candidate_sites_for_this_input {
            if ret[candidate_zia_row] == XC2ZIAInput::One {
                // It is possible to assign to this site
                ret[candidate_zia_row] = choice;
                input_to_row_map.insert(input, candidate_zia_row as u32);
                *most_routed = working_on_idx as u32 + 1;
                if backtrack_inner(most_routed, ret, candidate_sites, working_on_idx + 1, input_to_row_map) {
                    return true;
                }
                ret[candidate_zia_row] = XC2ZIAInput::One;
                input_to_row_map.remove(&input);
            }
        }
        return false;
    };

    if !backtrack_inner(&mut most_routed, &mut ret_zia, &candidate_sites, 0, &mut input_to_row_map) {
        return ZIAAssignmentResult::FailureUnroutable(candidate_sites.len() as u32 - most_routed);
    }

    // Now we search through all the inputs and record which row they go in
    for &pt_idx in &collected_pterms {
        let andterm_node = g.pterms.get_mut(pt_idx);
        for input_net in &andterm_node.inputs_true {
            andterm_node.inputs_true_zia.push(*input_to_row_map.get(input_net).unwrap());
        }
        for input_net in &andterm_node.inputs_comp {
            andterm_node.inputs_comp_zia.push(*input_to_row_map.get(input_net).unwrap());
        }
    }

    ZIAAssignmentResult::Success(ret_zia)
}

enum FBAssignmentResultInner {
    Success(PARZIAAssignment),
    // badness
    Failure(u32),
}

pub enum FBAssignmentResult {
    Success(PARZIAAssignment),
    // macrocell assignment mc, score
    Failure(Vec<(u32, u32)>),
}

fn try_assign_fb_inner(g: &mut InputGraph, mc_assignments: &[PARFBAssignment], fb_i: u32) -> FBAssignmentResultInner {
    let mut failing_score = 0;
    // TODO: Weight factors?

    // Can we even assign p-terms?
    let pterm_assign_result = try_assign_andterms(g, &mc_assignments[fb_i as usize], fb_i);
    let zia_assign_result = try_assign_zia(g, &mc_assignments[fb_i as usize]);

    if pterm_assign_result == AndTermAssignmentResult::Success {
        if let ZIAAssignmentResult::Success(zia_assignment) = zia_assign_result {
            return FBAssignmentResultInner::Success(zia_assignment);
        }
    }

    match pterm_assign_result {
        AndTermAssignmentResult::FailurePtermConflict(x) => {
            failing_score += x;
        },
        AndTermAssignmentResult::FailurePtermExceeded(x) => {
            failing_score += x;
        },
        _ => unreachable!(),
    }

    match zia_assign_result {
        ZIAAssignmentResult::FailureTooManyInputs(x) => {
            failing_score += x;
        },
        ZIAAssignmentResult::FailureUnroutable(x) => {
            failing_score += x;
        },
        _ => unreachable!(),
    }

    FBAssignmentResultInner::Failure(failing_score)
}

pub fn try_assign_fb(g: &mut InputGraph, mc_assignments: &mut [PARFBAssignment], fb_i: u32,
    constraint_violations: &mut HashMap<PARFBAssignLoc, u32>) -> FBAssignmentResult {
    let initial_assign_result = try_assign_fb_inner(g, mc_assignments, fb_i);

    // Check for pairing violations
    // TODO: Fix copypasta
    // XXX: Explain why this doesn't need the "remove and try again" logic?
    for mc_i in 0..MCS_PER_FB {
        if let PARMCAssignment::MC(mc_idx_0) = mc_assignments[fb_i as usize][mc_i].0 {
            if let PARMCAssignment::MC(mc_idx_1) = mc_assignments[fb_i as usize][mc_i].1 {
                let type_0 = g.mcs.get(mc_idx_0).get_type();
                let type_1 = g.mcs.get(mc_idx_1).get_type();
                match (type_0, type_1) {
                    (InputGraphMacrocellType::PinInputUnreg, InputGraphMacrocellType::BuriedComb) |
                    (InputGraphMacrocellType::PinInputReg, InputGraphMacrocellType::BuriedComb) => {},
                    (InputGraphMacrocellType::PinInputUnreg, InputGraphMacrocellType::BuriedReg) => {
                        if g.mcs.get(mc_idx_0).xor_feedback_used {
                            // Conflict
                            let x = constraint_violations.insert((fb_i, mc_i as u32, false), 1);
                            assert!(x.is_none());
                            let x = constraint_violations.insert((fb_i, mc_i as u32, true), 1);
                            assert!(x.is_none());
                        }
                    },
                    _ => {
                        // Conflict
                        let x = constraint_violations.insert((fb_i, mc_i as u32, false), 1);
                        assert!(x.is_none());
                        let x = constraint_violations.insert((fb_i, mc_i as u32, true), 1);
                        assert!(x.is_none());
                    }
                }
            }
        }
    }

    match initial_assign_result {
        FBAssignmentResultInner::Success(x) => FBAssignmentResult::Success(x),
        FBAssignmentResultInner::Failure(base_failing_score) => {
            // Not a success. Delete one macrocell at a time and see what happens.
            let mut failure_scores = Vec::new();
            for mc_i in 0..MCS_PER_FB {
                let old_assign = mc_assignments[fb_i as usize][mc_i].0;
                if let PARMCAssignment::MC(_) = old_assign {
                    mc_assignments[fb_i as usize][mc_i].0 = PARMCAssignment::None;
                    let new_failing_score = match try_assign_fb_inner(g, mc_assignments, fb_i) {
                        FBAssignmentResultInner::Success(_) => 0,
                        FBAssignmentResultInner::Failure(x) => x,
                    };

                    if new_failing_score > base_failing_score {
                        panic!("scores are borked");
                    }

                    if base_failing_score - new_failing_score > 0 {
                        // Deleting this thing made the score better (as opposed to no change)
                        let my_loc = (fb_i, mc_i as u32, false);
                        let old_score = if constraint_violations.contains_key(&my_loc) {
                            *constraint_violations.get(&my_loc).unwrap()
                        } else { 0 };
                        let new_score = (base_failing_score - new_failing_score) as u32;
                        constraint_violations.insert(my_loc, old_score + new_score);
                        failure_scores.push((mc_i as u32, new_score));
                    }
                    mc_assignments[fb_i as usize][mc_i].0 = old_assign;
                }
            }

            FBAssignmentResult::Failure(failure_scores)
        }
    }
}

#[derive(Copy, Clone, PartialEq, Eq, Hash, Debug)]
pub enum PARSanityResult {
    Ok,
    FailurePTCNeverSatisfiable,
    FailureGlobalNetWrongLoc,
    FailureTooManyMCs,
    FailureTooManyPTerms,
    FailureTooManyBufgClk,
    FailureTooManyBufgGTS,
    FailureTooManyBufgGSR,
}

// FIXME: What happens in netlist.rs and what happens here?
pub fn do_par_sanity_check(g: &mut InputGraph) -> PARSanityResult {
    // Check if everything fits in the device
    if g.mcs.len() > 2 * (2 * MCS_PER_FB) {
        // Note that this is a conservative fail-early check. It is incomplete because it doesn't account for
        // which macrocells can actually be paired together or which buried sites (in larger devices) can be used.
        return PARSanityResult::FailureTooManyMCs;
    }

    let pterms_set: HashSet<InputGraphPTerm> = HashSet::from_iter(g.pterms.iter().cloned());
    if pterms_set.len() > 2 * ANDTERMS_PER_FB {
        return PARSanityResult::FailureTooManyPTerms;
    }

    if g.bufg_clks.len() > NUM_BUFG_CLK {
        return PARSanityResult::FailureTooManyBufgClk;
    }
    if g.bufg_gts.len() > NUM_BUFG_GTS{
        return PARSanityResult::FailureTooManyBufgGTS;
    }
    if g.bufg_gsr.len() > NUM_BUFG_GSR {
        return PARSanityResult::FailureTooManyBufgGSR;
    }

    // Check for impossible-to-satisfy PTC usage
    for mc in g.mcs.iter() {
        if let Some(ref reg_bits) = mc.reg_bits {
            if let Some(oe_node_idx) = reg_bits.ce_input {
                if let Some(ref xor_bits) = mc.xor_bits {
                    if let Some(xor_ptc_node_idx) = xor_bits.andterm_input {
                        if g.pterms.get(oe_node_idx) != g.pterms.get(xor_ptc_node_idx) {
                            return PARSanityResult::FailurePTCNeverSatisfiable;
                        }
                    }
                }
            }
        }
    }

    // Check the LOC constraints for global nets
    for buf in g.bufg_clks.iter_mut() {
        let buf_req_loc = buf.requested_loc;
        let mc_req_loc = g.mcs.get(buf.input).requested_loc;

        match (buf_req_loc, mc_req_loc) {
            (Some(RequestedLocation{i: Some(buf_idx), ..}), Some(mc_loc)) => {
                // Both the pin and the buffer have a preference for where to be.

                // These two need to match
                let actual_mc_loc = get_gck(XC2Device::XC2C32A, buf_idx as usize).unwrap();
                if actual_mc_loc.0 != mc_loc.fb || (mc_loc.i.is_some() && mc_loc.i.unwrap() != actual_mc_loc.1) {
                    return PARSanityResult::FailureGlobalNetWrongLoc;
                }

                // Now force the MC to have the full location
                g.mcs.get_mut(buf.input).requested_loc = Some(RequestedLocation{
                    fb: actual_mc_loc.0, i: Some(actual_mc_loc.1)});
            },
            (Some(RequestedLocation{i: Some(buf_idx), ..}), None) => {
                // There is a preference for the buffer, but no preference for the pin.

                let actual_mc_loc = get_gck(XC2Device::XC2C32A, buf_idx as usize).unwrap();

                // Now force the MC to have the full location
                g.mcs.get_mut(buf.input).requested_loc = Some(RequestedLocation{
                    fb: actual_mc_loc.0, i: Some(actual_mc_loc.1)});
            },
            (Some(RequestedLocation{i: None, ..}), Some(_)) | (None, Some(_)) => {
                // There is a preference for the pin, but no preference for the buffer.
                // Do nothing for now, we can fail this in the greedy assignment step
            },
            _ => {},
        }
    }

    // FIXME: Copypasta
    for buf in g.bufg_gts.iter_mut() {
        let buf_req_loc = buf.requested_loc;
        let mc_req_loc = g.mcs.get(buf.input).requested_loc;

        match (buf_req_loc, mc_req_loc) {
            (Some(RequestedLocation{i: Some(buf_idx), ..}), Some(mc_loc)) => {
                // Both the pin and the buffer have a preference for where to be.

                // These two need to match
                let actual_mc_loc = get_gts(XC2Device::XC2C32A, buf_idx as usize).unwrap();
                if actual_mc_loc.0 != mc_loc.fb || (mc_loc.i.is_some() && mc_loc.i.unwrap() != actual_mc_loc.1) {
                    return PARSanityResult::FailureGlobalNetWrongLoc;
                }

                // Now force the MC to have the full location
                g.mcs.get_mut(buf.input).requested_loc = Some(RequestedLocation{
                    fb: actual_mc_loc.0, i: Some(actual_mc_loc.1)});
            },
            (Some(RequestedLocation{i: Some(buf_idx), ..}), None) => {
                // There is a preference for the buffer, but no preference for the pin.

                let actual_mc_loc = get_gts(XC2Device::XC2C32A, buf_idx as usize).unwrap();

                // Now force the MC to have the full location
                g.mcs.get_mut(buf.input).requested_loc = Some(RequestedLocation{
                    fb: actual_mc_loc.0, i: Some(actual_mc_loc.1)});
            },
            (Some(RequestedLocation{i: None, ..}), Some(_)) | (None, Some(_)) => {
                // There is a preference for the pin, but no preference for the buffer.
                // Do nothing for now, we can fail this in the greedy assignment step
            },
            _ => {},
        }
    }

    for buf in g.bufg_gsr.iter_mut() {
        let buf_req_loc = buf.requested_loc;
        let mc_req_loc = g.mcs.get(buf.input).requested_loc;

        match (buf_req_loc, mc_req_loc) {
            (Some(RequestedLocation{i: Some(_), ..}), Some(mc_loc)) => {
                // Both the pin and the buffer have a preference for where to be.

                // These two need to match
                let actual_mc_loc = get_gsr(XC2Device::XC2C32A);
                if actual_mc_loc.0 != mc_loc.fb || (mc_loc.i.is_some() && mc_loc.i.unwrap() != actual_mc_loc.1) {
                    return PARSanityResult::FailureGlobalNetWrongLoc;
                }

                // Now force the MC to have the full location
                g.mcs.get_mut(buf.input).requested_loc = Some(RequestedLocation{
                    fb: actual_mc_loc.0, i: Some(actual_mc_loc.1)});
            },
            (Some(RequestedLocation{i: Some(_), ..}), None) => {
                // There is a preference for the buffer, but no preference for the pin.

                let actual_mc_loc = get_gsr(XC2Device::XC2C32A);

                // Now force the MC to have the full location
                g.mcs.get_mut(buf.input).requested_loc = Some(RequestedLocation{
                    fb: actual_mc_loc.0, i: Some(actual_mc_loc.1)});
            },
            (Some(RequestedLocation{i: None, ..}), Some(_)) | (None, Some(_)) => {
                // There is a preference for the pin, but no preference for the buffer.
                // Do nothing for now, we can fail this in the greedy assignment step
            },
            _ => {},
        }
    }

    PARSanityResult::Ok
}

pub enum PARResult {
    Success(Vec<PARZIAAssignment>),
    FailureSanity(PARSanityResult),
    FailureIterationsExceeded,
}

pub fn do_par(g: &mut InputGraph) -> PARResult {
    let sanity_check = do_par_sanity_check(g);
    if sanity_check != PARSanityResult::Ok {
        return PARResult::FailureSanity(sanity_check);
    }

    let mut prng: XorShiftRng = SeedableRng::from_seed([0, 0, 0, 1]);

    let macrocell_placement = greedy_initial_placement(g);
    println!("{:?}", macrocell_placement);
    if macrocell_placement.is_none() {
        // XXX this is ugly
        return PARResult::FailureSanity(PARSanityResult::FailureTooManyMCs);
    }
    let mut macrocell_placement = macrocell_placement.unwrap();

    // Score whatever we got out of the greedy placement
    let mut best_par_results_per_fb = Vec::with_capacity(2);
    for _ in 0..2 {
        best_par_results_per_fb.push(None);
    }
    let mut best_placement = macrocell_placement.clone();
    let mut best_placement_violations = HashMap::new();
    for fb_i in 0..2 {
        let fb_assign_result = try_assign_fb(g, &mut macrocell_placement, fb_i as u32, &mut best_placement_violations);
        match fb_assign_result {
            FBAssignmentResult::Success(zia) => {
                best_par_results_per_fb[fb_i] = Some(zia);
            },
            FBAssignmentResult::Failure(_) => {
            }
        }
    }
    let mut best_placement_violations_score = 0;
    for x in best_placement_violations.values() {
        best_placement_violations_score += x;
    }

    for _iter_count in 0..1000 {
        // Update the "reverse" pointers
        // TODO: Fix copypasta
        for fb_i in 0..3 {
            for mc_i in 0..MCS_PER_FB {
                if let PARMCAssignment::MC(mc_idx) = best_placement[fb_i][mc_i].0 {
                    let mc = g.mcs.get_mut(mc_idx);
                    mc.loc = Some(AssignedLocationInner{
                        fb: fb_i as u32,
                        i: mc_i as u32,
                    });
                }
                if let PARMCAssignment::MC(mc_idx) = best_placement[fb_i][mc_i].1 {
                    let mc = g.mcs.get_mut(mc_idx);
                    mc.loc = Some(AssignedLocationInner{
                        fb: fb_i as u32,
                        i: mc_i as u32,
                    });
                }
            }
        }

        if best_placement_violations.len() == 0 {
            // It worked!
            let mut ret = Vec::new();
            for i in 0..2 {
                let result_i = std::mem::replace(&mut best_par_results_per_fb[i], None);
                let zia = result_i.unwrap();
                ret.push(zia);
            }

            return PARResult::Success(ret);
        }

        // Here, we need to swap some stuff around
        let mut bad_candidates = Vec::new();
        for (&k, &v) in &best_placement_violations {
            bad_candidates.push((k, v));
        }

        // Pick a candidate to move weighted by its badness
        let mut move_cand_rand = prng.gen_range(0, best_placement_violations_score);
        let mut move_cand_idx = 0;
        while move_cand_rand >= bad_candidates[move_cand_idx].1 {
            move_cand_rand -= bad_candidates[move_cand_idx].1;
            move_cand_idx += 1;
        }
        let ((move_fb, move_mc, move_pininput), _) = bad_candidates[move_cand_idx];

        // Find min-conflicts site
        let mut new_best_placement_violations_score = None;
        for cand_fb in 0..2 {
            for cand_mc in 0..MCS_PER_FB {
                // This site is not usable
                if !move_pininput {
                    if macrocell_placement[cand_fb][cand_mc].0 == PARMCAssignment::Banned {
                        continue;
                    }
                } else {
                    if macrocell_placement[cand_fb][cand_mc].1 == PARMCAssignment::Banned {
                        continue;
                    }
                }

                // Swap it into this site
                let (orig_move_assignment, orig_cand_assignment) = if !move_pininput {
                    let orig_move_assignment = macrocell_placement[move_fb as usize][move_mc as usize].0;
                    let orig_cand_assignment = macrocell_placement[cand_fb][cand_mc].0;
                    macrocell_placement[cand_fb][cand_mc].0 = orig_move_assignment;
                    macrocell_placement[move_fb as usize][move_mc as usize].0 = orig_cand_assignment;
                    (orig_move_assignment, orig_cand_assignment)
                } else {
                    let orig_move_assignment = macrocell_placement[move_fb as usize][move_mc as usize].1;
                    let orig_cand_assignment = macrocell_placement[cand_fb][cand_mc].1;
                    macrocell_placement[cand_fb][cand_mc].1 = orig_move_assignment;
                    macrocell_placement[move_fb as usize][move_mc as usize].1 = orig_cand_assignment;
                    (orig_move_assignment, orig_cand_assignment)
                };

                // Swap the "loc" field as well
                if let PARMCAssignment::MC(mc_idx) = orig_move_assignment {
                    g.mcs.get_mut(mc_idx).loc = Some(AssignedLocationInner {
                        fb: cand_fb as u32,
                        i: cand_mc as u32,
                    });
                }
                if let PARMCAssignment::MC(mc_idx) = orig_cand_assignment {
                    g.mcs.get_mut(mc_idx).loc = Some(AssignedLocationInner {
                        fb: move_fb,
                        i: move_mc,
                    });
                }

                // Score what we've got
                let mut par_results_per_fb = Vec::with_capacity(2);
                for _ in 0..2 {
                    par_results_per_fb.push(None);
                }
                let mut new_placement_violations = HashMap::new();
                for fb_i in 0..2 {
                    let fb_assign_result = try_assign_fb(g, &mut macrocell_placement, fb_i as u32,
                        &mut new_placement_violations);
                    match fb_assign_result {
                        FBAssignmentResult::Success(zia) => {
                            par_results_per_fb[fb_i] = Some(zia);
                        },
                        FBAssignmentResult::Failure(_) => {
                        }
                    }
                }
                let mut new_placement_violations_score = 0;
                for x in new_placement_violations.values() {
                    new_placement_violations_score += x;
                }

                // Is it better? Remember it
                if new_best_placement_violations_score.is_none() ||
                    (new_placement_violations_score < new_best_placement_violations_score.unwrap()) {

                    new_best_placement_violations_score = Some(new_placement_violations_score);
                    best_placement = macrocell_placement.clone();
                    best_placement_violations = new_placement_violations;
                    best_par_results_per_fb = par_results_per_fb;
                    best_placement_violations_score = new_placement_violations_score;
                }

                // Swap it back
                let (orig_move_assignment, orig_cand_assignment) = if !move_pininput {
                    let orig_move_assignment = macrocell_placement[move_fb as usize][move_mc as usize].0;
                    let orig_cand_assignment = macrocell_placement[cand_fb][cand_mc].0;
                    macrocell_placement[cand_fb][cand_mc].0 = orig_move_assignment;
                    macrocell_placement[move_fb as usize][move_mc as usize].0 = orig_cand_assignment;
                    (orig_move_assignment, orig_cand_assignment)
                } else {
                    let orig_move_assignment = macrocell_placement[move_fb as usize][move_mc as usize].1;
                    let orig_cand_assignment = macrocell_placement[cand_fb][cand_mc].1;
                    macrocell_placement[cand_fb][cand_mc].1 = orig_move_assignment;
                    macrocell_placement[move_fb as usize][move_mc as usize].1 = orig_cand_assignment;
                    (orig_move_assignment, orig_cand_assignment)
                };

                // Swap the "loc" field as well
                if let PARMCAssignment::MC(mc_idx) = orig_move_assignment {
                    g.mcs.get_mut(mc_idx).loc = Some(AssignedLocationInner {
                        fb: move_fb,
                        i: move_mc,
                    });
                }
                if let PARMCAssignment::MC(mc_idx) = orig_cand_assignment {
                    g.mcs.get_mut(mc_idx).loc = Some(AssignedLocationInner {
                        fb: cand_fb as u32,
                        i: cand_mc as u32,
                    });
                }
            }
        }
    }

    PARResult::FailureIterationsExceeded
}
