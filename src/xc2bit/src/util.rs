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

use std::collections::BTreeMap;

pub fn b2s(b: bool) -> &'static str {
    if b {"1"} else {"0"}
}

pub struct LinebreakSet {
    map: BTreeMap<usize, usize>,
}

impl LinebreakSet {
    pub fn new() -> Self {
        Self {
            map: BTreeMap::new(),
        }
    }

    pub fn add(&mut self, i: usize) {
        let new_val = self.map.get(&i).unwrap_or(&0) + 1;
        self.map.insert(i, new_val);
    }

    pub fn iter(&self) -> LinebreakSetIter {
        LinebreakSetIter {
            last_break_val: 0,
            last_break_rep: 0,
            set_iter: self.map.iter(),
        }
    }
}

pub struct LinebreakSetIter<'a> {
    last_break_val: usize,
    last_break_rep: usize,
    set_iter: ::std::collections::btree_map::Iter<'a, usize, usize>,
}

impl<'a> Iterator for LinebreakSetIter<'a> {
    type Item = usize;

    fn next(&mut self) -> Option<usize> {
        if self.last_break_rep == 0 {
            // Need to get another item now
            let next_item = self.set_iter.next();
            if next_item.is_none() {
                None
            } else {
                let (&k, &v) = next_item.unwrap();
                self.last_break_val = k;
                self.last_break_rep = v - 1;
                Some(k)
            }
        } else {
            self.last_break_rep -= 1;
            Some(self.last_break_val)
        }
    }
}
