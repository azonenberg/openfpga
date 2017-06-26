`default_nettype none
/***********************************************************************************************************************
 * Copyright (C) 2016-2017 Andrew Zonenberg and contributors                                                           *
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
module ConfigurableEdgeFlipflop(
	d, clk, ce, sr, srval, q, rising_en, falling_en
	);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// I/O declarations

	input wire	d;
	input wire	clk;
	input wire	ce;
	output wire q;
	input wire	sr;
	input wire	srval;

	input wire	rising_en;
	input wire	falling_en;

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// The actual FFs

	//Rising edge
	wire	rising_ff;
	FDCPE #(
		.INIT(1'b0)
	) rising (
		.Q(rising_ff),
		.C(clk),
		.CE(ce & rising_en),
		.CLR(sr && srval==0),
		.D(d ^ falling_ff),
		.PRE(sr && srval==1)
		);

	//Falling edge
	wire	falling_ff;
	FDCPE #(
		.INIT(1'b0)
	) falling (
		.Q(falling_ff),
		.C(!clk),
		.CE(ce & falling_en),
		.CLR(sr),	//always clear this FF to zero
		.D(d ^ rising_ff),
		.PRE(1'b0)
		);

	//XOR the outputs together
	assign q = rising_ff ^ falling_ff;

endmodule
