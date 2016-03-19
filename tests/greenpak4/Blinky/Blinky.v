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
 
module Blinky(a, b, o, a_copy);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// I/O declarations
	
	(* SCHMITT_TRIGGER *)
	(* PULLDOWN = "10k" *)
	(* LOC = "P20" *)
	input wire a;
	
	(* SCHMITT_TRIGGER *)
	(* PULLDOWN = "10k" *)
	(* LOC = "P19" *)
	input wire b;
	
	(* LOC = "P18" *)
	output wire o;
	
	(* LOC = "P3" *)
	output wire a_copy;

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Internal logic
	
	assign o = a | b;
	
	assign a_copy = a;

endmodule
