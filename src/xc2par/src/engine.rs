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
