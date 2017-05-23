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
	OUTPUTS:
		Bandgap OK on pin 20 (should be high after reset)
		DAC output on pin 19 (sweeping TBD)
		DAC output on pin 18 (constant 1V)

	TEST PROCEDURE:
		TODO
 */
module Dac(bg_ok, vout, vout2, wave_sync);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// I/O declarations

	(* LOC = "P20" *)
	output wire bg_ok;

	(* LOC = "P18" *)
	(* IBUF_TYPE = "ANALOG" *)
	output wire vout;

	(* LOC = "P19" *)
	(* IBUF_TYPE = "ANALOG" *)
	output wire vout2;

	(* LOC = "P17" *)
	output wire wave_sync;

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// System reset stuff

	//Power-on reset
	wire por_done;
	GP_POR #(
		.POR_TIME(500)
	) por (
		.RST_DONE(por_done)
	);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Oscillators

	//The 1730 Hz oscillator
	wire clk_1730hz;
	GP_LFOSC #(
		.PWRDN_EN(0),
		.AUTO_PWRDN(0),
		.OUT_DIV(1)
	) lfosc (
		.PWRDN(1'b0),
		.CLKOUT(clk_1730hz)
	);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// 1.0V voltage reference for the DAC

	//Configure the bandgap with default timing etc
	GP_BANDGAP #(
		.AUTO_PWRDN(0),
		.CHOPPER_EN(1),
		.OUT_DELAY(550)
	) bandgap (
		.OK(bg_ok)
	);

	//1.0V reference for the DAC
	wire vref_1v0;
	GP_VREF #(
		.VIN_DIV(4'd1),
		.VREF(16'd1000)
	) vr1000 (
		.VIN(1'b0),
		.VOUT(vref_1v0)
	);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// A counter running off the 108 Hz clock

	localparam COUNT_MAX = 255;

	//Inferred counter that drives both the DAC and an overflow output
	reg[7:0] count = COUNT_MAX;
	always @(posedge clk_1730hz) begin
		if(count == 0)
			count <= COUNT_MAX;
		else
			count <= count - 1'd1;
	end

	//Counter overflow signal to LED
	assign wave_sync = (count == 0);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// The DACs

	//This one drives a top-level output pin directly (GP_VREF and GP_ACMP are infrred)
	(* LOC = "DAC_1" *)
	GP_DAC dac(
		.DIN(count),
		.VOUT(vout),
		.VREF(vref_1v0)
	);

	//This one drives a GP_VREF explicitly (GP_ACMP is inferred)
	wire vdac2;
	(* LOC = "DAC_0" *)
	GP_DAC dac2(
		.DIN(8'hff),
		.VOUT(vdac2),
		.VREF(vref_1v0)
	);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Drive the other via a buffered reference

	GP_VREF #(
		.VIN_DIV(4'd1),
		.VREF(16'd00)
	) vrdac (
		.VIN(vdac2),
		.VOUT(vout2)
	);


endmodule
