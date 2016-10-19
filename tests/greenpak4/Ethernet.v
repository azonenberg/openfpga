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
	@brief Minimal 10baseT autonegotiation implementation
 */
module Ethernet(rst_done, clk_debug, txd, lcw);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// I/O declarations

	(* LOC = "P20" *)
	output reg rst_done = 0;

	(* LOC = "P19" *)
	output wire clk_debug;

	(* LOC = "P18" *)
	output wire txd;

	(* LOC = "P17" *)
	output wire lcw;

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Power-on-reset configuration

	wire por_done;
	GP_POR #(.POR_TIME(500)) por (.RST_DONE(por_done));

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Clock source - 2 MHz RC oscillator (500 ns per cycle)

	wire clk_hardip;
	wire clk_fabric;
	GP_RCOSC #(
		.PWRDN_EN(0),
		.AUTO_PWRDN(0),
		.OSC_FREQ("2M"),
		.PRE_DIV(1),
		.FABRIC_DIV(1)
	) rcosc (
		.PWRDN(1'b0),
		.CLKOUT_HARDIP(clk_hardip),
		.CLKOUT_FABRIC(clk_fabric)
	);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Edge detector for producing ~150 ns FLP/NLPs

	reg pulse_en = 0;
	GP_EDGEDET #(
		.DELAY_STEPS(1),
		.EDGE_DIRECTION("RISING"),
		.GLITCH_FILTER(0)
	) delay(
		.IN(pulse_en),
		.OUT(txd)
		);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Link codeword generation

	GP_PGEN #(
		.PATTERN_DATA(16'h55aa),
		.PATTERN_LEN(5'd16)
	) pgen (
		.nRST(1'b1),
		.CLK(clk_fabric),
		.OUT(lcw)
	);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// The actual logic

	always @(posedge clk_fabric) begin
		pulse_en <= ~pulse_en;
	end

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Debug outputs

	//Detect when the system reset has completed
	always @(posedge clk_fabric)
		rst_done <= 1;

	assign clk_debug = clk_fabric;

endmodule
