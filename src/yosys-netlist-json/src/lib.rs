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
use std::io::Write;

#[macro_use]
extern crate serde_derive;
extern crate serde_json;

/// Legal values for the direction of a port on a module
#[derive(Copy, Clone, Serialize, Deserialize, Debug, Eq, PartialEq, Hash)]
pub enum PortDirection {
    #[serde(rename = "input")]
    Input,
    #[serde(rename = "output")]
    Output,
    #[serde(rename = "inout")]
    InOut,
}

/// Special constant bit values
#[derive(Copy, Clone, Serialize, Deserialize, Debug, Eq, PartialEq, Hash)]
pub enum SpecialBit {
    /// Constant 0
    #[serde(rename = "0")]
    _0,
    /// Constant 1
    #[serde(rename = "1")]
    _1,
    /// Constant X (invalid)
    #[serde(rename = "x")]
    X,
    /// Constant Z (tri-state)
    #[serde(rename = "z")]
    Z,
}

/// A number representing a single bit of a wire
#[derive(Clone, Serialize, Deserialize, Debug, Eq, PartialEq, Hash)]
#[serde(untagged)]
pub enum BitVal {
    /// An actual signal number
    N(usize),
    /// A special constant value
    S(SpecialBit)
}

/// The value of an attribute/parameter
#[derive(Clone, Serialize, Deserialize, Debug, Eq, PartialEq, Hash)]
#[serde(untagged)]
pub enum AttributeVal {
    /// Numeric attribute value
    N(usize),
    /// String attribute value
    S(String),
}

/// Represents an entire .json file used by Yosys
#[derive(Serialize, Deserialize, Debug, Default)]
pub struct Netlist {
    /// The program that created this file.
    #[serde(default)]
    pub creator: String,
    /// A map from module names to module objects contained in this .json file
    #[serde(default)]
    pub modules: HashMap<String, Module>,
}

/// Represents one module in the Yosys hierarchy
#[derive(Serialize, Deserialize, Debug, Default)]
pub struct Module {
    /// Module attributes
    #[serde(default)]
    pub attributes: HashMap<String, AttributeVal>,
    /// Module ports (interfaces to other modules)
    #[serde(default)]
    pub ports: HashMap<String, Port>,
    /// Module cells (objects inside this module)
    #[serde(default)]
    pub cells: HashMap<String, Cell>,
    /// Module netnames (names of wires in this module)
    #[serde(default)]
    pub netnames: HashMap<String, Netname>,
}

/// Represents a port on a module
#[derive(Serialize, Deserialize, Debug)]
pub struct Port {
    /// Port direction
    pub direction: PortDirection,
    /// Bit value(s) representing the wire(s) connected to this port
    pub bits: Vec<BitVal>,
}

/// Represents a cell in a module
#[derive(Serialize, Deserialize, Debug)]
pub struct Cell {
    /// Indicates an internal/auto-generated name that starts with `$`
    #[serde(default)]
    pub hide_name: usize,
    /// Name of the type of this cell
    #[serde(rename="type")]
    pub cell_type: String,
    /// Parameters specified on this cell
    #[serde(default)]
    pub parameters: HashMap<String, AttributeVal>,
    /// Attributes specified on this cell
    #[serde(default)]
    pub attributes: HashMap<String, AttributeVal>,
    /// The direction of the ports on this cell
    #[serde(default)]
    pub port_directions: HashMap<String, String>,
    /// Bit value(s) representing the wire(s) connected to the inputs/outputs of this cell
    pub connections: HashMap<String, Vec<BitVal>>,
}

/// Represents the name of a net in a module
#[derive(Serialize, Deserialize, Debug)]
pub struct Netname {
    /// Indicates an internal/auto-generated name that starts with `$`
    #[serde(default)]
    pub hide_name: usize,
    /// Bit value(s) that should be given this name
    pub bits: Vec<BitVal>,
    /// Attributes for this netname
    #[serde(default)]
    pub attributes: HashMap<String, AttributeVal>,
}

impl Netlist {
    /// Read netlist data from a slice containing the bytes from a Yosys .json file
    pub fn from_slice(input: &[u8]) -> Result<Netlist, serde_json::Error> {
        serde_json::from_slice(input)
    }

    /// Serialize to a String
    pub fn to_string(&self) -> Result<String, serde_json::Error> {
        serde_json::to_string(self)
    }

    /// Serialize to a writer
    pub fn to_writer(&self, writer: &mut Write) -> Result<(), serde_json::Error> {
        serde_json::to_writer(writer, self)
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn super_empty_json() {
        let result = Netlist::from_slice(br#"
            {}"#).unwrap();
        assert_eq!(result.creator, "");
        assert_eq!(result.modules.len(), 0);
    }

    #[test]
    fn empty_json() {
        let result = Netlist::from_slice(br#"
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
        let result = Netlist::from_slice(br#"
            {
              "modules": {
              }
            }"#).unwrap();
        assert_eq!(result.creator, "");
        assert_eq!(result.modules.len(), 0);
    }

    #[test]
    fn bit_values_test() {
        let result = Netlist::from_slice(br#"
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
            &vec![BitVal::S(SpecialBit::X), BitVal::N(0), BitVal::S(SpecialBit::Z), BitVal::N(234),
                BitVal::S(SpecialBit::_1), BitVal::S(SpecialBit::_0)]);
    }

    #[test]
    #[should_panic]
    fn invalid_bit_value_test() {
        Netlist::from_slice(br#"
            {
              "modules": {
                "mymodule": {
                  "cells": {
                    "mycell": {
                      "type": "celltype",
                      "connections": {
                        "IN": [ "w" ]
                      }
                    }
                  }
                }
              }
            }"#).unwrap();
    }

    #[test]
    fn attribute_value_test() {
        let result = Netlist::from_slice(br#"
            {
              "modules": {
                "mymodule": {
                  "cells": {
                    "mycell": {
                      "type": "celltype",
                      "parameters": {
                        "testparam": 123
                      },
                      "connections": {}
                    }
                  }
                }
              }
            }"#).unwrap();
        assert_eq!(result.modules.get("mymodule").unwrap().cells.get("mycell").unwrap()
            .parameters.get("testparam").unwrap(), &AttributeVal::N(123));
    }

    #[test]
    #[should_panic]
    fn invalid_attribute_value_test() {
        Netlist::from_slice(br#"
            {
              "modules": {
                "mymodule": {
                  "cells": {
                    "mycell": {
                      "type": "celltype",
                      "parameters": {
                        "testparam": [123]
                      },
                      "connections": {}
                    }
                  }
                }
              }
            }"#).unwrap();
    }
}
