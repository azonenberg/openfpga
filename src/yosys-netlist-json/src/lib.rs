/*
Copyright (c) 2017, Robert Ou <rqou@robertou.com> and contributors
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

#[macro_use]
extern crate serde_derive;
extern crate serde_json;

#[derive(Serialize, Deserialize, Debug)]
pub struct YosysNetlist {
    #[serde(default)]
    pub creator: String,
    #[serde(default)]
    pub modules: HashMap<String, YosysNetlistModule>,
}

#[derive(Serialize, Deserialize, Debug, Eq, PartialEq)]
pub enum YosysBitSpecial {
    #[serde(rename = "0")]
    _0,
    #[serde(rename = "1")]
    _1,
    #[serde(rename = "x")]
    X,
    #[serde(rename = "z")]
    Z,
}

#[derive(Serialize, Deserialize, Debug, Eq, PartialEq)]
pub enum YosysPortDirection {
    #[serde(rename = "input")]
    Input,
    #[serde(rename = "output")]
    Output,
    #[serde(rename = "inout")]
    InOut,
}

#[derive(Serialize, Deserialize, Debug, Eq, PartialEq)]
#[serde(untagged)]
pub enum YosysBit {
    N(usize),
    S(YosysBitSpecial)
}

#[derive(Serialize, Deserialize, Debug, Eq, PartialEq)]
#[serde(untagged)]
pub enum YosysAttribute {
    N(usize),
    S(String),
}

#[derive(Serialize, Deserialize, Debug)]
pub struct YosysNetlistModule {
    #[serde(default)]
    pub attributes: HashMap<String, YosysAttribute>,
    #[serde(default)]
    pub ports: HashMap<String, YosysNetlistPort>,
    #[serde(default)]
    pub cells: HashMap<String, YosysNetlistCell>,
    #[serde(default)]
    pub netnames: HashMap<String, YosysNetlistNetname>,
}

#[derive(Serialize, Deserialize, Debug)]
pub struct YosysNetlistPort {
    pub direction: YosysPortDirection,
    pub bits: Vec<YosysBit>,
}

#[derive(Serialize, Deserialize, Debug)]
pub struct YosysNetlistCell {
    #[serde(default)]
    pub hide_name: usize,
    #[serde(rename="type")]
    pub cell_type: String,
    #[serde(default)]
    pub parameters: HashMap<String, YosysAttribute>,
    #[serde(default)]
    pub attributes: HashMap<String, YosysAttribute>,
    #[serde(default)]
    pub port_directions: HashMap<String, String>,
    pub connections: HashMap<String, Vec<YosysBit>>,
}

#[derive(Serialize, Deserialize, Debug)]
pub struct YosysNetlistNetname {
    #[serde(default)]
    pub hide_name: usize,
    pub bits: Vec<YosysBit>,
    #[serde(default)]
    pub attributes: HashMap<String, YosysAttribute>,
}

pub fn read_yosys_netlist(input: &[u8]) -> Result<YosysNetlist, serde_json::Error> {
    serde_json::from_slice(input)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn super_empty_json() {
        let result = read_yosys_netlist(br#"
            {}"#).unwrap();
        assert_eq!(result.creator, "");
        assert_eq!(result.modules.len(), 0);
    }

    #[test]
    fn empty_json() {
        let result = read_yosys_netlist(br#"
            {
              "creator": "this is a test",
              "modules": {
              }
            }"#).unwrap();
        assert_eq!(result.creator, "this is a test");
        assert_eq!(result.modules.len(), 0);
    }

    #[test]
    fn empty_json_2() {
        let result = read_yosys_netlist(br#"
            {
              "modules": {
              }
            }"#).unwrap();
        assert_eq!(result.creator, "");
        assert_eq!(result.modules.len(), 0);
    }

    #[test]
    fn bit_values_test() {
        let result = read_yosys_netlist(br#"
            {
              "modules": {
                "mymodule": {
                  "cells": {
                    "mycell": {
                      "type": "celltype",
                      "connections": {
                        "IN": [ "x", 0, "z", 234, "1", "0" ]
                      }
                    }
                  }
                }
              }
            }"#).unwrap();
        assert_eq!(result.modules.get("mymodule").unwrap().cells.get("mycell").unwrap().connections.get("IN").unwrap(),
            &vec![YosysBit::S(YosysBitSpecial::X), YosysBit::N(0), YosysBit::S(YosysBitSpecial::Z), YosysBit::N(234),
                YosysBit::S(YosysBitSpecial::_1), YosysBit::S(YosysBitSpecial::_0)]);
    }
}
