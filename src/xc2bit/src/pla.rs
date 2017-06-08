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

use *;

/// Represents one single AND term in the PLA. Each AND term can perform an AND function on any subset of its inputs
/// and the complement of those inputs. The index for each input is the corresponding ZIA row.
#[derive(Copy)]
pub struct XC2PLAAndTerm {
    /// Indicates whether a particular ZIA row output is a part of this AND term.
    ///
    /// `true` = part of and, `false` = not part of and
    pub input: [bool; INPUTS_PER_ANDTERM],
    /// Indicates whether the complement of a particular ZIA row output is a part of this AND term.
    ///
    /// `true` = part of and, `false` = not part of and
    pub input_b: [bool; INPUTS_PER_ANDTERM],
}

impl Clone for XC2PLAAndTerm {
    fn clone(&self) -> XC2PLAAndTerm {*self}
}

impl Default for XC2PLAAndTerm {
    /// Returns a "default" AND term. The default state is for none of the inputs to be selected.
    fn default() -> XC2PLAAndTerm {
        XC2PLAAndTerm {
            input: [false; INPUTS_PER_ANDTERM],
            input_b: [false; INPUTS_PER_ANDTERM],
        }
    }
}

/// Represents one single OR term in the PLA. Each OR term can perform an OR function on any subset of its inputs.
/// The index for each input is the index of the corresponding AND term in the same PLA.
#[derive(Copy)]
pub struct XC2PLAOrTerm {
    /// Indicates whether a particular PLA AND term is a part of this OR term.
    ///
    /// `true` = part of or, `false` = not part of or
    pub input: [bool; ANDTERMS_PER_FB],
}

impl Clone for XC2PLAOrTerm {
    fn clone(&self) -> XC2PLAOrTerm {*self}
}

impl Default for XC2PLAOrTerm {
    /// Returns a "default" OR term. The default state is for none of the inputs to be selected.
    fn default() -> XC2PLAOrTerm {
        XC2PLAOrTerm {
            input: [false; ANDTERMS_PER_FB],
        }
    }
}

/// Internal function that reads one single AND term from a block of fuses using logical fuse indexing
pub fn read_and_term_logical(fuses: &[bool], block_idx: usize, term_idx: usize) -> XC2PLAAndTerm {
    let mut input = [false; INPUTS_PER_ANDTERM];
    let mut input_b = [false; INPUTS_PER_ANDTERM];

    for i in 0..INPUTS_PER_ANDTERM {
        input[i]   = !fuses[block_idx + term_idx * INPUTS_PER_ANDTERM * 2 + i * 2 + 0];
        input_b[i] = !fuses[block_idx + term_idx * INPUTS_PER_ANDTERM * 2 + i * 2 + 1];
    }

    XC2PLAAndTerm {
        input: input,
        input_b: input_b,
    }
}

/// Internal function that reads one single OR term from a block of fuses using logical fuse indexing
pub fn read_or_term_logical(fuses: &[bool], block_idx: usize, term_idx: usize) -> XC2PLAOrTerm {
    let mut input = [false; ANDTERMS_PER_FB];

    for i in 0..ANDTERMS_PER_FB {
        input[i] = !fuses[block_idx + term_idx +i * MCS_PER_FB];
    }

    XC2PLAOrTerm {
        input: input,
    }
}
