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

// Toplevel bitstrem stuff

use std::io::Write;

use *;
use fb::{read_32_fb_logical};
use mc::{read_32_iob_logical, read_32_extra_ibuf_logical};

pub struct XC2Bitstream {
    pub speed_grade: String,
    pub package: String,
    pub bits: XC2BitstreamBits,
}

impl XC2Bitstream {
    pub fn dump_human_readable(&self, writer: &mut Write) {
        write!(writer, "xc2bit dump\n").unwrap();
        write!(writer, "device speed grade: {}\n", self.speed_grade).unwrap();
        write!(writer, "device package: {}\n", self.package).unwrap();
        self.bits.dump_human_readable(writer);
    }
}

pub struct XC2GlobalNets {
    pub gck_enable: [bool; 3],
    pub gsr_enable: bool,
    // false = active low, true = active high
    pub gsr_invert: bool,
    pub gts_enable: [bool; 4],
    // false = used as T, true = used as !T
    pub gts_invert: [bool; 4],
    // false = keeper, true = pull-up
    pub global_pu: bool,
}

impl XC2GlobalNets {
    pub fn dump_human_readable(&self, writer: &mut Write) {
        write!(writer, "\n").unwrap();
        write!(writer, "GCK0 {}\n", if self.gck_enable[0] {"enabled"} else {"disabled"}).unwrap();
        write!(writer, "GCK1 {}\n", if self.gck_enable[1] {"enabled"} else {"disabled"}).unwrap();
        write!(writer, "GCK2 {}\n", if self.gck_enable[2] {"enabled"} else {"disabled"}).unwrap();

        write!(writer, "GSR {}, active {}\n",
            if self.gsr_enable {"enabled"} else {"disabled"},
            if self.gsr_invert {"high"} else {"low"}).unwrap();

        write!(writer, "GTS0 {}, acts as {}\n",
            if self.gts_enable[0] {"enabled"} else {"disabled"},
            if self.gts_invert[0] {"!T"} else {"T"}).unwrap();
        write!(writer, "GTS1 {}, acts as {}\n",
            if self.gts_enable[1] {"enabled"} else {"disabled"},
            if self.gts_invert[1] {"!T"} else {"T"}).unwrap();
        write!(writer, "GTS2 {}, acts as {}\n",
            if self.gts_enable[2] {"enabled"} else {"disabled"},
            if self.gts_invert[2] {"!T"} else {"T"}).unwrap();
        write!(writer, "GTS3 {}, acts as {}\n",
            if self.gts_enable[3] {"enabled"} else {"disabled"},
            if self.gts_invert[3] {"!T"} else {"T"}).unwrap();

        write!(writer, "global termination is {}\n", if self.global_pu {"pull-up"} else {"bus hold"}).unwrap();
    }
}

fn read_32_global_nets_logical(fuses: &[bool]) -> XC2GlobalNets {
    XC2GlobalNets {
        gck_enable: [
            fuses[12256],
            fuses[12257],
            fuses[12258],
        ],
        gsr_enable: fuses[12260],
        gsr_invert: fuses[12259],
        gts_enable: [
            !fuses[12262],
            !fuses[12264],
            !fuses[12266],
            !fuses[12268],
        ],
        gts_invert: [
            fuses[12261],
            fuses[12263],
            fuses[12265],
            fuses[12267],
        ],
        global_pu: fuses[12269],
    }
}

pub enum XC2BitstreamBits {
    XC2C32A {
        fb: [XC2BistreamFB; 2],
        iobs: [XC2MCSmallIOB; 32],
        inpin: XC2ExtraIBuf,
        global_nets: XC2GlobalNets,
        // false = low, true = high
        legacy_ivoltage: bool,
        legacy_ovoltage: bool,
        ivoltage: [bool; 2],
        ovoltage: [bool; 2],
    },
}

impl XC2BitstreamBits {
    pub fn dump_human_readable(&self, writer: &mut Write) {
        match self {
            &XC2BitstreamBits::XC2C32A{
                ref fb, ref iobs, ref inpin, ref global_nets, ref legacy_ivoltage, ref legacy_ovoltage,
                ref ivoltage, ref ovoltage} => {

                write!(writer, "device type: XC2C32A\n").unwrap();
                write!(writer, "legacy output voltage range: {}\n", if *legacy_ovoltage {"high"} else {"low"}).unwrap();
                write!(writer, "legacy input voltage range: {}\n", if *legacy_ivoltage {"high"} else {"low"}).unwrap();
                write!(writer, "bank 0 output voltage range: {}\n", if ovoltage[0] {"high"} else {"low"}).unwrap();
                write!(writer, "bank 1 output voltage range: {}\n", if ovoltage[1] {"high"} else {"low"}).unwrap();
                write!(writer, "bank 0 input voltage range: {}\n", if ivoltage[0] {"high"} else {"low"}).unwrap();
                write!(writer, "bank 1 input voltage range: {}\n", if ivoltage[1] {"high"} else {"low"}).unwrap();
                global_nets.dump_human_readable(writer);

                for i in 0..32 {
                    iobs[i].dump_human_readable(i as u32, writer);
                }

                inpin.dump_human_readable(writer);

                fb[0].dump_human_readable(0, writer);
                fb[1].dump_human_readable(1, writer);
            },
            _ => panic!("don't know how to print this")
        }
    }
}

pub fn read_32_bitstream_logical(fuses: &[bool]) -> Result<XC2BitstreamBits, &'static str> {
    let mut fb = [XC2BistreamFB::default(); 2];
    for i in 0..fb.len() {
        let res = read_32_fb_logical(fuses, i);
        if let Err(err) = res {
            return Err(err);
        }
        fb[i] = res.unwrap();
    };

    let mut iobs = [XC2MCSmallIOB::default(); 32];
    for i in 0..iobs.len() {
        let base_fuse = if i < 16 {
            5696
        } else {
            11824
        };
        let res = read_32_iob_logical(fuses, base_fuse, i % 16);
        if let Err(err) = res {
            return Err(err);
        }
        iobs[i] = res.unwrap();
    }

    let inpin = read_32_extra_ibuf_logical(fuses);

    let global_nets = read_32_global_nets_logical(fuses);

    Ok(XC2BitstreamBits::XC2C32A {
        fb: fb,
        iobs: iobs,
        inpin: inpin,
        global_nets: global_nets,
        legacy_ovoltage: !fuses[12270],
        legacy_ivoltage: !fuses[12271],
        ivoltage: [
            !fuses[12274],
            !fuses[12276],
        ],
        ovoltage: [
            !fuses[12275],
            !fuses[12277],
        ]
    })
}
