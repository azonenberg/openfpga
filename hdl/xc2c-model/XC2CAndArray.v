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
module XC2CAndArray(zia_in, config_bits, pterm_out);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// No configuration, all AND arrays are the same.
	// Differences in bitstream ordering, if any, are handled by XC2CDevice

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// I/Os

	input wire[39:0]			zia_in;
	input wire[80*56 - 1 : 0]	config_bits;
	output reg[55:0]			pterm_out;

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Shuffle the config bits back to their proper 2D form

	integer nterm;
	integer nrow;
	integer nin;

	reg[79:0] and_config[55:0];
	always @(*) begin
		for(nterm=0; nterm<56; nterm=nterm+1) begin
			for(nin=0; nin<80; nin=nin + 1)
				and_config[nterm][nin]		<= config_bits[nterm*80 + nin];
		end
	end

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// The actual AND array

	//Higher value is !X, lower is X
	always @(*) begin
		for(nterm=0; nterm<56; nterm = nterm+1) begin
			pterm_out[nterm] = 1;		//default if no terms selected

			//AND in the ZIA stuff
			for(nrow=0; nrow<40; nrow=nrow+1) begin
				if(!and_config[nterm][nrow*2])
					pterm_out[nterm] = pterm_out[nterm] & zia_in[nrow];
				if(!and_config[nterm][nrow*2 + 1])
					pterm_out[nterm] = pterm_out[nterm] & ~zia_in[nrow];
			end
		end
	end

endmodule
