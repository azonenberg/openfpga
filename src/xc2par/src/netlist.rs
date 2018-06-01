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
use std::error;
use std::fmt;
use std::error::{Error};
use std::hash::{Hash, Hasher};
use std::iter::FromIterator;
use slog::Drain;

use xc2bit::*;

use *;
use objpool::*;

#[derive(Copy, Clone, PartialEq, Eq, Hash, Debug, Serialize, Deserialize)]
pub struct AssignedLocation {
    pub fb: u32,
    pub i: u32,
}

#[derive(Copy, Clone, PartialEq, Eq, Hash, Debug, Serialize, Deserialize)]
pub enum InputGraphIOInputType {
    Reg,
    Xor,
}

#[derive(Copy, Clone, PartialEq, Eq, Hash, Debug, Serialize, Deserialize)]
pub enum InputGraphIOOEType {
    PTerm(ObjPoolIndex<InputGraphPTerm>),
    GTS(ObjPoolIndex<InputGraphBufgGTS>),
    OpenDrain,
}

#[derive(Clone, Debug, PartialEq, Eq, Hash, Serialize, Deserialize)]
pub struct InputGraphIOBuf {
    pub input: Option<InputGraphIOInputType>,
    pub oe: Option<InputGraphIOOEType>,
    pub schmitt_trigger: bool,
    pub termination_enabled: bool,
    pub slew_is_fast: bool,
    pub uses_data_gate: bool,
}

#[derive(Copy, Clone, PartialEq, Eq, Hash, Debug, Serialize, Deserialize)]
pub enum InputGraphRegRSType {
    PTerm(ObjPoolIndex<InputGraphPTerm>),
    GSR(ObjPoolIndex<InputGraphBufgGSR>),
}

#[derive(Copy, Clone, PartialEq, Eq, Hash, Debug, Serialize, Deserialize)]
pub enum InputGraphRegInputType {
    Pin,
    Xor,
}

#[derive(Copy, Clone, PartialEq, Eq, Hash, Debug, Serialize, Deserialize)]
pub enum InputGraphRegClockType {
    PTerm(ObjPoolIndex<InputGraphPTerm>),
    GCK(ObjPoolIndex<InputGraphBufgClk>),
}

#[derive(Clone, Debug, PartialEq, Eq, Hash, Serialize, Deserialize)]
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

#[derive(Clone, Debug, PartialEq, Eq, Hash, Serialize, Deserialize)]
pub struct InputGraphXor {
    pub orterm_inputs: Vec<ObjPoolIndex<InputGraphPTerm>>,
    pub andterm_input: Option<ObjPoolIndex<InputGraphPTerm>>,
    pub invert_out: bool,
}

#[derive(Clone, Debug, PartialEq, Eq, Hash, Serialize, Deserialize)]
pub struct InputGraphMacrocell {
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

impl InputGraphMacrocellType {
    pub fn is_pininput(&self) -> bool {
        match self {
            &InputGraphMacrocellType::PinOutput |
            &InputGraphMacrocellType::BuriedComb |
            &InputGraphMacrocellType::BuriedReg => false,
            &InputGraphMacrocellType::PinInputReg |
            &InputGraphMacrocellType::PinInputUnreg => true,
        }
    }
}

#[derive(Copy, Clone, PartialEq, Eq, Hash, Debug, Serialize, Deserialize)]
pub enum InputGraphPTermInputType {
    Reg,
    Xor,
    Pin,
}

pub type InputGraphPTermInput = (InputGraphPTermInputType, ObjPoolIndex<InputGraphMacrocell>);

#[derive(Clone, Debug, Serialize, Deserialize)]
pub struct InputGraphPTerm {
    pub name: String,
    pub requested_loc: Option<RequestedLocation>,
    pub inputs_true: Vec<InputGraphPTermInput>,
    pub inputs_comp: Vec<InputGraphPTermInput>,
}

#[derive(Clone, PartialEq, Eq, Hash, Debug, Serialize, Deserialize)]
pub struct InputGraphBufgClk {
    pub name: String,
    pub requested_loc: Option<RequestedLocation>,
    pub input: ObjPoolIndex<InputGraphMacrocell>,
}

#[derive(Clone, PartialEq, Eq, Hash, Debug, Serialize, Deserialize)]
pub struct InputGraphBufgGTS {
    pub name: String,
    pub requested_loc: Option<RequestedLocation>,
    pub input: ObjPoolIndex<InputGraphMacrocell>,
    pub invert: bool,
}

#[derive(Clone, PartialEq, Eq, Hash, Debug, Serialize, Deserialize)]
pub struct InputGraphBufgGSR {
    pub name: String,
    pub requested_loc: Option<RequestedLocation>,
    pub input: ObjPoolIndex<InputGraphMacrocell>,
    pub invert: bool,
}

#[derive(Debug, PartialEq, Eq, Hash, Serialize, Deserialize)]
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

impl slog::Value for InputGraphAnyPoolIdx {
    fn serialize(&self, _record: &slog::Record, key: slog::Key, serializer: &mut slog::Serializer) -> slog::Result {
        match self {
            &InputGraphAnyPoolIdx::Macrocell(i) => {
                serializer.emit_usize(key, i.get_raw_i())?;
                serializer.emit_str("inputgraph type", "macrocell")
            },
            &InputGraphAnyPoolIdx::PTerm(i) => {
                serializer.emit_usize(key, i.get_raw_i())?;
                serializer.emit_str("inputgraph type", "pterm")
            },
            &InputGraphAnyPoolIdx::BufgClk(i) => {
                serializer.emit_usize(key, i.get_raw_i())?;
                serializer.emit_str("inputgraph type", "bufg")
            },
            &InputGraphAnyPoolIdx::BufgGTS(i) => {
                serializer.emit_usize(key, i.get_raw_i())?;
                serializer.emit_str("inputgraph type", "bufgts")
            },
            &InputGraphAnyPoolIdx::BufgGSR(i) => {
                serializer.emit_usize(key, i.get_raw_i())?;
                serializer.emit_str("inputgraph type", "bufgsr")
            },
        }
    }
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

fn combine_names(old: &str, additional: &str) -> String {
    if old == "" {
        additional.to_owned()
    } else {
        format!("{}_{}", old, additional)
    }
}

fn combine_locs(old: Option<RequestedLocation>, additional: Option<RequestedLocation>, logger: &slog::Logger)
    -> Result<Option<RequestedLocation>, IntermedToInputError> {

    if old.is_none() {
        Ok(additional)
    } else if additional.is_none() {
        Ok(old)
    } else {
        let old_ = old.unwrap();
        let new = additional.unwrap();

        if old_.fb != new.fb {
            error!(logger, "intermed2input - LOCs that need to agree do not agree on FB";
                "first index" => old_.fb,
                "second index" => new.fb);
            Err(IntermedToInputError::LocMismatchedFB(old_.fb, new.fb))
        } else {
            if old_.i.is_none() {
                Ok(additional)
            } else if new.i.is_none() {
                Ok(old)
            } else {
                let old_i = old_.i.unwrap();
                let new_i = new.i.unwrap();

                if old_i != new_i {
                    error!(logger, "intermed2input - LOCs that need to agree do not agree on macrocell";
                        "first index" => old_i,
                        "second index" => new_i);
                    Err(IntermedToInputError::LocMismatchedMC(old_i, new_i))
                } else {
                    Ok(old)
                }
            }
        }
    }
}

#[derive(Clone, Debug, PartialEq, Eq)]
pub enum IntermedToInputError {
    WrongConnectionType(String),
    WrongTiedValue(String),
    WrongPTermInputs(String),
    TooManyFeedbacksUsed(String),
    LocMismatchedFB(u32, u32),
    LocMismatchedMC(u32, u32),
    SanityCheckError(&'static str),
}

impl error::Error for IntermedToInputError {
    fn description(&self) -> &'static str {
        match self {
            &IntermedToInputError::WrongConnectionType(_) => "node is connected to another node of the wrong type",
            &IntermedToInputError::WrongTiedValue(_) => "node input is tied to a disallowed constant",
            &IntermedToInputError::WrongPTermInputs(_) => "p-term inputs incorrect (duplicate or in true+comp)",
            &IntermedToInputError::TooManyFeedbacksUsed(_) => "too many feedback paths used (XOR + register + IO)",
            &IntermedToInputError::LocMismatchedFB(_, _) => "two LOC constraints have mismatched FB index",
            &IntermedToInputError::LocMismatchedMC(_, _) => "two LOC constraints have mismatched macrocell index",
            &IntermedToInputError::SanityCheckError(_) => "sanity check failed",
        }
    }

    fn cause(&self) -> Option<&error::Error> {
        None
    }
}

impl fmt::Display for IntermedToInputError {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        match self {
            &IntermedToInputError::WrongConnectionType(ref s) |
            &IntermedToInputError::WrongTiedValue(ref s) |
            &IntermedToInputError::WrongPTermInputs(ref s) |
            &IntermedToInputError::TooManyFeedbacksUsed(ref s) => {
                write!(f, "{} - {}", self.description(), s)
            },
            &IntermedToInputError::SanityCheckError(s) => {
                write!(f, "{} - {}", self.description(), s)
            },
            &IntermedToInputError::LocMismatchedFB(i1, i2) |
            &IntermedToInputError::LocMismatchedMC(i1, i2) => {
                write!(f, "{} - {}/{}", self.description(), i1, i2)
            },
        }
    }
}

impl InputGraph {
    pub fn from_intermed_graph<L: Into<Option<slog::Logger>>>(g: &IntermediateGraph, logger: L)
        -> Result<Self, IntermedToInputError> {

        let logger = logger.into().unwrap_or(slog::Logger::root(slog_stdlog::StdLog.fuse(), o!()));

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
        let gathered_mcs = g.gather_macrocells(logger.new(o!())).unwrap();

        // Now we pre-create all the macrocell objects
        for i in 0..gathered_mcs.len() {
            let dummy_mc = InputGraphMacrocell {
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
            let n = g.nodes.get(oldg_idx);
            debug!(logger, "intermed2input - pre-mapping macrocell";
                "name" => &n.name,
                "intermed" => oldg_idx,
                "inputgraph" => newg_idx);

            // FIX: We need to add all of the IO/REG/XOR components to the map, not just the component pointed to
            // directly by the gather function
            // FIXME: This duplicates a lot of code
            match n.variant {
                IntermediateGraphNodeVariant::IOBuf{input, ..} => {
                    if input.is_some() {
                        let input = input.unwrap();

                        let source_node_idx = g.nets.get(input).source.unwrap();
                        let source_node = g.nodes.get(source_node_idx);
                        if let IntermediateGraphNodeVariant::Xor{..} = source_node.variant {
                            debug!(logger, "intermed2input - also pre-mapping IOBUFE XOR";
                                "name" => &source_node.name,
                                "intermed" => source_node_idx,
                                "inputgraph" => newg_idx);
                            mcs_map.insert(source_node_idx, newg_idx);
                        } else if let IntermediateGraphNodeVariant::Reg{dt_input, ..} = source_node.variant {
                            debug!(logger, "intermed2input - also pre-mapping IOBUFE reg";
                                "name" => &source_node.name,
                                "intermed" => source_node_idx,
                                "inputgraph" => newg_idx);
                            mcs_map.insert(source_node_idx, newg_idx);
                            // Registered output, look at the input into the register
                            let source_node_idx = g.nets.get(dt_input).source.unwrap();
                            let source_node = g.nodes.get(source_node_idx);
                            if let IntermediateGraphNodeVariant::Xor{..} = source_node.variant {
                                debug!(logger, "intermed2input - also pre-mapping IOBUFE XOR";
                                    "name" => &source_node.name,
                                    "intermed" => source_node_idx,
                                    "inputgraph" => newg_idx);
                                mcs_map.insert(source_node_idx, newg_idx);
                            }
                        }
                    }
                },
                IntermediateGraphNodeVariant::Reg{dt_input, ..} => {
                    // Look at the input into the register
                    let source_node_idx = g.nets.get(dt_input).source.unwrap();
                    let source_node = g.nodes.get(source_node_idx);
                    if let IntermediateGraphNodeVariant::Xor{..} = source_node.variant {
                        debug!(logger, "intermed2input - also pre-mapping buried register XOR";
                            "name" => &source_node.name,
                            "intermed" => source_node_idx,
                            "inputgraph" => newg_idx);
                        mcs_map.insert(source_node_idx, newg_idx);
                    }
                },
                IntermediateGraphNodeVariant::InBuf{output, ..} => {
                    let mut maybe_reg_index = None;
                    let mut maybe_reg_name = None;
                    for &sink_node_idx in &g.nets.get(output).sinks {
                        let sink_node = g.nodes.get(sink_node_idx);

                        if let IntermediateGraphNodeVariant::Reg{..} = sink_node.variant {
                            assert!(maybe_reg_index.is_none());
                            maybe_reg_index = Some(sink_node_idx);
                            maybe_reg_name = Some(&sink_node.name);
                        }
                    }

                    if let Some(reg_idx) = maybe_reg_index {
                        debug!(logger, "intermed2input - also pre-mapping registered IBUF FF";
                            "name" => maybe_reg_name.unwrap(),
                            "intermed" => reg_idx,
                            "inputgraph" => newg_idx);
                        mcs_map.insert(reg_idx, newg_idx);
                    }
                },
                _ => {}
            }
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
            n_idx: ObjPoolIndex<IntermediateGraphNode>, logger: &slog::Logger)
            -> Result<InputGraphAnyPoolIdx, IntermedToInputError> {

            let n = s.g.nodes.get(n_idx);

            if s.consumed_inputs.contains(&n_idx.get_raw_i()) {
                // We're already here, but what are we?
                let ret = match n.variant {
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
                };
                debug!(logger, "intermed2input - found existing node";
                    "name" => &n.name,
                    "intermed" => n_idx,
                    "inputgraph" => ret);
                return Ok(ret);
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
                                    error!(logger, "intermed2input - input is tied low but OE isn't used";
                                        "name" => &n.name);
                                    return Err(IntermedToInputError::WrongTiedValue(n.name.to_owned()));
                                }
                                // FIXME: Copypasta
                                let input_n = s.g.nets.get(oe.unwrap()).source.unwrap();
                                let input_type = match s.g.nodes.get(input_n).variant {
                                    IntermediateGraphNodeVariant::Xor{..} => {
                                        Some(InputGraphIOInputType::Xor)
                                    },
                                    IntermediateGraphNodeVariant::Reg{..} => {
                                        Some(InputGraphIOInputType::Reg)
                                    },
                                    _ => {
                                        error!(logger, "intermed2input - input is not an XOR or a register";
                                            "name" => &n.name);
                                        return Err(IntermedToInputError::WrongConnectionType(n.name.to_owned()));
                                    },
                                };

                                // We need to recursively process this. Update the relevant map as well to make sure we
                                // don't mess up along the way.
                                s.mcs_map.insert(input_n, newg_idx);
                                process_one_intermed_node(s, input_n, logger)?;

                                input_type
                            } else if input.unwrap() == s.g.vdd_net {
                                error!(logger, "intermed2input - input is tied to a constant one";
                                    "name" => &n.name);
                                return Err(IntermedToInputError::WrongTiedValue(n.name.to_owned()));
                            } else {
                                let input_n = s.g.nets.get(input.unwrap()).source.unwrap();
                                let input_type = match s.g.nodes.get(input_n).variant {
                                    IntermediateGraphNodeVariant::Xor{..} => {
                                        Some(InputGraphIOInputType::Xor)
                                    },
                                    IntermediateGraphNodeVariant::Reg{..} => {
                                        Some(InputGraphIOInputType::Reg)
                                    },
                                    _ => {
                                        error!(logger, "intermed2input - input is not an XOR or a register";
                                            "name" => &n.name);
                                        return Err(IntermedToInputError::WrongConnectionType(n.name.to_owned()));
                                    },
                                };

                                // We need to recursively process this. Update the relevant map as well to make sure we
                                // don't mess up along the way.
                                s.mcs_map.insert(input_n, newg_idx);
                                process_one_intermed_node(s, input_n, logger)?;

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
                                    let oe_n = s.g.nets.get(oe.unwrap()).source.unwrap();
                                    let oe_newg_n = process_one_intermed_node(s, oe_n, logger)?;
                                    Some(match oe_newg_n {
                                        InputGraphAnyPoolIdx::PTerm(x) => InputGraphIOOEType::PTerm(x),
                                        InputGraphAnyPoolIdx::BufgGTS(x) => InputGraphIOOEType::GTS(x),
                                        _ => {
                                            error!(logger, "intermed2input - OE is not a P-term or GTS";
                                                "name" => &n.name);
                                            return Err(IntermedToInputError::WrongConnectionType(n.name.to_owned()));
                                        },
                                    })
                                }
                            }
                        } else {
                            None
                        }
                    };

                    {
                        info!(logger, "intermed2input - adding IO bits";
                            "name" => &n.name,
                            "intermed" => n_idx,
                            "inputgraph" => newg_idx);
                        let newg_n = s.mcs.get_mut(newg_idx);
                        assert!(newg_n.io_bits.is_none());
                        newg_n.name = combine_names(&newg_n.name, &n.name);
                        newg_n.requested_loc = combine_locs(newg_n.requested_loc, n.location, logger)?;
                        newg_n.io_bits = Some(InputGraphIOBuf {
                            input,
                            oe,
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
                        info!(logger, "intermed2input - adding input bits";
                            "name" => &n.name,
                            "intermed" => n_idx,
                            "inputgraph" => newg_idx);
                        let newg_n = s.mcs.get_mut(newg_idx);
                        assert!(newg_n.io_bits.is_none());
                        newg_n.name = combine_names(&newg_n.name, &n.name);
                        newg_n.requested_loc = combine_locs(newg_n.requested_loc, n.location, logger)?;
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
                        let or_n = s.g.nets.get(orterm_input.unwrap()).source.unwrap();

                        // This is manually inlined so that we don't need to add hacky extra logic to return the
                        // desired array of actual Pterms.
                        s.consumed_inputs.insert(or_n.get_raw_i());
                        // Update the map even though this shouldn't actually be needed.
                        assert!(!s.mcs_map.contains_key(&or_n));
                        s.mcs_map.insert(or_n, newg_idx);

                        if let IntermediateGraphNodeVariant::OrTerm{ref inputs, ..} = s.g.nodes.get(or_n).variant {
                            for x in inputs {
                                let input_n = s.g.nets.get(*x).source.unwrap();
                                if let IntermediateGraphNodeVariant::AndTerm{..} =
                                    s.g.nodes.get(input_n).variant {} else {

                                    error!(logger, "intermed2input - input is not a P-term";
                                        "name" => &s.g.nodes.get(input_n).name);
                                    return Err(IntermedToInputError::WrongConnectionType(
                                        s.g.nodes.get(input_n).name.to_owned()));
                                }
                                // We need to recursively process this
                                let input_newg_any = process_one_intermed_node(s, input_n, logger)?;
                                let input_newg = if let InputGraphAnyPoolIdx::PTerm(x) = input_newg_any { x } else {
                                    panic!("Internal error - not a product term?");
                                };

                                orterm_inputs.push(input_newg);
                            }
                        } else {
                            error!(logger, "intermed2input - input is not an OR term";
                                "name" => &n.name);
                            return Err(IntermedToInputError::WrongConnectionType(n.name.to_owned()));
                        }
                    }

                    // Visit PTC
                    let mut invert_out = invert_out;
                    let ptc_input = {
                        if andterm_input.is_some() {
                            if andterm_input.unwrap() == s.g.vdd_net {
                                invert_out = !invert_out;
                                None
                            } else if andterm_input.unwrap() == s.g.vss_net {
                                None
                            } else {
                                let ptc_n = s.g.nets.get(andterm_input.unwrap()).source.unwrap();
                                let ptc_newg_n = process_one_intermed_node(s, ptc_n, logger)?;
                                if let InputGraphAnyPoolIdx::PTerm(x) = ptc_newg_n {
                                    Some(x)
                                } else {
                                    error!(logger, "intermed2input - PTC input is not a P-term";
                                        "name" => &n.name);
                                    return Err(IntermedToInputError::WrongConnectionType(n.name.to_owned()));
                                }
                            }
                        } else {
                            None
                        }
                    };

                    {
                        info!(logger, "intermed2input - adding XOR bits";
                            "name" => &n.name,
                            "intermed" => n_idx,
                            "inputgraph" => newg_idx);
                        let newg_n = s.mcs.get_mut(newg_idx);
                        assert!(newg_n.xor_bits.is_none());
                        newg_n.name = combine_names(&newg_n.name, &n.name);
                        newg_n.requested_loc = combine_locs(newg_n.requested_loc, n.location, logger)?;
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
                        let input_n = s.g.nets.get(*x).source.unwrap();
                        let input_type = match s.g.nodes.get(input_n).variant {
                            IntermediateGraphNodeVariant::Xor{..} => InputGraphPTermInputType::Xor,
                            IntermediateGraphNodeVariant::Reg{..} => InputGraphPTermInputType::Reg,
                            IntermediateGraphNodeVariant::IOBuf{..} |
                            IntermediateGraphNodeVariant::InBuf{..} => InputGraphPTermInputType::Pin,
                            _ => {
                                error!(logger, "intermed2input - input is not a register, XOR, or IO pad";
                                    "name" => &n.name);
                                return Err(IntermedToInputError::WrongConnectionType(n.name.to_owned()));
                            },
                        };

                        // We need to recursively process this
                        let input_newg_any = process_one_intermed_node(s, input_n, logger)?;
                        let input_newg = if let InputGraphAnyPoolIdx::Macrocell(x) = input_newg_any { x } else {
                            panic!("Internal error - not a macrocell?");
                        };

                        // Flag the appropriate bits as used
                        {
                            let input_newg_n = s.mcs.get_mut(input_newg);
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
                        let input_n = s.g.nets.get(*x).source.unwrap();
                        let input_type = match s.g.nodes.get(input_n).variant {
                            IntermediateGraphNodeVariant::Xor{..} => InputGraphPTermInputType::Xor,
                            IntermediateGraphNodeVariant::Reg{..} => InputGraphPTermInputType::Reg,
                            IntermediateGraphNodeVariant::IOBuf{..} |
                            IntermediateGraphNodeVariant::InBuf{..} => InputGraphPTermInputType::Pin,
                            _ => {
                                error!(logger, "intermed2input - input is not a register, XOR, or IO pad";
                                    "name" => &n.name);
                                return Err(IntermedToInputError::WrongConnectionType(n.name.to_owned()));
                            },
                        };

                        // We need to recursively process this
                        let input_newg_any = process_one_intermed_node(s, input_n, logger)?;
                        let input_newg = if let InputGraphAnyPoolIdx::Macrocell(x) = input_newg_any { x } else {
                            panic!("Internal error - not a macrocell?");
                        };

                        // Flag the appropriate bits as used
                        {
                            let input_newg_n = s.mcs.get_mut(input_newg);
                            match input_type {
                                InputGraphPTermInputType::Pin => input_newg_n.io_feedback_used = true,
                                InputGraphPTermInputType::Xor => input_newg_n.xor_feedback_used = true,
                                InputGraphPTermInputType::Reg => input_newg_n.reg_feedback_used = true,
                            }
                        }

                        inputs_comp_new.push((input_type, input_newg));
                    }

                    let newg_n = InputGraphPTerm {
                        name: n.name.clone(),
                        requested_loc: n.location,
                        inputs_true: inputs_true_new,
                        inputs_comp: inputs_comp_new,
                    };

                    let newg_idx = s.pterms.insert(newg_n);
                    info!(logger, "intermed2input - adding P-term";
                        "name" => &n.name,
                        "intermed" => n_idx,
                        "inputgraph" => newg_idx);
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
                        let input_n = s.g.nets.get(dt_input).source.unwrap();
                        let input_type = match s.g.nodes.get(input_n).variant {
                            IntermediateGraphNodeVariant::Xor{..} => {
                                InputGraphRegInputType::Xor
                            },
                            IntermediateGraphNodeVariant::IOBuf{..} |
                            IntermediateGraphNodeVariant::InBuf{..} => {
                                InputGraphRegInputType::Pin
                            },
                            _ => {
                                error!(logger, "intermed2input - D/T input is not an XOR or IO pad";
                                    "name" => &n.name);
                                return Err(IntermedToInputError::WrongConnectionType(n.name.to_owned()));
                            },
                        };

                        // We need to recursively process this. Update the relevant map as well to make sure we
                        // don't mess up along the way, but only if it's an XOR.
                        if input_type == InputGraphRegInputType::Xor {
                            s.mcs_map.insert(input_n, newg_idx);
                            process_one_intermed_node(s, input_n, logger)?;
                        }

                        input_type
                    };

                    // Visit clock input
                    let clk_input = {
                        let clk_n = s.g.nets.get(clk_input).source.unwrap();
                        let clk_newg_n = process_one_intermed_node(s, clk_n, logger)?;
                        if let InputGraphAnyPoolIdx::PTerm(x) = clk_newg_n {
                            InputGraphRegClockType::PTerm(x)
                        } else if let InputGraphAnyPoolIdx::BufgClk(x) = clk_newg_n {
                            InputGraphRegClockType::GCK(x)
                        } else {
                            error!(logger, "intermed2input - clock input is not a P-term or global clock";
                                "name" => &n.name);
                            return Err(IntermedToInputError::WrongConnectionType(n.name.to_owned()));
                        }
                    };

                    // Visit CE
                    let ce_input = {
                        if ce_input.is_some() {
                            let ce_n = s.g.nets.get(ce_input.unwrap()).source.unwrap();
                            let ce_newg_n = process_one_intermed_node(s, ce_n, logger)?;
                            if let InputGraphAnyPoolIdx::PTerm(x) = ce_newg_n {
                                Some(x)
                            } else {
                                error!(logger, "intermed2input - CE input is not a P-term";
                                    "name" => &n.name);
                                return Err(IntermedToInputError::WrongConnectionType(n.name.to_owned()));
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
                                error!(logger, "intermed2input - set input cannot be tied high";
                                    "name" => &n.name);
                                return Err(IntermedToInputError::WrongTiedValue(n.name.to_owned()));
                            } else {
                                let set_n = s.g.nets.get(set_input.unwrap()).source.unwrap();
                                let set_newg_n = process_one_intermed_node(s, set_n, logger)?;
                                if let InputGraphAnyPoolIdx::PTerm(x) = set_newg_n {
                                    Some(InputGraphRegRSType::PTerm(x))
                                } else if let InputGraphAnyPoolIdx::BufgGSR(x) = set_newg_n {
                                    Some(InputGraphRegRSType::GSR(x))
                                } else {
                                    error!(logger, "intermed2input - set input is not a P-term or GSR";
                                        "name" => &n.name);
                                    return Err(IntermedToInputError::WrongConnectionType(n.name.to_owned()));
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
                                error!(logger, "intermed2input - reset input cannot be tied high";
                                    "name" => &n.name);
                                return Err(IntermedToInputError::WrongTiedValue(n.name.to_owned()));
                            } else {
                                let reset_n = s.g.nets.get(reset_input.unwrap()).source.unwrap();
                                let reset_newg_n = process_one_intermed_node(s, reset_n, logger)?;
                                if let InputGraphAnyPoolIdx::PTerm(x) = reset_newg_n {
                                    Some(InputGraphRegRSType::PTerm(x))
                                } else if let InputGraphAnyPoolIdx::BufgGSR(x) = reset_newg_n {
                                    Some(InputGraphRegRSType::GSR(x))
                                } else {
                                    error!(logger, "intermed2input - reset input is not a P-term or GSR";
                                        "name" => &n.name);
                                    return Err(IntermedToInputError::WrongConnectionType(n.name.to_owned()));
                                }
                            }
                        } else {
                            None
                        }
                    };

                    {
                        info!(logger, "intermed2input - adding register bits";
                            "name" => &n.name,
                            "intermed" => n_idx,
                            "inputgraph" => newg_idx);
                        let newg_n = s.mcs.get_mut(newg_idx);
                        assert!(newg_n.reg_bits.is_none());
                        newg_n.name = combine_names(&newg_n.name, &n.name);
                        newg_n.requested_loc = combine_locs(newg_n.requested_loc, n.location, logger)?;
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

                    let input_n = s.g.nets.get(input).source.unwrap();
                    match s.g.nodes.get(input_n).variant {
                        IntermediateGraphNodeVariant::IOBuf{..} |
                        IntermediateGraphNodeVariant::InBuf{..} => {},
                        _ => {
                            error!(logger, "intermed2input - input is not an IO pad";
                                "name" => &n.name);
                            return Err(IntermedToInputError::WrongConnectionType(n.name.to_owned()));
                        },
                    };

                    // We need to recursively process this
                    let input_newg_any = process_one_intermed_node(s, input_n, logger)?;
                    let input_newg = if let InputGraphAnyPoolIdx::Macrocell(x) = input_newg_any { x } else {
                        panic!("Internal error - not a macrocell?");
                    };

                    let newg_n = InputGraphBufgClk {
                        name: n.name.clone(),
                        requested_loc: n.location,
                        input: input_newg,
                    };

                    let newg_idx = s.bufg_clks.insert(newg_n);
                    info!(logger, "intermed2input - adding BUFG";
                        "name" => &n.name,
                        "intermed" => n_idx,
                        "inputgraph" => newg_idx);
                    s.bufg_clks_map.insert(n_idx, newg_idx);
                    Ok(InputGraphAnyPoolIdx::BufgClk(newg_idx))
                },
                IntermediateGraphNodeVariant::BufgGTS{input, invert, ..} => {
                    let input_n = s.g.nets.get(input).source.unwrap();
                    match s.g.nodes.get(input_n).variant {
                        IntermediateGraphNodeVariant::IOBuf{..} |
                        IntermediateGraphNodeVariant::InBuf{..} => {},
                        _ => {
                            error!(logger, "intermed2input - input is not an IO pad";
                                "name" => &n.name);
                            return Err(IntermedToInputError::WrongConnectionType(n.name.to_owned()));
                        },
                    };

                    // We need to recursively process this
                    let input_newg_any = process_one_intermed_node(s, input_n, logger)?;
                    let input_newg = if let InputGraphAnyPoolIdx::Macrocell(x) = input_newg_any { x } else {
                        panic!("Internal error - not a macrocell?");
                    };

                    let newg_n = InputGraphBufgGTS {
                        name: n.name.clone(),
                        requested_loc: n.location,
                        input: input_newg,
                        invert,
                    };

                    let newg_idx = s.bufg_gts.insert(newg_n);
                    info!(logger, "intermed2input - adding BUFGTS";
                        "name" => &n.name,
                        "intermed" => n_idx,
                        "inputgraph" => newg_idx);
                    s.bufg_gts_map.insert(n_idx, newg_idx);
                    Ok(InputGraphAnyPoolIdx::BufgGTS(newg_idx))
                },
                IntermediateGraphNodeVariant::BufgGSR{input, invert, ..} => {
                    let input_n = s.g.nets.get(input).source.unwrap();
                    match s.g.nodes.get(input_n).variant {
                        IntermediateGraphNodeVariant::IOBuf{..} |
                        IntermediateGraphNodeVariant::InBuf{..} => {},
                        _ => {
                            error!(logger, "intermed2input - input is not an IO pad";
                                "name" => &n.name);
                            return Err(IntermedToInputError::WrongConnectionType(n.name.to_owned()));
                        },
                    };

                    // We need to recursively process this
                    let input_newg_any = process_one_intermed_node(s, input_n, logger)?;
                    let input_newg = if let InputGraphAnyPoolIdx::Macrocell(x) = input_newg_any { x } else {
                        panic!("Internal error - not a macrocell?");
                    };

                    let newg_n = InputGraphBufgGSR {
                        name: n.name.clone(),
                        requested_loc: n.location,
                        input: input_newg,
                        invert,
                    };

                    let newg_idx = s.bufg_gsr.insert(newg_n);
                    info!(logger, "intermed2input - adding BUFGSR";
                        "name" => &n.name,
                        "intermed" => n_idx,
                        "inputgraph" => newg_idx);
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
                process_one_intermed_node(&mut s, oldg_mc, &logger)?;
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

        ret.unfuse_pterms(&logger);
        ret.sanity_check(&logger)?;

        Ok(ret)
    }

    fn sanity_check(&self, logger: &slog::Logger) -> Result<(), IntermedToInputError> {
        // Check various connectivity rules in the CPLD
        for x in self.mcs.iter() {
            if x.io_feedback_used && x.io_bits.is_none() {
                error!(logger, "intermed2input (sanity) - the IO part of the macrocell was used, \
                    but the IO data is missing";
                    "name" => &x.name);
                return Err(IntermedToInputError::SanityCheckError(
                    "Used IO input but there is no IO data?"));
            }

            if x.reg_feedback_used && x.reg_bits.is_none() {
                error!(logger, "intermed2input (sanity) - the register part of the macrocell was used, \
                    but the register data is missing";
                    "name" => &x.name);
                return Err(IntermedToInputError::SanityCheckError(
                    "Used register input but there is no register data?"));
            }

            if x.xor_feedback_used && x.xor_bits.is_none() {
                error!(logger, "intermed2input (sanity) - the XOR part of the macrocell was used, \
                    but the XOR data is missing";
                    "name" => &x.name);
                return Err(IntermedToInputError::SanityCheckError(
                    "Used XOR input but there is no XOR data?"));
            }

            if x.io_feedback_used && x.reg_feedback_used && x.xor_feedback_used {
                error!(logger, "intermed2input (sanity) - used more than the possible feedback paths";
                    "name" => &x.name);
                return Err(IntermedToInputError::TooManyFeedbacksUsed(x.name.to_owned()));
            }
        }

        // Check for duplicate inputs to p-terms
        for pt in self.pterms.iter() {
            let inputs_true_set: HashSet<(InputGraphPTermInputType, ObjPoolIndex<InputGraphMacrocell>)> =
                HashSet::from_iter(pt.inputs_true.iter().cloned());
            let inputs_comp_set: HashSet<(InputGraphPTermInputType, ObjPoolIndex<InputGraphMacrocell>)> =
                HashSet::from_iter(pt.inputs_comp.iter().cloned());

            if inputs_true_set.len() != pt.inputs_true.len() {
                error!(logger, "intermed2input (sanity) - duplicate true input to p-term";
                    "name" => &pt.name);
                return Err(IntermedToInputError::WrongPTermInputs(pt.name.to_owned()));
            }
            if inputs_comp_set.len() != pt.inputs_comp.len() {
                error!(logger, "intermed2input (sanity) - duplicate comp input to p-term";
                    "name" => &pt.name);
                return Err(IntermedToInputError::WrongPTermInputs(pt.name.to_owned()));
            }

            if inputs_true_set.intersection(&inputs_comp_set).count() != 0 {
                error!(logger, "intermed2input (sanity) - input to both true and comp input of p-term";
                    "name" => &pt.name);
                return Err(IntermedToInputError::WrongPTermInputs(pt.name.to_owned()));
            }
        }

        // Check that LOC constraints between the macrocells and p-terms are possible
        for x in self.mcs.iter() {
            macro_rules! check_pterm {
                ($mc_req_loc:expr, $pt:expr) => {
                    if let Some(pt_req_loc) = self.pterms.get($pt).requested_loc {
                        if $mc_req_loc.fb != pt_req_loc.fb {
                            error!(logger, "intermed2input (sanity) - macrocell uses P-term in wrong FB";
                                "macrocell name" => &x.name,
                                "macrocell FB" => $mc_req_loc.fb,
                                "p-term FB" => pt_req_loc.fb);
                            return Err(IntermedToInputError::LocMismatchedFB($mc_req_loc.fb, pt_req_loc.fb));
                        }
                    }
                }
            }

            if let Some(mc_req_loc) = x.requested_loc {
                if let Some(ref xor) = x.xor_bits {
                    if let Some(pterm) = xor.andterm_input {
                        check_pterm!(mc_req_loc, pterm);
                    }

                    for &pterm in &xor.orterm_inputs {
                        check_pterm!(mc_req_loc, pterm);
                    }
                }

                if let Some(ref reg) = x.reg_bits {
                    if let Some(pterm) = reg.ce_input {
                        check_pterm!(mc_req_loc, pterm);
                    }

                    if let InputGraphRegClockType::PTerm(pterm) = reg.clk_input {
                        check_pterm!(mc_req_loc, pterm);
                    }

                    if let Some(InputGraphRegRSType::PTerm(pterm)) = reg.set_input {
                        check_pterm!(mc_req_loc, pterm);
                    }

                    if let Some(InputGraphRegRSType::PTerm(pterm)) = reg.reset_input {
                        check_pterm!(mc_req_loc, pterm);
                    }
                }

                if let Some(ref io) = x.io_bits {
                    if let Some(InputGraphIOOEType::PTerm(pterm)) = io.oe {
                        check_pterm!(mc_req_loc, pterm);
                    }
                }
            }
        }

        Ok(())
    }

    // XXX does this function actually do anything? Or can we get rid of it?
    fn unfuse_pterms(&mut self, logger: &slog::Logger) {
        let mut used_pterms = HashSet::new();

        for mc in self.mcs.iter_mut() {
            macro_rules! clone_pterm {
                ($pterm:expr) => {{
                    let cloned_pterm = self.pterms.get($pterm).clone();
                    debug!(logger, "cloning a P-term";
                        "pterm name" => &cloned_pterm.name,
                        "referencing node" => &mc.name);
                    let new_pterm = self.pterms.insert(cloned_pterm);
                    new_pterm
                }}
            }

            if mc.xor_bits.is_some() {
                let xor = mc.xor_bits.as_mut().unwrap();

                if let Some(pterm) = xor.andterm_input {
                    if used_pterms.contains(&pterm) {
                        xor.andterm_input = Some(clone_pterm!(pterm));
                    }
                    used_pterms.insert(pterm);
                }

                for i in 0..xor.orterm_inputs.len() {
                    let pterm = xor.orterm_inputs[i];
                    if used_pterms.contains(&pterm) {
                        xor.orterm_inputs[i] = clone_pterm!(pterm);
                    }
                    used_pterms.insert(pterm);
                }
            }

            if mc.reg_bits.is_some() {
                let reg = mc.reg_bits.as_mut().unwrap();

                if let Some(pterm) = reg.ce_input {
                    if used_pterms.contains(&pterm) {
                        reg.ce_input = Some(clone_pterm!(pterm));
                    }
                    used_pterms.insert(pterm);
                }

                if let InputGraphRegClockType::PTerm(pterm) = reg.clk_input {
                    if used_pterms.contains(&pterm) {
                        reg.clk_input = InputGraphRegClockType::PTerm(clone_pterm!(pterm));
                    }
                    used_pterms.insert(pterm);
                }

                if let Some(InputGraphRegRSType::PTerm(pterm)) = reg.set_input {
                    if used_pterms.contains(&pterm) {
                        reg.set_input = Some(InputGraphRegRSType::PTerm(clone_pterm!(pterm)));
                    }
                    used_pterms.insert(pterm);
                }

                if let Some(InputGraphRegRSType::PTerm(pterm)) = reg.reset_input {
                    if used_pterms.contains(&pterm) {
                        reg.reset_input = Some(InputGraphRegRSType::PTerm(clone_pterm!(pterm)));
                    }
                    used_pterms.insert(pterm);
                }
            }

            if mc.io_bits.is_some() {
                let io = mc.io_bits.as_mut().unwrap();

                if let Some(InputGraphIOOEType::PTerm(pterm)) = io.oe {
                    if used_pterms.contains(&pterm) {
                        io.oe = Some(InputGraphIOOEType::PTerm(clone_pterm!(pterm)));
                    }
                    used_pterms.insert(pterm);
                }
            }
        }
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
        let intermed_graph = serde_json::from_slice(&input_data).unwrap();
        // This is what we get
        let our_data_structure = InputGraph::from_intermed_graph(&intermed_graph, None).unwrap();

        // Read reference json
        let mut output_path = input_path.to_path_buf();
        output_path.set_extension("out");
        let mut output_data = Vec::new();
        File::open(&output_path).unwrap().read_to_end(&mut output_data).unwrap();
        let reference_data_structure = serde_json::from_slice(&output_data).unwrap();

        assert_eq!(our_data_structure, reference_data_structure);
    }

    // Include list of actual tests to run
    include!(concat!(env!("OUT_DIR"), "/netlist-reftests.rs"));
}
