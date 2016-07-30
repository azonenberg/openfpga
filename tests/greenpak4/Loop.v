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

module Loop(a, b);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// I/O declarations

	(* LOC = "P4" *)
	output wire a;

	(* LOC = "P6" *)
	output wire b;

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Logic loops

	wire x0;
	GP_INV inv(.IN(x0), .OUT(x0));
	assign a = x0;

	wire y0, y1, y2;
	GP_2LUT #(.INIT(4'b0010)) lut_inv0(.IN0(y0), .OUT(y1));
	GP_2LUT #(.INIT(4'b0010)) lut_inv1(.IN0(y1), .OUT(y2));
	GP_2LUT #(.INIT(4'b0010)) lut_inv2(.IN0(y2), .OUT(y0));
	assign b = y0;

endmodule
