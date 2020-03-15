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

use crate::util::{b2s};

use std::error;
use std::fmt;

use jedec::*;

/// Errors that can occur when parsing a bitstream
#[derive(Clone, Debug, PartialEq, Eq)]
pub enum XC2BitError {
    /// The .jed file could not be parsed
    JedParseError(JedParserError),
    /// The device name is invalid
    BadDeviceName(String),
    /// The number of fuses was incorrect for the device
    WrongFuseCount,
    /// An unknown value was used in the `Oe` field
    UnsupportedOeConfiguration((bool, bool, bool, bool)),
    /// An unknown value was used in the ZIA selection bits
    UnsupportedZIAConfiguration(Vec<bool>),
}

impl From<JedParserError> for XC2BitError {
    fn from(err: JedParserError) -> Self {
        XC2BitError::JedParseError(err)
    }
}

impl error::Error for XC2BitError {
    fn source(&self) -> Option<&(dyn error::Error + 'static)> {
        match self {
            &XC2BitError::JedParseError(ref err) => Some(err),
            &XC2BitError::BadDeviceName(_) => None,
            &XC2BitError::WrongFuseCount => None,
            &XC2BitError::UnsupportedOeConfiguration(_) => None,
            &XC2BitError::UnsupportedZIAConfiguration(_) => None,
        }
    }
}

impl fmt::Display for XC2BitError {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        match self {
            &XC2BitError::JedParseError(err) => {
                write!(f, ".jed parsing failed: {}", err)
            },
            &XC2BitError::BadDeviceName(ref devname) => {
                write!(f, "device name \"{}\" is invalid/unsupported", devname)
            },
            &XC2BitError::WrongFuseCount => {
                write!(f, "wrong number of fuses")
            },
            &XC2BitError::UnsupportedOeConfiguration(bits) => {
                write!(f, "unknown Oe field value {}{}{}{}",
                    b2s(bits.0), b2s(bits.1),
                    b2s(bits.2), b2s(bits.3))
            },
            &XC2BitError::UnsupportedZIAConfiguration(ref bits) => {
                write!(f, "unknown ZIA selection bit pattern ")?;
                for &bit in bits {
                    write!(f, "{}", b2s(bit))?;
                }
                Ok(())
            },
        }
    }
}
