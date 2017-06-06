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

// PLA stuff

pub struct XC2PLAAndTerm {
    // true = part of and, false = not part of and
    pub input: [bool; 40],
    pub input_b: [bool; 40],
}

pub struct XC2PLAOrTerm {
    // true = part of or, false = not part of or
    pub input: [bool; 56],
}

pub fn read_and_term_logical(fuses: &[bool], block_idx: usize, term_idx: usize) -> XC2PLAAndTerm {
    let mut input = [false; 40];
    let mut input_b = [false; 40];

    for i in 0..40 {
        input[i]   = !fuses[block_idx + term_idx * 80 + i * 2 + 0];
        input_b[i] = !fuses[block_idx + term_idx * 80 + i * 2 + 1];
    }

    XC2PLAAndTerm {
        input: input,
        input_b: input_b,
    }
}

pub fn read_or_term_logical(fuses: &[bool], block_idx: usize, term_idx: usize) -> XC2PLAOrTerm {
    let mut input = [false; 56];

    for i in 0..56 {
        input[i] = !fuses[block_idx + i * 56 + term_idx];
    }

    XC2PLAOrTerm {
        input: input,
    }
}
