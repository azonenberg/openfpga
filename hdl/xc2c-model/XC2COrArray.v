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
module XC2COrArray(pterms_in, config_bits, or_out);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// No configuration, all OR arrays are the same.
	// Differences in bitstream ordering, if any, are handled by XC2CDevice

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// I/Os

	input wire[55:0]			pterms_in;
	input wire[16*56 - 1 : 0]	config_bits;
	output reg[15:0]			or_out;

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Shuffle the config bits back to their proper 2D form

	integer nout;
	integer nterm;

	reg[55:0] or_config[15:0];
	always @(*) begin
		for(nterm=0; nterm<56; nterm=nterm+1) begin
			for(nout=0; nout<16; nout=nout + 1)
				or_config[nout][nterm]		<= config_bits[nout*56 + nterm];
		end
	end

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// The actual OR array

	always @(*) begin
		for(nout=0; nout<16; nout = nout+1) begin
			or_out[nout] = 0;		//default if no terms selected

			//OR in the PLA outputs
			for(nterm=0; nterm<56; nterm=nterm+1) begin
				if(!or_config[nout][nterm])
					or_out[nout] = or_out[nout] | pterms_in[nterm];
			end
		end
	end

endmodule
