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
		pin 19: 500 kHz clock output
		pin 18: same clock signal, lagging phase by ~165 ns

	TEST PROCEDURE:
		1) Verify pin 19 has a clock waveform with 50% duty cycle and 500 KHz frequency
		2) Verify pin 18 has the same waveform, delayed by 150-175 ns
 */
module Delay(clk, clk_delayed);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// I/O declarations

	(* LOC = "P19" *)
	output wire clk;

	(* LOC = "P18" *)
	output wire clk_delayed;

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Oscillators

	//The 2 MHz RC oscillator
	GP_RCOSC #(
		.PWRDN_EN(0),
		.AUTO_PWRDN(0),
		.OSC_FREQ("2M"),
		.PRE_DIV(1),
		.FABRIC_DIV(4)
	) rcosc (
		.PWRDN(1'b0),
		.CLKOUT_HARDIP(),
		.CLKOUT_FABRIC(clk)
	);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// The delay line

	GP_DELAY #(.DELAY_STEPS(1)) delay(
		.IN(clk),
		.OUT(clk_delayed)
		);

endmodule
