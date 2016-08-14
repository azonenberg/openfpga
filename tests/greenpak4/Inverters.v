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

module Inverters(i, a, b, c, d, e, f, g, h);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// I/O declarations

	(* LOC = "P3" *)
	input wire i;

	(* LOC = "P12" *)
	output wire a;
	
	(* LOC = "P13" *)
	output wire b;
	
	(* LOC = "P14" *)
	output wire c;
	
	(* LOC = "P15" *)
	output wire d;
	
	(* LOC = "P16" *)
	output wire e;
	
	(* LOC = "P17" *)
	output wire f;
	
	(* LOC = "P18" *)
	output wire g;
	
	(* LOC = "P19" *)
	output wire h;

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Lots of inverters. Explicitly instantiate to prevent Yosys from optimizing them out
		
	wire[8:0] q;
	assign q[0] = i;
	assign a = q[1];
	assign b = q[2];
	assign c = q[3];
	assign d = q[4];
	assign e = q[5];
	assign f = q[6];
	assign g = q[7];
	assign h = q[8];
	
	genvar j;
	generate
		for(j=0; j<8; j=j+1)
			GP_INV inv(.IN(q[j]), .OUT(q[j+1]));
	endgenerate

endmodule
