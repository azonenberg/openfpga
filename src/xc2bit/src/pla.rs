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

//! Contains functions pertaining to the PLA

use crate::*;

/// Represents one single AND term in the PLA. Each AND term can perform an AND function on any subset of its inputs
/// and the complement of those inputs. The index for each input is the corresponding ZIA row.
#[derive(Copy, Clone, Eq, PartialEq, Hash, Debug, Serialize, Deserialize)]
pub struct XC2PLAAndTerm {
    /// Indicates whether a particular ZIA row output is a part of this AND term.
    ///
    /// `true` = part of and, `false` = not part of and
    input: [u8; INPUTS_PER_ANDTERM / 8],
    /// Indicates whether the complement of a particular ZIA row output is a part of this AND term.
    ///
    /// `true` = part of and, `false` = not part of and
    input_b: [u8; INPUTS_PER_ANDTERM / 8],
}

impl Default for XC2PLAAndTerm {
    /// Returns a "default" AND term. The default state is for none of the inputs to be selected.
    fn default() -> Self {
        XC2PLAAndTerm {
            input: [0u8; INPUTS_PER_ANDTERM / 8],
            input_b: [0u8; INPUTS_PER_ANDTERM / 8],
        }
    }
}

impl XC2PLAAndTerm {
    /// Internal function that reads one single AND term from a block of fuses using logical fuse indexing
    pub fn from_jed(fuses: &[bool], block_idx: usize, term_idx: usize) -> XC2PLAAndTerm {
        let mut input = [0u8; INPUTS_PER_ANDTERM / 8];
        let mut input_b = [0u8; INPUTS_PER_ANDTERM / 8];

        for i in 0..INPUTS_PER_ANDTERM {
            if !fuses[block_idx + term_idx * INPUTS_PER_ANDTERM * 2 + i * 2 + 0] {
                input[i / 8] |= 1 << (i % 8);
            }
            if !fuses[block_idx + term_idx * INPUTS_PER_ANDTERM * 2 + i * 2 + 1] {
                input_b[i / 8] |= 1 << (i % 8);
            }
        }

        XC2PLAAndTerm {
            input,
            input_b,
        }
    }

    /// Returns `true` if the `i`th input is used in this AND term
    pub fn get(&self, i: usize) -> bool {
        self.input[i / 8] & (1 << (i % 8)) != 0
    }

    /// Returns `true` if the `i`th input complement is used in this AND term
    pub fn get_b(&self, i: usize) -> bool {
        self.input_b[i / 8] & (1 << (i % 8)) != 0
    }

    /// Sets whether the `i`th input is used in this AND term
    pub fn set(&mut self, i: usize, val: bool) {
        if !val {
            self.input[i / 8] &=  !(1 << (i % 8));
        } else {
            self.input[i / 8] |=  1 << (i % 8);
        }
    }

    /// Sets whether the `i`th input complement is used in this AND term
    pub fn set_b(&mut self, i: usize, val: bool) {
        if !val {
            self.input_b[i / 8] &=  !(1 << (i % 8));
        } else {
            self.input_b[i / 8] |=  1 << (i % 8);
        }
    }
}

/// Represents one single OR term in the PLA. Each OR term can perform an OR function on any subset of its inputs.
/// The index for each input is the index of the corresponding AND term in the same PLA.
#[derive(Copy, Clone, Eq, PartialEq, Hash, Debug, Serialize, Deserialize)]
pub struct XC2PLAOrTerm {
    /// Indicates whether a particular PLA AND term is a part of this OR term.
    ///
    /// `true` = part of or, `false` = not part of or
    input: [u8; ANDTERMS_PER_FB / 8],
}

impl Default for XC2PLAOrTerm {
    /// Returns a "default" OR term. The default state is for none of the inputs to be selected.
    fn default() -> Self {
        XC2PLAOrTerm {
            input: [0u8; ANDTERMS_PER_FB / 8],
        }
    }
}

impl XC2PLAOrTerm {
    /// Internal function that reads one single OR term from a block of fuses using logical fuse indexing
    pub fn from_jed(fuses: &[bool], block_idx: usize, term_idx: usize) -> XC2PLAOrTerm {
        let mut input = [0u8; ANDTERMS_PER_FB / 8];

        for i in 0..ANDTERMS_PER_FB {
            if !fuses[block_idx + term_idx +i * MCS_PER_FB] {
                input[i / 8] |= 1 << (i % 8);
            }
        }

        XC2PLAOrTerm {
            input,
        }
    }

    /// Returns `true` if the `i`th AND term is used in this OR term
    pub fn get(&self, i: usize) -> bool {
        self.input[i / 8] & (1 << (i % 8)) != 0
    }

    /// Sets whether the `i`th AND term is used in this OR term
    pub fn set(&mut self, i: usize, val: bool) {
        if !val {
            self.input[i / 8] &=  !(1 << (i % 8));
        } else {
            self.input[i / 8] |=  1 << (i % 8);
        }
    }
}
