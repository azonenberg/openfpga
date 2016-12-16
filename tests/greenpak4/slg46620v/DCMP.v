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
	INPUTS:
		FIXME

	OUTPUTS:
		FIXME

	TEST PROCEDURE:
		FIXME
 */
module DCMP(muxsel, outp, outn);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// I/O declarations

	(* LOC = "P20 P19" *)
	input wire[1:0] muxsel;

	(* LOC = "P18" *)
	output wire outp;

	(* LOC = "P17" *)
	output wire outn;

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
	// Reference inputs to the DCMP

	wire[7:0] ref0;
	wire[7:0] ref1;
	wire[7:0] ref2;
	wire[7:0] ref3;

	GP_DCMPREF #(.REF_VAL(8'h40)) rs0(.OUT(ref0));
	GP_DCMPREF #(.REF_VAL(8'h80)) rs1(.OUT(ref1));
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

	GP_DCMP dcmp(
		.INP(muxouta),
		.INN(ref0),
		.CLK(),
		.PWRDN(1'b0),
		.OUTP(outp),
		.OUTN(outn)
		);

endmodule
