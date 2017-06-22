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

use std::error;
use std::error::Error;
use std::fmt;
use std::num;
use std::str;

#[derive(Debug, PartialEq, Eq)]
pub enum JedParserError {
    MissingSTX,
    MissingETX,
    InvalidUtf8(str::Utf8Error),
    InvalidCharacter,
    UnexpectedEnd,
    BadFileChecksum,
    BadFuseChecksum,
    InvalidFuseIndex,
    MissingQF,
    MissingF,
    UnrecognizedField,
}

impl error::Error for JedParserError {
    fn description(&self) -> &'static str {
        match *self {
            JedParserError::MissingSTX => "STX not found",
            JedParserError::MissingETX => "ETX not found",
            JedParserError::InvalidUtf8(_) => "invalid utf8 character",
            JedParserError::InvalidCharacter => "invalid character in field",
            JedParserError::UnexpectedEnd => "unexpected end of file",
            JedParserError::BadFileChecksum => "invalid file checksum",
            JedParserError::BadFuseChecksum => "invalid fuse checksum",
            JedParserError::InvalidFuseIndex => "invalid fuse index value",
            JedParserError::MissingQF => "missing QF field",
            JedParserError::MissingF => "missing F field",
            JedParserError::UnrecognizedField => "unrecognized field",
        }
    }

    fn cause(&self) -> Option<&error::Error> {
        match *self {
            JedParserError::MissingSTX => None,
            JedParserError::MissingETX => None,
            JedParserError::InvalidUtf8(ref err) => Some(err),
            JedParserError::InvalidCharacter => None,
            JedParserError::UnexpectedEnd => None,
            JedParserError::BadFileChecksum => None,
            JedParserError::BadFuseChecksum => None,
            JedParserError::InvalidFuseIndex => None,
            JedParserError::MissingQF => None,
            JedParserError::MissingF => None,
            JedParserError::UnrecognizedField => None,
        }
    }
}

impl fmt::Display for JedParserError {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        if let Some(cause) = self.cause() {
            write!(f, "{}: {}", self.description(), cause)
        } else {
            write!(f, "{}", self.description())
        }
    }
}

impl From<str::Utf8Error> for JedParserError {
    fn from(err: str::Utf8Error) -> JedParserError {
        JedParserError::InvalidUtf8(err)
    }
}

impl From<num::ParseIntError> for JedParserError {
    fn from(_: num::ParseIntError) -> JedParserError {
        JedParserError::InvalidCharacter
    }
}

#[derive(Debug, PartialEq, Eq)]
pub enum XC2BitError {
    JedParseError(JedParserError),
    BadDeviceName(String),
    WrongFuseCount,
    UnsupportedOeConfiguration((bool, bool, bool, bool)),
    UnsupportedZIAConfiguration(Vec<bool>),
}

impl From<JedParserError> for XC2BitError {
    fn from(err: JedParserError) -> XC2BitError {
        XC2BitError::JedParseError(err)
    }
}

impl error::Error for XC2BitError {
    fn description(&self) -> &'static str {
        match *self {
            XC2BitError::JedParseError(_) => ".jed parsing failed",
            XC2BitError::BadDeviceName(_) => "device name is invalid/unsupported",
            XC2BitError::WrongFuseCount => "wrong number of fuses",
            XC2BitError::UnsupportedOeConfiguration(_) => "unknown Oe field value",
            XC2BitError::UnsupportedZIAConfiguration(_) => "unknown ZIA selection bit pattern",
        }
    }

    fn cause(&self) -> Option<&error::Error> {
        match *self {
            XC2BitError::JedParseError(ref err) => Some(err),
            XC2BitError::BadDeviceName(_) => None,
            XC2BitError::WrongFuseCount => None,
            XC2BitError::UnsupportedOeConfiguration(_) => None,
            XC2BitError::UnsupportedZIAConfiguration(_) => None,
        }
    }
}

impl fmt::Display for XC2BitError {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        match *self {
            XC2BitError::JedParseError(_) => {
                write!(f, "{}: {}", self.description(), self.cause().unwrap())
            },
            XC2BitError::BadDeviceName(ref devname) => {
                write!(f, "device name \"{}\" is invalid/unsupported", devname)
            },
            XC2BitError::WrongFuseCount => {
                write!(f, "{}", self.description())
            },
            XC2BitError::UnsupportedOeConfiguration(bits) => {
                write!(f, "unknown Oe field value {}{}{}{}",
                    if bits.0 {"1"} else {"0"}, if bits.1 {"1"} else {"0"},
                    if bits.2 {"1"} else {"0"}, if bits.3 {"1"} else {"0"})
            },
            XC2BitError::UnsupportedZIAConfiguration(ref bits) => {
                write!(f, "unknown ZIA selection bit pattern ")?;
                for bit in bits {
                    write!(f, "{}", if *bit {"1"} else {"0"})?;
                }
                Ok(())
            },
        }
    }
}
