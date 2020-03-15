/*
Copyright (c) 2018, Robert Ou <rqou@robertou.com> and contributors
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

use std::error;
use std::fmt;
use std::collections::{HashMap, HashSet};
use crate::objpool::*;
use slog;
use slog::Drain;
use slog_stdlog;
use xc2bit::*;
use yosys_netlist_json;


#[derive(Clone, PartialEq, Eq, Hash, Debug, Serialize, Deserialize)]
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

#[derive(Clone, PartialEq, Eq, Hash, Debug, Serialize, Deserialize)]
pub struct IntermediateGraphNode {
    pub variant: IntermediateGraphNodeVariant,
    pub name: String,
    pub location: Option<RequestedLocation>,
}

#[derive(Clone, PartialEq, Eq, Hash, Debug, Serialize, Deserialize)]
pub struct IntermediateGraphNet {
    pub name: Option<String>,
    pub source: Option<ObjPoolIndex<IntermediateGraphNode>>,
    pub sinks: Vec<ObjPoolIndex<IntermediateGraphNode>>,
}

#[derive(Clone, PartialEq, Eq, Hash, Debug, Serialize, Deserialize)]
pub struct IntermediateGraph {
    pub nodes: ObjPool<IntermediateGraphNode>,
    pub nets: ObjPool<IntermediateGraphNet>,
    pub vdd_net: ObjPoolIndex<IntermediateGraphNet>,
    pub vss_net: ObjPoolIndex<IntermediateGraphNet>,
}

#[derive(Clone, Debug, PartialEq, Eq)]
pub enum FrontendError {
    MultipleToplevelModules,
    NoToplevelModules,
    UnsupportedCellType(String),
    MultipleNetDrivers(String),
    NoNetDrivers(String),
    MalformedLoc(String),
    IllegalBitValue(yosys_netlist_json::BitVal),
    IllegalAttributeValue(yosys_netlist_json::AttributeVal),
    IllegalStringAttributeValue(String),
    MissingRequiredConnection(String),
    TooManyConnections(String),
    MissingRequiredParameter(String),
    MismatchedInputCount,
    ParseIntError(::std::num::ParseIntError),
}

impl error::Error for FrontendError {
    fn cause(&self) -> Option<&dyn error::Error> {
        match self {
            &FrontendError::ParseIntError(ref inner) => Some(inner),
            _ => None,
        }
    }
}

impl fmt::Display for FrontendError {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        match self {
            &FrontendError::UnsupportedCellType(ref s) => {
                write!(f, "unsupported cell type - {}", s)
            },
            &FrontendError::MultipleNetDrivers(ref s) => {
                write!(f, "multiple drivers for net - {}", s)
            },
            &FrontendError::NoNetDrivers(ref s) => {
                write!(f, "no drivers for net - {}", s)
            },
            &FrontendError::MalformedLoc(ref s) => {
                write!(f, "malformed LOC attribute - {}", s)
            },
            &FrontendError::IllegalStringAttributeValue(ref s) => {
                write!(f, "illegal string attribute value - {}", s)
            },
            &FrontendError::MissingRequiredConnection(ref s) => {
                write!(f, "missing required connection - {}", s)
            },
            &FrontendError::TooManyConnections(ref s) => {
                write!(f, "too many net connections - {}", s)
            },
            &FrontendError::MissingRequiredParameter(ref s) => {
                write!(f, "missing required parameter - {}", s)
            },
            &FrontendError::IllegalBitValue(v) => {
                write!(f, "illegal bit value - {:?}", v)
            },
            &FrontendError::IllegalAttributeValue(ref v) => {
                write!(f, "illegal attribute value - {:?}", v)
            },
            &FrontendError::ParseIntError(ref inner) => {
                write!(f, "{}", inner)
            },
            &FrontendError::MultipleToplevelModules => {
                write!(f, "multiple top-level modules")
            },
            &FrontendError::NoToplevelModules => {
                write!(f, "no top-level modules")
            },
            &FrontendError::MismatchedInputCount => {
                write!(f, "mismatched input count")
            },
        }
    }
}

impl From<::std::num::ParseIntError> for FrontendError {
    fn from(err: ::std::num::ParseIntError) -> Self {
        FrontendError::ParseIntError(err)
    }
}

impl IntermediateGraph {
    pub fn from_yosys_netlist<L: Into<Option<slog::Logger>>>(
        yosys_net: &yosys_netlist_json::Netlist, logger: L) -> Result<Self, FrontendError> {

        let logger = logger.into().unwrap_or(slog::Logger::root(slog_stdlog::StdLog.fuse(), o!()));

        let mut top_module_name = "";
        let mut top_module_found = false;
        for (module_name, module) in &yosys_net.modules {
            debug!(logger, "found yosys netlist module"; "module name" => module_name);

            if let Some(top_attr) = module.attributes.get("top") {
                if let Some(n) = top_attr.to_number() {
                    if n != 0 {
                        // Claims to be a top-level module
                        debug!(logger, "found toplevel yosys netlist module"; "module name" => module_name);

                        if top_module_found {
                            error!(logger, "found multiple toplevel yosys netlist modules";
                                "second module name" => module_name);
                            return Err(FrontendError::MultipleToplevelModules);
                        }

                        top_module_found = true;
                        top_module_name = module_name;
                    }
                }
            }
        }

        if !top_module_found {
            error!(logger, "found no toplevel yosys netlist modules");
            return Err(FrontendError::NoToplevelModules);
        }
        let top_module = yosys_net.modules.get(top_module_name).unwrap();

        let logger = logger.new(o!("top module" => top_module_name.to_owned()));

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
                            debug!(logger, "nets - skipping module port for cell";
                                "cell name" => cell_name,
                                "connection name" => connection_name,
                                "net index" => yosys_edge_idx);
                            continue;
                        }

                        if net_map.get(&yosys_edge_idx).is_none() {
                            // Need to add a new one
                            debug!(logger, "nets - adding new net because of cell";
                                "cell name" => cell_name,
                                "connection name" => connection_name,
                                "net index" => yosys_edge_idx);
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
                        debug!(logger, "nets - skipping module port for netname";
                            "name" => netname_name,
                            "index" => yosys_edge_idx);
                        continue;
                    }

                    if net_map.get(&yosys_edge_idx).is_none() {
                        // Need to add a new one
                        debug!(logger, "nets - adding new net because of netname";
                            "name" => netname_name,
                            "index" => yosys_edge_idx);
                        let our_edge_idx = nets.insert(IntermediateGraphNet {
                            name: Some(netname_name.to_owned()),
                            source: None,
                            sinks: Vec::new(),
                        });
                        net_map.insert(yosys_edge_idx, our_edge_idx);
                    } else {
                        // Naming an existing one
                        debug!(logger, "nets - naming existing net";
                            "name" => netname_name,
                            "index" => yosys_edge_idx);
                        let existing_net_our_idx = net_map.get(&yosys_edge_idx).unwrap();
                        let existing_net = nets.get_mut(*existing_net_our_idx);
                        if let Some(ref old_name) = existing_net.name {
                            warn!(logger, "nets - overwrote net name";
                                "old name" => old_name,
                                "new name" => netname_name,
                                "index" => yosys_edge_idx);
                        }
                        existing_net.name = Some(netname_name.to_owned());
                    }
                }
            }
        }

        // Now we can actually process objects
        let mut nodes = ObjPool::new();

        let bitval_to_net = |bitval, conn_name: &str, logger: &slog::Logger| {
            match bitval {
                yosys_netlist_json::BitVal::N(n) => Ok(*net_map.get(&n).unwrap()),
                yosys_netlist_json::BitVal::S(yosys_netlist_json::SpecialBit::_0) => Ok(vss_net),
                yosys_netlist_json::BitVal::S(yosys_netlist_json::SpecialBit::_1) => Ok(vdd_net),
                // We should never see an x/z in our processing
                _ => {
                    error!(logger, "cells - illegal bit value";
                        "connection name" => conn_name,
                        "value" => bitval);
                    Err(FrontendError::IllegalBitValue(bitval))
                },
            }
        };

        // and finally process cells
        for &cell_name in &cell_names {
            let cell_obj = &top_module.cells[cell_name];

            let logger = logger.new(o!("cell name" => cell_name.to_owned()));

            // Helper to retrieve a parameter
            let numeric_param = |name: &str| {
                let param_option = cell_obj.parameters.get(name);
                if param_option.is_none() {
                    error!(logger, "cells - missing required parameter";
                        "name" => name);
                    return Err(FrontendError::MissingRequiredParameter(name.to_owned()));
                }
                let param_option_copy = *param_option.as_ref().unwrap();
                if let Some(n) = param_option.unwrap().to_number() {
                    debug!(logger, "cells - numeric parameter";
                        "name" => name,
                        "value" => n);
                    return Ok(n)
                } else {
                    error!(logger, "cells - parameter not a number";
                        "name" => name,
                        "value" => param_option_copy);
                    return Err(FrontendError::IllegalAttributeValue(param_option_copy.clone()));
                };
            };

            // Helper to retrieve an optional string attribute
            let optional_string_attrib = |name: &str| -> Result<Option<&str>, FrontendError> {
                let param_option = cell_obj.attributes.get(name);
                if param_option.is_none() {
                    return Ok(None);
                }
                let param_option_copy = *param_option.as_ref().unwrap();
                if let Some(ref s) = param_option.unwrap().to_string_if_string() {
                    debug!(logger, "cells - string parameter";
                        "name" => name,
                        "value" => s);
                    return Ok(Some(s))
                } else {
                    error!(logger, "cells - parameter not a string";
                        "name" => name,
                        "value" => param_option_copy);
                    return Err(FrontendError::IllegalAttributeValue(param_option_copy.clone()));
                };
            };

            // Helper to retrieve an optional string attribute containing a boolean
            let optional_string_bool_attrib = |name: &str| -> Result<bool, FrontendError> {
                let attrib = optional_string_attrib(name)?;
                Ok(if let Some(attrib) = attrib {
                    if attrib.eq_ignore_ascii_case("true") {
                        debug!(logger, "cells - bool parameter";
                            "name" => name,
                            "value" => true);
                        true
                    } else if attrib.eq_ignore_ascii_case("false") {
                        debug!(logger, "cells - bool parameter";
                            "name" => name,
                            "value" => false);
                        false
                    } else {
                        error!(logger, "cells - parameter not a boolean";
                            "name" => name,
                            "value" => attrib);
                        return Err(FrontendError::IllegalStringAttributeValue(attrib.to_owned()));
                    }
                } else {
                    false   // TODO: Default should be?
                })
            };

            // Helper to retrieve a single net that is definitely required
            let single_required_connection = |name: &str, logger: &slog::Logger| {
                let conn_obj = cell_obj.connections.get(name);
                if conn_obj.is_none() {
                    error!(logger, "cells - missing required connection";
                        "connection name" => name);
                    return Err(FrontendError::MissingRequiredConnection(name.to_owned()));
                }
                let conn_obj = conn_obj.unwrap();
                if conn_obj.len() != 1 {
                    error!(logger, "cells - too many nets";
                        "connection name" => name);
                    return Err(FrontendError::TooManyConnections(name.to_owned()));
                }
                let result = bitval_to_net(conn_obj[0].clone(), name, logger)?;
                debug!(logger, "cells - mapped required connection";
                    "connection name" => name,
                    "yosys net idx" => conn_obj[0].clone(),
                    "net pool idx" => result);
                return Ok(result);
            };

            // Helper to retrieve a single net that is optional
            let single_optional_connection = |name: &str, logger: &slog::Logger| {
                let conn_obj = cell_obj.connections.get(name);
                if conn_obj.is_none() {
                    return Ok(None);
                }
                let conn_obj = conn_obj.unwrap();
                if conn_obj.len() != 1 {
                    error!(logger, "cells - too many nets";
                        "connection name" => name);
                    return Err(FrontendError::TooManyConnections(name.to_owned()));
                }
                let result = bitval_to_net(conn_obj[0].clone(), name, logger)?;
                debug!(logger, "cells - mapped optional connection";
                    "connection name" => name,
                    "yosys net idx" => conn_obj[0].clone(),
                    "net pool idx" => result);
                return Ok(Some(result));
            };

            // Helper to retrieve an array of nets that is required
            let multiple_required_connection = |name: &str, logger: &slog::Logger| {
                let conn_obj = cell_obj.connections.get(name);
                if conn_obj.is_none() {
                    error!(logger, "cells - missing required connection";
                        "connection name" => name);
                    return Err(FrontendError::MissingRequiredConnection(name.to_owned()));
                }
                let mut result = Vec::new();
                let conn_obj_copy = conn_obj.as_ref().unwrap().clone();
                for x in conn_obj.unwrap() {
                    result.push(bitval_to_net(x.clone(), name, logger)?);
                }
                for i in 0..result.len() {
                    debug!(logger, "cells - mapped multiple connections";
                        "connection name" => name,
                        "yosys net idx" => &conn_obj_copy[i],
                        "net pool idx" => result[i]);
                }
                return Ok(result);
            };

            info!(logger, "cells - processing";
                "type" => &cell_obj.cell_type);

            match cell_obj.cell_type.as_ref() {
                "IOBUFE"  => {
                    // FIXME: Check that IO goes to a module port

                    let slew_attrib = optional_string_attrib("SLEW")?;
                    let slew_is_fast = if let Some(attrib) = slew_attrib {
                        if attrib.eq_ignore_ascii_case("fast") {
                            debug!(logger, "cells - IOBUFE - slow is fast");
                            true
                        } else if attrib.eq_ignore_ascii_case("slow") {
                            debug!(logger, "cells - IOBUFE - slow is slow");
                            false
                        } else {
                            error!(logger, "cells - IOBUFE - invalid slew rate";
                                "value" => attrib);
                            return Err(FrontendError::IllegalStringAttributeValue(attrib.to_owned()));
                        }
                    } else {
                        false   // TODO: Default should be?
                    };

                    let uses_data_gate = optional_string_bool_attrib("DATA_GATE")?;
                    // FIXME: Check if this is requested on parts where this is not supported

                    nodes.insert(IntermediateGraphNode {
                        name: cell_name.to_owned(),
                        variant: IntermediateGraphNodeVariant::IOBuf {
                            input: single_optional_connection("I", &logger)?,
                            oe: single_optional_connection("E", &logger)?,
                            output: single_optional_connection("O", &logger)?,
                            schmitt_trigger: optional_string_bool_attrib("SCHMITT_TRIGGER")?,
                            termination_enabled: optional_string_bool_attrib("TERM")?,
                            slew_is_fast,
                            uses_data_gate,
                        },
                        location: RequestedLocation::parse_location(optional_string_attrib("LOC")?, &logger)?,
                    });
                },
                "IBUF" => {
                    // FIXME: Check that IO goes to a module port

                    let uses_data_gate = optional_string_bool_attrib("DATA_GATE")?;
                    // FIXME: Check if this is requested on parts where this is not supported

                    nodes.insert(IntermediateGraphNode {
                        name: cell_name.to_owned(),
                        variant: IntermediateGraphNodeVariant::InBuf {
                            output: single_required_connection("O", &logger)?,
                            schmitt_trigger: optional_string_bool_attrib("SCHMITT_TRIGGER")?,
                            termination_enabled: optional_string_bool_attrib("TERM")?,
                            uses_data_gate,
                        },
                        location: RequestedLocation::parse_location(optional_string_attrib("LOC")?, &logger)?,
                    });
                },
                "ANDTERM" => {
                    let num_true_inputs = numeric_param("TRUE_INP")?;
                    let num_comp_inputs = numeric_param("COMP_INP")?;

                    let inputs_true = multiple_required_connection("IN", &logger)?;
                    let inputs_comp = multiple_required_connection("IN_B", &logger)?;

                    if num_true_inputs != inputs_true.len() || num_comp_inputs != inputs_comp.len() {
                        error!(logger, "cells - ANDTERM - mismatched number of inputs";
                            "true input attrib" => num_true_inputs,
                            "comp input attrib" => num_comp_inputs,
                            "true input conn" => inputs_true.len(),
                            "comp input conn" => inputs_comp.len());
                        return Err(FrontendError::MismatchedInputCount);
                    }

                    nodes.insert(IntermediateGraphNode {
                        name: cell_name.to_owned(),
                        variant: IntermediateGraphNodeVariant::AndTerm {
                            inputs_true,
                            inputs_comp,
                            output: single_required_connection("OUT", &logger)?,
                        },
                        location: RequestedLocation::parse_location(optional_string_attrib("LOC")?, &logger)?,
                    });
                },
                "ORTERM" => {
                    let num_inputs = numeric_param("WIDTH")?;

                    let inputs = multiple_required_connection("IN", &logger)?;

                    if num_inputs != inputs.len() {
                        error!(logger, "cells - ORTERM - mismatched number of inputs";
                            "input attrib" => num_inputs,
                            "input conn" => inputs.len());
                        return Err(FrontendError::MismatchedInputCount);
                    }

                    nodes.insert(IntermediateGraphNode {
                        name: cell_name.to_owned(),
                        variant: IntermediateGraphNodeVariant::OrTerm {
                            inputs,
                            output: single_required_connection("OUT", &logger)?,
                        },
                        location: RequestedLocation::parse_location(optional_string_attrib("LOC")?, &logger)?,
                    });
                },
                "MACROCELL_XOR" => {
                    nodes.insert(IntermediateGraphNode {
                        name: cell_name.to_owned(),
                        variant: IntermediateGraphNodeVariant::Xor {
                            andterm_input: single_optional_connection("IN_PTC", &logger)?,
                            orterm_input: single_optional_connection("IN_ORTERM", &logger)?,
                            invert_out: numeric_param("INVERT_OUT")? != 0,
                            output: single_required_connection("OUT", &logger)?,
                        },
                        location: RequestedLocation::parse_location(optional_string_attrib("LOC")?, &logger)?,
                    });
                },
                "BUFG" => {
                    nodes.insert(IntermediateGraphNode {
                        name: cell_name.to_owned(),
                        variant: IntermediateGraphNodeVariant::BufgClk {
                            input: single_required_connection("I", &logger)?,
                            output: single_required_connection("O", &logger)?,
                        },
                        location: RequestedLocation::parse_location(optional_string_attrib("LOC")?, &logger)?,
                    });
                },
                "BUFGTS" => {
                    nodes.insert(IntermediateGraphNode {
                        name: cell_name.to_owned(),
                        variant: IntermediateGraphNodeVariant::BufgGTS {
                            input: single_required_connection("I", &logger)?,
                            output: single_required_connection("O", &logger)?,
                            invert: numeric_param("INVERT")? != 0,
                        },
                        location: RequestedLocation::parse_location(optional_string_attrib("LOC")?, &logger)?,
                    });
                },
                "BUFGSR" => {
                    nodes.insert(IntermediateGraphNode {
                        name: cell_name.to_owned(),
                        variant: IntermediateGraphNodeVariant::BufgGSR {
                            input: single_required_connection("I", &logger)?,
                            output: single_required_connection("O", &logger)?,
                            invert: numeric_param("INVERT")? != 0,
                        },
                        location: RequestedLocation::parse_location(optional_string_attrib("LOC")?, &logger)?,
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
                        ce_input = Some(single_required_connection("CE", &logger)?);
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
                            set_input: single_optional_connection("PRE", &logger)?,
                            reset_input: single_optional_connection("CLR", &logger)?,
                            ce_input,
                            dt_input: single_required_connection(dt_name, &logger)?,
                            clk_input: single_required_connection(clk_name, &logger)?,
                            output: single_required_connection("Q", &logger)?,
                        },
                        location: RequestedLocation::parse_location(optional_string_attrib("LOC")?, &logger)?,
                    });
                }
                _ => {
                    error!(logger, "cells - unsupported cell type");
                    return Err(FrontendError::UnsupportedCellType(cell_obj.cell_type.to_owned()));
                }
            }
        }

        // Now that we are done processing, hook up sources/sinks in the edges

        // This helper is used to check if a net already has a source and raise an error if it does
        let set_net_source = |nets: &mut ObjPool<IntermediateGraphNet>, output, x| {
            let output_net = nets.get_mut(output);
            if output_net.source.is_some() {
                error!(logger, "connectivity - multiple drivers for net";
                    "net" => &output_net.name,
                    "old driver" => output_net.source.unwrap(),
                    "new driver" => x);
                // FIXME: Wtf is &"lit".to_owned()?!
                return Err(FrontendError::MultipleNetDrivers(output_net.name.as_ref()
                    .unwrap_or(&"<no name>".to_owned()).to_owned()));
            }
            debug!(logger, "connectivity - connecting driver";
                "net" => &output_net.name,
                "driver" => x);
            output_net.source = Some(x);
            Ok(())
        };

        for node_idx in nodes.iter_idx() {
            let node = nodes.get(node_idx);
            match node.variant {
                IntermediateGraphNodeVariant::AndTerm{ref inputs_true, ref inputs_comp, output} => {
                    for &input in inputs_true {
                        nets.get_mut(input).sinks.push(node_idx);
                    }
                    for &input in inputs_comp {
                        nets.get_mut(input).sinks.push(node_idx);
                    }
                    set_net_source(&mut nets, output, node_idx)?;
                },
                IntermediateGraphNodeVariant::OrTerm{ref inputs, output} => {
                    for &input in inputs {
                        nets.get_mut(input).sinks.push(node_idx);
                    }
                    set_net_source(&mut nets, output, node_idx)?;
                },
                IntermediateGraphNodeVariant::Xor{orterm_input, andterm_input, output, ..} => {
                    if orterm_input.is_some() {
                        nets.get_mut(orterm_input.unwrap()).sinks.push(node_idx);
                    }
                    if andterm_input.is_some() {
                        nets.get_mut(andterm_input.unwrap()).sinks.push(node_idx);
                    }
                    set_net_source(&mut nets, output, node_idx)?;
                },
                IntermediateGraphNodeVariant::Reg{set_input, reset_input, ce_input, dt_input, clk_input, output, ..} => {
                    if set_input.is_some() {
                        nets.get_mut(set_input.unwrap()).sinks.push(node_idx);
                    }
                    if reset_input.is_some() {
                        nets.get_mut(reset_input.unwrap()).sinks.push(node_idx);
                    }
                    if ce_input.is_some() {
                        nets.get_mut(ce_input.unwrap()).sinks.push(node_idx);
                    }
                    nets.get_mut(dt_input).sinks.push(node_idx);
                    nets.get_mut(clk_input).sinks.push(node_idx);
                    set_net_source(&mut nets, output, node_idx)?;
                },
                IntermediateGraphNodeVariant::BufgClk{input, output, ..} |
                IntermediateGraphNodeVariant::BufgGTS{input, output, ..} |
                IntermediateGraphNodeVariant::BufgGSR{input, output, ..} => {
                    nets.get_mut(input).sinks.push(node_idx);
                    set_net_source(&mut nets, output, node_idx)?;
                },
                IntermediateGraphNodeVariant::IOBuf{input, oe, output, ..} => {
                    if input.is_some() {
                        nets.get_mut(input.unwrap()).sinks.push(node_idx);
                    }
                    if oe.is_some() {
                        nets.get_mut(oe.unwrap()).sinks.push(node_idx);
                    }
                    if output.is_some() {
                        set_net_source(&mut nets, output.unwrap(), node_idx)?;
                    }
                },
                IntermediateGraphNodeVariant::InBuf{output, ..} => {
                    set_net_source(&mut nets, output, node_idx)?;
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
                error!(logger, "connectivity - undriven net";
                    "net" => &net.name);
                return Err(FrontendError::NoNetDrivers(net.name.as_ref()
                    .unwrap_or(&"<no name>".to_owned()).to_owned()));
            }
        }

        Ok(Self {
            nodes,
            nets,
            vdd_net,
            vss_net,
        })
    }

    pub fn gather_macrocells<L: Into<Option<slog::Logger>>>(&self, logger: L)
        -> Result<Vec<ObjPoolIndex<IntermediateGraphNode>>, GatherMacrocellError> {

        let logger = logger.into().unwrap_or(slog::Logger::root(slog_stdlog::StdLog.fuse(), o!()));

        let mut ret = Vec::new();
        let mut encountered_xors = HashSet::new();

        // XXX note: The types of macrocells _must_ be searched in this order (or at least unregistered IBUFs must be
        // last). This property is relied on by the greedy initial placement algorithm.

        // First iteration: Find IOBUFE
        for node_idx in self.nodes.iter_idx() {
            let node = self.nodes.get(node_idx);

            if let IntermediateGraphNodeVariant::IOBuf{input, ..} = node.variant {
                debug!(logger, "gather - found IOBUFE";
                    "name" => &node.name);
                ret.push(node_idx);

                if input.is_some() {
                    let input = input.unwrap();

                    if input == self.vdd_net || input == self.vss_net {
                        debug!(logger, "gather - IOBUFE set to constant value";
                            "iobufe name" => &node.name,
                            "value" => if input == self.vdd_net {"VDD"} else {"VSS"});
                        continue;
                    }

                    let source_node_idx = self.nets.get(input).source.unwrap();
                    let source_node = self.nodes.get(source_node_idx);
                    if let IntermediateGraphNodeVariant::Xor{..} = source_node.variant {
                        // Combinatorial output
                        debug!(logger, "gather - remembering XOR for combinatorial";
                            "iobufe name" => &node.name,
                            "xor name" => &source_node.name);
                        encountered_xors.insert(source_node_idx);
                    } else if let IntermediateGraphNodeVariant::Reg{dt_input, ..} = source_node.variant {
                        // Registered output, look at the input into the register
                        let reg_name_copy = &source_node.name;
                        let source_node_idx = self.nets.get(dt_input).source.unwrap();
                        let source_node = self.nodes.get(source_node_idx);
                        if let IntermediateGraphNodeVariant::Xor{..} = source_node.variant {
                            debug!(logger, "gather - remembering XOR for registered";
                                "iobufe name" => &node.name,
                                "xor name" => &source_node.name,
                                "reg name" => reg_name_copy);
                            encountered_xors.insert(source_node_idx);
                        } else if let IntermediateGraphNodeVariant::IOBuf{..} = source_node.variant {
                            if node_idx != source_node_idx {
                                // Trying to go from a different pin into the direct input path of this pin
                                error!(logger, "gather - invalid path IOBUFE -> FF -> IOBUFE";
                                    "node 1" => &node.name,
                                    "node 2" => &source_node.name);
                                return Err(GatherMacrocellError::IllegalNodeDriver(node.name.to_owned()));
                            }
                            // Otherwise ignore this for now. This is a bit strange, but possible.
                        } else {
                            error!(logger, "gather - invalid path ![XOR|IOBUFE] -> FF -> IOBUFE";
                                "iobuf node" => &node.name,
                                "invalid node" => &source_node.name);
                            return Err(GatherMacrocellError::IllegalNodeDriver(node.name.to_owned()));
                        }
                    } else {
                        error!(logger, "gather - invalid path ![XOR|FF] -> IOBUFE";
                            "iobuf node" => &node.name,
                            "invalid node" => &source_node.name);
                        return Err(GatherMacrocellError::IllegalNodeDriver(node.name.to_owned()));
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
                let mut maybe_reg_name = None;
                for &sink_node_idx in &self.nets.get(output).sinks {
                    let sink_node = self.nodes.get(sink_node_idx);

                    if let IntermediateGraphNodeVariant::Reg{..} = sink_node.variant {
                        if maybe_reg_index.is_some() {
                            error!(logger, "gather - invalid multiple FF sinks for node";
                                "node" => &node.name,
                                "ff node 1" => maybe_reg_name.unwrap(),
                                "ff node 2" => &sink_node.name);
                            return Err(GatherMacrocellError::IllegalNodeSink(node.name.to_owned()));
                        }

                        maybe_reg_index = Some(sink_node_idx);
                        maybe_reg_name = Some(&sink_node.name);
                    }
                }

                if maybe_reg_index.is_none() {
                    // Buried combinatorial
                    debug!(logger, "gather - found buried combinatorial";
                        "name" => &node.name);
                    ret.push(node_idx);
                } else {
                    // Buried register
                    debug!(logger, "gather - found buried register";
                        "xor name" => &node.name,
                        "reg name" => maybe_reg_name.unwrap());
                    ret.push(maybe_reg_index.unwrap());
                }
            }
        }

        // Third iteration: Find registered IBUF
        for node_idx in self.nodes.iter_idx() {
            let node = self.nodes.get(node_idx);

            if let IntermediateGraphNodeVariant::InBuf{output, ..} = node.variant {
                let mut maybe_reg_index = None;
                let mut maybe_reg_name = None;
                for &sink_node_idx in &self.nets.get(output).sinks {
                    let sink_node = self.nodes.get(sink_node_idx);

                    if let IntermediateGraphNodeVariant::Reg{..} = sink_node.variant {
                        if maybe_reg_index.is_some() {
                            error!(logger, "gather - invalid multiple FF sinks for node";
                                "node" => &node.name,
                                "ff node 1" => maybe_reg_name.unwrap(),
                                "ff node 2" => &sink_node.name);
                            return Err(GatherMacrocellError::IllegalNodeSink(node.name.to_owned()));
                        }

                        maybe_reg_index = Some(sink_node_idx);
                        maybe_reg_name = Some(&sink_node.name);
                    }
                }

                if maybe_reg_index.is_some() {
                    debug!(logger, "gather - found registered IBUF";
                        "ibuf name" => &node.name,
                        "reg name" => maybe_reg_name.unwrap());
                    ret.push(node_idx);
                }
            }
        }

        // Fourth iteration: Find unregistered IBUF
        for node_idx in self.nodes.iter_idx() {
            let node = self.nodes.get(node_idx);

            if let IntermediateGraphNodeVariant::InBuf{output, ..} = node.variant {
                let mut maybe_reg_index = None;
                for &sink_node_idx in &self.nets.get(output).sinks {
                    let sink_node = self.nodes.get(sink_node_idx);

                    if let IntermediateGraphNodeVariant::Reg{..} = sink_node.variant {
                        maybe_reg_index = Some(sink_node_idx);
                    }
                }

                if maybe_reg_index.is_none() {
                    debug!(logger, "gather - found unregistered IBUF";
                        "name" => &node.name);
                    ret.push(node_idx);
                }
            }
        }

        Ok(ret)
    }
}

#[derive(Clone, Debug, PartialEq, Eq)]
pub enum GatherMacrocellError {
    IllegalNodeDriver(String),
    IllegalNodeSink(String),
}

impl error::Error for GatherMacrocellError {
    fn cause(&self) -> Option<&dyn error::Error> {
        None
    }
}

impl fmt::Display for GatherMacrocellError {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        match self {
            &GatherMacrocellError::IllegalNodeDriver(ref s) => {
                write!(f, "node is driven by illegal type of node - {}", s)
            },
            &GatherMacrocellError::IllegalNodeSink(ref s) => {
                write!(f, "node sinks to too many nodes - {}", s)
            },
        }
    }
}

#[derive(Copy, Clone, PartialEq, Eq, Hash, Debug, Serialize, Deserialize)]
pub struct RequestedLocation {
    pub fb: u32,
    pub i: Option<u32>,
}

impl RequestedLocation {
    fn parse_location(loc: Option<&str>, logger: &slog::Logger) -> Result<Option<Self>, FrontendError> {
        if loc.is_none() {
            return Ok(None);
        }

        let loc = loc.unwrap();

        let f = |loc: &str| {
            if loc.starts_with("FB") {
                let loc_fb_i = loc.split("_").collect::<Vec<_>>();
                if loc_fb_i.len() == 1 {
                    // FBn
                    let fb = loc_fb_i[0][2..].parse::<u32>()? - 1;
                    debug!(logger, "loc - FB";
                        "fb" => fb);
                    Ok(Some(RequestedLocation {
                        fb,
                        i: None,
                    }))
                } else if loc_fb_i.len() == 2 {
                    if !loc_fb_i[1].starts_with("P") {
                        // FBn_i
                        let fb = loc_fb_i[0][2..].parse::<u32>()? - 1;
                        let i = loc_fb_i[1].parse::<u32>()? - 1;
                        debug!(logger, "loc - FB/MC";
                            "fb" => fb,
                            "mc" => i);
                        Ok(Some(RequestedLocation {
                            fb,
                            i: Some(i),
                        }))
                    } else {
                        // FBn_Pi
                        let fb = loc_fb_i[0][2..].parse::<u32>()? - 1;
                        let i = loc_fb_i[1][1..].parse::<u32>()?;
                        debug!(logger, "loc - FB/Pterm";
                            "fb" => fb,
                            "pt" => i);
                        Ok(Some(RequestedLocation {
                            fb,
                            i: Some(i),
                        }))
                    }
                } else {
                    Err(FrontendError::MalformedLoc(loc.to_owned()))
                }
            } else {
                // TODO: Pin names
                Err(FrontendError::MalformedLoc(loc.to_owned()))
            }
        };

        let result = f(loc);
        if result.is_err() {
            error!(logger, "loc - malformed";
                "loc" => loc);
        }
        result
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    use std;
    use std::fs::File;
    use std::io::Read;

    extern crate serde_json;

    fn run_one_reftest(input_filename: &'static str) {
        // Read original json
        let input_path = std::path::Path::new(input_filename);
        let mut input_data = Vec::new();
        File::open(&input_path).unwrap().read_to_end(&mut input_data).unwrap();
        let yosys_netlist = yosys_netlist_json::Netlist::from_slice(&input_data).unwrap();
        // This is what we get
        let our_data_structure = IntermediateGraph::from_yosys_netlist(&yosys_netlist, None).unwrap();

        // Read reference json
        let mut output_path = input_path.to_path_buf();
        output_path.set_extension("out");
        let mut output_data = Vec::new();
        File::open(&output_path).unwrap().read_to_end(&mut output_data).unwrap();
        let reference_data_structure = serde_json::from_slice(&output_data).unwrap();

        assert_eq!(our_data_structure, reference_data_structure);
    }

    // Include list of actual tests to run
    include!(concat!(env!("OUT_DIR"), "/frontend-reftests.rs"));
}
