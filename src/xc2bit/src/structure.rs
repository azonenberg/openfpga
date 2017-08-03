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

/// This function calls the passed-in callbacks to provide information about the structure of the CPLD. `node_callback`
/// is called to "create" a new node, `wire_callback` is called to "create" a new wire, and `connection_callback` is
/// called to connect one port on a node to a wire. The arguments to the callbacks are:
///
/// * `node_callback`: node unique name, node type, function block index, index within function block
/// * `wire_callback`: wire unique name
/// * `connection_callback`: node unique name, node type, function block index, index within function block,
///    wire name, port name, index within port
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
    where N: FnMut(&str, &str, u32, u32) -> (),
          W: FnMut(&str) -> (),
          C: FnMut(&str, &str, u32, u32, &str, &str, u32) -> () {

    // Global buffers and the output wires
    // Cannot create the input wires until after IO stuff is created
    // GCK
    for i in 0..3 {
        wire_callback(&format!("gck_{}", i));
        node_callback(&format!("bufg_gck_{}", i), "BUFG", 0, i);
        connection_callback(&format!("bufg_gck_{}", i), "BUFG", 0, i,
            &format!("gck_{}", i), "O", 0);
    }
    // GTS
    for i in 0..4 {
        wire_callback(&format!("gts_{}", i));
        node_callback(&format!("bufg_gts_{}", i), "BUFGTS", 0, i);
        connection_callback(&format!("bufg_gts_{}", i), "BUFGTS", 0, i,
            &format!("gts_{}", i), "O", 0);
    }
    // GSR
    wire_callback("gsr");
    node_callback("bufg_gsr_{}", "BUFGSR", 0, 0);
    connection_callback("bufg_gsr", "BUFGSR", 0, 0,
        "gsr", "O", 0);

    // Function blocks
    for fb in 0..device.num_fbs() as u32 {
        // AND terms
        for i in 0..ANDTERMS_PER_FB {
            wire_callback(&format!("fb{}_pterm{}", fb, i));
        }
        // OR terms
        for i in 0..MCS_PER_FB {
            wire_callback(&format!("fb{}_or{}", fb, i));
        }
        // XOR terms
        for i in 0..MCS_PER_FB {
            wire_callback(&format!("fb{}_xor{}", fb, i));
        }
        
        // AND gates
        for i in 0..ANDTERMS_PER_FB as u32 {
            node_callback(&format!("fb{}_andgate{}", fb, i), "ANDTERM", fb, i);
            connection_callback(&format!("fb{}_andgate{}", fb, i), "ANDTERM", fb, i,
                &format!("fb{}_pterm{}", fb, i), "OUT", 0);
        }
        
        // OR gates
        for i in 0..MCS_PER_FB as u32 {
            node_callback(&format!("fb{}_orgate{}", fb, i), "ORTERM", fb, i);
            connection_callback(&format!("fb{}_orgate{}", fb, i), "ORTERM", fb, i,
                &format!("fb{}_or{}", fb, i), "OUT", 0);

            // Inputs
            for j in 0..ANDTERMS_PER_FB as u32 {
                connection_callback(&format!("fb{}_orgate{}", fb, i), "ORTERM", fb, i,
                    &format!("fb{}_pterm{}", fb, j), "IN", j);
            }
        }
        
        // XOR gates
        for i in 0..MCS_PER_FB as u32 {
            node_callback(&format!("fb{}_xorgate{}", fb, i), "MACROCELL_XOR", fb, i);
            connection_callback(&format!("fb{}_xorgate{}", fb, i), "MACROCELL_XOR", fb, i,
                &format!("fb{}_xor{}", fb, i), "OUT", 0);

            // Inputs
            connection_callback(&format!("fb{}_xorgate{}", fb, i), "MACROCELL_XOR", fb, i,
                &format!("fb{}_or{}", fb, i), "IN_ORTERM", 0);
            connection_callback(&format!("fb{}_xorgate{}", fb, i), "MACROCELL_XOR", fb, i,
                &format!("fb{}_pterm{}", fb, get_ptc(i)), "IN_PTC", 0);
        }
    }

    // IO buffers
    for iob_idx in 0..device.num_iobs() as u32 {
        // Wire that will go into the ZIA
        wire_callback(&format!("from_iob_{}", iob_idx));

        // Wire that goes into the IOB from various sources
        wire_callback(&format!("to_iob_{}", iob_idx));

        // The node
        node_callback(&format!("iob_{}", iob_idx), "IOBUFE", 0, iob_idx);

        // The input to the IOB (from the macrocell, to the outside world)
        connection_callback(&format!("iob_{}", iob_idx), "IOBUFE", 0, iob_idx,
            &format!("to_iob_{}", iob_idx), "I", 0);

        // The output from the IOB (from the outside world, into the circuitry)
        connection_callback(&format!("iob_{}", iob_idx), "IOBUFE", 0, iob_idx,
            &format!("from_iob_{}", iob_idx), "O", 0);

        // The output enables
        let (iob_ff, iob_mc) = iob_num_to_fb_mc_num(device, iob_idx).unwrap();
        // GTS
        for i in 0..4 {
            connection_callback(&format!("iob_{}", iob_idx), "IOBUFE", 0, iob_idx,
                &format!("gts_{}", i), "E", 0);
        }
        // CTE
        connection_callback(&format!("iob_{}", iob_idx), "IOBUFE", 0, iob_idx,
            &format!("fb{}_pterm{}", iob_ff, CTE), "E", 0);
        // PTB
        connection_callback(&format!("iob_{}", iob_idx), "IOBUFE", 0, iob_idx,
            &format!("fb{}_pterm{}", iob_ff, get_ptb(iob_mc)), "E", 0);
    }

    // Input-only pad
    match device {
        XC2Device::XC2C32 | XC2Device::XC2C32A => {
            // Wire that will go into the ZIA
            wire_callback("from_ipad");

            // The node
            node_callback("ipad", "IBUF", 0, 0);

            // The output from the IOB (from the outside world, into the circuitry)
            connection_callback("ipad", "IBUF", 0, 0,
                "from_ipad", "O", 0);
        },
        _ => {}
    }

    // Inputs into the global buffers
    // GCK
    for i in 0..3 {
        let (fb, mc) = get_gck(device, i as usize).unwrap();
        let iob_idx = fb_mc_num_to_iob_num(device, fb, mc).unwrap();
        connection_callback(&format!("bufg_gck_{}", i), "BUFG", 0, i,
            &format!("from_iob_{}", iob_idx), "I", 0);
    }
    // GTS
    for i in 0..4 {
        let (fb, mc) = get_gts(device, i as usize).unwrap();
        let iob_idx = fb_mc_num_to_iob_num(device, fb, mc).unwrap();
        connection_callback(&format!("bufg_gts_{}", i), "BUFGTS", 0, i,
            &format!("from_iob_{}", iob_idx), "I", 0);
    }
    // GSR
    {
        let (fb, mc) = get_gsr(device);
        let iob_idx = fb_mc_num_to_iob_num(device, fb, mc).unwrap();
        connection_callback("bufg_gsr", "BUFGSR", 0, 0,
            &format!("from_iob_{}", iob_idx), "I", 0);
    }
}
