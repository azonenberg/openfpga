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
	dedicated_input, macrocell_io);

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

	inout wire[MACROCELLS-1:0]	macrocell_io;		//The actual device I/O pins.
													//Note that not all of these are broken out to bond pads;
													//buried macrocells drive a constant 0 here

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// The SRAM copy of the config bitstream (directly drives device behavior)

	reg[SHREG_WIDTH-1:0] ram_bitstream[MEM_DEPTH-1:0];

	/*
		Row configuration, left to right:
		1		259			Transfer bit (ignored)
		9		258:250		FB2 macrocells
		112		249:138		FB2 PLA
		16		137:122		ZIA (interleaved)
		112		121:10		FB1 PLA
		9		9:1			FB1 macrocells
		1		0			Transfer bit (ignored)
	 */

	integer i;
	initial begin
		for(i=0; i<MEM_DEPTH; i=i+1)
			ram_bitstream[i] <= {SHREG_WIDTH{1'b1}};	//copied from blank EEPROM = all 1s
	end

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// The EEPROM copy of the config bitstream (used to configure ram_bitstream at startup)

	//TODO

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// JTAG stuff

	wire					config_erase;

	wire					config_read_en;
	wire[ADDR_BITS-1:0]		config_read_addr;
	reg[SHREG_WIDTH-1:0]	config_read_data = 0;

	wire					config_write_en;
	wire[ADDR_BITS-1:0]		config_write_addr;
	wire[SHREG_WIDTH-1:0]	config_write_data;

	//Read/write the EEPROM
	//TODO: add read enable?
	always @(posedge jtag_tck) begin

		if(config_read_en)
			config_read_data <= ram_bitstream[config_read_addr];

		if(config_write_en)
			ram_bitstream[config_write_addr]	<= config_write_data;

		//Wipe the config memory
		//TODO: pipeline this or are we OK in one cycle?
		//If we go multicycle, how do we handle this with no clock? Real chip is self-timed internally
		if(config_erase) begin
			for(i=0; i<MEM_DEPTH; i=i+1)
				ram_bitstream[i] <= {SHREG_WIDTH{1'b1}};
		end

	end

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
	wire[31:0]		ibuf_to_zia = macrocell_io;

	//Left side (FB2)
	wire[39:0]		left_zia_out;
	reg[40*8-1:0]	left_zia_config;
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
	reg[40*8-1:0]	right_zia_config;
	XC2CZIA #(
		.MACROCELLS(MACROCELLS)
	) right_zia (
		.dedicated_in(dedicated_input),
		.ibuf_in(ibuf_to_zia),
		.macrocell_in(macrocell_to_zia),
		.zia_out(right_zia_out),
		.config_bits(right_zia_config)
		);

	//Hook up the config bits
	always @(*) begin
		for(i=0; i<40; i=i+1) begin

			//The ZIA is bits 137:122
			//MSB is FB1, next is FB2.
			//We have stuff at the top and bottom of array, with global config in the middle
			if(i > 20) begin
				right_zia_config[i*8 +: 8] <=
				{
					ram_bitstream[i-8][137],
					ram_bitstream[i-8][135],
					ram_bitstream[i-8][133],
					ram_bitstream[i-8][131],
					ram_bitstream[i-8][129],
					ram_bitstream[i-8][127],
					ram_bitstream[i-8][125],
					ram_bitstream[i-8][123]
				};

				left_zia_config[i*8 +: 8] <=
				{
					ram_bitstream[i-8][136],
					ram_bitstream[i-8][134],
					ram_bitstream[i-8][132],
					ram_bitstream[i-8][130],
					ram_bitstream[i-8][128],
					ram_bitstream[i-8][126],
					ram_bitstream[i-8][124],
					ram_bitstream[i-8][122]
				};
			end
			else begin
				right_zia_config[i*8 +: 8] <=
				{
					ram_bitstream[i][137],
					ram_bitstream[i][135],
					ram_bitstream[i][133],
					ram_bitstream[i][131],
					ram_bitstream[i][129],
					ram_bitstream[i][127],
					ram_bitstream[i][125],
					ram_bitstream[i][123]
				};

				left_zia_config[i*8 +: 8] <=
				{
					ram_bitstream[i][136],
					ram_bitstream[i][134],
					ram_bitstream[i][132],
					ram_bitstream[i][130],
					ram_bitstream[i][128],
					ram_bitstream[i][126],
					ram_bitstream[i][124],
					ram_bitstream[i][122]
				};
			end

		end
	end

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// The actual CPLD function blocks

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Debug stuff

	assign macrocell_io[6:3] = right_zia_out[7:4];

	assign macrocell_io[7] = 1'b0;

endmodule
