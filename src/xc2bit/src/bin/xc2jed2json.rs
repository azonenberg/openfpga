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

//! Tool that converts a jed to a json file

use std::cell::RefCell;
use std::collections::HashMap;
use std::fs::File;
use std::io::Read;

extern crate xc2bit;
use xc2bit::*;

extern crate yosys_netlist_json;
use yosys_netlist_json::*;

fn main() {
    let args = ::std::env::args().collect::<Vec<_>>();

    if args.len() != 2 {
        println!("Usage: {} file.jed", args[0]);
        ::std::process::exit(1);
    }

    // Load the jed
    let mut f = File::open(&args[1]).expect("failed to open file");
    let mut data = Vec::new();
    f.read_to_end(&mut data).expect("failed to read data");

    let bits_result = read_jed(&data);
    let (bits, device_name_option) = bits_result.expect("failed to read jed");
    let device_name = device_name_option.expect("missing device name in jed");

    let bitstream_result = XC2Bitstream::from_jed(&bits, &device_name);
    let bitstream = bitstream_result.expect("failed to process jed");

    // Because of how the closures work, we unfortunately need to use a RefCell here
    let output_netlist = RefCell::new(Netlist::default());
    let node_vec = RefCell::new(Vec::new());
    // Skip 0 and 1 just like yosys does
    let wire_idx = RefCell::new(2);

    // Create common/global stuff ahead of time
    {
        let mut output_netlist_mut = output_netlist.borrow_mut();
        // TOOD: Proper version number
        output_netlist_mut.creator = String::from("xc2bit xc2jed2json utility - DEVELOPMENT VERSION");

        let mut module = Module::default();
        module.attributes.insert(String::from("PART_NAME"),
            AttributeVal::S(format!("{}", bitstream.bits.device_type())));
        module.attributes.insert(String::from("PART_SPEED"),
            AttributeVal::S(format!("{}", bitstream.speed_grade)));
        module.attributes.insert(String::from("PART_PKG"),
            AttributeVal::S(format!("{}", bitstream.package)));
        if bitstream.bits.get_global_nets().global_pu {
            module.attributes.insert(String::from("GLOBAL_TERM"),
                AttributeVal::S(String::from("PULLUP")));
        } else {
            module.attributes.insert(String::from("GLOBAL_TERM"),
                AttributeVal::S(String::from("KEEPER")));
        }

        output_netlist_mut.modules.insert(String::from("top"), module);
    }

    // Call the giant structure callback
    get_device_structure(bitstream.bits.device_type(),
        |node_name: &str, node_type: &str, fb: u32, idx: u32| {
            // Start constructing the cell object
            let mut cell_type = node_type.to_owned();
            let mut parameters = HashMap::new();
            let mut attributes = HashMap::new();
            let mut connections = HashMap::new();

            match node_type {
                "BUFG" => {
                    connections.insert(String::from("I"), Vec::new());
                    connections.insert(String::from("O"), Vec::new());
                },
                "BUFGSR" => {
                    parameters.insert(String::from("INVERT"),
                        AttributeVal::N(bitstream.bits.get_global_nets().gsr_invert as usize));

                    connections.insert(String::from("I"), Vec::new());
                    connections.insert(String::from("O"), Vec::new());
                },
                "BUFGTS" => {
                    parameters.insert(String::from("INVERT"),
                        AttributeVal::N(bitstream.bits.get_global_nets().gts_invert[idx as usize] as usize));

                    connections.insert(String::from("I"), Vec::new());
                    connections.insert(String::from("O"), Vec::new());
                },
                "IOBUFE" => {
                    let (fb, mc) = iob_num_to_fb_mc_num(bitstream.bits.device_type(), idx).unwrap();

                    let mut has_output = true;

                    attributes.insert(String::from("LOC"), AttributeVal::S(format!("FB{}_{}", fb + 1, mc + 1)));
                    // Grab attributes from the bitstream
                    if let Some(iobs) = bitstream.bits.get_small_iobs() {
                        if iobs[idx as usize].slew_is_fast {
                            attributes.insert(String::from("SLEW"), AttributeVal::S(String::from("FAST")));
                        } else {
                            attributes.insert(String::from("SLEW"), AttributeVal::S(String::from("SLOW")));
                        }

                        if iobs[idx as usize].termination_enabled {
                            attributes.insert(String::from("TERM"), AttributeVal::S(String::from("TRUE")));
                        } else {
                            attributes.insert(String::from("TERM"), AttributeVal::S(String::from("FALSE")));
                        }

                        if iobs[idx as usize].obuf_mode == XC2IOBOBufMode::CGND {
                            attributes.insert(String::from("CGND"), AttributeVal::N(1));
                        }

                        if iobs[idx as usize].schmitt_trigger {
                            attributes.insert(String::from("SCHMITT_TRIGGER"), AttributeVal::S(String::from("TRUE")));
                        } else {
                            attributes.insert(String::from("SCHMITT_TRIGGER"), AttributeVal::S(String::from("FALSE")));
                        }

                        has_output = iobs[idx as usize].obuf_mode != XC2IOBOBufMode::Disabled;
                    }
                    if let Some(iobs) = bitstream.bits.get_large_iobs() {
                        if iobs[idx as usize].slew_is_fast {
                            attributes.insert(String::from("SLEW"), AttributeVal::S(String::from("FAST")));
                        } else {
                            attributes.insert(String::from("SLEW"), AttributeVal::S(String::from("SLOW")));
                        }

                        if iobs[idx as usize].termination_enabled {
                            attributes.insert(String::from("TERM"), AttributeVal::S(String::from("TRUE")));
                        } else {
                            attributes.insert(String::from("TERM"), AttributeVal::S(String::from("FALSE")));
                        }

                        if iobs[idx as usize].obuf_mode == XC2IOBOBufMode::CGND {
                            attributes.insert(String::from("CGND"), AttributeVal::N(1));
                        }

                        match iobs[idx as usize].ibuf_mode {
                            XC2IOBIbufMode::NoVrefNoSt => {
                                attributes.insert(String::from("SCHMITT_TRIGGER"), AttributeVal::S(String::from("FALSE")));
                            },
                            XC2IOBIbufMode::NoVrefSt => {
                                attributes.insert(String::from("SCHMITT_TRIGGER"), AttributeVal::S(String::from("TRUE")));
                            },
                            XC2IOBIbufMode::UsesVref => {
                                // FIXME
                                unimplemented!();
                            },
                            XC2IOBIbufMode::IsVref => {
                                // FIXME
                                unimplemented!();
                            }
                        }

                        if iobs[idx as usize].uses_data_gate {
                            // FIXME
                            unimplemented!();
                        }

                        has_output = iobs[idx as usize].obuf_mode != XC2IOBOBufMode::Disabled;
                    }

                    // Construct the wire for the toplevel port
                    let toplevel_wire = {
                        let orig_wire_idx = {*wire_idx.borrow()};
                        let mut output_netlist_mut = output_netlist.borrow_mut();
                        let mut netnames = &mut output_netlist_mut.modules.get_mut("top").unwrap().netnames;
                        netnames.insert(format!("PAD_FB{}_{}", fb + 1, mc + 1), Netname {
                            hide_name: 0,
                            bits: vec![BitVal::N(orig_wire_idx)],
                            attributes: HashMap::new(),
                        });

                        // Return the current wire index
                        *wire_idx.borrow_mut() += 1;
                        orig_wire_idx
                    };

                    // Construct the toplevel port
                    {
                        let mut output_netlist_mut = output_netlist.borrow_mut();
                        let mut ports = &mut output_netlist_mut.modules.get_mut("top").unwrap().ports;
                        ports.insert(format!("PAD_FB{}_{}", fb + 1, mc + 1), Port {
                            direction: if has_output {PortDirection::InOut} else {PortDirection::Input},
                            bits: vec![BitVal::N(toplevel_wire)],
                        });
                    }

                    if has_output {
                        connections.insert(String::from("I"), Vec::new());
                        connections.insert(String::from("E"), Vec::new());
                        connections.insert(String::from("O"), Vec::new());
                        connections.insert(String::from("IO"), vec![BitVal::N(toplevel_wire)]);
                        cell_type = String::from("IOBUFE");
                    } else {
                        connections.insert(String::from("O"), Vec::new());
                        connections.insert(String::from("I"), vec![BitVal::N(toplevel_wire)]);
                        cell_type = String::from("IBUF");
                    }
                },
                // FIXME: Boilerplate, copy-pasta
                "IBUF" => {
                    // Construct the wire for the toplevel port
                    let toplevel_wire = {
                        let orig_wire_idx = {*wire_idx.borrow()};
                        let mut output_netlist_mut = output_netlist.borrow_mut();
                        let mut netnames = &mut output_netlist_mut.modules.get_mut("top").unwrap().netnames;
                        netnames.insert(String::from("INPAD"), Netname {
                            hide_name: 0,
                            bits: vec![BitVal::N(orig_wire_idx)],
                            attributes: HashMap::new(),
                        });

                        // Return the current wire index
                        *wire_idx.borrow_mut() += 1;
                        orig_wire_idx
                    };

                    // Construct the toplevel port
                    {
                        let mut output_netlist_mut = output_netlist.borrow_mut();
                        let mut ports = &mut output_netlist_mut.modules.get_mut("top").unwrap().ports;
                        ports.insert(String::from("INPAD"), Port {
                            direction: PortDirection::Input,
                            bits: vec![BitVal::N(toplevel_wire)],
                        });
                    }

                    attributes.insert(String::from("LOC"), AttributeVal::S(String::from("INPAD")));
                    match bitstream.bits {
                        XC2BitstreamBits::XC2C32{ref inpin, ..} | XC2BitstreamBits::XC2C32A{ref inpin, ..} => {
                            if inpin.termination_enabled {
                                attributes.insert(String::from("TERM"), AttributeVal::S(String::from("TRUE")));
                            } else {
                                attributes.insert(String::from("TERM"), AttributeVal::S(String::from("FALSE")));
                            }

                            if inpin.schmitt_trigger {
                                attributes.insert(String::from("SCHMITT_TRIGGER"), AttributeVal::S(String::from("TRUE")));
                            } else {
                                attributes.insert(String::from("SCHMITT_TRIGGER"), AttributeVal::S(String::from("FALSE")));
                            }
                        },
                        _ => {
                            // Cannot happen
                            unreachable!();
                        }
                    }

                    connections.insert(String::from("O"), Vec::new());
                    connections.insert(String::from("I"), vec![BitVal::N(toplevel_wire)]);
                },
                "ANDTERM" => {
                    attributes.insert(String::from("LOC"), AttributeVal::S(format!("FB{}_P{}", fb + 1, idx)));

                    connections.insert(String::from("IN"), Vec::new());
                    connections.insert(String::from("IN_B"), Vec::new());
                    connections.insert(String::from("OUT"), Vec::new());
                },
                "ORTERM" => {
                    attributes.insert(String::from("LOC"), AttributeVal::S(format!("FB{}_{}", fb + 1, idx + 1)));

                    connections.insert(String::from("IN"), Vec::new());
                    connections.insert(String::from("OUT"), Vec::new());
                },
                "MACROCELL_XOR" => {
                    let mc = &bitstream.bits.get_fb()[fb as usize].mcs[idx as usize];

                    if mc.xor_mode == XC2MCXorMode::ZERO || mc.xor_mode == XC2MCXorMode::PTC {
                        parameters.insert(String::from("INVERT_OUT"), AttributeVal::N(0));
                    } else {
                        parameters.insert(String::from("INVERT_OUT"), AttributeVal::N(1));
                    }

                    attributes.insert(String::from("LOC"), AttributeVal::S(format!("FB{}_{}", fb + 1, idx + 1)));

                    connections.insert(String::from("IN_PTC"), Vec::new());
                    connections.insert(String::from("IN_ORTERM"), Vec::new());
                    connections.insert(String::from("OUT"), Vec::new());
                },
                "REG" => {
                    let mc = &bitstream.bits.get_fb()[fb as usize].mcs[idx as usize];

                    parameters.insert(String::from("INIT"), AttributeVal::N(mc.init_state as usize));

                    attributes.insert(String::from("LOC"), AttributeVal::S(format!("FB{}_{}", fb + 1, idx + 1)));

                    connections.insert(String::from("Q"), Vec::new());
                    connections.insert(String::from("PRE"), Vec::new());
                    connections.insert(String::from("CLR"), Vec::new());

                    match mc.reg_mode {
                        XC2MCRegMode::DFF => {
                            connections.insert(String::from("C"), Vec::new());
                            connections.insert(String::from("D"), Vec::new());

                            if mc.is_ddr {
                                cell_type = String::from("FDDCP");
                            } else {
                                if !mc.clk_invert_pol {
                                    cell_type = String::from("FDCP");
                                } else {
                                    cell_type = String::from("FDCP_N");
                                }
                            }
                        },
                        XC2MCRegMode::LATCH => {
                            connections.insert(String::from("G"), Vec::new());
                            connections.insert(String::from("D"), Vec::new());

                            if !mc.clk_invert_pol {
                                cell_type = String::from("LDCP");
                            } else {
                                cell_type = String::from("LDCP_N");
                            }
                        },
                        XC2MCRegMode::TFF => {
                            connections.insert(String::from("C"), Vec::new());
                            connections.insert(String::from("T"), Vec::new());

                            if mc.is_ddr {
                                cell_type = String::from("FTDCP");
                            } else {
                                if !mc.clk_invert_pol {
                                    cell_type = String::from("FTCP");
                                } else {
                                    cell_type = String::from("FTCP_N");
                                }
                            }
                        },
                        XC2MCRegMode::DFFCE => {
                            connections.insert(String::from("C"), Vec::new());
                            connections.insert(String::from("D"), Vec::new());
                            connections.insert(String::from("CE"), Vec::new());

                            if mc.is_ddr {
                                cell_type = String::from("FDDCPE");
                            } else {
                                if !mc.clk_invert_pol {
                                    cell_type = String::from("FDCPE");
                                } else {
                                    cell_type = String::from("FDCPE_N");
                                }
                            }
                        }
                    }
                },
                _ => unreachable!()
            }

            // Create the cell in the output module
            let mut output_netlist_mut = output_netlist.borrow_mut();
            let mut cells = &mut output_netlist_mut.modules.get_mut("top").unwrap().cells;
            cells.insert(node_name.to_owned(), Cell {
                hide_name: 0,
                cell_type,
                parameters,
                attributes,
                port_directions: HashMap::new(),
                connections,
            });

            // Memoization needed for the callback interface
            let mut node_vec = node_vec.borrow_mut();
            node_vec.push((node_name.to_owned(), node_type.to_owned(), fb, idx));
            node_vec.len() - 1
        },
        |wire_name: &str| {
            // Create the net in the output module
            let orig_wire_idx = {*wire_idx.borrow()};
            let mut output_netlist_mut = output_netlist.borrow_mut();
            let mut netnames = &mut output_netlist_mut.modules.get_mut("top").unwrap().netnames;
            netnames.insert(wire_name.to_owned(), Netname {
                hide_name: 0,
                bits: vec![BitVal::N(orig_wire_idx)],
                attributes: HashMap::new(),
            });

            // Return the current wire index
            *wire_idx.borrow_mut() += 1;
            orig_wire_idx
        },
        |node_ref: usize, wire_ref: usize, port_name: &str, port_idx: u32, extra_data: (u32, u32)| {
            let (ref node_name, ref node_type, fb, idx) = node_vec.borrow()[node_ref];

            let mut should_add_wire = false;
            let mut port_name = port_name;
            let mut wire_bitval = BitVal::S(SpecialBit::X);
            // This is kinda hacky but is used for the P-terms where two wires can be generated in one call
            let mut should_add_wire_2 = false;
            let mut port_name_2 = "";
            let mut wire_bitval_2 = BitVal::S(SpecialBit::X);

            match node_type.as_ref() {
                "BUFG" => {
                    if port_name == "I" {
                        should_add_wire = true;
                        if bitstream.bits.get_global_nets().gck_enable[idx as usize] {
                            wire_bitval = BitVal::N(wire_ref);
                        } else {
                            wire_bitval = BitVal::S(SpecialBit::_0);
                        }
                    } else if port_name == "O" {
                        should_add_wire = true;
                        wire_bitval = BitVal::N(wire_ref);
                    } else {
                        unreachable!();
                    }
                },
                "BUFGSR" => {
                    // FIXME: Test the interaction with the invert bit
                    if port_name == "I" {
                        should_add_wire = true;
                        if bitstream.bits.get_global_nets().gsr_enable {
                            wire_bitval = BitVal::N(wire_ref);
                        } else {
                            wire_bitval = BitVal::S(SpecialBit::_0);
                        }
                    } else if port_name == "O" {
                        should_add_wire = true;
                        wire_bitval = BitVal::N(wire_ref);
                    } else {
                        unreachable!();
                    }
                },
                "BUFGTS" => {
                    // FIXME: Test the interaction with the invert bit
                    if port_name == "I" {
                        should_add_wire = true;
                        if bitstream.bits.get_global_nets().gts_enable[idx as usize] {
                            wire_bitval = BitVal::N(wire_ref);
                        } else {
                            wire_bitval = BitVal::S(SpecialBit::_0);
                        }
                    } else if port_name == "O" {
                        should_add_wire = true;
                        wire_bitval = BitVal::N(wire_ref);
                    } else {
                        unreachable!();
                    }
                },
                "IOBUFE" => {
                    if port_name == "I" {
                        // FIXME: Verify the CGND behavior on hardware
                        let mut obuf_mode = None;
                        if let Some(iobs) = bitstream.bits.get_small_iobs() {
                            obuf_mode = Some(iobs[idx as usize].obuf_mode);
                        }
                        if let Some(iobs) = bitstream.bits.get_large_iobs() {
                            obuf_mode = Some(iobs[idx as usize].obuf_mode);
                        }

                        if obuf_mode.unwrap() == XC2IOBOBufMode::CGND ||
                           obuf_mode.unwrap() == XC2IOBOBufMode::OpenDrain {

                            should_add_wire = true;
                            wire_bitval = BitVal::S(SpecialBit::_0);
                        } else if obuf_mode.unwrap() != XC2IOBOBufMode::Disabled {
                            should_add_wire = true;
                            wire_bitval = BitVal::N(wire_ref);
                        }
                    } else if port_name == "E" {
                        let mut obuf_mode = None;
                        if let Some(iobs) = bitstream.bits.get_small_iobs() {
                            obuf_mode = Some(iobs[idx as usize].obuf_mode);
                        }
                        if let Some(iobs) = bitstream.bits.get_large_iobs() {
                            obuf_mode = Some(iobs[idx as usize].obuf_mode);
                        }

                        // FIXME: This idx == 0 is a hack
                        if (obuf_mode.unwrap() == XC2IOBOBufMode::PushPull && port_idx == 0) ||
                           (obuf_mode.unwrap() == XC2IOBOBufMode::CGND && port_idx == 0) {
                            
                            should_add_wire = true;
                            wire_bitval = BitVal::S(SpecialBit::_1);
                        } else if (obuf_mode.unwrap() == XC2IOBOBufMode::OpenDrain && port_idx == 4) ||
                                  (obuf_mode.unwrap() == XC2IOBOBufMode::TriStateGTS0 && port_idx == 0) ||
                                  (obuf_mode.unwrap() == XC2IOBOBufMode::TriStateGTS1 && port_idx == 1) ||
                                  (obuf_mode.unwrap() == XC2IOBOBufMode::TriStateGTS2 && port_idx == 2) ||
                                  (obuf_mode.unwrap() == XC2IOBOBufMode::TriStateGTS3 && port_idx == 3) ||
                                  (obuf_mode.unwrap() == XC2IOBOBufMode::TriStateCTE && port_idx == 5) ||
                                  (obuf_mode.unwrap() == XC2IOBOBufMode::TriStatePTB && port_idx == 6) {
                            should_add_wire = true;
                            wire_bitval = BitVal::N(wire_ref);
                        }
                    } else if port_name == "O" {
                        // This is always driven
                        should_add_wire = true;
                        wire_bitval = BitVal::N(wire_ref);
                    } else {
                        unreachable!();
                    }
                },
                "IBUF" => {
                    if port_name == "O" {
                        // This is always driven
                        should_add_wire = true;
                        wire_bitval = BitVal::N(wire_ref);
                    } else {
                        unreachable!();
                    }
                },
                "ANDTERM" => {
                    if port_name == "OUT" {
                        // This is always driven
                        should_add_wire = true;
                        wire_bitval = BitVal::N(wire_ref);
                    } else if port_name == "IN" {
                        // Whee, ZIA goes here

                        let mut should_add_andterm_wire = false;

                        let zia_row = zia_table_get_row(bitstream.bits.device_type(), port_idx as usize);
                        // FIXME: extra_data checking is a hack
                        if bitstream.bits.get_fb()[fb as usize].zia_bits[port_idx as usize].selected ==
                            XC2ZIAInput::One && extra_data == (0, 0) {

                            should_add_andterm_wire = true;
                            wire_bitval = BitVal::S(SpecialBit::_1);
                        } else if bitstream.bits.get_fb()[fb as usize].zia_bits[port_idx as usize].selected ==
                            XC2ZIAInput::Zero && extra_data == (0, 0) {

                            should_add_andterm_wire = true;
                            wire_bitval = BitVal::S(SpecialBit::_0);
                        } else if bitstream.bits.get_fb()[fb as usize].zia_bits[port_idx as usize].selected ==
                            zia_row[extra_data.0 as usize] {

                            let zia_choice = zia_row[extra_data.0 as usize];
                            match &zia_choice {
                                &XC2ZIAInput::Macrocell{fb: zia_fb, mc: zia_mc} => {
                                    // FIXME Hack
                                    if bitstream.bits.get_fb()[zia_fb as usize].mcs[zia_mc as usize].fb_mode ==
                                        XC2MCFeedbackMode::Disabled && extra_data.1 == 0 {

                                        should_add_andterm_wire = true;
                                        wire_bitval = BitVal::S(SpecialBit::_0);
                                    } else if (bitstream.bits.get_fb()[zia_fb as usize].mcs[zia_mc as usize].fb_mode ==
                                        XC2MCFeedbackMode::COMB && extra_data.1 == 0) ||
                                        (bitstream.bits.get_fb()[zia_fb as usize].mcs[zia_mc as usize].fb_mode ==
                                        XC2MCFeedbackMode::REG && extra_data.1 == 1) {

                                        should_add_andterm_wire = true;
                                        wire_bitval = BitVal::N(wire_ref);
                                    }
                                },
                                &XC2ZIAInput::IBuf{ibuf: zia_iob} => {
                                    let mut zia_mode = None;
                                    if let Some(iobs) = bitstream.bits.get_small_iobs() {
                                        zia_mode = Some(iobs[zia_iob as usize].zia_mode);
                                    }
                                    if let Some(iobs) = bitstream.bits.get_large_iobs() {
                                        zia_mode = Some(iobs[zia_iob as usize].zia_mode);
                                    }

                                    // FIXME: Hack
                                    if zia_mode.unwrap() == XC2IOBZIAMode::Disabled && extra_data.1 == 0 {
                                        should_add_andterm_wire = true;
                                        wire_bitval = BitVal::S(SpecialBit::_0);
                                    } else if (zia_mode.unwrap() == XC2IOBZIAMode::PAD && extra_data.1 == 0) ||
                                        (zia_mode.unwrap() == XC2IOBZIAMode::REG && extra_data.1 == 1) {

                                        should_add_andterm_wire = true;
                                        wire_bitval = BitVal::N(wire_ref);
                                    }
                                },
                                &XC2ZIAInput::DedicatedInput => {
                                    should_add_andterm_wire = true;
                                    wire_bitval = BitVal::N(wire_ref);
                                },
                                // These cannot be in the choices table; they are special cases
                                _ => unreachable!(),
                            };
                        }

                        if should_add_andterm_wire {
                            if bitstream.bits.get_fb()[fb as usize].and_terms[idx as usize].input[port_idx as usize] {
                                should_add_wire = true;
                                port_name = "IN";
                            }
                            if bitstream.bits.get_fb()[fb as usize].and_terms[idx as usize].input_b[port_idx as usize] {
                                should_add_wire_2 = true;
                                port_name_2 = "IN_B";
                                wire_bitval_2 = wire_bitval.clone();
                            }
                        }
                    } else {
                        unreachable!();
                    }
                },
                "ORTERM" => {
                    if port_name == "OUT" {
                        // This is always driven
                        should_add_wire = true;
                        wire_bitval = BitVal::N(wire_ref);
                    } else if port_name == "IN" {
                        if bitstream.bits.get_fb()[fb as usize].or_terms[idx as usize].input[port_idx as usize] {
                            should_add_wire = true;
                            wire_bitval = BitVal::N(wire_ref);
                        }
                    } else {
                        unreachable!();
                    }
                },
                "MACROCELL_XOR" => {
                    if port_name == "IN_PTC" {
                        should_add_wire = true;
                        if (bitstream.bits.get_fb()[fb as usize].mcs[idx as usize].xor_mode == XC2MCXorMode::PTC) ||
                           (bitstream.bits.get_fb()[fb as usize].mcs[idx as usize].xor_mode == XC2MCXorMode::PTCB) {

                            wire_bitval = BitVal::N(wire_ref);
                        } else {
                            wire_bitval = BitVal::S(SpecialBit::_0);
                        }
                    } else if port_name == "IN_ORTERM" {
                        should_add_wire = true;
                        wire_bitval = BitVal::N(wire_ref);
                    } else if port_name == "OUT" {
                        // This wire is always driven
                        if port_idx == 0 {
                            should_add_wire = true;
                            wire_bitval = BitVal::N(wire_ref);
                        } else {
                            if let Some(iob_idx) = fb_mc_num_to_iob_num(bitstream.bits.device_type(), fb, idx) {
                                let mut obuf_uses_ff = false;
                                if let Some(iobs) = bitstream.bits.get_small_iobs() {
                                    obuf_uses_ff = iobs[iob_idx as usize].obuf_uses_ff;
                                }
                                if let Some(iobs) = bitstream.bits.get_large_iobs() {
                                    obuf_uses_ff = iobs[iob_idx as usize].obuf_uses_ff;
                                }

                                if !obuf_uses_ff {
                                    should_add_wire = true;
                                    wire_bitval = BitVal::N(wire_ref);
                                }
                            }
                        }
                    } else {
                        unreachable!();
                    }
                },
                "REG" => {
                    if port_name == "Q" {
                        // This is always driven
                        if port_idx == 0 {
                            should_add_wire = true;
                            wire_bitval = BitVal::N(wire_ref);
                        } else {
                            if let Some(iob_idx) = fb_mc_num_to_iob_num(bitstream.bits.device_type(), fb, idx) {
                                let mut obuf_uses_ff = false;
                                if let Some(iobs) = bitstream.bits.get_small_iobs() {
                                    obuf_uses_ff = iobs[iob_idx as usize].obuf_uses_ff;
                                }
                                if let Some(iobs) = bitstream.bits.get_large_iobs() {
                                    obuf_uses_ff = iobs[iob_idx as usize].obuf_uses_ff;
                                }

                                if obuf_uses_ff {
                                    should_add_wire = true;
                                    wire_bitval = BitVal::N(wire_ref);
                                }
                            }
                        }
                    } else if port_name == "CE" {
                        if bitstream.bits.get_fb()[fb as usize].mcs[idx as usize].reg_mode == XC2MCRegMode::DFFCE {
                            should_add_wire = true;
                            wire_bitval = BitVal::N(wire_ref);
                        }
                    } else if port_name == "CLK" {
                        let clk_src = bitstream.bits.get_fb()[fb as usize].mcs[idx as usize].clk_src;
                        if (clk_src == XC2MCRegClkSrc::GCK0 && port_idx == 0) ||
                           (clk_src == XC2MCRegClkSrc::GCK1 && port_idx == 1) ||
                           (clk_src == XC2MCRegClkSrc::GCK2 && port_idx == 2) ||
                           (clk_src == XC2MCRegClkSrc::CTC && port_idx == 3) ||
                           (clk_src == XC2MCRegClkSrc::PTC && port_idx == 4) {

                            should_add_wire = true;
                            port_name = if bitstream.bits.get_fb()[fb as usize]
                                .mcs[idx as usize].reg_mode == XC2MCRegMode::LATCH {

                                "G"
                            } else {
                                "C"
                            };
                            wire_bitval = BitVal::N(wire_ref);
                        }
                    } else if port_name == "S" {
                        let s_src = bitstream.bits.get_fb()[fb as usize].mcs[idx as usize].s_src;
                        if (s_src == XC2MCRegSetSrc::GSR && port_idx == 0) ||
                           (s_src == XC2MCRegSetSrc::CTS && port_idx == 1) ||
                           (s_src == XC2MCRegSetSrc::PTA && port_idx == 2) {

                            should_add_wire = true;
                            port_name = "PRE";
                            wire_bitval = BitVal::N(wire_ref);
                        } else if s_src == XC2MCRegSetSrc::Disabled && port_idx == 0 {
                            should_add_wire = true;
                            port_name = "PRE";
                            wire_bitval = BitVal::S(SpecialBit::_0);
                        }
                    } else if port_name == "R" {
                        let r_src = bitstream.bits.get_fb()[fb as usize].mcs[idx as usize].r_src;
                        if (r_src == XC2MCRegResetSrc::GSR && port_idx == 0) ||
                           (r_src == XC2MCRegResetSrc::CTR && port_idx == 1) ||
                           (r_src == XC2MCRegResetSrc::PTA && port_idx == 2) {

                            should_add_wire = true;
                            port_name = "CLR";
                            wire_bitval = BitVal::N(wire_ref);
                        } else if r_src == XC2MCRegResetSrc::Disabled && port_idx == 0 {
                            should_add_wire = true;
                            port_name = "CLR";
                            wire_bitval = BitVal::S(SpecialBit::_0);
                        }
                    } else if port_name == "D/T" {
                            port_name = if bitstream.bits.get_fb()[fb as usize]
                                .mcs[idx as usize].reg_mode == XC2MCRegMode::TFF {

                                "T"
                            } else {
                                "D"
                            };

                        if bitstream.bits.get_fb()[fb as usize].mcs[idx as usize].ff_in_ibuf {
                            if port_idx == 1 {
                                should_add_wire = true;
                                wire_bitval = BitVal::N(wire_ref);
                            }
                        } else {
                            if port_idx == 0 {
                                should_add_wire = true;
                                wire_bitval = BitVal::N(wire_ref);
                            }
                        }
                    } else {
                        unreachable!();
                    }
                },
                _ => {
                    unreachable!();
                }
            };

            let mut output_netlist_mut = output_netlist.borrow_mut();
            let mut cells = &mut output_netlist_mut.modules.get_mut("top").unwrap().cells;
            if should_add_wire {
                cells.get_mut(node_name).unwrap().connections.get_mut(port_name).unwrap().push(wire_bitval);
            }
            if should_add_wire_2 {
                cells.get_mut(node_name).unwrap().connections.get_mut(port_name_2).unwrap().push(wire_bitval_2);
            }
        }
    );

    // We need to fill in some widths here
    {
        let mut output_netlist_mut = output_netlist.borrow_mut();
        let mut cells = &mut output_netlist_mut.modules.get_mut("top").unwrap().cells;
        for (_, cell) in cells.iter_mut() {
            if cell.cell_type == "ANDTERM" {
                cell.parameters.insert(String::from("TRUE_INP"),
                    AttributeVal::N(cell.connections.get("IN").unwrap().len()));
                cell.parameters.insert(String::from("COMP_INP"),
                    AttributeVal::N(cell.connections.get("IN_B").unwrap().len()));
            }

            if cell.cell_type == "ORTERM" {
                cell.parameters.insert(String::from("WIDTH"),
                    AttributeVal::N(cell.connections.get("IN").unwrap().len()));
            }
        }
    }

    // Fix up driving multiple nets (none of our cells have outputs with width > 1)
    {
        let mut output_netlist_mut = output_netlist.borrow_mut();
        let mut cells = &mut output_netlist_mut.modules.get_mut("top").unwrap().cells;
        let mut cells_to_add = HashMap::new();
        for (cell_name, cell) in cells.iter_mut() {
            let out = match cell.cell_type.as_ref() {
                "IBUF" | "IOBUFE" | "BUFG" | "BUFGSR" | "BUFGTS" => cell.connections.get_mut("O").unwrap(),
                "ANDTERM" | "ORTERM" | "MACROCELL_XOR" => cell.connections.get_mut("OUT").unwrap(),
                "FDCP" | "FDCP_N" | "LDCP" | "LDCP_N" | "FDDCP" | "FTCP" | "FTCP_N" | "FTDCP" |
                    "FDCPE" | "FDCPE_N" | "FDDCPE" => cell.connections.get_mut("Q").unwrap(),
                _ => unreachable!(),
            };

            if out.len() > 1 {
                let other_outs = out[1..].to_vec();
                out.truncate(1);

                for (i, other_out) in other_outs.into_iter().enumerate() {
                    let mut connections = HashMap::new();
                    connections.insert(String::from("A"), vec![out[0].clone()]);
                    connections.insert(String::from("Y"), vec![other_out]);

                    cells_to_add.insert(format!("autobuf{}_{}", i, cell_name), Cell {
                        hide_name: 0,
                        cell_type: String::from("$_BUF_"),
                        parameters: HashMap::new(),
                        attributes: HashMap::new(),
                        port_directions: HashMap::new(),
                        connections
                    });
                }
            }
        }

        for (cell_name, cell) in cells_to_add {
            cells.insert(cell_name, cell);
        }
    }

    // Write the final output
    output_netlist.borrow().to_writer(&mut ::std::io::stdout()).expect("failed to write json");
}
