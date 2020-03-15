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

extern crate serde_json;

use std::error;
use std::fmt;
use slog::Drain;
use xc2bit::*;

use crate::*;

#[derive(Copy, Clone, PartialEq, Eq, Hash, Debug, Serialize, Deserialize)]
pub enum ParOutputFormat {
    Jed,
    Crbit,
}

#[derive(Clone, Debug)]
pub struct XC2ParOptions {
    pub(crate) max_iter: u32,
    pub(crate) rng_seed: [u32; 4],
    output_fmt: ParOutputFormat,
}

impl XC2ParOptions {
    pub fn new() -> Self {
        Self {
            max_iter: 1000,
            rng_seed: [0, 0, 0, 1],
            output_fmt: ParOutputFormat::Jed,
        }
    }

    pub fn max_iter(&mut self, max_iter: u32) -> &mut Self {
        self.max_iter = max_iter;

        self
    }

    pub fn with_prng_seed(&mut self, seed: [u32; 4]) -> &mut Self {
        self.rng_seed = seed;

        self
    }

    pub fn output_format(&mut self, format: ParOutputFormat) -> &mut Self {
        self.output_fmt = format;

        self
    }
}

impl Default for XC2ParOptions {
    fn default() -> Self {
        Self::new()
    }
}

#[derive(Debug)]
pub enum PARFlowError {
    SerdeError(serde_json::Error),
    FrontendError(FrontendError),
    IntermedToInputError(IntermedToInputError),
    OutputWriteError(std::io::Error),
    PARIterationsExceeded,
    PARSanityCheckFailed(PARSanityResult),
}

impl error::Error for PARFlowError {
    fn description(&self) -> &'static str {
        match self {
            &PARFlowError::SerdeError(_) => "json read failed",
            &PARFlowError::FrontendError(_) => "frontend pass failed",
            &PARFlowError::IntermedToInputError(_) => "intermediate pass failed",
            &PARFlowError::OutputWriteError(_) => "writing output failed",
            &PARFlowError::PARIterationsExceeded => "",
            &PARFlowError::PARSanityCheckFailed(_) => "",
        }
    }

    fn source(&self) -> Option<&(dyn error::Error + 'static)> {
        match self {
            &PARFlowError::SerdeError(ref inner) => {
                Some(inner)
            },
            &PARFlowError::FrontendError(ref inner) => {
                Some(inner)
            },
            &PARFlowError::IntermedToInputError(ref inner) => {
                Some(inner)
            },
            &PARFlowError::OutputWriteError(ref inner) => {
                Some(inner)
            },
            _ => None,
        }
    }
}

impl fmt::Display for PARFlowError {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        match self {
            &PARFlowError::PARIterationsExceeded => {
                write!(f, "maximum iterations exceeded")
            },
            &PARFlowError::PARSanityCheckFailed(_) => {
                write!(f, "PAR sanity check failed")
            },
            &PARFlowError::SerdeError(ref inner) => {
                write!(f, "{}", inner)
            },
            &PARFlowError::FrontendError(ref inner) => {
                write!(f, "{}", inner)
            },
            &PARFlowError::IntermedToInputError(ref inner) => {
                write!(f, "{}", inner)
            },
            &PARFlowError::OutputWriteError(ref inner) => {
                write!(f, "{}", inner)
            },
        }
    }
}

impl From<serde_json::Error> for PARFlowError {
    fn from(inner: serde_json::Error) -> Self {
        PARFlowError::SerdeError(inner)
    }
}

impl From<FrontendError> for PARFlowError {
    fn from(inner: FrontendError) -> Self {
        PARFlowError::FrontendError(inner)
    }
}

impl From<IntermedToInputError> for PARFlowError {
    fn from(inner: IntermedToInputError) -> Self {
        PARFlowError::IntermedToInputError(inner)
    }
}

impl From<std::io::Error> for PARFlowError {
    fn from(inner: std::io::Error) -> Self {
        PARFlowError::OutputWriteError(inner)
    }
}

pub fn xc2par_complete_flow<R, W, L>(options: &XC2ParOptions, device_type: XC2DeviceSpeedPackage, input: R, output: W,
    logger: L) -> Result<(), PARFlowError> where R: std::io::Read, W: std::io::Write, L: Into<Option<slog::Logger>> {

    let logger = logger.into().unwrap_or(slog::Logger::root(slog_stdlog::StdLog.fuse(), o!()));

    let yosys_netlist = yosys_netlist_json::Netlist::from_reader(input)?;
    let intermediate_graph = IntermediateGraph::from_yosys_netlist(&yosys_netlist,
        logger.new(o!("pass" => "yosys -> intermediate")))?;
    let mut input_graph = InputGraph::from_intermed_graph(&intermediate_graph,
        logger.new(o!("pass" => "intermediate -> input")))?;
    let par_result = do_par(&mut input_graph, device_type, options,
        logger.new(o!("pass" => "PAR")));

    match par_result {
        PARResult::Success(x) => {
            let bitstream = produce_bitstream(device_type, &input_graph, &x);

            if options.output_fmt == ParOutputFormat::Jed {
                bitstream.to_jed(output)?;
            } else {
                bitstream.to_crbit().write_to_writer(output)?;
            }

            Ok(())
        },
        PARResult::FailureSanity(x) => Err(PARFlowError::PARSanityCheckFailed(x)),
        PARResult::FailureIterationsExceeded => Err(PARFlowError::PARIterationsExceeded),
    }
}
