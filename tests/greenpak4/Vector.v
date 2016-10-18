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
	OUTPUTS:
		2-bit status signal on pins 16, 15

	TEST PROCEDURE:
		Sweep all 16 values across pins 17-18-19-20.

		P16 should be P18 & P20.
		P15 should be P17 & P19.
 */
module Vector(a, b, c);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// I/O declarations

	(* LOC = "P20 P19" *)
	input wire[1:0] a;

	(* LOC = "P18 P17" *)
	input wire[1:0] b;

	(* LOC = "P16 P15" *)
	output wire[1:0] c;

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// The actual logic

	assign c = a & b;

endmodule
