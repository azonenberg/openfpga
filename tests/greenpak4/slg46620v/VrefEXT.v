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
		FIXME Vdd/4 on pin 19
		FIXME Vdd/3 on pin 18

	TEST PROCEDURE:
		FIXME
 */
module VrefEXT(vref_a, vref_b, vref_adiv2, vref_bdiv2);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// I/O declarations

	(* LOC = "P14" *)
	(* IBUF_TYPE = "ANALOG" *)
	input wire vref_b;

	(* LOC = "P10" *)
	(* IBUF_TYPE = "ANALOG" *)
	input wire vref_a;

	(* LOC = "P19" *)
	(* IBUF_TYPE = "ANALOG" *)
	output wire vref_adiv2;

	(* LOC = "P18" *)
	(* IBUF_TYPE = "ANALOG" *)
	output wire vref_bdiv2;

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
	// 1.0V bandgap voltage reference (used by a lot of the mixed signal IP)

	wire bg_ok;
	GP_BANDGAP #(
		.AUTO_PWRDN(0),
		.CHOPPER_EN(1),
		.OUT_DELAY(550)
	) bandgap (
		.OK(bg_ok)
	);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Voltage references for external voltages divided by 2

	GP_VREF #( .VIN_DIV(4'd2), .VREF(16'd0)  ) adiv2  ( .VIN(vref_a), .VOUT(vref_adiv2)  );
	GP_VREF #( .VIN_DIV(4'd2), .VREF(16'd0)  ) bdiv2  ( .VIN(vref_b), .VOUT(vref_bdiv2)  );

endmodule
