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
		None

	OUTPUTS:
		The same signal echoed to all other GPIOs

	TEST PROCEDURE:
		Apply 3.3V power to pin 1 (core / bank 1 power)
		Apply 1.8V power to pin 14 (bank 2 power)
		Program the device
		Read pin 10.
			If not 3.3V, device is missing or faulty.
		Read pin 20.
			If 3.3V, device is a SLG46620.
			If 1.8V, device is a SLG46621.
			If anything else, device is missing or faulty.
 */
module PowerRailDetector_STQFN20(dout_bank1, dout_bank2);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// I/O declarations

	(* LOC = "P10" *)
	output wire dout_bank1 = 1;

	(* LOC = "P20" *)
	output wire dout_bank2 = 1;

endmodule
