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

use std::collections::HashMap;

extern crate serde_json;

#[derive(Serialize, Deserialize, Debug)]
pub struct YosysNetlist {
    pub creator: String,
    pub modules: HashMap<String, YosysNetlistModule>,
}

#[derive(Serialize, Deserialize, Debug)]
pub struct YosysNetlistModule {
    pub attributes: HashMap<String, serde_json::Value>,
    pub ports: HashMap<String, YosysNetlistPort>,
    pub cells: HashMap<String, YosysNetlistCell>,
    pub netnames: HashMap<String, YosysNetlistNetname>,
}

#[derive(Serialize, Deserialize, Debug)]
pub struct YosysNetlistPort {
    pub direction: String,
    pub bits: Vec<usize>,
}

#[derive(Serialize, Deserialize, Debug)]
pub struct YosysNetlistCell {
    pub hide_name: usize,
    #[serde(rename="type")]
    pub cell_type: String,
    pub parameters: HashMap<String, serde_json::Value>,
    pub attributes: HashMap<String, serde_json::Value>,
    pub port_directions: HashMap<String, String>,
    pub connections: HashMap<String, Vec<serde_json::Value>>,
}

#[derive(Serialize, Deserialize, Debug)]
pub struct YosysNetlistNetname {
    pub hide_name: usize,
    pub bits: Vec<usize>,
    pub attributes: HashMap<String, serde_json::Value>,
}

pub fn read_yosys_netlist(input: &[u8]) -> Result<YosysNetlist, serde_json::Error> {
    serde_json::from_slice(input)
}
