/***********************************************************************************************************************
 * Copyright (C) 2017 Andrew Zonenberg and contributors                                                                *
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
	TODO
 */
module Adder(din_a, din_b, din_c, dout/*, xorin, xorout*/);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// I/O declarations

	(* LOC = "P20 P2 P7 P14" *)
	input wire[3:0] din_a;

	(* LOC = "P3 P19 P6 P15" *)
	input wire[3:0] din_b;

	(* LOC = "P13 P8 P10 P16" *)
	input wire[3:0] din_c;

	(* LOC = "P9 P4 P5 P12" *)
	output wire[3:0] dout;

	/*
	(* LOC = "P13 P8" *)
	input wire[1:0] xorin;

	(* LOC = "P10" *)
	output wire xorout;
	*/

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// The adder

	assign dout = din_a + din_b/* + din_c*/;
	//assign xorout = ^xorin;

endmodule
