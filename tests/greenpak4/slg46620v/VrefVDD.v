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
		Bandgap-OK on pin 20
		Vdd/4 on pin 19
		Vdd/3 on pin 18
		Brownout-detect on pin 17

	TEST PROCEDURE:
		FIXME
 */
module VrefVDD(bg_ok, vref_vdiv4, vref_vdiv3, vdd_low);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// I/O declarations

	(* LOC = "P20" *)
	output wire bg_ok;

	(* LOC = "P19" *)
	(* IBUF_TYPE = "ANALOG" *)
	output wire vref_vdiv4;

	(* LOC = "P18" *)
	(* IBUF_TYPE = "ANALOG" *)
	output wire vref_vdiv3;

	(* LOC = "P17" *)
	output wire vdd_low;

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

	GP_BANDGAP #(
		.AUTO_PWRDN(0),
		.CHOPPER_EN(1),
		.OUT_DELAY(550)
	) bandgap (
		.OK(bg_ok)
	);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Voltage references for Vdd/3 and Vdd/4

	GP_VREF #( .VIN_DIV(4'd4), .VREF(16'd0)  ) vdiv4  ( .VIN(1'b1), .VOUT(vref_vdiv4)  );
	GP_VREF #( .VIN_DIV(4'd3), .VREF(16'd0)  ) vdiv3  ( .VIN(1'b1), .VOUT(vref_vdiv3)  );

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Brownout detector

	GP_PWRDET pwrdet(.VDD_LOW(vdd_low));

endmodule
