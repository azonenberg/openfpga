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
		TODO

	TEST PROCEDURE:
		TODO
 */
module Counter(rst, dout);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// I/O declarations

	(* LOC = "P20" *)
	output wire dout;

	(* LOC = "P18" *)
	input wire rst;

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Oscillators

	//The 25 kHz RC oscillator
	wire clk_6khz_cnt;			//dedicated output to hard IP only
	wire clk_6khz;				//general fabric output
	GP_RCOSC #(
		.PWRDN_EN(0),
		.AUTO_PWRDN(0),
		.OSC_FREQ("25k"),
		.HARDIP_DIV(4),
		.FABRIC_DIV(1)
	) rcosc (
		.PWRDN(1'b0),
		.CLKOUT_HARDIP(clk_6khz_cnt),
		.CLKOUT_FABRIC(clk_6khz)
	);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// A counter with a pre-divider of 12

	GP_COUNT8 #(
		.RESET_MODE("LEVEL"),
		.COUNT_TO(253),
		.CLKIN_DIVIDE(1)
	) cnt (
		.CLK(clk_6khz_cnt),
		.RST(rst),
		.OUT(dout)
	);

endmodule
