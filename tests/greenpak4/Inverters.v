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
	
	GP_INV inva(.IN(i), .OUT(a));
	GP_INV invb(.IN(a), .OUT(b));
	GP_INV invc(.IN(b), .OUT(c));
	GP_INV invd(.IN(c), .OUT(d));
	GP_INV inve(.IN(d), .OUT(e));
	GP_INV invf(.IN(e), .OUT(f));
	GP_INV invg(.IN(f), .OUT(g));
	GP_INV invh(.IN(g), .OUT(h));
	
endmodule
