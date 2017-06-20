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

/**
	@brief Top level module for an XC2C-series device
 */
module XC2CDevice(
	jtag_tdi, jtag_tms, jtag_tck, jtag_tdo,
	dedicated_input, iob_out, iob_in, iob_t);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Device configuration

	parameter MACROCELLS = 32;		//A variant implied for 32/64, no support for base version

	parameter PACKAGE = "QFG32";	//Package code (lead-free G assumed)

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Helpers for figuring out dimensions etc of the device

	function integer ConfigMemoryWidth;
		input[15:0] cells;
		begin
			case(cells)
				32:			ConfigMemoryWidth = 260;
				64:			ConfigMemoryWidth = 274;
				128:		ConfigMemoryWidth = 752;
				256:		ConfigMemoryWidth = 1364;
				384:		ConfigMemoryWidth = 1868;
				512:		ConfigMemoryWidth = 1980;
				default:	ConfigMemoryWidth = 0;
			endcase
		end
	endfunction

	function integer ConfigMemoryDepth;
		input[15:0] cells;
		begin
			case(cells)
				32:			ConfigMemoryDepth = 49;
				64:			ConfigMemoryDepth = 97;
				128:		ConfigMemoryDepth = 81;
				256:		ConfigMemoryDepth = 97;
				384:		ConfigMemoryDepth = 121;
				512:		ConfigMemoryDepth = 161;
				default:	ConfigMemoryDepth = 0;
			endcase
		end
	endfunction

	function integer ConfigMemoryAbits;
		input[15:0] cells;
		begin
			case(cells)
				32:			ConfigMemoryAbits = 6;
				64:			ConfigMemoryAbits = 7;
				128:		ConfigMemoryAbits = 7;
				256:		ConfigMemoryAbits = 7;
				384:		ConfigMemoryAbits = 7;
				512:		ConfigMemoryAbits = 8;
				default:	ConfigMemoryAbits = 0;
			endcase
		end
	endfunction

	localparam SHREG_WIDTH	= ConfigMemoryWidth(MACROCELLS);
	localparam MEM_DEPTH	= ConfigMemoryDepth(MACROCELLS);
	localparam ADDR_BITS	= ConfigMemoryAbits(MACROCELLS);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Sanity checks

	initial begin
		if( (MACROCELLS != 32) &&
			(MACROCELLS != 64) &&
			(MACROCELLS != 128) &&
			(MACROCELLS != 256) &&
			(MACROCELLS != 384) &&
			(MACROCELLS != 512) ) begin
			$display("Invalid macrocell count");
			$finish;
		end
	end

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// I/Os

	input wire					jtag_tdi;
	input wire					jtag_tms;
	input wire					jtag_tck;
	output wire					jtag_tdo;

	input wire					dedicated_input;	//only present in 32a

	output wire[MACROCELLS-1:0]	iob_out;			//The actual device I/O pins.
													//Note that not all of these are broken out to bond pads;
													//buried macrocells drive a constant 0 here
	output wire[MACROCELLS-1:0]	iob_t;
	input wire[MACROCELLS-1:0]	iob_in;

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// The bitstream

	wire					config_erase;

	wire					config_read_en;
	wire[ADDR_BITS-1:0]		config_read_addr;
	wire[SHREG_WIDTH-1:0]	config_read_data;

	wire					config_write_en;
	wire[ADDR_BITS-1:0]		config_write_addr;
	wire[SHREG_WIDTH-1:0]	config_write_data;

	wire[40*8-1:0]			left_zia_config;
	wire[40*8-1:0]			right_zia_config;

	wire[80*56-1:0]			left_and_config;
	wire[80*56-1:0]			right_and_config;

	wire[16*56-1:0]			left_or_config;
	wire[16*56-1:0]			right_or_config;

	XC2CBitstream #(
		.ADDR_BITS(ADDR_BITS),
		.MEM_DEPTH(MEM_DEPTH),
		.SHREG_WIDTH(SHREG_WIDTH)
	) bitstream (
		.jtag_tck(jtag_tck),

		.config_erase(config_erase),

		.config_read_en(config_read_en),
		.config_read_addr(config_read_addr),
		.config_read_data(config_read_data),

		.config_write_en(config_write_en),
		.config_write_addr(config_write_addr),
		.config_write_data(config_write_data),

		.left_zia_config(left_zia_config),
		.left_and_config(left_and_config),
		.left_or_config(left_or_config),
		.right_zia_config(right_zia_config),
		.right_and_config(right_and_config),
		.right_or_config(right_or_config)
	);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// The JTAG TAP (for now, basic bitstream config only)

	XC2CJTAG #(
		.MACROCELLS(MACROCELLS),
		.PACKAGE(PACKAGE),
		.SHREG_WIDTH(SHREG_WIDTH),
		.ADDR_BITS(ADDR_BITS)
	) jtag (
		.tdi(jtag_tdi),
		.tdo(jtag_tdo),
		.tms(jtag_tms),
		.tck(jtag_tck),

		.config_erase(config_erase),

		.config_read_en(config_read_en),
		.config_read_addr(config_read_addr),
		.config_read_data(config_read_data),

		.config_write_en(config_write_en),
		.config_write_addr(config_write_addr),
		.config_write_data(config_write_data)
	);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Global routing

	//TODO: input buffers and muxes for these based on InZ etc
	wire[31:0]		macrocell_to_zia = 32'h0;
	wire[31:0]		ibuf_to_zia = iob_in;

	//Left side (FB2)
	wire[39:0]		left_zia_out;
	XC2CZIA #(
		.MACROCELLS(MACROCELLS)
	) left_zia (
		.dedicated_in(dedicated_input),
		.ibuf_in(ibuf_to_zia),
		.macrocell_in(macrocell_to_zia),
		.zia_out(left_zia_out),
		.config_bits(left_zia_config)
		);

	//Right side (FB1)
	wire[39:0]		right_zia_out;
	XC2CZIA #(
		.MACROCELLS(MACROCELLS)
	) right_zia (
		.dedicated_in(dedicated_input),
		.ibuf_in(ibuf_to_zia),
		.macrocell_in(macrocell_to_zia),
		.zia_out(right_zia_out),
		.config_bits(right_zia_config)
		);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// PLA AND array

	wire[55:0]			left_pterms;
	wire[55:0]			right_pterms;

	//FB2
	XC2CAndArray left_pla_and(
		.zia_in(left_zia_out),
		.config_bits(left_and_config),
		.pterm_out(left_pterms)
	);

	//FB1
	XC2CAndArray right_pla_and(
		.zia_in(right_zia_out),
		.config_bits(right_and_config),
		.pterm_out(right_pterms)
	);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// PLA OR array

	wire[15:0]		left_orterms;
	wire[15:0]		right_orterms;

	//FB2
	XC2COrArray left_pla_or(
		.pterms_in(left_pterms),
		.config_bits(left_or_config),
		.or_out(left_orterms)
	);

	//FB1
	XC2COrArray right_pla_or(
		.pterms_in(right_pterms),
		.config_bits(right_or_config),
		.or_out(right_orterms)
	);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Debug stuff

	//Tristate all pins except our outputs (6:3)
	assign iob_t[31:7] = 25'h1ffffff;
	assign iob_t[6:3] = 4'h0;
	assign iob_t[2:0] = 3'h7;

	//Drive all unused outputs to 0, then hook up our outputs
	//Should be X, !X, X, X
	assign iob_out[31:7] = 25'h0;
	//assign iob_out[6:3] = {right_pterms[19], right_pterms[22], right_pterms[25], right_pterms[28]};
	assign iob_out[6] = ^right_pterms;
	assign iob_out[5] = ^left_pterms;
	assign iob_out[4] = ^right_orterms;
	assign iob_out[3] = ^left_orterms;
	assign iob_out[2:0] = 3'h0;

endmodule
