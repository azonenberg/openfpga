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

use std::marker::PhantomData;

#[derive(Hash, Debug)]
pub struct ObjPoolIndex<T> {
    i: usize,
    type_marker: PhantomData<T>
}

impl<T> Copy for ObjPoolIndex<T> { }

impl<T> Clone for ObjPoolIndex<T> {
    fn clone(&self) -> ObjPoolIndex<T> {
        *self
    }
}

impl<T> PartialEq for ObjPoolIndex<T> {
    fn eq(&self, other: &ObjPoolIndex<T>) -> bool {
        self.i == other.i
    }
}

impl<T> Eq for ObjPoolIndex<T> { }

pub struct ObjPool<T> {
    storage: Vec<T>
}

impl<T: Default> ObjPool<T> {
    pub fn new() -> ObjPool<T> {
        ObjPool {storage: Vec::new()}
    }

    pub fn alloc(&mut self) -> ObjPoolIndex<T> {
        let i = self.storage.len();
        let o = T::default();

        self.storage.push(o);

        ObjPoolIndex::<T> {i: i, type_marker: PhantomData}
    }

    pub fn get(&self, i: ObjPoolIndex<T>) -> &T {
        &self.storage[i.i]
    }

    pub fn get_mut(&mut self, i: ObjPoolIndex<T>) -> &mut T {
        &mut self.storage[i.i]
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[derive(Default)]
    struct ObjPoolTestObject {
        foo: u32
    }

    #[test]
    fn objpool_basic_works() {
        let mut pool = ObjPool::<ObjPoolTestObject>::new();
        let x = pool.alloc();
        let y = pool.alloc();
        {
            let o = pool.get_mut(x);
            o.foo = 123;
        }
        {
            let o = pool.get_mut(y);
            o.foo = 456;
        }
        let ox = pool.get(x);
        let oy = pool.get(y);
        assert_eq!(ox.foo, 123);
        assert_eq!(oy.foo, 456);
    }
}
