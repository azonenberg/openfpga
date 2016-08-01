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
		PGA input on pin 8
	
	OUTPUTS:
		PGA output on pin 7
		
	TEST PROCEDURE:
		Sweep analog wavefrom from 0 to 500 mV on pin 8
		Output should be 2x the input voltage		
 */
module PGA(bg_ok, vin, pgaout);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// I/O declarations
	
	(* LOC = "P20" *)
	output wire bg_ok;

	(* LOC = "P8" *)
	(* IBUF_TYPE = "ANALOG" *)
	input wire vin;

	(* LOC = "P7" *)
	(* IBUF_TYPE = "ANALOG" *)
	output wire pgaout;
	
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
	// Programmable-gain analog amplifier
	
	GP_PGA #(
		.GAIN(2),
		.INPUT_MODE("SINGLE")
	) pga (
		.VIN_P(vin),
		.VIN_N(),
		.VIN_SEL(1'b1),
		.VOUT(pgaout)
	);
		
endmodule
