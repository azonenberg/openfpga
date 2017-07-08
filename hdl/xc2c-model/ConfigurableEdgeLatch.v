`default_nettype none
/***********************************************************************************************************************
 * Copyright (C) 2016-2017 Andrew Zonenberg and contributors                                                           *
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
module ConfigurableEdgeLatch(
	d, clk, sr, srval, q, pass_high
	);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// I/O declarations

	input wire	d;
	input wire	clk;
	output wire q;
	input wire	sr;
	input wire	srval;

	input wire	pass_high;

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// The actual latches

	//Transparent when high
	reg			pass_when_high;
	always @(*) begin

		//Pass data when high
		if(clk)
			pass_when_high				<= d;

		//Async set/reset
		if(sr)
			pass_when_high				<= srval;

	end

	//Transparent when low
	reg			pass_when_low;
	always @(*) begin

		//Pass data when high
		if(!clk)
			pass_when_low				<= d;

		//Async set/reset
		if(sr)
			pass_when_low				<= srval;

	end

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Output mux

	assign q = pass_high ? pass_when_high : pass_when_low;

endmodule
