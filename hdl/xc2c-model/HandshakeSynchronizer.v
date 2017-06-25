`timescale 1ns / 1ps
/***********************************************************************************************************************
*                                                                                                                      *
* ANTIKERNEL v0.1                                                                                                      *
*                                                                                                                      *
* Copyright (c) 2012-2017 Andrew D. Zonenberg                                                                          *
* All rights reserved.                                                                                                 *
*                                                                                                                      *
* Redistribution and use in source and binary forms, with or without modification, are permitted provided that the     *
* following conditions are met:                                                                                        *
*                                                                                                                      *
*    * Redistributions of source code must retain the above copyright notice, this list of conditions, and the         *
*      following disclaimer.                                                                                           *
*                                                                                                                      *
*    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the       *
*      following disclaimer in the documentation and/or other materials provided with the distribution.                *
*                                                                                                                      *
*    * Neither the name of the author nor the names of any contributors may be used to endorse or promote products     *
*      derived from this software without specific prior written permission.                                           *
*                                                                                                                      *
* THIS SOFTWARE IS PROVIDED BY THE AUTHORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED   *
* TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL *
* THE AUTHORS BE HELD LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES        *
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR       *
* BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT *
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE       *
* POSSIBILITY OF SUCH DAMAGE.                                                                                          *
*                                                                                                                      *
***********************************************************************************************************************/

/**
	@file
	@author Andrew D. Zonenberg
	@brief Handshake synchronizer for sharing a data buffer across two clock domains

	This module arbitrates a unidirectional transfer of data from port A to port B through an external buffer.

	To send data (port A)
		Load data into the buffer
		Assert en_a for one cycle
			busy_a goes high
		Wait for ack_a to go high
		The buffer can now be written to again

	To receive data (port B)
		Wait for en_b to go high
		Read data
		Assert ack_b for one cycle
		Data buffer is now considered invalid, do not touch
 */
module HandshakeSynchronizer(
	input wire clk_a,
	input wire en_a,
	output reg ack_a = 0,
	output wire busy_a,
	
	input wire clk_b,
	output reg en_b = 0,
	input wire ack_b
	);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Flag synchronizers

	reg tx_en_in = 0;
	wire tx_en_out;

	reg tx_ack_in = 0;
	wire tx_ack_out;

	ThreeStageSynchronizer sync_tx_en
		(.clk_in(clk_a), .din(tx_en_in), .clk_out(clk_b), .dout(tx_en_out));
	ThreeStageSynchronizer sync_tx_ack
		(.clk_in(clk_b), .din(tx_ack_in), .clk_out(clk_a), .dout(tx_ack_out));

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Control logic, transmit side

	reg[1:0] state_a = 0;

	assign busy_a = state_a[1] || en_a;

	always @(posedge clk_a) begin

		ack_a <= 0;

		case(state_a)

			//Idle - sit around and wait for input to arrive
			0: begin
				if(en_a) begin
					tx_en_in <= 1;
					state_a <= 1;
				end
			end

			//Data sent, wait for it to be acknowledged
			1: begin
				if(tx_ack_out) begin
					tx_en_in <= 0;
					state_a <= 2;
				end
			end

			//When acknowledge flag goes low the receiver is done with the data.
			//Tell the sender and we're done.
			2: begin
				if(!tx_ack_out) begin
					ack_a <= 1;
					state_a <= 0;
				end
			end

		endcase

	end

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Control logic, receive side

	reg[1:0] state_b = 0;
	always @(posedge clk_b) begin

		en_b <= 0;

		case(state_b)

			//Idle - sit around and wait for input to arrive
			0: begin
				if(tx_en_out) begin
					en_b <= 1;
					state_b <= 1;
				end
			end

			//Data received, wait for caller to process it
			1: begin
				if(ack_b) begin
					tx_ack_in <= 1;
					state_b <= 2;
				end
			end

			//When transmit flag goes low the sender knows we're done
			2: begin
				if(!tx_en_out) begin
					state_b <= 0;
					tx_ack_in <= 0;
				end
			end

		endcase

	end

endmodule
