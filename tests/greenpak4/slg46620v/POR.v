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
		Pin 8: Power-on reset flag
		Pin 9: Always high
		Pin 10: Power-on reset flag

	TEST PROCEDURE:
		Verify pins 8, 9, and 10 all go high after the device is programmed.

		TODO: is there a good way to characterize timing?
 */
module POR(rst_out1, one, rst_out2);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// I/O declarations

	(* LOC = "P8" *)
	output wire rst_out1;

	(* LOC = "P9" *)
	output wire one;

	(* LOC = "P10" *)
	output wire rst_out2;

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Reset logic

	//Power-on reset
	wire por_done;
	GP_POR #(
		.POR_TIME(500)
	) por (
		.RST_DONE(por_done)
	);

	assign one = 1'b1;
	assign rst_out1 = por_done;
	assign rst_out2 = por_done;

endmodule
