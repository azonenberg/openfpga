/***********************************************************************************************************************
 * Copyright (C) 2016 Andrew Zonenberg and contributors                                                                *
 *                                                                                                                     *
 * This program is free software; you can redistribute it and/or modify it under the terms of the GNU Lesser General   *
 * Public License as published by the Free Software Foundation; either version 2.1 of the License, or (at your option) *
 * any later version.                                                                                                  *
 *                                                                                                                     *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied  *
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for     *
 * more details.                                                                                                       *
 *                                                                                                                     *
 * You should have received a copy of the GNU Lesser General Public License along with this program; if not, you may   *
 * find one here:                                                                                                      *
 * https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt                                                              *
 * or you may search the http://www.gnu.org website for the version 2.1 license, or you may write to the Free Software *
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA                                      *
 **********************************************************************************************************************/

`default_nettype none

/**
	INPUTS:
		Test input on pin 2

	OUTPUTS:
		The same signal echoed to all other GPIOs

	TEST PROCEDURE:
		Send arbitrary digital waveform into pin 2
		Expect the same waveform echoed from all other pins

		Note that this will detect open circuit or stuck-at faults in the socket, but not shorts between pins.
 */
module SocketTestLoopback_STQFN14(din, dout);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// I/O declarations

	(* LOC = "P2" *)
	input wire din;

	(* LOC = "P14 P13 P12 P11 P10 P9 P7 P6 P5 P4 P3" *)
	output wire[10:0] dout;

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// The actual logic

	//... if you can even call it that!
	assign dout = {11{din}};

endmodule
