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

//! Contains routines that provide the CPLD structure to other programs.

use *;

/// Returns the function block and macrocell index of the global clock signal GCKn for the given device
pub fn get_gck(device: XC2Device, idx: usize) -> Option<(u32, u32)> {
    if idx >= 3 {
        return None;
    }

    match device {
        XC2Device::XC2C32 | XC2Device::XC2C32A => match idx {
            0 => Some((1, 4)), 1 => Some((1, 5)), 2 => Some((1, 6)), _ => unreachable!()
        },
        XC2Device::XC2C64 | XC2Device::XC2C64A => match idx {
            0 => Some((1, 6)), 1 => Some((1, 7)), 2 => Some((1, 9)), _ => unreachable!()
        },
        XC2Device::XC2C128 => match idx {
            0 => Some((1, 12)), 1 => Some((1, 13)), 2 => Some((1, 15)), _ => unreachable!()
        },
        XC2Device::XC2C256 => match idx {
            0 => Some((4, 5)), 1 => Some((4, 3)), 2 => Some((5, 3)), _ => unreachable!()
        },
        XC2Device::XC2C384 => match idx {
            0 => Some((6, 14)), 1 => Some((6, 11)), 2 => Some((7, 1)), _ => unreachable!()
        },
        XC2Device::XC2C512 => match idx {
            0 => Some((10, 2)), 1 => Some((8, 15)), 2 => Some((9, 2)), _ => unreachable!()
        }
    }
}

/// Returns the function block and macrocell index of the global tristate signal GTSn for the given device
pub fn get_gts(device: XC2Device, idx: usize) -> Option<(u32, u32)> {
    if idx >= 4 {
        return None;
    }

    match device {
        XC2Device::XC2C32 | XC2Device::XC2C32A => match idx {
            0 => Some((0, 4)), 1 => Some((0, 3)), 2 => Some((0, 6)), 3 => Some((0, 5)), _ => unreachable!()
        },
        XC2Device::XC2C64 | XC2Device::XC2C64A => match idx {
            0 => Some((0, 9)), 1 => Some((0, 8)), 2 => Some((0, 11)), 3 => Some((0, 10)), _ => unreachable!()
        },
        XC2Device::XC2C128 => match idx {
            0 => Some((0, 15)), 1 => Some((0, 14)), 2 => Some((2, 2)), 3 => Some((2, 1)), _ => unreachable!()
        },
        XC2Device::XC2C256 => match idx {
            0 => Some((1, 4)), 1 => Some((1, 11)), 2 => Some((1, 0)), 3 => Some((1, 2)), _ => unreachable!()
        },
        XC2Device::XC2C384 => match idx {
            0 => Some((1, 4)), 1 => Some((1, 14)), 2 => Some((1, 0)), 3 => Some((1, 2)), _ => unreachable!()
        },
        XC2Device::XC2C512 => match idx {
            0 => Some((0, 0)), 1 => Some((1, 13)), 2 => Some((0, 12)), 3 => Some((0, 2)), _ => unreachable!()
        }
    }
}

/// Returns the function block and macrocell index of the global set/reset signal GSR for the given device
pub fn get_gsr(device: XC2Device) -> (u32, u32) {
    match device {
        XC2Device::XC2C32 | XC2Device::XC2C32A => (0, 7),
        XC2Device::XC2C64 | XC2Device::XC2C64A => (0, 12),
        XC2Device::XC2C128 => (2, 3),
        XC2Device::XC2C256 => (0, 2),
        XC2Device::XC2C384 => (0, 2),
        XC2Device::XC2C512 => (0, 15),
    }
}

/// Returns the function block and macrocell index of the global clock divider reset signal CDRST for the given device
pub fn get_cdrst(device: XC2Device) -> Option<(u32, u32)> {
    match device {
        XC2Device::XC2C32 | XC2Device::XC2C32A => None,
        XC2Device::XC2C64 | XC2Device::XC2C64A => None,
        XC2Device::XC2C128 => Some((1, 14)),
        XC2Device::XC2C256 => Some((5, 1)),
        XC2Device::XC2C384 => Some((6, 0)),
        XC2Device::XC2C512 => Some((9, 0)),
    }
}

/// Returns the function block and macrocell index of the global DataGATE enable signal DGE for the given device
pub fn get_dge(device: XC2Device) -> Option<(u32, u32)> {
    match device {
        XC2Device::XC2C32 | XC2Device::XC2C32A => None,
        XC2Device::XC2C64 | XC2Device::XC2C64A => None,
        XC2Device::XC2C128 => Some((3, 0)),
        XC2Device::XC2C256 => Some((5, 11)),
        XC2Device::XC2C384 => Some((7, 4)),
        XC2Device::XC2C512 => Some((9, 13)),
    }
}

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

/// This function calls the passed-in callbacks to provide information about the structure of the CPLD. `node_callback`
/// is called to "create" a new node, `wire_callback` is called to "create" a new wire, and `connection_callback` is
/// called to connect one port on a node to a wire. The arguments to the callbacks are:
///
/// * `node_callback`: node unique name, node type, function block index, index within function block
/// * `wire_callback`: wire unique name
/// * `connection_callback`: value returned from `node_callback`, value returned from `wire_callback`,
///    port name, index within port
///
/// This interface was designed specifically for the place-and-route tool to build up a model of the device. However,
/// it is sufficiently generic to be useful for other programs as well. The interface is designed around callbacks
/// so that it does not dictate the data structures that the calling program will use, and so that it does not
/// needlessly build up a graph data structure that the calling program will quickly discard. Nodes and wires are
/// identified with strings (rather than numbers) as a compromise between performance and convenience to the caller.
///
/// Note that mux sites are not represented here. They just appear as multiple drivers onto the same wire.
pub fn get_device_structure<N, W, C>(device: XC2Device,
    mut node_callback: N, mut wire_callback: W, mut connection_callback: C)
    where N: FnMut(&str, &str, u32, u32) -> usize,
          W: FnMut(&str) -> usize,
          C: FnMut(usize, usize, &str, u32) -> () {

    // Global buffers and the output wires
    // Cannot create the input wires until after IO stuff is created
    // GCK
    let gck = (0..3).map(|i| {
        let w = wire_callback(&format!("gck_{}", i));
        let n = node_callback(&format!("bufg_gck_{}", i), "BUFG", 0, i);
        connection_callback(n, w, "O", 0);
        (w, n)
    }).collect::<Vec<_>>();
    // GTS
    let gts = (0..4).map(|i| {
        let w = wire_callback(&format!("gts_{}", i));
        let n = node_callback(&format!("bufg_gts_{}", i), "BUFGTS", 0, i);
        connection_callback(n, w, "O", 0);
        (w, n)
    }).collect::<Vec<_>>();
    // GSR
    let gsr_wire = wire_callback("gsr");
    let gsr_node = node_callback("bufg_gsr", "BUFGSR", 0, 0);
    connection_callback(gsr_node, gsr_wire, "O", 0);

    // Function blocks
    let fb_things = (0..device.num_fbs() as u32).map(|fb| {
        // AND terms
        let pterm_wires = (0..ANDTERMS_PER_FB).map(|i| {
            wire_callback(&format!("fb{}_pterm{}", fb, i))
        }).collect::<Vec<_>>();
        // OR terms
        let orterm_wires = (0..MCS_PER_FB).map(|i| {
            wire_callback(&format!("fb{}_or{}", fb, i))
        }).collect::<Vec<_>>();
        // XOR terms
        let xorterm_wires = (0..MCS_PER_FB).map(|i| {
            wire_callback(&format!("fb{}_xor{}", fb, i))
        }).collect::<Vec<_>>();
        // Register output
        let regout_wires = (0..MCS_PER_FB).map(|i| {
            wire_callback(&format!("fb{}_regout{}", fb, i))
        }).collect::<Vec<_>>();

        // AND gates
        let and_nodes = (0..ANDTERMS_PER_FB).map(|i| {
            let n = node_callback(&format!("fb{}_andgate{}", fb, i), "ANDTERM", fb, i as u32);
            connection_callback(n, pterm_wires[i], "OUT", 0);

            n
        }).collect::<Vec<_>>();
        
        // OR gates
        for i in 0..MCS_PER_FB {
            let n = node_callback(&format!("fb{}_orgate{}", fb, i), "ORTERM", fb, i as u32);
            connection_callback(n, orterm_wires[i], "OUT", 0);

            // Inputs
            for j in 0..ANDTERMS_PER_FB {
                connection_callback(n, pterm_wires[j], "IN", j as u32);
            }
        }

        // XOR gates
        let xor_nodes = (0..MCS_PER_FB).map(|i| {
            let n = node_callback(&format!("fb{}_xorgate{}", fb, i), "MACROCELL_XOR", fb, i as u32);
            connection_callback(n, xorterm_wires[i], "OUT", 0);

            // Inputs
            connection_callback(n, orterm_wires[i], "IN_ORTERM", 0);
            connection_callback(n, pterm_wires[get_ptc(i as u32) as usize], "IN_PTC", 0);

            n
        }).collect::<Vec<_>>();

        // Registers
        let reg_nodes = (0..MCS_PER_FB).map(|i| {
            let n = node_callback(&format!("fb{}_reg{}", fb, i), "REG", fb, i as u32);

            // Output
            connection_callback(n, regout_wires[i], "Q", 0);

            // D/T input
            connection_callback(n, xorterm_wires[i], "D/T", 0);

            // CE input
            connection_callback(n, pterm_wires[get_ptc(i as u32) as usize], "CE", 0);

            // Clock sources
            // GCK
            for j in 0..3 {
                connection_callback(n, gck[j].0, "CLK", j as u32);
            }
            // CTC
            connection_callback(n, pterm_wires[CTC as usize], "CLK", 3);
            // PTC
            connection_callback(n, pterm_wires[get_ptc(i as u32) as usize], "CLK", 4);

            // Set
            // GSR
            connection_callback(n, gsr_wire, "S", 0);
            // CTS
            connection_callback(n, pterm_wires[CTS as usize], "S", 1);
            // PTA
            connection_callback(n, pterm_wires[get_pta(i as u32) as usize], "S", 2);

            // Reset
            // GSR
            connection_callback(n, gsr_wire, "R", 0);
            // CTR
            connection_callback(n, pterm_wires[CTR as usize], "R", 1);
            // PTA
            connection_callback(n, pterm_wires[get_pta(i as u32) as usize], "R", 2);

            n
        }).collect::<Vec<_>>();

        (reg_nodes, xor_nodes, pterm_wires, and_nodes, regout_wires, xorterm_wires)
    }).collect::<Vec<_>>();

    // Input/output to IOB
    let to_from_iob_wires = (0..device.num_iobs() as u32).map(|iob_idx| {
        let (fb, i) = iob_num_to_fb_mc_num(device, iob_idx).unwrap();
        // Wire that goes into the IOB
        let to_w = wire_callback(&format!("to_iob_{}", iob_idx));

        // Wire that will go into the ZIA
        let from_w = wire_callback(&format!("from_iob_{}", iob_idx));

        // To the ZIA
        connection_callback(fb_things[fb as usize].0[i as usize], from_w, "D/T", 1);

        // From the XOR
        connection_callback(fb_things[fb as usize].1[i as usize], to_w, "OUT", 1);
        // From the register
        connection_callback(fb_things[fb as usize].0[i as usize], to_w, "Q", 1);

        (to_w, from_w)
    }).collect::<Vec<_>>();

    // IO buffers
    for iob_idx in 0..device.num_iobs() {
        // The node
        let n = node_callback(&format!("iob_{}", iob_idx), "IOBUFE", 0, iob_idx as u32);

        // The input to the IOB (from the macrocell, to the outside world)
        connection_callback(n, to_from_iob_wires[iob_idx].0, "I", 0);

        // The output from the IOB (from the outside world, into the circuitry)
        connection_callback(n, to_from_iob_wires[iob_idx].1, "O", 0);

        // The output enables
        let (iob_fb, iob_mc) = iob_num_to_fb_mc_num(device, iob_idx as u32).unwrap();
        // GTS
        for i in 0..4 {
            connection_callback(n, gts[i].0, "E", i as u32);
        }
        // Open-drain mode
        connection_callback(n, to_from_iob_wires[iob_idx].0, "E", 4);
        // CTE
        connection_callback(n, fb_things[iob_fb as usize].2[CTE as usize], "E", 5);
        // PTB
        connection_callback(n, fb_things[iob_fb as usize].2[get_ptb(iob_mc) as usize], "E", 6);
    }

    // Input-only pad
    let from_ipad_w;
    match device {
        XC2Device::XC2C32 | XC2Device::XC2C32A => {
            // Wire that will go into the ZIA
            let w = wire_callback("from_ipad");
            from_ipad_w = Some(w);

            // The node
            let n = node_callback("ipad", "IBUF", 0, 0);

            // The output from the IOB (from the outside world, into the circuitry)
            connection_callback(n, w, "O", 0);
        },
        _ => {
            from_ipad_w = None;
        }
    }

    // Inputs into the global buffers
    // GCK
    for i in 0..3 {
        let (fb, mc) = get_gck(device, i as usize).unwrap();
        let iob_idx = fb_mc_num_to_iob_num(device, fb, mc).unwrap();
        connection_callback(gck[i].1, to_from_iob_wires[iob_idx as usize].1, "I", 0);
    }
    // GTS
    for i in 0..4 {
        let (fb, mc) = get_gts(device, i as usize).unwrap();
        let iob_idx = fb_mc_num_to_iob_num(device, fb, mc).unwrap();
        connection_callback(gts[i].1, to_from_iob_wires[iob_idx as usize].1, "I", 0);
    }
    // GSR
    {
        let (fb, mc) = get_gsr(device);
        let iob_idx = fb_mc_num_to_iob_num(device, fb, mc).unwrap();
        connection_callback(gsr_node, to_from_iob_wires[iob_idx as usize].1, "I", 0);
    }

    // The ZIA
    for zia_row_i in 0..INPUTS_PER_ANDTERM as u32 {
        for zia_choice in zia_table_lookup(device, zia_row_i as usize) {
            for and_fb in 0..device.num_fbs() as u32 {
                for and_i in 0..ANDTERMS_PER_FB as u32 {
                    match zia_choice {
                        &XC2ZIAInput::Macrocell{fb: zia_fb, mc: zia_mc} => {
                            // From the XOR gate
                            connection_callback(fb_things[and_fb as usize].3[and_i as usize],
                                fb_things[zia_fb as usize].5[zia_mc as usize], "IN", zia_row_i);
                            // From the register
                            connection_callback(fb_things[and_fb as usize].3[and_i as usize],
                                fb_things[zia_fb as usize].4[zia_mc as usize], "IN", zia_row_i);
                        },
                        &XC2ZIAInput::IBuf{ibuf: zia_iob} => {
                            let (iob_fb, iob_mc) = iob_num_to_fb_mc_num(device, zia_iob).unwrap();
                            // From the pad
                            connection_callback(fb_things[and_fb as usize].3[and_i as usize],
                                to_from_iob_wires[zia_iob as usize].1, "IN", zia_row_i);
                            // From the register
                            connection_callback(fb_things[and_fb as usize].3[and_i as usize],
                                fb_things[iob_fb as usize].4[iob_mc as usize], "IN", zia_row_i);
                        },
                        &XC2ZIAInput::DedicatedInput => {
                            connection_callback(fb_things[and_fb as usize].3[and_i as usize],
                                from_ipad_w.unwrap(), "IN", zia_row_i);
                        },
                        // These cannot be in the choices table; they are special cases
                        _ => unreachable!(),
                    }
                }
            }
        }
    }
}
