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

extern crate xbpar_rs;
use self::xbpar_rs::*;

extern crate xc2bit;
use self::xc2bit::*;

extern crate serde_json;
use self::serde_json::Value;

use *;
use objpool::*;

#[derive(Debug)]
pub enum NetlistGraphNodeVariant {
    AndTerm {
        // Input and invert bit
        inputs: Vec<(ObjPoolIndex<NetlistGraphNet>, bool)>,
        output: ObjPoolIndex<NetlistGraphNet>,
    },
    OrTerm {
        inputs: Vec<ObjPoolIndex<NetlistGraphNet>>,
        output: ObjPoolIndex<NetlistGraphNet>,
    },
    Xor {
        orterm_input: Option<ObjPoolIndex<NetlistGraphNet>>,
        andterm_input: Option<ObjPoolIndex<NetlistGraphNet>>,
        invert_out: bool,
        output: ObjPoolIndex<NetlistGraphNet>,
    },
    Reg {
        mode: XC2MCFFMode,
        clkinv: bool,
        set_input: Option<ObjPoolIndex<NetlistGraphNet>>,
        reset_input: Option<ObjPoolIndex<NetlistGraphNet>>,
        ce_input: Option<ObjPoolIndex<NetlistGraphNet>>,
        dt_input: ObjPoolIndex<NetlistGraphNet>,
        clk_input: ObjPoolIndex<NetlistGraphNet>,
        output: ObjPoolIndex<NetlistGraphNet>,
    },
    BufgClk {
        input: ObjPoolIndex<NetlistGraphNet>,
        output: ObjPoolIndex<NetlistGraphNet>,
    },
    BufgGTS {
        input: ObjPoolIndex<NetlistGraphNet>,
        output: ObjPoolIndex<NetlistGraphNet>,
    },
    BufgGSR {
        input: ObjPoolIndex<NetlistGraphNet>,
        output: ObjPoolIndex<NetlistGraphNet>,
    },
    IOBuf {
        input: Option<ObjPoolIndex<NetlistGraphNet>>,
        oe: Option<ObjPoolIndex<NetlistGraphNet>>,
        output: Option<ObjPoolIndex<NetlistGraphNet>>,
    },
    InBuf {
        output: ObjPoolIndex<NetlistGraphNet>,
    }
}

#[derive(Debug)]
pub struct NetlistGraphNode {
    variant: NetlistGraphNodeVariant,
    name: String,
}

#[derive(Debug)]
pub struct NetlistGraphNet {
    name: Option<String>,
    source: Option<(ObjPoolIndex<NetlistGraphNode>, &'static str)>,
    sinks: Vec<(ObjPoolIndex<NetlistGraphNode>, &'static str)>,
}

#[derive(Debug)]
pub struct NetlistGraph {
    nodes: ObjPool<NetlistGraphNode>,
    nets: ObjPool<NetlistGraphNet>,
    vdd_net: ObjPoolIndex<NetlistGraphNet>,
    vss_net: ObjPoolIndex<NetlistGraphNet>,
}

impl NetlistGraph {
    pub fn from_yosys_netlist(yosys_net: &YosysNetlist) -> Result<NetlistGraph, &'static str> {
        let mut top_module_name = "";
        let mut top_module_found = false;
        for (module_name, module) in &yosys_net.modules {
            if let Some(top_attr) = module.attributes.get("top") {
                if let &Value::Number(ref n) = top_attr {
                    if n.is_u64() && n.as_u64().unwrap() != 0 {
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
        let vdd_net = nets.insert(NetlistGraphNet {
            name: Some(String::from("<internal virtual Vdd net>")),
            source: None,
            sinks: Vec::new(),
        });
        let vss_net = nets.insert(NetlistGraphNet {
            name: Some(String::from("<internal virtual Vss net>")),
            source: None,
            sinks: Vec::new(),
        });

        // This maps from a Yosys net number to an internal net number
        let mut net_map: HashMap<usize, ObjPoolIndex<NetlistGraphNet>> = HashMap::new();

        // Ports can refer to a net, so loop through them
        for (_, port) in &top_module.ports {
            for &yosys_edge_idx in &port.bits {
                if net_map.get(&yosys_edge_idx).is_none() {
                    // Need to add a new one
                    let our_edge_idx = nets.insert(NetlistGraphNet {
                        name: None,
                        source: None,
                        sinks: Vec::new(),
                    });
                    net_map.insert(yosys_edge_idx, our_edge_idx);
                }
            }
        }

        // Cells can refer to a net, so loop through these as well
        for (_, cell) in &top_module.cells {
            for (_, connection_vec) in &cell.connections {
                for connection in connection_vec.iter() {
                    match connection {
                        &Value::Number(ref n) => {
                            if !n.is_u64() {
                                return Err("invalid number in cell connection");
                            }

                            let yosys_edge_idx = n.as_u64().unwrap() as usize;
                            if net_map.get(&yosys_edge_idx).is_none() {
                                // Need to add a new one
                                let our_edge_idx = nets.insert(NetlistGraphNet {
                                    name: None,
                                    source: None,
                                    sinks: Vec::new(),
                                });
                                net_map.insert(yosys_edge_idx, our_edge_idx);
                            }
                        }
                        // This will be either a constant 0 or 1. We don't actually care about this at this point
                        &Value::String(..) => {},
                        _ => {
                            return Err("invalid JSON type in cell connection");
                        }
                    }
                }
            }
        }

        // Now we want to loop through netnames and *) insert any new names (dangling?) and *) assign human-readable
        // net names
        for (netname_name, netname_obj) in &top_module.netnames {
            for &yosys_edge_idx in &netname_obj.bits {
                if net_map.get(&yosys_edge_idx).is_none() {
                    // Need to add a new one
                    let our_edge_idx = nets.insert(NetlistGraphNet {
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

        // Now we can actually process objects
        let mut nodes = ObjPool::new();

        // This maps from a Yosys cell/port name to an internal node number
        // FIXME: Use of owned strings is inefficient
        let mut cell_map: HashMap<String, ObjPoolIndex<NetlistGraphNode>> = HashMap::new();

        // We now process ports and create buffer objects for them
        for (port_name, port_obj) in &top_module.ports {
            if cell_map.get(port_name).is_some() {
                return Err("duplicate cell/port name");
            }

            if port_obj.direction == "input" {
                if port_obj.bits.len() != 1 {
                    return Err("too many bits entries for port");
                }

                let connected_net_idx = *net_map.get(&port_obj.bits[0]).unwrap();

                let node_idx = nodes.insert(NetlistGraphNode {
                    name: port_name.to_owned(),
                    variant: NetlistGraphNodeVariant::IOBuf {
                        input: None,
                        oe: None,
                        output: Some(connected_net_idx),
                    }
                });

                cell_map.insert(port_name.to_owned(), node_idx);
            } else if port_obj.direction == "output" {
                if port_obj.bits.len() != 1 {
                    return Err("too many bits entries for port");
                }

                let connected_net_idx = *net_map.get(&port_obj.bits[0]).unwrap();

                let node_idx = nodes.insert(NetlistGraphNode {
                    name: port_name.to_owned(),
                    variant: NetlistGraphNodeVariant::IOBuf {
                        input: Some(connected_net_idx),
                        oe: None,
                        output: None,
                    }
                });

                cell_map.insert(port_name.to_owned(), node_idx);
            } else {
                return Err("invalid port direction");
            }
        }

        // and finally process cells
        for (cell_name, cell_obj) in &top_module.cells {
            if cell_map.get(cell_name).is_some() {
                return Err("duplicate cell/port name");
            }

            // FIXME FIXME FIXME: This isn't how we're supposed to do techmapping
            if cell_obj.cell_type == "$sop" {
                let num_and_terms = cell_obj.parameters.get("DEPTH");
                if num_and_terms.is_none() {
                    return Err("required parameter DEPTH missing");
                }
                let num_and_terms = if let &Value::Number(ref n) = num_and_terms.unwrap() {
                    if !n.is_u64() {
                        return Err("parameter DEPTH is not a number")
                    }

                    n.as_u64().unwrap()
                } else {
                    return Err("parameter DEPTH is not a number")
                };

                let num_inputs = cell_obj.parameters.get("WIDTH");
                if num_inputs.is_none() {
                    return Err("required parameter WIDTH missing");
                }
                let num_inputs = if let &Value::Number(ref n) = num_inputs.unwrap() {
                    if !n.is_u64() {
                        return Err("parameter WIDTH is not a number")
                    }

                    n.as_u64().unwrap()
                } else {
                    return Err("parameter WIDTH is not a number")
                };

                // FIXME: larger than 32 bits
                let table = cell_obj.parameters.get("TABLE");
                if table.is_none() {
                    return Err("required parameter TABLE missing");
                }
                let table = if let &Value::Number(ref n) = table.unwrap() {
                    if !n.is_u64() {
                        return Err("parameter TABLE is not a number")
                    }

                    n.as_u64().unwrap()
                } else {
                    return Err("parameter TABLE is not a number")
                };

                // Create nets to connect AND outputs to OR
                let and_to_or_nets = (0..num_and_terms).map(|_| {
                    nets.insert(NetlistGraphNet {
                        name: None,
                        source: None,
                        sinks: Vec::new(),
                    })
                }).collect::<Vec<_>>();

                // Net to connect OR to XOR
                let or_to_xor_net = nets.insert(NetlistGraphNet {
                    name: None,
                    source: None,
                    sinks: Vec::new(),
                });

                // Create AND cells
                for and_i in 0..num_and_terms {
                    let mut inputs = Vec::new();
                    for input_i in 0..num_inputs {
                        let input_used_comp = (table & (1 << (and_i * num_inputs * 2 + input_i * 2 + 0))) != 0;
                        let input_used_true = (table & (1 << (and_i * num_inputs * 2 + input_i * 2 + 1))) != 0;

                        if input_used_comp && input_used_true {
                            return Err("AND term tried to use both true and complement inputs");
                        }

                        if input_used_comp | input_used_true {
                            let input_a_obj = cell_obj.connections.get("A");
                            if input_a_obj.is_none() {
                                return Err("$sop cell is missing A input");
                            }
                            let input_a_obj = input_a_obj.unwrap();

                            let input_used_value_obj = &input_a_obj[input_i as usize];
                            let input_used_net_idx = if let &Value::Number(ref n) = input_used_value_obj {
                                *net_map.get(&(n.as_u64().unwrap() as usize)).unwrap()
                            } else if let &Value::String(ref s) = input_used_value_obj {
                                if s == "1" {
                                    vdd_net
                                } else if s == "0" {
                                    vss_net
                                } else {
                                    return Err("invalid connection in cell");
                                }
                            } else {
                                // We already checked for this earlier
                                unreachable!();
                            };

                            inputs.push((input_used_net_idx, input_used_comp));
                        }
                    }

                    let node_idx_and = nodes.insert(NetlistGraphNode {
                        name: format!("{}__and_{}", cell_name, and_i),
                        variant: NetlistGraphNodeVariant::AndTerm {
                            inputs: inputs,
                            output: and_to_or_nets[and_i as usize],
                        }
                    });
                    // FIXME: Copypasta
                    cell_map.insert(format!("{}__and_{}", cell_name, and_i), node_idx_and);
                }

                // Create OR cell
                let node_idx_or = nodes.insert(NetlistGraphNode {
                    name: format!("{}__or", cell_name),
                    variant: NetlistGraphNodeVariant::OrTerm {
                        inputs: and_to_or_nets,
                        output: or_to_xor_net,
                    }
                });
                // FIXME: Copypasta
                cell_map.insert(format!("{}__or", cell_name), node_idx_or);

                // Create XOR cell
                let output_y_obj = cell_obj.connections.get("Y");
                if output_y_obj.is_none() {
                    return Err("$sop cell is missing Y output");
                }
                let output_y_obj = output_y_obj.unwrap();
                if output_y_obj.len() != 1 {
                    return Err("cell Y output has too many nets")
                }
                let output_y_obj = &output_y_obj[0];

                let output_y_net_idx = if let &Value::Number(ref n) = output_y_obj {
                    *net_map.get(&(n.as_u64().unwrap() as usize)).unwrap()
                } else if let &Value::String(..) = output_y_obj {
                    return Err("invalid connection in cell");
                } else {
                    // We already checked for this earlier
                    unreachable!();
                };

                let node_idx_xor = nodes.insert(NetlistGraphNode {
                    name: format!("{}__xor", cell_name),
                    variant: NetlistGraphNodeVariant::Xor {
                        andterm_input: None,
                        orterm_input: Some(or_to_xor_net),
                        invert_out: false,
                        output: output_y_net_idx,
                    }
                });
                // FIXME: Copypasta
                cell_map.insert(format!("{}__xor", cell_name), node_idx_xor);
            } else {
                return Err("unsupported cell type");
            }
        }

        // Now that we are done processing, hook up sources/sinks in the edges
        for node_idx in nodes.iter() {
            let node = nodes.get(node_idx);
            match node.variant {
                NetlistGraphNodeVariant::AndTerm{ref inputs, output} => {
                    for &input in inputs {
                        nets.get_mut(input.0).sinks.push((node_idx, "IN"));
                    }
                    let output_net = nets.get_mut(output);
                    if output_net.source.is_some() {
                        return Err("multiple drivers for net");
                    }
                    output_net.source = Some((node_idx, "OUT"));
                },
                NetlistGraphNodeVariant::OrTerm{ref inputs, output} => {
                    for &input in inputs {
                        nets.get_mut(input).sinks.push((node_idx, "IN"));
                    }
                    let output_net = nets.get_mut(output);
                    if output_net.source.is_some() {
                        return Err("multiple drivers for net");
                    }
                    output_net.source = Some((node_idx, "OUT"));
                },
                NetlistGraphNodeVariant::Xor{orterm_input, andterm_input, output, ..} => {
                    if orterm_input.is_some() {
                        nets.get_mut(orterm_input.unwrap()).sinks.push((node_idx, "IN_ORTERM"));
                    }
                    if andterm_input.is_some() {
                        nets.get_mut(andterm_input.unwrap()).sinks.push((node_idx, "IN_PTC"));
                    }
                    let output_net = nets.get_mut(output);
                    if output_net.source.is_some() {
                        return Err("multiple drivers for net");
                    }
                    output_net.source = Some((node_idx, "OUT"));
                },
                NetlistGraphNodeVariant::Reg{set_input, reset_input, ce_input, dt_input, clk_input, output, ..} => {
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
                    let output_net = nets.get_mut(output);
                    if output_net.source.is_some() {
                        return Err("multiple drivers for net");
                    }
                    output_net.source = Some((node_idx, "Q"));
                },
                NetlistGraphNodeVariant::BufgClk{input, output, ..} |
                NetlistGraphNodeVariant::BufgGTS{input, output, ..} |
                NetlistGraphNodeVariant::BufgGSR{input, output, ..} => {
                    nets.get_mut(input).sinks.push((node_idx, "IN"));
                    let output_net = nets.get_mut(output);
                    if output_net.source.is_some() {
                        return Err("multiple drivers for net");
                    }
                    output_net.source = Some((node_idx, "OUT"));
                },
                NetlistGraphNodeVariant::IOBuf{input, oe, output} => {
                    if input.is_some() {
                        nets.get_mut(input.unwrap()).sinks.push((node_idx, "IN"));
                    }
                    if oe.is_some() {
                        nets.get_mut(oe.unwrap()).sinks.push((node_idx, "OE"));
                    }
                    if output.is_some() {
                        let output_net = nets.get_mut(output.unwrap());
                        if output_net.source.is_some() {
                            return Err("multiple drivers for net");
                        }
                        output_net.source = Some((node_idx, "OUT"));
                    }
                },
                NetlistGraphNodeVariant::InBuf{output} => {
                    let output_net = nets.get_mut(output);
                    if output_net.source.is_some() {
                        return Err("multiple drivers for net");
                    }
                    output_net.source = Some((node_idx, "OUT"));
                },
            }
        }

        // Check for undriven nets
        for net_idx in nets.iter() {
            if net_idx == vdd_net || net_idx == vss_net {
                continue;
            }

            let net = nets.get(net_idx);
            if net.source.is_none() {
                return Err("net without driver");
            }
        }

        Ok(NetlistGraph {
            nodes: nodes,
            nets: nets,
            vdd_net: vdd_net,
            vss_net: vss_net,
        })
    }

    pub fn insert_into_par_graph(&self,
        par_graphs: &mut PARGraphPair<ObjPoolIndex<DeviceGraphNode>, ObjPoolIndex<NetlistGraphNode>>,
        lmap: &HashMap<u32, &'static str>) {

        // We first need an inverse label map
        let mut ilmap = HashMap::new();
        for (l, &name) in lmap {
            ilmap.insert(name, *l);
        }

        // Now fetch the appropriate labels
        let iopad_l = *ilmap.get("IOPAD").unwrap();
        let inpad_l = *ilmap.get("INPAD").unwrap();
        let reg_l = *ilmap.get("REG").unwrap();
        let xor_l = *ilmap.get("XOR").unwrap();
        let orterm_l = *ilmap.get("ORTERM").unwrap();
        let andterm_l = *ilmap.get("ANDTERM").unwrap();
        let bufg_l = *ilmap.get("BUFG").unwrap();

        // Create corresponding nodes
        let mut node_par_idx_map = HashMap::new();
        for node_idx_ours in self.nodes.iter() {
            let node = self.nodes.get(node_idx_ours);

            let lbl = match node.variant {
                NetlistGraphNodeVariant::AndTerm{..} => andterm_l,
                NetlistGraphNodeVariant::OrTerm{..} => orterm_l,
                NetlistGraphNodeVariant::Xor{..} => xor_l,
                NetlistGraphNodeVariant::Reg{..} => reg_l,
                NetlistGraphNodeVariant::BufgClk{..} => bufg_l,
                NetlistGraphNodeVariant::BufgGTS{..} => bufg_l,
                NetlistGraphNodeVariant::BufgGSR{..} => bufg_l,
                NetlistGraphNodeVariant::IOBuf{..} => iopad_l,
                NetlistGraphNodeVariant::InBuf{..} => inpad_l,
            };

            let node_idx_par = par_graphs.borrow_mut_n().add_new_node(lbl, node_idx_ours);

            node_par_idx_map.insert(node_idx_ours, node_idx_par);
        }

        // Add all the edges
        for net_idx in self.nets.iter() {
            if net_idx == self.vdd_net || net_idx == self.vss_net {
                continue;
            }

            let net = self.nets.get(net_idx);

            let source_node_our_tup = net.source.unwrap();
            let source_node_par_idx = *node_par_idx_map.get(&source_node_our_tup.0).unwrap();
            let source_node_out_port = source_node_our_tup.1;
            
            for sink_node_our_tup in &net.sinks {
                let sink_node_par_idx = *node_par_idx_map.get(&sink_node_our_tup.0).unwrap();
                let sink_node_in_port = sink_node_our_tup.1;

                par_graphs.borrow_mut_n().add_edge(source_node_par_idx, source_node_out_port,
                    sink_node_par_idx, sink_node_in_port);
            }
        }
    }
}
