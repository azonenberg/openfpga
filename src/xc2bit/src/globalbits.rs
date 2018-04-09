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

//! Contains functions pertaining to global control bits (e.g. clocks)

use std::io;
use std::io::Write;

use *;
use fusemap_logical::{gck_fuse_idx, gsr_fuse_idx, gts_fuse_idx, global_term_fuse_idx, clock_div_fuse_idx};
use fusemap_physical::{gck_fuse_coords, gsr_fuse_coords, gts_fuse_coords, global_term_fuse_coord,
                       clock_div_fuse_coord};

/// Represents the configuration of the global nets. Coolrunner-II parts have various global control signals that have
/// dedicated low-skew paths.
#[derive(Copy, Clone, Eq, PartialEq, Debug, Serialize, Deserialize)]
pub struct XC2GlobalNets {
    /// Controls whether the three global clock nets are enabled or not
    pub gck_enable: [bool; 3],
    /// Controls whether the global set/reset net is enabled or not
    pub gsr_enable: bool,
    /// Controls the polarity of the global set/reset signal
    ///
    /// `false` = active low, `true` = active high
    pub gsr_invert: bool,
    /// Controls whether the four global tristate nets are enabled or not
    pub gts_enable: [bool; 4],
    /// Controls the polarity of the global tristate signal
    ///
    /// `false` = used as T, `true` = used as !T
    pub gts_invert: [bool; 4],
    /// Controls the mode of the global termination
    ///
    /// `false` = keeper, `true` = pull-up
    pub global_pu: bool,
}

impl Default for XC2GlobalNets {
    /// Returns a "default" global net configuration which has everything disabled.
    fn default() -> Self {
        XC2GlobalNets {
            gck_enable: [false; 3],
            gsr_enable: false,
            gsr_invert: false,
            gts_enable: [false; 4],
            gts_invert: [true; 4],
            global_pu: true,
        }
    }
}

impl XC2GlobalNets {
    /// Dump a human-readable explanation of the global net configuration to the given `writer` object.
    pub fn dump_human_readable<W: Write>(&self, mut writer: W) -> Result<(), io::Error> {
        write!(writer, "\n")?;
        write!(writer, "GCK0 {}\n", if self.gck_enable[0] {"enabled"} else {"disabled"})?;
        write!(writer, "GCK1 {}\n", if self.gck_enable[1] {"enabled"} else {"disabled"})?;
        write!(writer, "GCK2 {}\n", if self.gck_enable[2] {"enabled"} else {"disabled"})?;

        write!(writer, "GSR {}, active {}\n",
            if self.gsr_enable {"enabled"} else {"disabled"},
            if self.gsr_invert {"high"} else {"low"})?;

        write!(writer, "GTS0 {}, acts as {}\n",
            if self.gts_enable[0] {"enabled"} else {"disabled"},
            if self.gts_invert[0] {"!T"} else {"T"})?;
        write!(writer, "GTS1 {}, acts as {}\n",
            if self.gts_enable[1] {"enabled"} else {"disabled"},
            if self.gts_invert[1] {"!T"} else {"T"})?;
        write!(writer, "GTS2 {}, acts as {}\n",
            if self.gts_enable[2] {"enabled"} else {"disabled"},
            if self.gts_invert[2] {"!T"} else {"T"})?;
        write!(writer, "GTS3 {}, acts as {}\n",
            if self.gts_enable[3] {"enabled"} else {"disabled"},
            if self.gts_invert[3] {"!T"} else {"T"})?;

        write!(writer, "global termination is {}\n", if self.global_pu {"pull-up"} else {"bus hold"})?;

        Ok(())
    }

    /// Write the crbit representation of the global net settings to the given `fuse_array`.
    pub fn to_crbit(&self, device: XC2Device, fuse_array: &mut FuseArray) {
        let ((gck0x, gck0y), (gck1x, gck1y), (gck2x, gck2y)) = gck_fuse_coords(device);
        fuse_array.set(gck0x, gck0y, self.gck_enable[0]);
        fuse_array.set(gck1x, gck1y, self.gck_enable[1]);
        fuse_array.set(gck2x, gck2y, self.gck_enable[2]);

        let ((gsren_x, gsren_y), (gsrinv_x, gsrinv_y)) = gsr_fuse_coords(device);
        fuse_array.set(gsren_x, gsren_y, self.gsr_enable);
        fuse_array.set(gsrinv_x, gsrinv_y, self.gsr_invert);

        let (((gts0en_x, gts0en_y), (gts0inv_x, gts0inv_y)), ((gts1en_x, gts1en_y), (gts1inv_x, gts1inv_y)),
             ((gts2en_x, gts2en_y), (gts2inv_x, gts2inv_y)), ((gts3en_x, gts3en_y), (gts3inv_x, gts3inv_y))) =
                gts_fuse_coords(device);
        fuse_array.set(gts0en_x, gts0en_y, !self.gts_enable[0]);
        fuse_array.set(gts0inv_x, gts0inv_y, self.gts_invert[0]);
        fuse_array.set(gts1en_x, gts1en_y, !self.gts_enable[1]);
        fuse_array.set(gts1inv_x, gts1inv_y, self.gts_invert[1]);
        fuse_array.set(gts2en_x, gts2en_y, !self.gts_enable[2]);
        fuse_array.set(gts2inv_x, gts2inv_y, self.gts_invert[2]);
        fuse_array.set(gts3en_x, gts3en_y, !self.gts_enable[3]);
        fuse_array.set(gts3inv_x, gts3inv_y, self.gts_invert[3]);

        let (term_x, term_y) = global_term_fuse_coord(device);
        fuse_array.set(term_x, term_y, self.global_pu);
    }

    /// Internal function to read the global nets
    pub fn from_jed(device: XC2Device, fuses: &[bool]) -> Self {
        XC2GlobalNets {
            gck_enable: [
                fuses[gck_fuse_idx(device) + 0],
                fuses[gck_fuse_idx(device) + 1],
                fuses[gck_fuse_idx(device) + 2],
            ],
            gsr_enable: fuses[gsr_fuse_idx(device) + 1],
            gsr_invert: fuses[gsr_fuse_idx(device) + 0],
            gts_enable: [
                !fuses[gts_fuse_idx(device) + 1],
                !fuses[gts_fuse_idx(device) + 3],
                !fuses[gts_fuse_idx(device) + 5],
                !fuses[gts_fuse_idx(device) + 7],
            ],
            gts_invert: [
                fuses[gts_fuse_idx(device) + 0],
                fuses[gts_fuse_idx(device) + 2],
                fuses[gts_fuse_idx(device) + 4],
                fuses[gts_fuse_idx(device) + 6],
            ],
            global_pu: fuses[global_term_fuse_idx(device)],
        }
    }

    /// Internal function to read the global nets
    pub fn from_crbit(device: XC2Device, fuse_array: &FuseArray) -> Self {
        let ((gck0x, gck0y), (gck1x, gck1y), (gck2x, gck2y)) = gck_fuse_coords(device);

        let ((gsren_x, gsren_y), (gsrinv_x, gsrinv_y)) = gsr_fuse_coords(device);

        let (((gts0en_x, gts0en_y), (gts0inv_x, gts0inv_y)), ((gts1en_x, gts1en_y), (gts1inv_x, gts1inv_y)),
             ((gts2en_x, gts2en_y), (gts2inv_x, gts2inv_y)), ((gts3en_x, gts3en_y), (gts3inv_x, gts3inv_y))) =
                gts_fuse_coords(device);

        let (term_x, term_y) = global_term_fuse_coord(device);

        XC2GlobalNets {
            gck_enable: [
                fuse_array.get(gck0x, gck0y),
                fuse_array.get(gck1x, gck1y),
                fuse_array.get(gck2x, gck2y),
            ],
            gsr_enable: fuse_array.get(gsren_x, gsren_y),
            gsr_invert: fuse_array.get(gsrinv_x, gsrinv_y),
            gts_enable: [
                !fuse_array.get(gts0en_x, gts0en_y),
                !fuse_array.get(gts1en_x, gts1en_y),
                !fuse_array.get(gts2en_x, gts2en_y),
                !fuse_array.get(gts3en_x, gts3en_y),
            ],
            gts_invert: [
                fuse_array.get(gts0inv_x, gts0inv_y),
                fuse_array.get(gts1inv_x, gts1inv_y),
                fuse_array.get(gts2inv_x, gts2inv_y),
                fuse_array.get(gts3inv_x, gts3inv_y),
            ],
            global_pu: fuse_array.get(term_x, term_y),
        }
    }
}

/// Possible clock divide ratios for the programmable clock divider
#[derive(Copy, Clone, Eq, PartialEq, Debug, Serialize, Deserialize)]
#[derive(BitPattern)]
pub enum XC2ClockDivRatio {
    #[bits = "000"]
    Div2,
    #[bits = "001"]
    Div4,
    #[bits = "010"]
    Div6,
    #[bits = "011"]
    Div8,
    #[bits = "100"]
    Div10,
    #[bits = "101"]
    Div12,
    #[bits = "110"]
    Div14,
    #[bits = "111"]
    Div16,
}

/// Represents the configuration of the programmable clock divider in devices with 128 macrocells or more. This is
/// hard-wired onto the GCK2 clock pin.
#[derive(Copy, Clone, Eq, PartialEq, Debug, Serialize, Deserialize)]
pub struct XC2ClockDiv {
    /// Ratio that input clock is divided by
    pub div_ratio: XC2ClockDivRatio,
    /// Whether the "delay" feature is enabled
    pub delay: bool,
    /// Whether the clock divider is enabled (other settings are ignored if not)
    pub enabled: bool,
}

impl XC2ClockDiv {
    /// Dump a human-readable explanation of the clock divider to the given `writer` object.
    pub fn dump_human_readable<W: Write>(&self, mut writer: W) -> Result<(), io::Error> {
        write!(writer, "\n")?;
        write!(writer, "GCK2 clock divider {}\n", if self.enabled {"enabled"} else {"disabled"})?;
        write!(writer, "clock divider delay {}\n", if self.delay {"enabled"} else {"disabled"})?;

        write!(writer, "clock division ratio: {}\n", match self.div_ratio {
            XC2ClockDivRatio::Div2 => "2",
            XC2ClockDivRatio::Div4 => "4",
            XC2ClockDivRatio::Div6 => "6",
            XC2ClockDivRatio::Div8 => "8",
            XC2ClockDivRatio::Div10 => "10",
            XC2ClockDivRatio::Div12 => "12",
            XC2ClockDivRatio::Div14 => "14",
            XC2ClockDivRatio::Div16 => "16",
        })?;

        Ok(())
    }
}

impl Default for XC2ClockDiv {
    /// Returns a "default" clock divider configuration, which is one that is not used
    fn default() -> Self {
        XC2ClockDiv {
            div_ratio: XC2ClockDivRatio::Div16,
            delay: false,
            enabled: false,
        }
    }
}

impl XC2ClockDiv {
    /// Internal function to read the clock divider configuration from a 128-macrocell part
    pub fn from_jed(device: XC2Device, fuses: &[bool]) -> Self {
        let clock_fuse_block = clock_div_fuse_idx(device);

        XC2ClockDiv {
            delay: !fuses[clock_fuse_block + 4],
            enabled: !fuses[clock_fuse_block],
            div_ratio: XC2ClockDivRatio::decode((fuses[clock_fuse_block + 1], fuses[clock_fuse_block + 2], fuses[clock_fuse_block + 3])),
        }
    }

    /// Internal function to read the clock divider configuration from a 128-macrocell part
    pub fn from_crbit(device: XC2Device, fuse_array: &FuseArray) -> Self {
        let ((clken_x, clken_y), (clkdiv0_x, clkdiv0_y), (clkdiv1_x, clkdiv1_y), (clkdiv2_x, clkdiv2_y),
            (clkdelay_x, clkdelay_y)) = clock_div_fuse_coord(device);

        let div_ratio_bits = (fuse_array.get(clkdiv0_x, clkdiv0_y),
                              fuse_array.get(clkdiv1_x, clkdiv1_y),
                              fuse_array.get(clkdiv2_x, clkdiv2_y));

        XC2ClockDiv {
            delay: !fuse_array.get(clkdelay_x, clkdelay_y),
            enabled: !fuse_array.get(clken_x, clken_y),
            div_ratio: XC2ClockDivRatio::decode(div_ratio_bits),
        }
    }
}
