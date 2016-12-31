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
	@brief HiL test case for GP_INV, GP_LUTx

	OUTPUTS:
		Bitwise NAND of inputs (1 to 4 bits wide)
 */
module Luts(din, dout_instantiated, dout_inferred);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// I/O declarations

	(* LOC = "P5 P4 P3 P2" *)
	input wire[3:0] din;

	(* LOC = "P9 P8 P7 P6" *)
	output wire[3:0] dout_instantiated;

	(* LOC = "P15 P14 P13 P12" *)
	output wire[3:0] dout_inferred;

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// LUTs created from direct instantiation

	GP_INV inv_inst (
		.IN(din[0]), .OUT(dout_instantiated[0]));
	GP_2LUT #(.INIT(4'h7)) lut2_inst (
		.IN0(din[0]), .IN1(din[1]), .OUT(dout_instantiated[1]));
	GP_3LUT #(.INIT(8'h7F)) lut3_inst (
		.IN0(din[0]), .IN1(din[1]), .IN2(din[2]), .OUT(dout_instantiated[2]));
	GP_4LUT #(.INIT(16'h7FFF)) lut4_inst (
		.IN0(din[0]), .IN1(din[1]), .IN2(din[2]), .IN3(din[3]), .OUT(dout_instantiated[3]));

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// LUTs created from inference

	assign dout_inferred[0] = ~din[0];
	assign dout_inferred[1] = ~(din[0] & din[1]);
	assign dout_inferred[2] = ~(din[0] & din[1] & din[2]);
	assign dout_inferred[3] = ~(din[0] & din[1] & din[2] & din[3]);

endmodule
