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
	mc_to_zia, mc_to_obuf,
	config_done_rst
	);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// I/O declarations

	input wire[26:0]			config_bits;

	input wire					pterm_a;
	input wire					pterm_b;
	input wire					pterm_c;

	input wire					or_term;
	input wire					config_done_rst;

	output reg					mc_to_zia;
	output reg					mc_to_obuf;

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
	// Macrocell flipflop

	reg							mc_dff = 1;

	//Flipflop reset
	always @(/*posedge config_done_rst*/*) begin
		mc_dff					<= !config_bits[0];
	end

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Final output muxing

	//TODO: mux for bit 15 (ibuf/mc flipflop)

	always @(*) begin

		//Enable for macrocell->ZIA driver (active low so we're powered down when device is blank)
		if(!config_bits[12]) begin

			//See where macrocell->ZIA driver should go
			if(config_bits[13])
				mc_to_zia				<= mc_dff;
			else
				mc_to_zia				<= xor_out;

		end

		//Output driver powered down, feed a constant zero into the ZIA to prevent spurious toggles
		else
			mc_to_zia					<= 1'b0;

		//Mux for pad driver
		if(config_bits[7])
			mc_to_obuf					<= xor_out;
		else
			mc_to_obuf					<= mc_dff;

	end

endmodule
