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

module Tristate(a, b, c, d, dir);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// I/O declarations

	(* LOC = "P19" *)
	(* PULLDOWN = "10k" *)
	inout wire a;

	(* LOC = "P18" *)
	(* PULLDOWN = "10k" *)
	inout wire b;

	(* LOC = "P17" *)
	(* PULLDOWN = "10k" *)
	input wire d;

	(* LOC = "P16" *)
	output wire c;

	(* LOC = "P15" *)
	(* PULLDOWN = "10k" *)
	input wire dir;

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Tristate stuff

	assign a = dir ? b|d : 1'bz;
	assign b = ~dir ? ~a : 1'bz;
	assign c = dir ? a & b : 1'bz;

endmodule
