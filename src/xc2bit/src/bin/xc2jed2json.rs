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

// FIXME: Un-copypasta
fn zia_table_lookup(device: XC2Device, row: usize) -> &'static [XC2ZIAInput] {
    match device {
        XC2Device::XC2C32 | XC2Device::XC2C32A => &ZIA_MAP_32[row],
        XC2Device::XC2C64 | XC2Device::XC2C64A => &ZIA_MAP_64[row],
        XC2Device::XC2C128 => &ZIA_MAP_128[row],
        XC2Device::XC2C256 => &ZIA_MAP_256[row],
        XC2Device::XC2C384 => &ZIA_MAP_384[row],
        XC2Device::XC2C512 => &ZIA_MAP_512[row],   
    }
}

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
            let cell;

            match node_type {
                "BUFG" => {
                    let mut connections = HashMap::new();
                    connections.insert(String::from("I"), Vec::new());
                    connections.insert(String::from("O"), Vec::new());
                    cell = Cell {
                        hide_name: 0,
                        cell_type: node_type.to_owned(),
                        parameters: HashMap::new(),
                        attributes: HashMap::new(),
                        port_directions: HashMap::new(),
                        connections,
                    }
                },
                "BUFGSR" => {
                    let mut parameters = HashMap::new();
                    parameters.insert(String::from("INVERT"),
                        AttributeVal::N(bitstream.bits.get_global_nets().gsr_invert as usize));
                    let mut connections = HashMap::new();
                    connections.insert(String::from("I"), Vec::new());
                    connections.insert(String::from("O"), Vec::new());
                    cell = Cell {
                        hide_name: 0,
                        cell_type: node_type.to_owned(),
                        parameters,
                        attributes: HashMap::new(),
                        port_directions: HashMap::new(),
                        connections,
                    }
                },
                "BUFGTS" => {
                    let mut parameters = HashMap::new();
                    parameters.insert(String::from("INVERT"),
                        AttributeVal::N(bitstream.bits.get_global_nets().gts_invert[idx as usize] as usize));
                    let mut connections = HashMap::new();
                    connections.insert(String::from("I"), Vec::new());
                    connections.insert(String::from("O"), Vec::new());
                    cell = Cell {
                        hide_name: 0,
                        cell_type: node_type.to_owned(),
                        parameters,
                        attributes: HashMap::new(),
                        port_directions: HashMap::new(),
                        connections,
                    }
                },
                "IOBUFE" => {
                    let (fb, mc) = iob_num_to_fb_mc_num(bitstream.bits.device_type(), idx).unwrap();

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
                            direction: PortDirection::InOut,
                            bits: vec![BitVal::N(toplevel_wire)],
                        });
                    }

                    let mut attributes = HashMap::new();
                    attributes.insert(String::from("LOC"), AttributeVal::S(format!("FB{}_{}", fb + 1, mc + 1)));
                    // Grab attributes
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
                    }

                    let mut connections = HashMap::new();
                    connections.insert(String::from("I"), Vec::new());
                    connections.insert(String::from("E"), Vec::new());
                    connections.insert(String::from("O"), Vec::new());
                    connections.insert(String::from("IO"), vec![BitVal::N(toplevel_wire)]);
                    cell = Cell {
                        hide_name: 0,
                        cell_type: node_type.to_owned(),
                        parameters: HashMap::new(),
                        attributes,
                        port_directions: HashMap::new(),
                        connections,
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

                    let mut attributes = HashMap::new();
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

                    let mut connections = HashMap::new();
                    connections.insert(String::from("O"), Vec::new());
                    connections.insert(String::from("I"), vec![BitVal::N(toplevel_wire)]);
                    cell = Cell {
                        hide_name: 0,
                        cell_type: node_type.to_owned(),
                        parameters: HashMap::new(),
                        attributes,
                        port_directions: HashMap::new(),
                        connections,
                    }
                },
                "ANDTERM" => {
                    let mut attributes = HashMap::new();
                    attributes.insert(String::from("LOC"), AttributeVal::S(format!("FB{}_P{}", fb + 1, idx)));
                    let mut connections = HashMap::new();
                    connections.insert(String::from("IN"), Vec::new());
                    connections.insert(String::from("IN_B"), Vec::new());
                    connections.insert(String::from("OUT"), Vec::new());
                    cell = Cell {
                        hide_name: 0,
                        cell_type: node_type.to_owned(),
                        parameters: HashMap::new(),
                        attributes,
                        port_directions: HashMap::new(),
                        connections,
                    }
                },
                "ORTERM" => {
                    let mut attributes = HashMap::new();
                    attributes.insert(String::from("LOC"), AttributeVal::S(format!("FB{}_{}", fb + 1, idx + 1)));
                    let mut connections = HashMap::new();
                    connections.insert(String::from("IN"), Vec::new());
                    connections.insert(String::from("OUT"), Vec::new());
                    cell = Cell {
                        hide_name: 0,
                        cell_type: node_type.to_owned(),
                        parameters: HashMap::new(),
                        attributes,
                        port_directions: HashMap::new(),
                        connections,
                    }
                },
                "MACROCELL_XOR" => {
                    let mc = &bitstream.bits.get_fb()[fb as usize].mcs[idx as usize];
                    let mut parameters = HashMap::new();
                    if mc.xor_mode == XC2MCXorMode::ZERO || mc.xor_mode == XC2MCXorMode::PTC {
                        parameters.insert(String::from("INVERT_OUT"), AttributeVal::N(0));
                    } else {
                        parameters.insert(String::from("INVERT_OUT"), AttributeVal::N(1));
                    }
                    let mut attributes = HashMap::new();
                    attributes.insert(String::from("LOC"), AttributeVal::S(format!("FB{}_{}", fb + 1, idx + 1)));
                    let mut connections = HashMap::new();
                    connections.insert(String::from("IN_PTC"), Vec::new());
                    connections.insert(String::from("IN_ORTERM"), Vec::new());
                    connections.insert(String::from("OUT"), Vec::new());
                    cell = Cell {
                        hide_name: 0,
                        cell_type: node_type.to_owned(),
                        parameters,
                        attributes,
                        port_directions: HashMap::new(),
                        connections,
                    }
                },
                "REG" => {
                    let mc = &bitstream.bits.get_fb()[fb as usize].mcs[idx as usize];
                    let mut parameters = HashMap::new();
                    parameters.insert(String::from("INIT"), AttributeVal::N(mc.init_state as usize));
                    let mut attributes = HashMap::new();
                    attributes.insert(String::from("LOC"), AttributeVal::S(format!("FB{}_{}", fb + 1, idx + 1)));
                    let mut connections = HashMap::new();
                    connections.insert(String::from("Q"), Vec::new());
                    connections.insert(String::from("PRE"), Vec::new());
                    connections.insert(String::from("CLR"), Vec::new());

                    let cell_type;
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

                    cell = Cell {
                        hide_name: 0,
                        cell_type,
                        parameters,
                        attributes,
                        port_directions: HashMap::new(),
                        connections,
                    }
                },
                _ => unreachable!()
            }

            // Create the cell in the output module
            let mut output_netlist_mut = output_netlist.borrow_mut();
            let mut cells = &mut output_netlist_mut.modules.get_mut("top").unwrap().cells;
            cells.insert(node_name.to_owned(), cell);

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
            let mut output_netlist_mut = output_netlist.borrow_mut();
            let mut cells = &mut output_netlist_mut.modules.get_mut("top").unwrap().cells;

            match node_type.as_ref() {
                "BUFG" => {
                    if port_name == "I" {
                        if bitstream.bits.get_global_nets().gck_enable[idx as usize] {
                            cells.get_mut(node_name).unwrap().connections.get_mut(port_name).unwrap()
                                .push(BitVal::N(wire_ref));
                        } else {
                            cells.get_mut(node_name).unwrap().connections.get_mut(port_name).unwrap()
                                .push(BitVal::S(SpecialBit::_0));
                        }
                    } else if port_name == "O" {
                        cells.get_mut(node_name).unwrap().connections.get_mut(port_name).unwrap()
                            .push(BitVal::N(wire_ref));
                    } else {
                        unreachable!();
                    }
                },
                "BUFGSR" => {
                    // FIXME: Test the interaction with the invert bit
                    if port_name == "I" {
                        if bitstream.bits.get_global_nets().gsr_enable {
                            cells.get_mut(node_name).unwrap().connections.get_mut(port_name).unwrap()
                                .push(BitVal::N(wire_ref));
                        } else {
                            cells.get_mut(node_name).unwrap().connections.get_mut(port_name).unwrap()
                                .push(BitVal::S(SpecialBit::_0));
                        }
                    } else if port_name == "O" {
                        cells.get_mut(node_name).unwrap().connections.get_mut(port_name).unwrap()
                            .push(BitVal::N(wire_ref));
                    } else {
                        unreachable!();
                    }
                },
                "BUFGTS" => {
                    // FIXME: Test the interaction with the invert bit
                    if port_name == "I" {
                        if bitstream.bits.get_global_nets().gts_enable[idx as usize] {
                            cells.get_mut(node_name).unwrap().connections.get_mut(port_name).unwrap()
                                .push(BitVal::N(wire_ref));
                        } else {
                            cells.get_mut(node_name).unwrap().connections.get_mut(port_name).unwrap()
                                .push(BitVal::S(SpecialBit::_0));
                        }
                    } else if port_name == "O" {
                        cells.get_mut(node_name).unwrap().connections.get_mut(port_name).unwrap()
                            .push(BitVal::N(wire_ref));
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
                            cells.get_mut(node_name).unwrap().connections.get_mut(port_name).unwrap()
                                .push(BitVal::S(SpecialBit::_0));
                        } else {
                            cells.get_mut(node_name).unwrap().connections.get_mut(port_name).unwrap()
                                .push(BitVal::N(wire_ref));
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
                        if obuf_mode.unwrap() == XC2IOBOBufMode::Disabled && port_idx == 0 {
                            cells.get_mut(node_name).unwrap().connections.get_mut(port_name).unwrap()
                                .push(BitVal::S(SpecialBit::_0));
                        } else if (obuf_mode.unwrap() == XC2IOBOBufMode::PushPull && port_idx == 0) ||
                                  (obuf_mode.unwrap() == XC2IOBOBufMode::CGND && port_idx == 0) {
                            cells.get_mut(node_name).unwrap().connections.get_mut(port_name).unwrap()
                                .push(BitVal::S(SpecialBit::_1));
                        } else if (obuf_mode.unwrap() == XC2IOBOBufMode::OpenDrain && port_idx == 4) ||
                                  (obuf_mode.unwrap() == XC2IOBOBufMode::TriStateGTS0 && port_idx == 0) ||
                                  (obuf_mode.unwrap() == XC2IOBOBufMode::TriStateGTS1 && port_idx == 1) ||
                                  (obuf_mode.unwrap() == XC2IOBOBufMode::TriStateGTS2 && port_idx == 2) ||
                                  (obuf_mode.unwrap() == XC2IOBOBufMode::TriStateGTS3 && port_idx == 3) ||
                                  (obuf_mode.unwrap() == XC2IOBOBufMode::TriStateCTE && port_idx == 5) ||
                                  (obuf_mode.unwrap() == XC2IOBOBufMode::TriStatePTB && port_idx == 6) {
                            cells.get_mut(node_name).unwrap().connections.get_mut(port_name).unwrap()
                                .push(BitVal::N(wire_ref));
                        }
                    } else if port_name == "O" {
                        // This is always driven
                        cells.get_mut(node_name).unwrap().connections.get_mut(port_name).unwrap()
                            .push(BitVal::N(wire_ref));
                    } else {
                        unreachable!();
                    }
                },
                "IBUF" => {
                    if port_name == "O" {
                        // This is always driven
                        cells.get_mut(node_name).unwrap().connections.get_mut(port_name).unwrap()
                            .push(BitVal::N(wire_ref));
                    } else {
                        unreachable!();
                    }
                },
                "ANDTERM" => {
                    if port_name == "OUT" {
                        // This is always driven
                        cells.get_mut(node_name).unwrap().connections.get_mut(port_name).unwrap()
                            .push(BitVal::N(wire_ref));
                    } else if port_name == "IN" {
                        // Whee, ZIA goes here

                        let zia_row = zia_table_lookup(bitstream.bits.device_type(), port_idx as usize);
                        // FIXME: port_idx is a hack
                        if bitstream.bits.get_fb()[fb as usize].zia_bits[port_idx as usize].selected ==
                            XC2ZIAInput::One && port_idx == 0{

                            if bitstream.bits.get_fb()[fb as usize].and_terms[idx as usize]
                                .input[port_idx as usize] {

                                cells.get_mut(node_name).unwrap().connections.get_mut("IN").unwrap()
                                    .push(BitVal::S(SpecialBit::_1));
                            }
                            if bitstream.bits.get_fb()[fb as usize].and_terms[idx as usize]
                                .input_b[port_idx as usize] {

                                cells.get_mut(node_name).unwrap().connections.get_mut("IN_B").unwrap()
                                    .push(BitVal::S(SpecialBit::_1));
                            }
                        } else if bitstream.bits.get_fb()[fb as usize].zia_bits[port_idx as usize].selected ==
                            XC2ZIAInput::Zero && port_idx == 0 {

                            if bitstream.bits.get_fb()[fb as usize].and_terms[idx as usize]
                                .input[port_idx as usize] {

                                cells.get_mut(node_name).unwrap().connections.get_mut("IN").unwrap()
                                    .push(BitVal::S(SpecialBit::_1));
                            }
                            if bitstream.bits.get_fb()[fb as usize].and_terms[idx as usize]
                                .input_b[port_idx as usize] {

                                cells.get_mut(node_name).unwrap().connections.get_mut("IN_B").unwrap()
                                    .push(BitVal::S(SpecialBit::_1));
                            }
                        } else if bitstream.bits.get_fb()[fb as usize].zia_bits[port_idx as usize].selected ==
                            zia_row[extra_data.0 as usize] {

                            let zia_choice = zia_row[extra_data.0 as usize];
                            let is_connected = match &zia_choice {
                                &XC2ZIAInput::Macrocell{fb: zia_fb, mc: zia_mc} => {
                                    // FIXME Hack
                                    if bitstream.bits.get_fb()[zia_fb as usize].mcs[zia_mc as usize].fb_mode ==
                                        XC2MCFeedbackMode::Disabled && extra_data.1 == 0 {

                                        if bitstream.bits.get_fb()[fb as usize].and_terms[idx as usize]
                                            .input[port_idx as usize] {

                                            cells.get_mut(node_name).unwrap().connections.get_mut("IN").unwrap()
                                                .push(BitVal::S(SpecialBit::_0));
                                        }
                                        if bitstream.bits.get_fb()[fb as usize].and_terms[idx as usize]
                                            .input_b[port_idx as usize] {

                                            cells.get_mut(node_name).unwrap().connections.get_mut("IN_B").unwrap()
                                                .push(BitVal::S(SpecialBit::_0));
                                        }
                                    }

                                    (bitstream.bits.get_fb()[zia_fb as usize].mcs[zia_mc as usize].fb_mode ==
                                        XC2MCFeedbackMode::COMB && extra_data.1 == 0) ||
                                        (bitstream.bits.get_fb()[zia_fb as usize].mcs[zia_mc as usize].fb_mode ==
                                            XC2MCFeedbackMode::REG && extra_data.1 == 1)
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
                                        if bitstream.bits.get_fb()[fb as usize].and_terms[idx as usize]
                                            .input[port_idx as usize] {

                                            cells.get_mut(node_name).unwrap().connections.get_mut("IN").unwrap()
                                                .push(BitVal::S(SpecialBit::_0));
                                        }
                                        if bitstream.bits.get_fb()[fb as usize].and_terms[idx as usize]
                                            .input_b[port_idx as usize] {

                                            cells.get_mut(node_name).unwrap().connections.get_mut("IN_B").unwrap()
                                                .push(BitVal::S(SpecialBit::_0));
                                        }
                                    }

                                    (zia_mode.unwrap() == XC2IOBZIAMode::PAD && extra_data.1 == 0) ||
                                        (zia_mode.unwrap() == XC2IOBZIAMode::REG && extra_data.1 == 1)
                                },
                                &XC2ZIAInput::DedicatedInput => true,
                                // These cannot be in the choices table; they are special cases
                                _ => unreachable!(),
                            };

                            if is_connected {
                                if bitstream.bits.get_fb()[fb as usize].and_terms[idx as usize]
                                    .input[port_idx as usize] {

                                    cells.get_mut(node_name).unwrap().connections.get_mut("IN").unwrap()
                                        .push(BitVal::N(wire_ref));
                                }
                                if bitstream.bits.get_fb()[fb as usize].and_terms[idx as usize]
                                    .input_b[port_idx as usize] {

                                    cells.get_mut(node_name).unwrap().connections.get_mut("IN_B").unwrap()
                                        .push(BitVal::N(wire_ref));
                                }
                            }
                        }
                    } else {
                        unreachable!();
                    }
                },
                "ORTERM" => {
                    if port_name == "OUT" {
                        // This is always driven
                        cells.get_mut(node_name).unwrap().connections.get_mut(port_name).unwrap()
                            .push(BitVal::N(wire_ref));
                    } else if port_name == "IN" {
                        if bitstream.bits.get_fb()[fb as usize].or_terms[idx as usize].input[port_idx as usize] {
                            cells.get_mut(node_name).unwrap().connections.get_mut(port_name).unwrap()
                                .push(BitVal::N(wire_ref));
                        }
                    } else {
                        unreachable!();
                    }
                },
                "MACROCELL_XOR" => {
                    if port_name == "IN_PTC" {
                        if (bitstream.bits.get_fb()[fb as usize].mcs[idx as usize].xor_mode == XC2MCXorMode::PTC) ||
                           (bitstream.bits.get_fb()[fb as usize].mcs[idx as usize].xor_mode == XC2MCXorMode::PTCB) {

                            cells.get_mut(node_name).unwrap().connections.get_mut(port_name).unwrap()
                                .push(BitVal::N(wire_ref));
                        } else {
                            cells.get_mut(node_name).unwrap().connections.get_mut(port_name).unwrap()
                                .push(BitVal::S(SpecialBit::_0));
                        }
                    } else if port_name == "IN_ORTERM" {
                        cells.get_mut(node_name).unwrap().connections.get_mut(port_name).unwrap()
                            .push(BitVal::N(wire_ref));
                    } else if port_name == "OUT" {
                        // This wire is always driven
                        if port_idx == 0 {
                            cells.get_mut(node_name).unwrap().connections.get_mut(port_name).unwrap()
                                .push(BitVal::N(wire_ref));
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
                                    cells.get_mut(node_name).unwrap().connections.get_mut(port_name).unwrap()
                                        .push(BitVal::N(wire_ref));
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
                            cells.get_mut(node_name).unwrap().connections.get_mut(port_name).unwrap()
                                .push(BitVal::N(wire_ref));
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
                                    cells.get_mut(node_name).unwrap().connections.get_mut(port_name).unwrap()
                                        .push(BitVal::N(wire_ref));
                                }
                            }
                        }
                    } else if port_name == "CE" {
                        if bitstream.bits.get_fb()[fb as usize].mcs[idx as usize].reg_mode == XC2MCRegMode::DFFCE {
                            cells.get_mut(node_name).unwrap().connections.get_mut(port_name).unwrap()
                                .push(BitVal::N(wire_ref));
                        }
                    } else if port_name == "CLK" {
                        let clk_src = bitstream.bits.get_fb()[fb as usize].mcs[idx as usize].clk_src;
                        if (clk_src == XC2MCRegClkSrc::GCK0 && port_idx == 0) ||
                           (clk_src == XC2MCRegClkSrc::GCK1 && port_idx == 1) ||
                           (clk_src == XC2MCRegClkSrc::GCK2 && port_idx == 2) ||
                           (clk_src == XC2MCRegClkSrc::CTC && port_idx == 3) ||
                           (clk_src == XC2MCRegClkSrc::PTC && port_idx == 4) {

                            let port_name = if bitstream.bits.get_fb()[fb as usize]
                                .mcs[idx as usize].reg_mode == XC2MCRegMode::LATCH {

                                "G"
                            } else {
                                "C"
                            };

                            cells.get_mut(node_name).unwrap().connections.get_mut(port_name).unwrap()
                                .push(BitVal::N(wire_ref));
                        }
                    } else if port_name == "S" {
                        let s_src = bitstream.bits.get_fb()[fb as usize].mcs[idx as usize].s_src;
                        if (s_src == XC2MCRegSetSrc::GSR && port_idx == 0) ||
                           (s_src == XC2MCRegSetSrc::CTS && port_idx == 1) ||
                           (s_src == XC2MCRegSetSrc::PTA && port_idx == 2) {

                            cells.get_mut(node_name).unwrap().connections.get_mut("PRE").unwrap()
                                .push(BitVal::N(wire_ref));
                        } else if s_src == XC2MCRegSetSrc::Disabled && port_idx == 0 {
                            cells.get_mut(node_name).unwrap().connections.get_mut("PRE").unwrap()
                                .push(BitVal::S(SpecialBit::_0));
                        }
                    } else if port_name == "R" {
                        let r_src = bitstream.bits.get_fb()[fb as usize].mcs[idx as usize].r_src;
                        if (r_src == XC2MCRegResetSrc::GSR && port_idx == 0) ||
                           (r_src == XC2MCRegResetSrc::CTR && port_idx == 1) ||
                           (r_src == XC2MCRegResetSrc::PTA && port_idx == 2) {

                            cells.get_mut(node_name).unwrap().connections.get_mut("CLR").unwrap()
                                .push(BitVal::N(wire_ref));
                        } else if r_src == XC2MCRegResetSrc::Disabled && port_idx == 0 {
                            cells.get_mut(node_name).unwrap().connections.get_mut("CLR").unwrap()
                                .push(BitVal::S(SpecialBit::_0));
                        }
                    } else if port_name == "D/T" {
                            let port_name = if bitstream.bits.get_fb()[fb as usize]
                                .mcs[idx as usize].reg_mode == XC2MCRegMode::TFF {

                                "T"
                            } else {
                                "D"
                            };

                        if bitstream.bits.get_fb()[fb as usize].mcs[idx as usize].ff_in_ibuf {
                            if port_idx == 1 {
                                cells.get_mut(node_name).unwrap().connections.get_mut(port_name).unwrap()
                                    .push(BitVal::N(wire_ref));
                            }
                        } else {
                            if port_idx == 0 {
                                cells.get_mut(node_name).unwrap().connections.get_mut(port_name).unwrap()
                                    .push(BitVal::N(wire_ref));
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
        }
    );

    // Write the final output
    output_netlist.borrow().to_writer(&mut ::std::io::stdout()).expect("failed to write json");
}
