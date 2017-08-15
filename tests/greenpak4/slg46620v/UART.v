/***********************************************************************************************************************
 * Copyright (C) 2017 Andrew Zonenberg and contributors                                                                *
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

/**
	TODO
 */
module UART(txd, rxd);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// I/O declarations

	(* LOC = "P9" *)	//not yet used
	input wire rxd;

	(* LOC = "P7" *)
	output reg txd = 0;

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Clocking

	//The 2 MHz RC oscillator
	wire clk_250khz_cnt;			//dedicated output to hard IP only
	wire clk_250khz;				//general fabric output
	GP_RCOSC #(
		.PWRDN_EN(0),
		.AUTO_PWRDN(0),
		.OSC_FREQ("2M"),
		.HARDIP_DIV(8),
		.FABRIC_DIV(1)
	) rcosc (
		.PWRDN(1'b0),
		.CLKOUT_HARDIP(clk_250khz_cnt),
		.CLKOUT_FABRIC(clk_250khz)
	);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Baud rate generator

	localparam UART_BAUD	= 9600;
	localparam CLOCK_FREQ	= 250000;
	localparam UART_BDIV	= CLOCK_FREQ / UART_BAUD;
	localparam UART_BMAX	= UART_BDIV - 1;

	reg[7:0] baud_count		= UART_BMAX;
	wire	 baud_edge		= (baud_count == 0);

	reg		sending = 0;

	always @(posedge clk_250khz_cnt) begin

		baud_count		<= baud_count - 1'h1;
		if(baud_edge)
			baud_count	<= UART_BMAX;

	end

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Transmit counter

	localparam MESSAGE_MAX = 14'h3fff;

	reg[13:0]	message_count	= MESSAGE_MAX;
	wire 		message_edge	= (message_count == 0);

	always @(posedge clk_250khz_cnt) begin
		message_count		<= message_count - 1'h1;

		if(message_edge)
			message_count	<= MESSAGE_MAX;
	end

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Pattern generator for the actual message we're sending

	//Start bit is in LSB since that's what we output during reset.
	//High 6 bits of PGEN ignored, then bit-reversed payload
	//We need an extra stop bit so we don't glitch for one clock when the counter wraps
	//(before we know the byte is over).
	wire	txd_raw;
	GP_PGEN #(
		.PATTERN_LEN(5'd11),
		.PATTERN_DATA({6'h0, 8'h82, 1'h1, 1'h1})
	) tx_pgen (
		.nRST(!message_edge),
		.CLK(baud_edge),
		.OUT(txd_raw)
	);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Timer so we know how long the actual message lasts

	localparam BYTE_MAX = 8'd10;

	//Can't infer GP_COUNTx_ADV yet, so instantiate
	wire	byte_done;
	GP_COUNT8_ADV #(
		.CLKIN_DIVIDE(1),
		.COUNT_TO(BYTE_MAX),
		.RESET_MODE("LEVEL"),
		.RESET_VALUE("COUNT_TO")
	) byte_count (
		.CLK(clk_250khz_cnt),
		.RST(!sending),
		.UP(1'b0),
		.KEEP(!baud_edge),
		.OUT(byte_done),
		.POUT()
	);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Send logic

	reg		send_pending	= 0;
	always @(posedge clk_250khz) begin

		if(message_edge) begin
			send_pending	<= 1;
		end

		if(send_pending && baud_edge) begin
			send_pending	<= 0;
			sending			<= 1;
		end

		if(byte_done) begin
			sending		<= 0;
		end

	end

	always @(*) begin

		if(!sending || byte_done)
			txd			<= 1;
		else
			txd			<= txd_raw;
	end

endmodule
