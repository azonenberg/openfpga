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

extern crate xbpar_rs;
use self::xbpar_rs::*;

extern crate xc2bit;
use self::xc2bit::*;

extern crate yosys_netlist_json;

use *;
use objpool::*;

#[derive(Debug)]
pub enum NetlistGraphNodeVariant {
    AndTerm {
        inputs_true: Vec<ObjPoolIndex<NetlistGraphNet>>,
        inputs_comp: Vec<ObjPoolIndex<NetlistGraphNet>>,
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
        mode: XC2MCRegMode,
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
    },
    ZIADummyBuf {
        input: ObjPoolIndex<NetlistGraphNet>,
        output: ObjPoolIndex<NetlistGraphNet>,
    }
}

#[derive(Debug)]
pub struct NetlistGraphNode {
    pub variant: NetlistGraphNodeVariant,
    pub name: String,
    pub par_idx: Option<u32>,
}

#[derive(Debug)]
pub struct NetlistGraphNet {
    pub name: Option<String>,
    pub source: Option<(ObjPoolIndex<NetlistGraphNode>, &'static str)>,
    pub sinks: Vec<(ObjPoolIndex<NetlistGraphNode>, &'static str)>,
}

#[derive(Debug)]
pub struct NetlistGraph {
    pub nodes: ObjPool<NetlistGraphNode>,
    pub nets: ObjPool<NetlistGraphNet>,
    pub vdd_net: ObjPoolIndex<NetlistGraphNet>,
    pub vss_net: ObjPoolIndex<NetlistGraphNet>,
}

impl NetlistGraph {
    pub fn from_yosys_netlist(yosys_net: &yosys_netlist_json::Netlist) -> Result<NetlistGraph, &'static str> {
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

        // Keep track of module ports
        let mut module_ports = HashSet::new();
        for (_, port) in &top_module.ports {
            for yosys_edge_idx in &port.bits {
                if let &yosys_netlist_json::BitVal::N(n) = yosys_edge_idx {
                    module_ports.insert(n);
                }
            }
        }

        // Cells can refer to a net, so loop through these as well
        for (_, cell) in &top_module.cells {
            for (_, connection_vec) in &cell.connections {
                for connection in connection_vec.iter() {
                    if let &yosys_netlist_json::BitVal::N(yosys_edge_idx) = connection {
                        // Don't create nets for the pad side of io buffers
                        if module_ports.contains(&yosys_edge_idx) {
                            continue;
                        }

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
            }
        }

        // Now we want to loop through netnames and *) insert any new names (dangling?) and *) assign human-readable
        // net names
        for (netname_name, netname_obj) in &top_module.netnames {
            for yosys_edge_idx in &netname_obj.bits {
                if let &yosys_netlist_json::BitVal::N(yosys_edge_idx) = yosys_edge_idx {
                    // Don't create nets for the pad side of io buffers
                    if module_ports.contains(&yosys_edge_idx) {
                        continue;
                    }

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
        }

        // Now we can actually process objects
        let mut nodes = ObjPool::new();

        // This maps from a Yosys cell/port name to an internal node number
        // FIXME: Use of owned strings is inefficient
        let mut cell_map: HashMap<String, ObjPoolIndex<NetlistGraphNode>> = HashMap::new();

        let bitval_to_net = |bitval| {
            match bitval {
                yosys_netlist_json::BitVal::N(n) => Ok(*net_map.get(&n).unwrap()),
                yosys_netlist_json::BitVal::S(yosys_netlist_json::SpecialBit::_0) => Ok(vdd_net),
                yosys_netlist_json::BitVal::S(yosys_netlist_json::SpecialBit::_1) => Ok(vss_net),
                // We should never see an x/z in our processing
                _ => Err("Illegal bit value in JSON"),
            }
        };

        // and finally process cells
        for (cell_name, cell_obj) in &top_module.cells {
            if cell_map.get(cell_name).is_some() {
                return Err("duplicate cell/port name");
            }

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

            if cell_obj.cell_type == "IOBUFE" {
                // FIXME: Check that IO goes to a module port

                let node_variant = NetlistGraphNodeVariant::IOBuf {
                    input: single_optional_connection("I")?,
                    oe: single_optional_connection("E")?,
                    output: single_optional_connection("O")?,
                };

                let node_idx = nodes.insert(NetlistGraphNode {
                    name: cell_name.to_owned(),
                    variant: node_variant,
                    par_idx: None,
                });

                cell_map.insert(cell_name.to_owned(), node_idx);
            } else if cell_obj.cell_type == "IBUF" {
                // FIXME: Check that IO goes to a module port

                let node_variant = NetlistGraphNodeVariant::InBuf {
                    output: single_required_connection("O")?,
                };

                let node_idx = nodes.insert(NetlistGraphNode {
                    name: cell_name.to_owned(),
                    variant: node_variant,
                    par_idx: None,
                });

                cell_map.insert(cell_name.to_owned(), node_idx);
            } else if cell_obj.cell_type == "ANDTERM" {
                let num_true_inputs = numeric_param("TRUE_INP")?;
                let num_comp_inputs = numeric_param("COMP_INP")?;

                let inputs_true = multiple_required_connection("IN")?;
                let inputs_comp = multiple_required_connection("IN_B")?;

                if num_true_inputs != inputs_true.len() || num_comp_inputs != inputs_comp.len() {
                    return Err("ANDTERM cell has a mismatched number of inputs");
                }

                let output_y_net_idx = single_required_connection("OUT")?;

                // Create dummy buffer nodes for all inputs
                // TODO: What about redundant ones?
                let inputs_true = inputs_true.into_iter().map(|before_ziabuf_net| {
                    let after_ziabuf_net = nets.insert(NetlistGraphNet {
                        name: None,
                        source: None,
                        sinks: Vec::new(),
                    });

                    let node_idx_ziabuf = nodes.insert(NetlistGraphNode {
                        name: format!("__ziabuf_{}", cell_name),
                        variant: NetlistGraphNodeVariant::ZIADummyBuf {
                            input: before_ziabuf_net,
                            output: after_ziabuf_net,
                        },
                        par_idx: None,
                    });
                    cell_map.insert(format!("__ziabuf_{}", cell_name), node_idx_ziabuf);

                    after_ziabuf_net
                }).collect::<Vec<_>>();
                let inputs_comp = inputs_comp.into_iter().map(|before_ziabuf_net| {
                    let after_ziabuf_net = nets.insert(NetlistGraphNet {
                        name: None,
                        source: None,
                        sinks: Vec::new(),
                    });

                    let node_idx_ziabuf = nodes.insert(NetlistGraphNode {
                        name: format!("__ziabuf_{}", cell_name),
                        variant: NetlistGraphNodeVariant::ZIADummyBuf {
                            input: before_ziabuf_net,
                            output: after_ziabuf_net,
                        },
                        par_idx: None,
                    });
                    cell_map.insert(format!("__ziabuf_{}", cell_name), node_idx_ziabuf);

                    after_ziabuf_net
                }).collect::<Vec<_>>();

                let node_idx_and = nodes.insert(NetlistGraphNode {
                    name: cell_name.to_owned(),
                    variant: NetlistGraphNodeVariant::AndTerm {
                        inputs_true,
                        inputs_comp,
                        output: output_y_net_idx,
                    },
                    par_idx: None,
                });
                // FIXME: Copypasta
                cell_map.insert(cell_name.to_owned(), node_idx_and);
            } else if cell_obj.cell_type == "ORTERM" {
                let num_inputs = numeric_param("WIDTH")?;

                let inputs = multiple_required_connection("IN")?;

                if num_inputs != inputs.len() {
                    return Err("ORTERM cell has a mismatched number of inputs");
                }

                let output_y_net_idx = single_required_connection("OUT")?;

                let node_idx_and = nodes.insert(NetlistGraphNode {
                    name: cell_name.to_owned(),
                    variant: NetlistGraphNodeVariant::OrTerm {
                        inputs,
                        output: output_y_net_idx,
                    },
                    par_idx: None,
                });
                // FIXME: Copypasta
                cell_map.insert(cell_name.to_owned(), node_idx_and);
            } else if cell_obj.cell_type == "MACROCELL_XOR" {
                let node_idx_xor = nodes.insert(NetlistGraphNode {
                    name: cell_name.to_owned(),
                    variant: NetlistGraphNodeVariant::Xor {
                        andterm_input: single_optional_connection("IN_PTC")?,
                        orterm_input: single_optional_connection("IN_ORTERM")?,
                        invert_out: numeric_param("INVERT_OUT")? != 0,
                        output: single_required_connection("OUT")?,
                    },
                    par_idx: None,
                });
                // FIXME: Copypasta
                cell_map.insert(cell_name.to_owned(), node_idx_xor);
            } else {
                return Err("unsupported cell type");
            }
        }

        // Now that we are done processing, hook up sources/sinks in the edges
        for node_idx in nodes.iter() {
            let node = nodes.get(node_idx);
            match node.variant {
                NetlistGraphNodeVariant::AndTerm{ref inputs_true, ref inputs_comp, output} => {
                    for &input in inputs_true {
                        nets.get_mut(input).sinks.push((node_idx, "IN"));
                    }
                    for &input in inputs_comp {
                        nets.get_mut(input).sinks.push((node_idx, "IN"));
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
                NetlistGraphNodeVariant::ZIADummyBuf{input, output} => {
                    nets.get_mut(input).sinks.push((node_idx, "IN"));
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

    pub fn insert_into_par_graph(&mut self,
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
        let ziabuf_l = *ilmap.get("ZIA dummy buffer").unwrap();

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
                NetlistGraphNodeVariant::ZIADummyBuf{..} => ziabuf_l,
            };

            let node_idx_par = par_graphs.borrow_mut_n().add_new_node(lbl, node_idx_ours);

            node_par_idx_map.insert(node_idx_ours, node_idx_par);
        }

        for (&node_idx_ours, &node_idx_par) in &node_par_idx_map {
            // Stash the PAR index
            let node_mut = self.nodes.get_mut(node_idx_ours);
            node_mut.par_idx = Some(node_idx_par);
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
