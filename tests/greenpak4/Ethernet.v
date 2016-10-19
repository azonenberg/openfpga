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
	@brief Minimal 10baseT autonegotiation
 */
module Ethernet(rst_done, clk_debug, txd);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// I/O declarations

	(* LOC = "P20" *)
	output reg rst_done = 0;

	(* LOC = "P19" *)
	output wire clk_debug;

	(* LOC = "P18" *)
	output wire txd;

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// System reset stuff

	//Power-on reset flag
	wire por_done;
	GP_POR #(.POR_TIME(500)) por (.RST_DONE(por_done));

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Clock source - 2 MHz RC oscillator

	//500 ns per cycle

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
	// The actual logic

	reg pulse_en = 0;

	always @(posedge clk_fabric) begin
		pulse_en <= ~pulse_en;
	end

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Delay line + LUT for generating 125 ns pulses

	//TODO: just use an edge detector once that's implemented?

	wire pulse_en_delayed;
	GP_DELAY #(
		.DELAY_STEPS(1)
	) delay(
		.IN(pulse_en),
		.OUT(pulse_en_delayed)
		);

	wire pulse_out = pulse_en_delayed && !pulse_en;

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Debug outputs

	//Detect when the system reset has completed
	always @(posedge clk_fabric)
		rst_done <= 1;	

	assign clk_debug = clk_fabric;
	assign txd = pulse_out;

endmodule
