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
	@brief HiL test case for GP_RINGOSC

	OUTPUTS:
		Oscillator output, divided by 2 (should be ~13.5 MHz)
 */
module RingOsc(clkout);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// I/O declarations

	(* LOC = "P13" *)
	output wire clkout;

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// The oscillator

	GP_RINGOSC #(
		.PWRDN_EN(1),
		.AUTO_PWRDN(0),
		.HARDIP_DIV(1),
		.FABRIC_DIV(2)
	) ringosc (
		.PWRDN(1'b0),
		.CLKOUT_HARDIP(),
		.CLKOUT_FABRIC(clkout)
	);

endmodule
