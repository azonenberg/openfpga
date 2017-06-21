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
module XC2CMacrocell(
	config_bits,
	pterm_a, pterm_b, pterm_c,
	or_term,
	mc_out
	);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// I/O declarations

	input wire[26:0]			config_bits;

	input wire					pterm_a;
	input wire					pterm_b;
	input wire					pterm_c;

	input wire					or_term;

	output reg					mc_out;

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Macrocell XOR

	reg							xor_out;

	always @(*) begin

		case(config_bits[9:8])
			2'h0:	xor_out		<= or_term ^ 1'b0;
			2'h1:	xor_out		<= or_term ^ ~pterm_c;
			2'h2:	xor_out		<= or_term ^ pterm_c;
			2'h3:	xor_out		<= or_term ^ 1'b1;
		endcase
	end

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Final output muxing

	always @(*) begin
		mc_out					<= xor_out;
	end

endmodule
