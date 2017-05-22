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
	INPUTS:
		FIXME

	OUTPUTS:
		FIXME

	TEST PROCEDURE:
		FIXME
 */
module DCMP(muxsel, greater, equal, count_overflow);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// I/O declarations

	(* LOC = "P20 P19" *)
	input wire[1:0] muxsel;

	(* LOC = "P18" *)
	output wire greater;

	(* LOC = "P17" *)
	output wire equal;

	(* LOC = "P16" *)
	output wire count_overflow;

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
	// RC oscillator clock

	wire clk_2mhz;
	GP_RCOSC #(
		.PWRDN_EN(0),
		.AUTO_PWRDN(0),
		.OSC_FREQ("2M"),
		.HARDIP_DIV(1),
		.FABRIC_DIV(1)
	) rcosc (
		.PWRDN(1'b0),
		.CLKOUT_HARDIP(clk_2mhz),
		.CLKOUT_FABRIC()
	);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Buffer the RC oscillator clock

	//This drives the MuxedClockBuffer for the ADC/DCMP
	wire clk_2mhz_buf;
	GP_CLKBUF clkbuf (
		.IN(clk_2mhz),
		.OUT(clk_2mhz_buf));

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// A counter driving the input of the DCMP/PWM

	localparam COUNT_MAX = 255;

	//Explicitly instantiated counter b/c we don't yet have inference support when using POUT
	wire[7:0] count_pout;
	GP_COUNT8 #(
		.CLKIN_DIVIDE(1),
		.COUNT_TO(COUNT_MAX),
		.RESET_MODE("RISING")
	) cnt (
		.CLK(clk_2mhz),
		.RST(1'b0),
		.OUT(count_overflow),
		.POUT(count_pout)
	);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Reference inputs to the DCMP

	wire[7:0] ref0;
	wire[7:0] ref1;
	wire[7:0] ref2;
	wire[7:0] ref3;

	GP_DCMPREF #(.REF_VAL(8'h80)) rs0(.OUT(ref0));
	GP_DCMPREF #(.REF_VAL(8'h40)) rs1(.OUT(ref1));
	GP_DCMPREF #(.REF_VAL(8'hc0)) rs2(.OUT(ref2));
	GP_DCMPREF #(.REF_VAL(8'hf0)) rs3(.OUT(ref3));

	wire[7:0] muxouta;
	wire[7:0] muxoutb;
	GP_DCMPMUX mux(
		.SEL(muxsel),
		.OUTA(muxouta),
		.OUTB(muxoutb),
		.IN0(ref0),
		.IN1(ref1),
		.IN2(ref2),
		.IN3(ref3));

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// The DCMP itself

	GP_DCMP #(
		.GREATER_OR_EQUAL(1'b0),
		.CLK_EDGE("RISING"),
		.PWRDN_SYNC(1'b1)
	) dcmp(
		.INP(muxouta),
		.INN(count_pout),
		.CLK(clk_2mhz_buf),
		.PWRDN(1'b0),
		.GREATER(greater),
		.EQUAL(equal)
	);

endmodule
