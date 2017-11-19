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


use std::collections::{HashMap, HashSet};
use std::hash::{Hash, Hasher};
use std::iter::FromIterator;

extern crate xc2bit;
use self::xc2bit::*;

extern crate yosys_netlist_json;

use objpool::*;

#[derive(Debug)]
pub enum IntermediateGraphNodeVariant {
    AndTerm {
        inputs_true: Vec<ObjPoolIndex<IntermediateGraphNet>>,
        inputs_comp: Vec<ObjPoolIndex<IntermediateGraphNet>>,
        output: ObjPoolIndex<IntermediateGraphNet>,
    },
    OrTerm {
        inputs: Vec<ObjPoolIndex<IntermediateGraphNet>>,
        output: ObjPoolIndex<IntermediateGraphNet>,
    },
    Xor {
        orterm_input: Option<ObjPoolIndex<IntermediateGraphNet>>,
        andterm_input: Option<ObjPoolIndex<IntermediateGraphNet>>,
        invert_out: bool,
        output: ObjPoolIndex<IntermediateGraphNet>,
    },
    Reg {
        mode: XC2MCRegMode,
        clkinv: bool,
        clkddr: bool,
        init_state: bool,
        set_input: Option<ObjPoolIndex<IntermediateGraphNet>>,
        reset_input: Option<ObjPoolIndex<IntermediateGraphNet>>,
        ce_input: Option<ObjPoolIndex<IntermediateGraphNet>>,
        dt_input: ObjPoolIndex<IntermediateGraphNet>,
        clk_input: ObjPoolIndex<IntermediateGraphNet>,
        output: ObjPoolIndex<IntermediateGraphNet>,
    },
    BufgClk {
        input: ObjPoolIndex<IntermediateGraphNet>,
        output: ObjPoolIndex<IntermediateGraphNet>,
    },
    BufgGTS {
        input: ObjPoolIndex<IntermediateGraphNet>,
        output: ObjPoolIndex<IntermediateGraphNet>,
        invert: bool,
    },
    BufgGSR {
        input: ObjPoolIndex<IntermediateGraphNet>,
        output: ObjPoolIndex<IntermediateGraphNet>,
        invert: bool,
    },
    IOBuf {
        input: Option<ObjPoolIndex<IntermediateGraphNet>>,
        oe: Option<ObjPoolIndex<IntermediateGraphNet>>,
        output: Option<ObjPoolIndex<IntermediateGraphNet>>,
        schmitt_trigger: bool,
        termination_enabled: bool,
        slew_is_fast: bool,
        uses_data_gate: bool,
    },
    InBuf {
        output: ObjPoolIndex<IntermediateGraphNet>,
        schmitt_trigger: bool,
        termination_enabled: bool,
        uses_data_gate: bool,
    },
}

#[derive(Copy, Clone, PartialEq, Eq, Debug)]
pub struct RequestedLocation {
    pub fb: u32,
    pub i: Option<u32>,
}

impl RequestedLocation {
    fn parse_location(loc: Option<&str>) -> Result<Option<Self>, &'static str> {
        if loc.is_none() {
            return Ok(None);
        }

        let loc = loc.unwrap();

        if loc.starts_with("FB") {
            let loc_fb_i = loc.split("_").collect::<Vec<_>>();
            if loc_fb_i.len() == 1 {
                // FBn
                Ok(Some(RequestedLocation {
                    fb: loc_fb_i[0][2..].parse::<u32>().unwrap(),
                    i: None,
                }))
            } else if loc_fb_i.len() == 2 {
                if !loc_fb_i[1].starts_with("P") {
                    // FBn_i
                    Ok(Some(RequestedLocation {
                        fb: loc_fb_i[0][2..].parse::<u32>().unwrap(),
                        i: Some(loc_fb_i[1].parse::<u32>().unwrap()),
                    }))
                } else {
                    // FBn_Pi
                    Ok(Some(RequestedLocation {
                        fb: loc_fb_i[0][2..].parse::<u32>().unwrap(),
                        i: Some(loc_fb_i[1][1..].parse::<u32>().unwrap()),
                    }))
                }
            } else {
                Err("Malformed LOC constraint")
            }
        } else {
            // TODO: Pin names
            Err("Malformed LOC constraint")
        }
    }
}

#[derive(Debug)]
pub struct IntermediateGraphNode {
    pub variant: IntermediateGraphNodeVariant,
    pub name: String,
    pub location: Option<RequestedLocation>,
}

#[derive(Debug)]
pub struct IntermediateGraphNet {
    pub name: Option<String>,
    pub source: Option<(ObjPoolIndex<IntermediateGraphNode>, &'static str)>,
    pub sinks: Vec<(ObjPoolIndex<IntermediateGraphNode>, &'static str)>,
}

#[derive(Debug)]
pub struct IntermediateGraph {
    pub nodes: ObjPool<IntermediateGraphNode>,
    pub nets: ObjPool<IntermediateGraphNet>,
    pub vdd_net: ObjPoolIndex<IntermediateGraphNet>,
    pub vss_net: ObjPoolIndex<IntermediateGraphNet>,
}

impl IntermediateGraph {
    pub fn from_yosys_netlist(yosys_net: &yosys_netlist_json::Netlist) -> Result<Self, &'static str> {
        let mut top_module_name = "";
        let mut top_module_found = false;
        for (module_name, module) in &yosys_net.modules {
            if let Some(top_attr) = module.attributes.get("top") {
                if let &yosys_netlist_json::AttributeVal::N(n) = top_attr {
                    if n != 0 {
                        // Claims to be a top-level module
                        if top_module_found {
                            return Err("Multiple top-level modules found in netlist");
                        }

                        top_module_found = true;
                        top_module_name = module_name;
                    }
                }
            }
        }

        if !top_module_found {
            return Err("No top-level modules found in netlist");
        }
        let top_module = yosys_net.modules.get(top_module_name).unwrap();

        // First, we must process all nets
        let mut nets = ObjPool::new();
        // These "magic" nets correspond to a constant 1/0 signal
        let vdd_net = nets.insert(IntermediateGraphNet {
            name: Some(String::from("<internal virtual Vdd net>")),
            source: None,
            sinks: Vec::new(),
        });
        let vss_net = nets.insert(IntermediateGraphNet {
            name: Some(String::from("<internal virtual Vss net>")),
            source: None,
            sinks: Vec::new(),
        });

        // This maps from a Yosys net number to an internal net number
        let mut net_map: HashMap<usize, ObjPoolIndex<IntermediateGraphNet>> = HashMap::new();

        // Keep track of module ports
        let mut module_ports = HashSet::new();
        for (_, port) in &top_module.ports {
            for yosys_edge_idx in &port.bits {
                if let &yosys_netlist_json::BitVal::N(n) = yosys_edge_idx {
                    module_ports.insert(n);
                }
            }
        }

        // Process in order so that the results do not differ across runs
        let mut cell_names = top_module.cells.keys().collect::<Vec<_>>();
        cell_names.sort();

        // Cells can refer to a net, so loop through these as well
        for &cell_name in &cell_names {
            let cell = &top_module.cells[cell_name];
            let mut connection_names = cell.connections.keys().collect::<Vec<_>>();
            connection_names.sort();
            for connection_name in connection_names {
                let connection_vec = &cell.connections[connection_name];
                for connection in connection_vec.iter() {
                    if let &yosys_netlist_json::BitVal::N(yosys_edge_idx) = connection {
                        // Don't create nets for the pad side of io buffers
                        if module_ports.contains(&yosys_edge_idx) {
                            continue;
                        }

                        if net_map.get(&yosys_edge_idx).is_none() {
                            // Need to add a new one
                            let our_edge_idx = nets.insert(IntermediateGraphNet {
                                name: None,
                                source: None,
                                sinks: Vec::new(),
                            });
                            net_map.insert(yosys_edge_idx, our_edge_idx);
                        }
                    }
                }
            }
        }

        // Now we want to loop through netnames and *) insert any new names (dangling?) and *) assign human-readable
        // net names
        let mut netname_names = top_module.netnames.keys().collect::<Vec<_>>();
        netname_names.sort();
        for netname_name in netname_names {
            let netname_obj = &top_module.netnames[netname_name];
            for yosys_edge_idx in &netname_obj.bits {
                if let &yosys_netlist_json::BitVal::N(yosys_edge_idx) = yosys_edge_idx {
                    // Don't create nets for the pad side of io buffers
                    if module_ports.contains(&yosys_edge_idx) {
                        continue;
                    }

                    if net_map.get(&yosys_edge_idx).is_none() {
                        // Need to add a new one
                        let our_edge_idx = nets.insert(IntermediateGraphNet {
                            name: Some(netname_name.to_owned()),
                            source: None,
                            sinks: Vec::new(),
                        });
                        net_map.insert(yosys_edge_idx, our_edge_idx);
                    } else {
                        // Naming an existing one
                        let existing_net_our_idx = net_map.get(&yosys_edge_idx).unwrap();
                        let existing_net = nets.get_mut(*existing_net_our_idx);
                        existing_net.name = Some(netname_name.to_owned());
                    }
                }
            }
        }

        // Now we can actually process objects
        let mut nodes = ObjPool::new();

        let bitval_to_net = |bitval| {
            match bitval {
                yosys_netlist_json::BitVal::N(n) => Ok(*net_map.get(&n).unwrap()),
                yosys_netlist_json::BitVal::S(yosys_netlist_json::SpecialBit::_0) => Ok(vss_net),
                yosys_netlist_json::BitVal::S(yosys_netlist_json::SpecialBit::_1) => Ok(vdd_net),
                // We should never see an x/z in our processing
                _ => Err("Illegal bit value in JSON"),
            }
        };

        // and finally process cells
        for &cell_name in &cell_names {
            let cell_obj = &top_module.cells[cell_name];

            // Helper to retrieve a parameter
            let numeric_param = |name: &str| {
                let param_option = cell_obj.parameters.get(name);
                if param_option.is_none() {
                    return Err("required cell parameter is missing");
                }
                if let &yosys_netlist_json::AttributeVal::N(n) = param_option.unwrap() {
                    return Ok(n)
                } else {
                    return Err("cell parameter is not a number");
                };
            };

            // Helper to retrieve an optional string parameter
            let optional_string_param = |name: &str| -> Result<Option<&str>, &'static str> {
                let param_option = cell_obj.parameters.get(name);
                if param_option.is_none() {
                    return Ok(None);
                }
                if let &yosys_netlist_json::AttributeVal::S(ref s) = param_option.unwrap() {
                    return Ok(Some(s))
                } else {
                    return Err("cell parameter is not a string");
                };
            };

            // Helper to retrieve a single net that is definitely required
            let single_required_connection = |name : &str| {
                let conn_obj = cell_obj.connections.get(name);
                if conn_obj.is_none() {
                    return Err("cell is missing required connection");
                }
                let conn_obj = conn_obj.unwrap();
                if conn_obj.len() != 1 {
                    return Err("cell connection has too many nets")
                }
                return Ok(bitval_to_net(conn_obj[0].clone())?);
            };

            // Helper to retrieve a single net that is optional
            let single_optional_connection = |name: &str| {
                let conn_obj = cell_obj.connections.get(name);
                if conn_obj.is_none() {
                    return Ok(None);
                }
                let conn_obj = conn_obj.unwrap();
                if conn_obj.len() != 1 {
                    return Err("cell connection has too many nets")
                }
                return Ok(Some(bitval_to_net(conn_obj[0].clone())?));
            };

            // Helper to retrieve an array of nets that is required
            let multiple_required_connection = |name : &str| {
                let conn_obj = cell_obj.connections.get(name);
                if conn_obj.is_none() {
                    return Err("cell is missing required connection");
                }
                let mut result = Vec::new();
                for x in conn_obj.unwrap() {
                    result.push(bitval_to_net(x.clone())?);
                }
                return Ok(result);
            };

            match cell_obj.cell_type.as_ref() {
                "IOBUFE"  => {
                    // FIXME: Check that IO goes to a module port

                    nodes.insert(IntermediateGraphNode {
                        name: cell_name.to_owned(),
                        variant: IntermediateGraphNodeVariant::IOBuf {
                            input: single_optional_connection("I")?,
                            oe: single_optional_connection("E")?,
                            output: single_optional_connection("O")?,
                            // TODO
                            schmitt_trigger: false,
                            termination_enabled: false,
                            slew_is_fast: false,
                            uses_data_gate: false,
                        },
                        location: RequestedLocation::parse_location(optional_string_param("LOC")?)?,
                    });
                },
                "IBUF" => {
                    // FIXME: Check that IO goes to a module port

                    nodes.insert(IntermediateGraphNode {
                        name: cell_name.to_owned(),
                        variant: IntermediateGraphNodeVariant::InBuf {
                            output: single_required_connection("O")?,
                            // TODO
                            schmitt_trigger: false,
                            termination_enabled: false,
                            uses_data_gate: false,
                        },
                        location: RequestedLocation::parse_location(optional_string_param("LOC")?)?,
                    });
                },
                "ANDTERM" => {
                    let num_true_inputs = numeric_param("TRUE_INP")?;
                    let num_comp_inputs = numeric_param("COMP_INP")?;

                    let inputs_true = multiple_required_connection("IN")?;
                    let inputs_comp = multiple_required_connection("IN_B")?;

                    if num_true_inputs != inputs_true.len() || num_comp_inputs != inputs_comp.len() {
                        return Err("ANDTERM cell has a mismatched number of inputs");
                    }

                    nodes.insert(IntermediateGraphNode {
                        name: cell_name.to_owned(),
                        variant: IntermediateGraphNodeVariant::AndTerm {
                            inputs_true,
                            inputs_comp,
                            output: single_required_connection("OUT")?,
                        },
                        location: RequestedLocation::parse_location(optional_string_param("LOC")?)?,
                    });
                },
                "ORTERM" => {
                    let num_inputs = numeric_param("WIDTH")?;

                    let inputs = multiple_required_connection("IN")?;

                    if num_inputs != inputs.len() {
                        return Err("ORTERM cell has a mismatched number of inputs");
                    }

                    nodes.insert(IntermediateGraphNode {
                        name: cell_name.to_owned(),
                        variant: IntermediateGraphNodeVariant::OrTerm {
                            inputs,
                            output: single_required_connection("OUT")?,
                        },
                        location: RequestedLocation::parse_location(optional_string_param("LOC")?)?,
                    });
                },
                "MACROCELL_XOR" => {
                    nodes.insert(IntermediateGraphNode {
                        name: cell_name.to_owned(),
                        variant: IntermediateGraphNodeVariant::Xor {
                            andterm_input: single_optional_connection("IN_PTC")?,
                            orterm_input: single_optional_connection("IN_ORTERM")?,
                            invert_out: numeric_param("INVERT_OUT")? != 0,
                            output: single_required_connection("OUT")?,
                        },
                        location: RequestedLocation::parse_location(optional_string_param("LOC")?)?,
                    });
                },
                "BUFG" => {
                    nodes.insert(IntermediateGraphNode {
                        name: cell_name.to_owned(),
                        variant: IntermediateGraphNodeVariant::BufgClk {
                            input: single_required_connection("I")?,
                            output: single_required_connection("O")?,
                        },
                        location: RequestedLocation::parse_location(optional_string_param("LOC")?)?,
                    });
                },
                "BUFGTS" => {
                    nodes.insert(IntermediateGraphNode {
                        name: cell_name.to_owned(),
                        variant: IntermediateGraphNodeVariant::BufgGTS {
                            input: single_required_connection("I")?,
                            output: single_required_connection("O")?,
                            invert: numeric_param("INVERT")? != 0,
                        },
                        location: RequestedLocation::parse_location(optional_string_param("LOC")?)?,
                    });
                },
                "BUFGSR" => {
                    nodes.insert(IntermediateGraphNode {
                        name: cell_name.to_owned(),
                        variant: IntermediateGraphNodeVariant::BufgGSR {
                            input: single_required_connection("I")?,
                            output: single_required_connection("O")?,
                            invert: numeric_param("INVERT")? != 0,
                        },
                        location: RequestedLocation::parse_location(optional_string_param("LOC")?)?,
                    });
                },
                "FDCP" | "FDCP_N" | "FDDCP" |
                "LDCP" | "LDCP_N" |
                "FTCP" | "FTCP_N" | "FTDCP" |
                "FDCPE" | "FDCPE_N" | "FDDCPE" => {
                    let mode = match cell_obj.cell_type.as_ref() {
                        "FDCP" | "FDCP_N" | "FDDCP"     => XC2MCRegMode::DFF,
                        "LDCP" | "LDCP_N"               => XC2MCRegMode::LATCH,
                        "FTCP" | "FTCP_N" | "FTDCP"     => XC2MCRegMode::TFF,
                        "FDCPE" | "FDCPE_N" | "FDDCPE"  => XC2MCRegMode::DFFCE,
                        _ => unreachable!()
                    };

                    let clkinv = match cell_obj.cell_type.as_ref() {
                        "FDCP_N" | "LDCP_N" | "FTCP_N" | "FDCPE_N" => true,
                        _ => false,
                    };

                    let clkddr = match cell_obj.cell_type.as_ref() {
                        "FDDCP" | "FTDCP" | "FDDCPE" => true,
                        _ => false,
                    };

                    let mut ce_input = None;
                    if mode == XC2MCRegMode::DFFCE {
                        ce_input = Some(single_required_connection("CE")?);
                    }

                    let dt_name = if mode == XC2MCRegMode::TFF {"T"} else {"D"};
                    let clk_name = if mode == XC2MCRegMode::LATCH {"G"} else {"C"};

                    nodes.insert(IntermediateGraphNode {
                        name: cell_name.to_owned(),
                        variant: IntermediateGraphNodeVariant::Reg {
                            mode,
                            clkinv,
                            clkddr,
                            init_state: numeric_param("INIT")? != 0,
                            set_input: single_optional_connection("PRE")?,
                            reset_input: single_optional_connection("CLR")?,
                            ce_input,
                            dt_input: single_required_connection(dt_name)?,
                            clk_input: single_required_connection(clk_name)?,
                            output: single_required_connection("Q")?,
                        },
                        location: RequestedLocation::parse_location(optional_string_param("LOC")?)?,
                    });
                }
                _ => return Err("unsupported cell type")
            }
        }

        // Now that we are done processing, hook up sources/sinks in the edges

        // This helper is used to check if a net already has a source and raise an error if it does
        let set_net_source = |nets: &mut ObjPool<IntermediateGraphNet>, output, x| {
            let output_net = nets.get_mut(output);
            if output_net.source.is_some() {
                return Err("multiple drivers for net");
            }
            output_net.source = Some(x);
            Ok(())
        };

        for node_idx in nodes.iter_idx() {
            let node = nodes.get(node_idx);
            match node.variant {
                IntermediateGraphNodeVariant::AndTerm{ref inputs_true, ref inputs_comp, output} => {
                    for &input in inputs_true {
                        nets.get_mut(input).sinks.push((node_idx, "IN"));
                    }
                    for &input in inputs_comp {
                        nets.get_mut(input).sinks.push((node_idx, "IN"));
                    }
                    set_net_source(&mut nets, output, (node_idx, "OUT"))?;
                },
                IntermediateGraphNodeVariant::OrTerm{ref inputs, output} => {
                    for &input in inputs {
                        nets.get_mut(input).sinks.push((node_idx, "IN"));
                    }
                    set_net_source(&mut nets, output, (node_idx, "OUT"))?;
                },
                IntermediateGraphNodeVariant::Xor{orterm_input, andterm_input, output, ..} => {
                    if orterm_input.is_some() {
                        nets.get_mut(orterm_input.unwrap()).sinks.push((node_idx, "IN_ORTERM"));
                    }
                    if andterm_input.is_some() {
                        nets.get_mut(andterm_input.unwrap()).sinks.push((node_idx, "IN_PTC"));
                    }
                    set_net_source(&mut nets, output, (node_idx, "OUT"))?;
                },
                IntermediateGraphNodeVariant::Reg{set_input, reset_input, ce_input, dt_input, clk_input, output, ..} => {
                    if set_input.is_some() {
                        nets.get_mut(set_input.unwrap()).sinks.push((node_idx, "S"));
                    }
                    if reset_input.is_some() {
                        nets.get_mut(reset_input.unwrap()).sinks.push((node_idx, "R"));
                    }
                    if ce_input.is_some() {
                        nets.get_mut(ce_input.unwrap()).sinks.push((node_idx, "CE"));
                    }
                    nets.get_mut(dt_input).sinks.push((node_idx, "D/T"));
                    nets.get_mut(clk_input).sinks.push((node_idx, "CLK"));
                    set_net_source(&mut nets, output, (node_idx, "Q"))?;
                },
                IntermediateGraphNodeVariant::BufgClk{input, output, ..} |
                IntermediateGraphNodeVariant::BufgGTS{input, output, ..} |
                IntermediateGraphNodeVariant::BufgGSR{input, output, ..} => {
                    nets.get_mut(input).sinks.push((node_idx, "I"));
                    set_net_source(&mut nets, output, (node_idx, "O"))?;
                },
                IntermediateGraphNodeVariant::IOBuf{input, oe, output, ..} => {
                    if input.is_some() {
                        nets.get_mut(input.unwrap()).sinks.push((node_idx, "I"));
                    }
                    if oe.is_some() {
                        nets.get_mut(oe.unwrap()).sinks.push((node_idx, "E"));
                    }
                    if output.is_some() {
                        set_net_source(&mut nets, output.unwrap(), (node_idx, "O"))?;
                    }
                },
                IntermediateGraphNodeVariant::InBuf{output, ..} => {
                    set_net_source(&mut nets, output, (node_idx, "O"))?;
                },
            }
        }

        // Check for undriven nets
        for net_idx in nets.iter_idx() {
            if net_idx == vdd_net || net_idx == vss_net {
                continue;
            }

            let net = nets.get(net_idx);
            if net.source.is_none() {
                return Err("net without driver");
            }
        }

        Ok(Self {
            nodes,
            nets,
            vdd_net,
            vss_net,
        })
    }

    fn gather_macrocells(&self) -> Vec<ObjPoolIndex<IntermediateGraphNode>> {
        let mut ret = Vec::new();
        let mut encountered_xors = HashSet::new();

        // XXX note: The types of macrocells _must_ be searched in this order (or at least unregistered IBUFs must be
        // last). This property is relied on by the greedy initial placement algorithm.

        // First iteration: Find IOBUFE
        for node_idx in self.nodes.iter_idx() {
            let node = self.nodes.get(node_idx);

            if let IntermediateGraphNodeVariant::IOBuf{input, ..} = node.variant {
                ret.push(node_idx);

                if input.is_some() {
                    let input = input.unwrap();

                    let source_node_idx = self.nets.get(input).source.unwrap().0;
                    let source_node = self.nodes.get(source_node_idx);
                    if let IntermediateGraphNodeVariant::Xor{..} = source_node.variant {
                        // Combinatorial output
                        encountered_xors.insert(source_node_idx);
                    } else if let IntermediateGraphNodeVariant::Reg{dt_input, ..} = source_node.variant {
                        // Registered output, look at the input into the register
                        let source_node_idx = self.nets.get(dt_input).source.unwrap().0;
                        let source_node = self.nodes.get(source_node_idx);
                        if let IntermediateGraphNodeVariant::Xor{..} = source_node.variant {
                            encountered_xors.insert(source_node_idx);
                        } else if let IntermediateGraphNodeVariant::IOBuf{..} = source_node.variant {
                            if node_idx != source_node_idx {
                                // Trying to go from a different pin into the direct input path of this pin
                                panic!("mismatched graph node types");
                            }
                            // Otherwise ignore this for now. This is a bit strange, but possible.
                        } else {
                            panic!("mismatched graph node types");
                        }
                    } else {
                        panic!("mismatched graph node types");
                    }
                }
            }
        }

        // Second iteration: Find buried macrocells
        for node_idx in self.nodes.iter_idx() {
            let node = self.nodes.get(node_idx);

            if let IntermediateGraphNodeVariant::Xor{output, ..} = node.variant {
                if encountered_xors.contains(&node_idx) {
                    continue;
                }

                let mut maybe_reg_index = None;
                for &(sink_node_idx, _) in &self.nets.get(output).sinks {
                    let sink_node = self.nodes.get(sink_node_idx);

                    if let IntermediateGraphNodeVariant::Reg{..} = sink_node.variant {
                        maybe_reg_index = Some(sink_node_idx);
                    }
                }

                if maybe_reg_index.is_none() {
                    // Buried combinatorial
                    ret.push(node_idx);
                } else {
                    // Buried register
                    ret.push(maybe_reg_index.unwrap());
                }
            }
        }

        // Third iteration: Find registered IBUF
        for node_idx in self.nodes.iter_idx() {
            let node = self.nodes.get(node_idx);

            if let IntermediateGraphNodeVariant::InBuf{output, ..} = node.variant {
                let mut maybe_reg_index = None;
                for &(sink_node_idx, _) in &self.nets.get(output).sinks {
                    let sink_node = self.nodes.get(sink_node_idx);

                    if let IntermediateGraphNodeVariant::Reg{..} = sink_node.variant {
                        maybe_reg_index = Some(sink_node_idx);
                    }
                }

                if maybe_reg_index.is_some() {
                    ret.push(node_idx);
                }
            }
        }

        // Fourth iteration: Find unregistered IBUF
        for node_idx in self.nodes.iter_idx() {
            let node = self.nodes.get(node_idx);

            if let IntermediateGraphNodeVariant::InBuf{output, ..} = node.variant {
                let mut maybe_reg_index = None;
                for &(sink_node_idx, _) in &self.nets.get(output).sinks {
                    let sink_node = self.nodes.get(sink_node_idx);

                    if let IntermediateGraphNodeVariant::Reg{..} = sink_node.variant {
                        maybe_reg_index = Some(sink_node_idx);
                    }
                }

                if maybe_reg_index.is_none() {
                    ret.push(node_idx);
                }
            }
        }

        ret
    }
}

#[derive(Copy, Clone, PartialEq, Eq, Hash, Debug)]
pub struct AssignedLocationInner {
    pub fb: u32,
    pub i: u32,
}
pub type AssignedLocation = Option<AssignedLocationInner>;

#[derive(Copy, Clone, PartialEq, Eq, Hash, Debug)]
pub enum InputGraphIOInputType {
    Reg,
    Xor,
}

#[derive(Copy, Clone, PartialEq, Eq, Hash, Debug)]
pub enum InputGraphIOOEType {
    PTerm(ObjPoolIndex<InputGraphPTerm>),
    GTS(ObjPoolIndex<InputGraphBufgGTS>),
    OpenDrain,
}

#[derive(Clone, Debug)]
pub struct InputGraphIOBuf {
    pub input: Option<InputGraphIOInputType>,
    pub oe: Option<InputGraphIOOEType>,
    pub schmitt_trigger: bool,
    pub termination_enabled: bool,
    pub slew_is_fast: bool,
    pub uses_data_gate: bool,
}

#[derive(Copy, Clone, PartialEq, Eq, Hash, Debug)]
pub enum InputGraphRegRSType {
    PTerm(ObjPoolIndex<InputGraphPTerm>),
    GSR(ObjPoolIndex<InputGraphBufgGSR>),
}

#[derive(Copy, Clone, PartialEq, Eq, Hash, Debug)]
pub enum InputGraphRegInputType {
    Pin,
    Xor,
}

#[derive(Copy, Clone, PartialEq, Eq, Hash, Debug)]
pub enum InputGraphRegClockType {
    PTerm(ObjPoolIndex<InputGraphPTerm>),
    GCK(ObjPoolIndex<InputGraphBufgClk>),
}

#[derive(Clone, Debug)]
pub struct InputGraphReg {
    pub mode: XC2MCRegMode,
    pub clkinv: bool,
    pub clkddr: bool,
    pub init_state: bool,
    pub set_input: Option<InputGraphRegRSType>,
    pub reset_input: Option<InputGraphRegRSType>,
    pub ce_input: Option<ObjPoolIndex<InputGraphPTerm>>,
    pub dt_input: InputGraphRegInputType,
    pub clk_input: InputGraphRegClockType,
}

#[derive(Clone, Debug)]
pub struct InputGraphXor {
    pub orterm_inputs: Vec<ObjPoolIndex<InputGraphPTerm>>,
    pub andterm_input: Option<ObjPoolIndex<InputGraphPTerm>>,
    pub invert_out: bool,
}

#[derive(Clone, Debug)]
pub struct InputGraphMacrocell {
    pub loc: AssignedLocation,
    pub name: String,
    pub requested_loc: Option<RequestedLocation>,
    pub io_bits: Option<InputGraphIOBuf>,
    pub reg_bits: Option<InputGraphReg>,
    pub xor_bits: Option<InputGraphXor>,
    pub io_feedback_used: bool,
    pub reg_feedback_used: bool,
    pub xor_feedback_used: bool,
}

// BuriedComb is compatible with PinInputUnreg and PinInputReg.
// BuriedReg is compatible with PinInputUnreg as long as has_comb_fb is false.
#[derive(Copy, Clone, PartialEq, Eq, Hash, Debug)]
pub enum InputGraphMacrocellType {
    PinOutput,
    PinInputUnreg,
    PinInputReg,
    BuriedComb,
    BuriedReg,
}

#[derive(Copy, Clone, PartialEq, Eq, Hash, Debug)]
pub enum InputGraphPTermInputType {
    Reg,
    Xor,
    Pin,
}

pub type InputGraphPTermInput = (InputGraphPTermInputType, ObjPoolIndex<InputGraphMacrocell>);

#[derive(Clone, Debug)]
pub struct InputGraphPTerm {
    pub loc: AssignedLocation,
    pub name: String,
    pub requested_loc: Option<RequestedLocation>,
    pub inputs_true: Vec<InputGraphPTermInput>,
    pub inputs_comp: Vec<InputGraphPTermInput>,
}

#[derive(Clone, Debug)]
pub struct InputGraphBufgClk {
    pub loc: AssignedLocation,
    pub name: String,
    pub requested_loc: Option<RequestedLocation>,
    pub input: ObjPoolIndex<InputGraphMacrocell>,
}

#[derive(Clone, Debug)]
pub struct InputGraphBufgGTS {
    pub loc: AssignedLocation,
    pub name: String,
    pub requested_loc: Option<RequestedLocation>,
    pub input: ObjPoolIndex<InputGraphMacrocell>,
    pub invert: bool,
}

#[derive(Clone, Debug)]
pub struct InputGraphBufgGSR {
    pub loc: AssignedLocation,
    pub name: String,
    pub requested_loc: Option<RequestedLocation>,
    pub input: ObjPoolIndex<InputGraphMacrocell>,
    pub invert: bool,
}

#[derive(Debug)]
pub struct InputGraph {
    pub mcs: ObjPool<InputGraphMacrocell>,
    pub pterms: ObjPool<InputGraphPTerm>,
    pub bufg_clks: ObjPool<InputGraphBufgClk>,
    pub bufg_gts: ObjPool<InputGraphBufgGTS>,
    pub bufg_gsr: ObjPool<InputGraphBufgGSR>,
}

#[derive(Copy, Clone, PartialEq, Eq, Hash, Debug)]
enum InputGraphAnyPoolIdx {
    Macrocell(ObjPoolIndex<InputGraphMacrocell>),
    PTerm(ObjPoolIndex<InputGraphPTerm>),
    BufgClk(ObjPoolIndex<InputGraphBufgClk>),
    BufgGTS(ObjPoolIndex<InputGraphBufgGTS>),
    BufgGSR(ObjPoolIndex<InputGraphBufgGSR>),
}

impl InputGraphMacrocell {
    pub fn get_type(&self) -> InputGraphMacrocellType {
        if self.io_bits.is_some() {
            if self.io_bits.as_ref().unwrap().input.is_some() {
                InputGraphMacrocellType::PinOutput
            } else {
                if self.reg_bits.is_some() {
                    InputGraphMacrocellType::PinInputReg
                } else {
                    InputGraphMacrocellType::PinInputUnreg
                }
            }
        } else {
            if self.reg_bits.is_some() {
                InputGraphMacrocellType::BuriedReg
            } else {
                InputGraphMacrocellType::BuriedComb
            }
        }
    }
}

impl PartialEq for InputGraphPTerm {
    // WARNING WARNING does not check loc/name/requested_loc. Only checks if the inputs are the same
    fn eq(&self, other: &InputGraphPTerm) -> bool {
        let a_inp_true_hash: HashSet<(InputGraphPTermInputType, ObjPoolIndex<InputGraphMacrocell>)> =
            HashSet::from_iter(self.inputs_true.iter().cloned());
        let a_inp_comp_hash: HashSet<(InputGraphPTermInputType, ObjPoolIndex<InputGraphMacrocell>)> =
            HashSet::from_iter(self.inputs_comp.iter().cloned());
        let b_inp_true_hash: HashSet<(InputGraphPTermInputType, ObjPoolIndex<InputGraphMacrocell>)> =
            HashSet::from_iter(other.inputs_true.iter().cloned());
        let b_inp_comp_hash: HashSet<(InputGraphPTermInputType, ObjPoolIndex<InputGraphMacrocell>)> =
            HashSet::from_iter(other.inputs_comp.iter().cloned());

        a_inp_true_hash == b_inp_true_hash && a_inp_comp_hash == b_inp_comp_hash
    }
}
impl Eq for InputGraphPTerm { }

impl Hash for InputGraphPTerm {
    // WARNING WARNING assumes that there are no duplicates in the inputs (why would there be?)
    fn hash<H: Hasher>(&self, state: &mut H) {
        for x in &self.inputs_true {
            x.hash(state);
        }
        for x in &self.inputs_comp {
            x.hash(state);
        }
    }
}

impl InputGraph {
    pub fn from_intermed_graph(g: &IntermediateGraph) -> Result<Self, &'static str> {
        // TODO: Implement location checking

        let mut mcs = ObjPool::new();
        let mut pterms = ObjPool::new();
        let mut bufg_clks = ObjPool::new();
        let mut bufg_gts = ObjPool::new();
        let mut bufg_gsr = ObjPool::new();

        // These are used to map nodes in the intermediate graph to nodes in the new graph
        let mut mcs_map = HashMap::new();
        let mut pterms_map = HashMap::new();
        let mut bufg_clks_map = HashMap::new();
        let mut bufg_gts_map = HashMap::new();
        let mut bufg_gsr_map = HashMap::new();

        // This is for sanity checking to make sure the entire input gets consumed
        // FIXME: HashSets are not necessarily the most efficient data structure here
        let mut consumed_inputs = HashSet::new();

        // The first step is to invoke the "old" macrocell-gathering function so that:
        // * we can generate our macrocells in the "correct" order
        // * we can create all the needed macrocell objects ahead of time to break the cycle of references
        let gathered_mcs = g.gather_macrocells();

        // Now we pre-create all the macrocell objects
        for i in 0..gathered_mcs.len() {
            let dummy_mc = InputGraphMacrocell {
                loc: None,
                name: String::from(""),
                requested_loc: None,
                io_bits: None,
                reg_bits: None,
                xor_bits: None,
                io_feedback_used: false,
                reg_feedback_used: false,
                xor_feedback_used: false,
            };

            let newg_idx = mcs.insert(dummy_mc);
            let oldg_idx = gathered_mcs[i];
            mcs_map.insert(oldg_idx, newg_idx);
        }

        #[allow(non_camel_case_types)]
        struct process_one_intermed_node_state<'a> {
            g: &'a IntermediateGraph,

            mcs: &'a mut ObjPool<InputGraphMacrocell>,
            pterms: &'a mut ObjPool<InputGraphPTerm>,
            bufg_clks: &'a mut ObjPool<InputGraphBufgClk>,
            bufg_gts: &'a mut ObjPool<InputGraphBufgGTS>,
            bufg_gsr: &'a mut ObjPool<InputGraphBufgGSR>,

            mcs_map: &'a mut HashMap<ObjPoolIndex<IntermediateGraphNode>, ObjPoolIndex<InputGraphMacrocell>>,
            pterms_map: &'a mut HashMap<ObjPoolIndex<IntermediateGraphNode>, ObjPoolIndex<InputGraphPTerm>>,
            bufg_clks_map: &'a mut HashMap<ObjPoolIndex<IntermediateGraphNode>, ObjPoolIndex<InputGraphBufgClk>>,
            bufg_gts_map: &'a mut HashMap<ObjPoolIndex<IntermediateGraphNode>, ObjPoolIndex<InputGraphBufgGTS>>,
            bufg_gsr_map: &'a mut HashMap<ObjPoolIndex<IntermediateGraphNode>, ObjPoolIndex<InputGraphBufgGSR>>,

            consumed_inputs: &'a mut HashSet<usize>,
        }

        fn process_one_intermed_node<'a>(s: &mut process_one_intermed_node_state<'a>,
            n_idx: ObjPoolIndex<IntermediateGraphNode>) -> Result<InputGraphAnyPoolIdx, &'static str> {

            let n = s.g.nodes.get(n_idx);

            if s.consumed_inputs.contains(&n_idx.get_raw_i()) {
                // We're already here, but what are we?
                return Ok(match n.variant {
                    IntermediateGraphNodeVariant::IOBuf{..} |
                    IntermediateGraphNodeVariant::InBuf{..} |
                    IntermediateGraphNodeVariant::Reg{..} |
                    IntermediateGraphNodeVariant::Xor{..} |
                    IntermediateGraphNodeVariant::OrTerm{..} =>
                        InputGraphAnyPoolIdx::Macrocell(*s.mcs_map.get(&n_idx).unwrap()),
                    IntermediateGraphNodeVariant::AndTerm{..} =>
                        InputGraphAnyPoolIdx::PTerm(*s.pterms_map.get(&n_idx).unwrap()),
                    IntermediateGraphNodeVariant::BufgClk{..} =>
                        InputGraphAnyPoolIdx::BufgClk(*s.bufg_clks_map.get(&n_idx).unwrap()),
                    IntermediateGraphNodeVariant::BufgGTS{..} =>
                        InputGraphAnyPoolIdx::BufgGTS(*s.bufg_gts_map.get(&n_idx).unwrap()),
                    IntermediateGraphNodeVariant::BufgGSR{..} =>
                        InputGraphAnyPoolIdx::BufgGSR(*s.bufg_gsr_map.get(&n_idx).unwrap()),
                })
            }
            s.consumed_inputs.insert(n_idx.get_raw_i());

            match n.variant {
                IntermediateGraphNodeVariant::IOBuf{oe, input, schmitt_trigger, termination_enabled,
                    slew_is_fast, uses_data_gate, ..} => {

                    let newg_idx = *s.mcs_map.get(&n_idx).unwrap();

                    // Visit input
                    let old_input = input;
                    let input = {
                        if oe.is_some() && oe.unwrap() == s.g.vss_net {
                            // The output enable is permanently off, so force this to not be used
                            None
                        } else if input.is_some() {
                            if input.unwrap() == s.g.vss_net {
                                // Open-drain mode
                                if oe.is_none() || oe.unwrap() == s.g.vdd_net || oe.unwrap() == s.g.vss_net {
                                    return Err("broken open-drain connection");
                                }
                                // FIXME: Copypasta
                                let input_n = s.g.nets.get(oe.unwrap()).source.unwrap().0;
                                let input_type = match s.g.nodes.get(input_n).variant {
                                    IntermediateGraphNodeVariant::Xor{..} => {
                                        Some(InputGraphIOInputType::Xor)
                                    },
                                    IntermediateGraphNodeVariant::Reg{..} => {
                                        Some(InputGraphIOInputType::Reg)
                                    },
                                    _ => return Err("mis-connected nodes"),
                                };

                                // We need to recursively process this. Update the relevant map as well to make sure we
                                // don't mess up along the way.
                                assert!(!s.mcs_map.contains_key(&input_n));
                                s.mcs_map.insert(input_n, newg_idx);
                                process_one_intermed_node(s, input_n)?;

                                input_type
                            } else if input.unwrap() == s.g.vdd_net {
                                return Err("broken constant-1 IO");
                            } else {
                                let input_n = s.g.nets.get(input.unwrap()).source.unwrap().0;
                                let input_type = match s.g.nodes.get(input_n).variant {
                                    IntermediateGraphNodeVariant::Xor{..} => {
                                        Some(InputGraphIOInputType::Xor)
                                    },
                                    IntermediateGraphNodeVariant::Reg{..} => {
                                        Some(InputGraphIOInputType::Reg)
                                    },
                                    _ => return Err("mis-connected nodes"),
                                };

                                // We need to recursively process this. Update the relevant map as well to make sure we
                                // don't mess up along the way.
                                assert!(!s.mcs_map.contains_key(&input_n));
                                s.mcs_map.insert(input_n, newg_idx);
                                process_one_intermed_node(s, input_n)?;

                                input_type
                            }
                        } else {
                            None
                        }
                    };

                    // Visit OE
                    let oe = {
                        if oe.is_some() {
                            if oe.unwrap() == s.g.vdd_net || oe.unwrap() == s.g.vss_net {
                                None
                            } else {
                                if old_input.is_some() && old_input.unwrap() == s.g.vss_net {
                                    Some(InputGraphIOOEType::OpenDrain)
                                } else {
                                    let oe_n = s.g.nets.get(oe.unwrap()).source.unwrap().0;
                                    let oe_newg_n = process_one_intermed_node(s, oe_n)?;
                                    Some(match oe_newg_n {
                                        InputGraphAnyPoolIdx::PTerm(x) => InputGraphIOOEType::PTerm(x),
                                        InputGraphAnyPoolIdx::BufgGTS(x) => InputGraphIOOEType::GTS(x),
                                        _ => return Err("mis-connected nodes"),
                                    })
                                }
                            }
                        } else {
                            None
                        }
                    };

                    {
                        let mut newg_n = s.mcs.get_mut(newg_idx);
                        assert!(newg_n.io_bits.is_none());
                        newg_n.name = n.name.clone();
                        newg_n.requested_loc = n.location;
                        newg_n.io_bits = Some(InputGraphIOBuf {
                            input: input,
                            oe: oe,
                            schmitt_trigger,
                            termination_enabled,
                            slew_is_fast,
                            uses_data_gate,
                        });
                    }

                    Ok(InputGraphAnyPoolIdx::Macrocell(newg_idx))
                },
                IntermediateGraphNodeVariant::InBuf{schmitt_trigger, termination_enabled, uses_data_gate, ..} => {
                    let newg_idx = *s.mcs_map.get(&n_idx).unwrap();

                    {
                        let mut newg_n = s.mcs.get_mut(newg_idx);
                        assert!(newg_n.io_bits.is_none());
                        newg_n.name = n.name.clone();
                        newg_n.requested_loc = n.location;
                        newg_n.io_bits = Some(InputGraphIOBuf {
                            input: None,
                            oe: None,
                            schmitt_trigger,
                            termination_enabled,
                            slew_is_fast: true,
                            uses_data_gate,
                        });
                    }

                    Ok(InputGraphAnyPoolIdx::Macrocell(newg_idx))
                },
                IntermediateGraphNodeVariant::Xor{orterm_input, andterm_input, invert_out, ..} => {
                    let newg_idx = *s.mcs_map.get(&n_idx).unwrap();

                    // Visit OR gate
                    let mut orterm_inputs = Vec::new();
                    if orterm_input.is_some() {
                        let or_n = s.g.nets.get(orterm_input.unwrap()).source.unwrap().0;

                        // This is manually inlined so that we don't need to add hacky extra logic to return the
                        // desired array of actual Pterms.
                        s.consumed_inputs.insert(or_n.get_raw_i());
                        // Update the map even though this shouldn't actually be needed.
                        assert!(!s.mcs_map.contains_key(&or_n));
                        s.mcs_map.insert(or_n, newg_idx);

                        if let IntermediateGraphNodeVariant::OrTerm{ref inputs, ..} = s.g.nodes.get(or_n).variant {
                            for x in inputs {
                                let input_n = s.g.nets.get(*x).source.unwrap().0;
                                if let IntermediateGraphNodeVariant::AndTerm{..} =
                                    s.g.nodes.get(input_n).variant {} else {

                                    return Err("mis-connected nodes");
                                }
                                // We need to recursively process this
                                let input_newg_any = process_one_intermed_node(s, input_n)?;
                                let input_newg = if let InputGraphAnyPoolIdx::PTerm(x) = input_newg_any { x } else {
                                    panic!("Internal error - not a product term?");
                                };

                                orterm_inputs.push(input_newg);
                            }
                        } else {
                            return Err("mis-connected nodes");
                        }
                    }

                    // Visit PTC
                    let ptc_input = {
                        if andterm_input.is_some() {
                            let ptc_n = s.g.nets.get(andterm_input.unwrap()).source.unwrap().0;
                            let ptc_newg_n = process_one_intermed_node(s, ptc_n)?;
                            if let InputGraphAnyPoolIdx::PTerm(x) = ptc_newg_n {
                                Some(x)
                            } else {
                                return Err("mis-connected nodes");
                            }
                        } else {
                            None
                        }
                    };

                    {
                        let mut newg_n = s.mcs.get_mut(newg_idx);
                        assert!(newg_n.xor_bits.is_none());
                        newg_n.name = n.name.clone();
                        newg_n.requested_loc = n.location;
                        newg_n.xor_bits = Some(InputGraphXor {
                            orterm_inputs,
                            andterm_input: ptc_input,
                            invert_out,
                        });
                    }

                    Ok(InputGraphAnyPoolIdx::Macrocell(newg_idx))
                },
                IntermediateGraphNodeVariant::AndTerm{ref inputs_true, ref inputs_comp, ..} => {
                    // Unlike the above cases, this always inserts a new item

                    let mut inputs_true_new = Vec::new();
                    for x in inputs_true {
                        let input_n = s.g.nets.get(*x).source.unwrap().0;
                        let input_type = match s.g.nodes.get(input_n).variant {
                            IntermediateGraphNodeVariant::Xor{..} => InputGraphPTermInputType::Xor,
                            IntermediateGraphNodeVariant::Reg{..} => InputGraphPTermInputType::Reg,
                            IntermediateGraphNodeVariant::IOBuf{..} |
                            IntermediateGraphNodeVariant::InBuf{..} => InputGraphPTermInputType::Pin,
                            _ => return Err("mis-connected nodes"),
                        };

                        // We need to recursively process this
                        let input_newg_any = process_one_intermed_node(s, input_n)?;
                        let input_newg = if let InputGraphAnyPoolIdx::Macrocell(x) = input_newg_any { x } else {
                            panic!("Internal error - not a macrocell?");
                        };

                        // Flag the appropriate bits as used
                        {
                            let mut input_newg_n = s.mcs.get_mut(input_newg);
                            match input_type {
                                InputGraphPTermInputType::Pin => input_newg_n.io_feedback_used = true,
                                InputGraphPTermInputType::Xor => input_newg_n.xor_feedback_used = true,
                                InputGraphPTermInputType::Reg => input_newg_n.reg_feedback_used = true,
                            }
                        }

                        inputs_true_new.push((input_type, input_newg));
                    }

                    let mut inputs_comp_new = Vec::new();
                    for x in inputs_comp {
                        let input_n = s.g.nets.get(*x).source.unwrap().0;
                        let input_type = match s.g.nodes.get(input_n).variant {
                            IntermediateGraphNodeVariant::Xor{..} => InputGraphPTermInputType::Xor,
                            IntermediateGraphNodeVariant::Reg{..} => InputGraphPTermInputType::Reg,
                            IntermediateGraphNodeVariant::IOBuf{..} |
                            IntermediateGraphNodeVariant::InBuf{..} => InputGraphPTermInputType::Pin,
                            _ => return Err("mis-connected nodes"),
                        };

                        // We need to recursively process this
                        let input_newg_any = process_one_intermed_node(s, input_n)?;
                        let input_newg = if let InputGraphAnyPoolIdx::Macrocell(x) = input_newg_any { x } else {
                            panic!("Internal error - not a macrocell?");
                        };

                        // Flag the appropriate bits as used
                        {
                            let mut input_newg_n = s.mcs.get_mut(input_newg);
                            match input_type {
                                InputGraphPTermInputType::Pin => input_newg_n.io_feedback_used = true,
                                InputGraphPTermInputType::Xor => input_newg_n.xor_feedback_used = true,
                                InputGraphPTermInputType::Reg => input_newg_n.reg_feedback_used = true,
                            }
                        }

                        inputs_comp_new.push((input_type, input_newg));
                    }

                    let newg_n = InputGraphPTerm {
                        loc: None,
                        name: n.name.clone(),
                        requested_loc: n.location,
                        inputs_true: inputs_true_new,
                        inputs_comp: inputs_comp_new,
                    };

                    let newg_idx = s.pterms.insert(newg_n);
                    s.pterms_map.insert(n_idx, newg_idx);
                    Ok(InputGraphAnyPoolIdx::PTerm(newg_idx))
                },
                IntermediateGraphNodeVariant::OrTerm{..} => {
                    panic!("Internal error - unexpected OR gate here");
                },
                IntermediateGraphNodeVariant::Reg{mode, clkinv, clkddr, init_state, set_input, reset_input,
                    ce_input, dt_input, clk_input, ..} => {

                    // This one once again expects the item to already be present
                    let newg_idx = *s.mcs_map.get(&n_idx).unwrap();

                    // Visit data input
                    let dt_input = {
                        let input_n = s.g.nets.get(dt_input).source.unwrap().0;
                        let input_type = match s.g.nodes.get(input_n).variant {
                            IntermediateGraphNodeVariant::Xor{..} => {
                                InputGraphRegInputType::Xor
                            },
                            IntermediateGraphNodeVariant::IOBuf{..} |
                            IntermediateGraphNodeVariant::InBuf{..} => {
                                InputGraphRegInputType::Pin
                            },
                            _ => return Err("mis-connected nodes"),
                        };

                        // We need to recursively process this. Update the relevant map as well to make sure we
                        // don't mess up along the way, but only if it's an XOR.
                        if input_type == InputGraphRegInputType::Xor {
                            assert!(!s.mcs_map.contains_key(&input_n));
                            s.mcs_map.insert(input_n, newg_idx);
                            process_one_intermed_node(s, input_n)?;
                        }

                        input_type
                    };

                    // Visit clock input
                    let clk_input = {
                        let clk_n = s.g.nets.get(clk_input).source.unwrap().0;
                        let clk_newg_n = process_one_intermed_node(s, clk_n)?;
                        if let InputGraphAnyPoolIdx::PTerm(x) = clk_newg_n {
                            InputGraphRegClockType::PTerm(x)
                        } else if let InputGraphAnyPoolIdx::BufgClk(x) = clk_newg_n {
                            InputGraphRegClockType::GCK(x)
                        } else {
                            return Err("mis-connected nodes");
                        }
                    };

                    // Visit CE
                    let ce_input = {
                        if ce_input.is_some() {
                            let ce_n = s.g.nets.get(ce_input.unwrap()).source.unwrap().0;
                            let ce_newg_n = process_one_intermed_node(s, ce_n)?;
                            if let InputGraphAnyPoolIdx::PTerm(x) = ce_newg_n {
                                Some(x)
                            } else {
                                return Err("mis-connected nodes");
                            }
                        } else {
                            None
                        }
                    };

                    // Visit set
                    let set_input = {
                        if set_input.is_some() {
                            if set_input.unwrap() == s.g.vss_net {
                                None
                            } else if set_input.unwrap() == s.g.vdd_net {
                                return Err("cannot tie set input high");
                            } else {
                                let set_n = s.g.nets.get(set_input.unwrap()).source.unwrap().0;
                                let set_newg_n = process_one_intermed_node(s, set_n)?;
                                if let InputGraphAnyPoolIdx::PTerm(x) = set_newg_n {
                                    Some(InputGraphRegRSType::PTerm(x))
                                } else if let InputGraphAnyPoolIdx::BufgGSR(x) = set_newg_n {
                                    Some(InputGraphRegRSType::GSR(x))
                                } else {
                                    return Err("mis-connected nodes");
                                }
                            }
                        } else {
                            None
                        }
                    };

                    // Visit reset
                    let reset_input = {
                        if reset_input.is_some() {
                            if reset_input.unwrap() == s.g.vss_net {
                                None
                            } else if reset_input.unwrap() == s.g.vdd_net {
                                return Err("cannot tie reset input high");
                            } else {
                                let reset_n = s.g.nets.get(reset_input.unwrap()).source.unwrap().0;
                                let reset_newg_n = process_one_intermed_node(s, reset_n)?;
                                if let InputGraphAnyPoolIdx::PTerm(x) = reset_newg_n {
                                    Some(InputGraphRegRSType::PTerm(x))
                                } else if let InputGraphAnyPoolIdx::BufgGSR(x) = reset_newg_n {
                                    Some(InputGraphRegRSType::GSR(x))
                                } else {
                                    return Err("mis-connected nodes");
                                }
                            }
                        } else {
                            None
                        }
                    };

                    {
                        let mut newg_n = s.mcs.get_mut(newg_idx);
                        assert!(newg_n.reg_bits.is_none());
                        newg_n.name = n.name.clone();
                        newg_n.requested_loc = n.location;
                        newg_n.reg_bits = Some(InputGraphReg {
                            mode,
                            clkinv,
                            clkddr,
                            init_state,
                            set_input,
                            reset_input,
                            ce_input,
                            dt_input,
                            clk_input,
                        });
                    }

                    Ok(InputGraphAnyPoolIdx::Macrocell(newg_idx))
                },
                IntermediateGraphNodeVariant::BufgClk{input, ..} => {
                    // This always inserts a new item again

                    let input_n = s.g.nets.get(input).source.unwrap().0;
                    match s.g.nodes.get(input_n).variant {
                        IntermediateGraphNodeVariant::IOBuf{..} |
                        IntermediateGraphNodeVariant::InBuf{..} => {},
                        _ => return Err("mis-connected nodes"),
                    };

                    // We need to recursively process this
                    let input_newg_any = process_one_intermed_node(s, input_n)?;
                    let input_newg = if let InputGraphAnyPoolIdx::Macrocell(x) = input_newg_any { x } else {
                        panic!("Internal error - not a macrocell?");
                    };

                    let newg_n = InputGraphBufgClk {
                        loc: None,
                        name: n.name.clone(),
                        requested_loc: n.location,
                        input: input_newg,
                    };

                    let newg_idx = s.bufg_clks.insert(newg_n);
                    s.bufg_clks_map.insert(n_idx, newg_idx);
                    Ok(InputGraphAnyPoolIdx::BufgClk(newg_idx))
                },
                IntermediateGraphNodeVariant::BufgGTS{input, invert, ..} => {
                    let input_n = s.g.nets.get(input).source.unwrap().0;
                    match s.g.nodes.get(input_n).variant {
                        IntermediateGraphNodeVariant::IOBuf{..} |
                        IntermediateGraphNodeVariant::InBuf{..} => {},
                        _ => return Err("mis-connected nodes"),
                    };

                    // We need to recursively process this
                    let input_newg_any = process_one_intermed_node(s, input_n)?;
                    let input_newg = if let InputGraphAnyPoolIdx::Macrocell(x) = input_newg_any { x } else {
                        panic!("Internal error - not a macrocell?");
                    };

                    let newg_n = InputGraphBufgGTS {
                        loc: None,
                        name: n.name.clone(),
                        requested_loc: n.location,
                        input: input_newg,
                        invert,
                    };

                    let newg_idx = s.bufg_gts.insert(newg_n);
                    s.bufg_gts_map.insert(n_idx, newg_idx);
                    Ok(InputGraphAnyPoolIdx::BufgGTS(newg_idx))
                },
                IntermediateGraphNodeVariant::BufgGSR{input, invert, ..} => {
                    let input_n = s.g.nets.get(input).source.unwrap().0;
                    match s.g.nodes.get(input_n).variant {
                        IntermediateGraphNodeVariant::IOBuf{..} |
                        IntermediateGraphNodeVariant::InBuf{..} => {},
                        _ => return Err("mis-connected nodes"),
                    };

                    // We need to recursively process this
                    let input_newg_any = process_one_intermed_node(s, input_n)?;
                    let input_newg = if let InputGraphAnyPoolIdx::Macrocell(x) = input_newg_any { x } else {
                        panic!("Internal error - not a macrocell?");
                    };

                    let newg_n = InputGraphBufgGSR {
                        loc: None,
                        name: n.name.clone(),
                        requested_loc: n.location,
                        input: input_newg,
                        invert,
                    };

                    let newg_idx = s.bufg_gsr.insert(newg_n);
                    s.bufg_gsr_map.insert(n_idx, newg_idx);
                    Ok(InputGraphAnyPoolIdx::BufgGSR(newg_idx))
                },
            }
        }

        // Now we actually run through the list and process everything
        {
            let mut s = process_one_intermed_node_state {
                g,
                mcs: &mut mcs,
                pterms: &mut pterms,
                bufg_clks: &mut bufg_clks,
                bufg_gts: &mut bufg_gts,
                bufg_gsr: &mut bufg_gsr,
                mcs_map: &mut mcs_map,
                pterms_map: &mut pterms_map,
                bufg_clks_map: &mut bufg_clks_map,
                bufg_gts_map: &mut bufg_gts_map,
                bufg_gsr_map: &mut bufg_gsr_map,
                consumed_inputs: &mut consumed_inputs,
            };
            for oldg_mc in gathered_mcs {
                process_one_intermed_node(&mut s, oldg_mc)?;
            }
        }

        // Check to make sure we visited all the nodes
        for i in 0..g.nodes.len() {
            if !consumed_inputs.contains(&i) {
                panic!("Internal error - did not consume all intermediate inputs");
            }
        }

        let mut ret = Self {
            mcs,
            pterms,
            bufg_clks,
            bufg_gts,
            bufg_gsr,
        };

        ret.unfuse_pterms();
        ret.sanity_check()?;

        Ok(ret)
    }

    fn sanity_check(&self) -> Result<(), &'static str> {
        for x in self.mcs.iter() {
            if x.io_feedback_used && x.io_bits.is_none() {
                return Err("Used IO input but there is no IO data?");
            }

            if x.reg_feedback_used && x.reg_bits.is_none() {
                return Err("Used register input but there is no register data?");
            }

            if x.xor_feedback_used && x.xor_bits.is_none() {
                return Err("Used XOR input but there is no XOR data?");
            }

            if x.io_feedback_used && x.reg_feedback_used && x.xor_feedback_used {
                return Err("Too many feedback paths used");
            }
        }

        Ok(())
    }

    fn unfuse_pterms(&mut self) {
        let mut used_pterms = HashSet::new();

        for mc in self.mcs.iter_mut() {
            if mc.xor_bits.is_some() {
                let mut xor = mc.xor_bits.as_mut().unwrap();

                if let Some(pterm) = xor.andterm_input {
                    if used_pterms.contains(&pterm) {
                        let cloned_pterm = self.pterms.get(pterm).clone();
                        let new_pterm = self.pterms.insert(cloned_pterm);
                        xor.andterm_input = Some(new_pterm);
                    }
                    used_pterms.insert(pterm);
                }

                for i in 0..xor.orterm_inputs.len() {
                    let pterm = xor.orterm_inputs[i];
                    if used_pterms.contains(&pterm) {
                        let cloned_pterm = self.pterms.get(pterm).clone();
                        let new_pterm = self.pterms.insert(cloned_pterm);
                        xor.orterm_inputs[i] = new_pterm;
                    }
                    used_pterms.insert(pterm);
                }
            }

            if mc.reg_bits.is_some() {
                let mut reg = mc.reg_bits.as_mut().unwrap();

                if let Some(pterm) = reg.ce_input {
                    if used_pterms.contains(&pterm) {
                        let cloned_pterm = self.pterms.get(pterm).clone();
                        let new_pterm = self.pterms.insert(cloned_pterm);
                        reg.ce_input = Some(new_pterm);
                    }
                    used_pterms.insert(pterm);
                }

                if let InputGraphRegClockType::PTerm(pterm) = reg.clk_input {
                    if used_pterms.contains(&pterm) {
                        let cloned_pterm = self.pterms.get(pterm).clone();
                        let new_pterm = self.pterms.insert(cloned_pterm);
                        reg.clk_input = InputGraphRegClockType::PTerm(new_pterm);
                    }
                    used_pterms.insert(pterm);
                }

                if let Some(InputGraphRegRSType::PTerm(pterm)) = reg.set_input {
                    if used_pterms.contains(&pterm) {
                        let cloned_pterm = self.pterms.get(pterm).clone();
                        let new_pterm = self.pterms.insert(cloned_pterm);
                        reg.set_input = Some(InputGraphRegRSType::PTerm(new_pterm));
                    }
                    used_pterms.insert(pterm);
                }

                if let Some(InputGraphRegRSType::PTerm(pterm)) = reg.reset_input {
                    if used_pterms.contains(&pterm) {
                        let cloned_pterm = self.pterms.get(pterm).clone();
                        let new_pterm = self.pterms.insert(cloned_pterm);
                        reg.reset_input = Some(InputGraphRegRSType::PTerm(new_pterm));
                    }
                    used_pterms.insert(pterm);
                }
            }

            if mc.io_bits.is_some() {
                let mut io = mc.io_bits.as_mut().unwrap();

                if let Some(InputGraphIOOEType::PTerm(pterm)) = io.oe {
                    if used_pterms.contains(&pterm) {
                        let cloned_pterm = self.pterms.get(pterm).clone();
                        let new_pterm = self.pterms.insert(cloned_pterm);
                        io.oe = Some(InputGraphIOOEType::PTerm(new_pterm));
                    }
                    used_pterms.insert(pterm);
                }
            }
        }
    }
}
