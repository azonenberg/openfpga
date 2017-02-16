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

module Location(a, b, c, d, e, f);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// I/O declarations

	(* LOC = "P20" *)
	(* PULLDOWN = "10k" *)
	input wire a;

	(* LOC = "P19" *)
	(* PULLDOWN = "10k" *)
	input wire b;

	(* LOC = "P18" *)
	(* PULLDOWN = "10k" *)
	output wire c;

	(* LOC = "P17" *)
	output wire d;

	(* LOC = "P16" *)
	input wire e;

	//LOC'd by constraint file to P15
	output wire f;

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Clock/reset stuff

	//The 1730 Hz oscillator
	wire clk_108hz;
	GP_LFOSC #(
		.PWRDN_EN(1),
		.AUTO_PWRDN(0),
		.OUT_DIV(16)
	) lfosc (
		.PWRDN(1'b0),
		.CLKOUT(clk_108hz)
	);

	//Power-on reset
	wire por_done;
	GP_POR #(
		.POR_TIME(500)
	) por (
		.RST_DONE(por_done)
	);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Counter to blink an LED

	localparam COUNT_MAX = 'd31;
	wire led_lfosc_raw;

	(* LOC = "COUNT8_ADV_4" *)
	GP_COUNT8 #(
		.RESET_MODE("LEVEL"),
		.COUNT_TO(COUNT_MAX),
		.CLKIN_DIVIDE(1)
	) lfosc_cnt (
		.CLK(clk_108hz),
		.RST(1'b0),
		.OUT(led_lfosc_raw)
	);

	//Toggle the output every time the counters underflow
	(* LOC = "DFF_5" *)
	reg led_out = 0;
	assign c = led_out;
	always @(posedge clk_108hz) begin

		//Gate toggle signals with POR to prevent glitches
		//caused by blocks resetting at different times during boot
		if(por_done) begin

			if(led_lfosc_raw)
				led_out <= ~led_out;

		end

	end

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// LUT to drive another output

	(* LOC = "LUT3_1" *)
	wire d_int = (a & b & e);
	assign d = d_int;

endmodule
