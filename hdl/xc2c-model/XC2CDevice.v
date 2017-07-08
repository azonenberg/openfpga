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
	dedicated_input, iob_out, iob_in, iob_t,
	done, dbgout
	);

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

	output wire					done;
	output wire					dbgout;

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

	wire[27*16-1:0]			left_mc_config;
	wire[27*16-1:0]			right_mc_config;

	wire[2:0]				global_ce;
	wire					global_sr_invert;
	wire					global_sr_en;
	wire[3:0]				global_tris_invert;
	wire[3:0]				global_tris_en;

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
		.left_mc_config(left_mc_config),
		.right_zia_config(right_zia_config),
		.right_and_config(right_and_config),
		.right_or_config(right_or_config),
		.right_mc_config(right_mc_config),

		.global_ce(global_ce),
		.global_sr_invert(global_sr_invert),
		.global_sr_en(global_sr_en),
		.global_tris_invert(global_tris_invert),
		.global_tris_en(global_tris_en),
	);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// The JTAG TAP (for now, basic bitstream config only)

	wire			config_done_rst;

	wire			config_erase_raw;
	BUFG bufg_reset(.I(config_erase_raw), .O(config_erase));

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

		.config_erase(config_erase_raw),

		.config_read_en(config_read_en),
		.config_read_addr(config_read_addr),
		.config_read_data(config_read_data),

		.config_write_en(config_write_en),
		.config_write_addr(config_write_addr),
		.config_write_data(config_write_data),

		.config_done(done),
		.config_done_rst(config_done_rst)
	);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Global routing

	wire[31:0]		macrocell_to_zia;
	wire[31:0]		ibuf_to_zia;

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
	// Global buffers

	//Clocks
	wire[2:0]		global_clk;
	BUFGCE bufg_gclk0(.I(iob_in[20]), .O(global_clk[0]), .CE(global_ce[0]));
	BUFGCE bufg_gclk1(.I(iob_in[21]), .O(global_clk[1]), .CE(global_ce[1]));
	BUFGCE bufg_gclk2(.I(iob_in[22]), .O(global_clk[2]), .CE(global_ce[2]));

	//Set/reset
	reg				global_sr_raw;
	always @(*) begin
		if(global_sr_invert)
			global_sr_raw	<= !iob_in[7];
		else
			global_sr_raw	<= iob_in[7];
	end
	wire			global_sr;
	BUFGCE bufg_gsr(.I(global_sr_raw), .O(global_sr), .CE(global_sr_en));

	//Global tristates
	reg[3:0]		global_tris;
	always @(*) begin

		if(!global_tris_en[3])
			global_tris[3]	<= 0;
		else if(global_tris_invert[3])
			global_tris[3]	<= !iob_in[5];
		else
			global_tris[3]	<= iob_in[5];

		if(!global_tris_en[2])
			global_tris[2]	<= 0;
		else if(global_tris_invert[2])
			global_tris[2]	<= !iob_in[6];
		else
			global_tris[2]	<= iob_in[6];

		if(!global_tris_en[1])
			global_tris[1]	<= 0;
		else if(global_tris_invert[1])
			global_tris[1]	<= !iob_in[3];
		else
			global_tris[1]	<= iob_in[3];

		if(!global_tris_en[0])
			global_tris[0]	<= 0;
		else if(global_tris_invert[0])
			global_tris[0]	<= !iob_in[4];
		else
			global_tris[0]	<= iob_in[4];

	end

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Macrocells

	wire			left_cterm_clk;
	wire			right_cterm_clk;
	BUFG bufg_left_ctc(.I(left_pterms[4]), .O(left_cterm_clk));
	BUFG bufg_right_ctc(.I(right_pterms[4]), .O(right_cterm_clk));

	//Global buffers
	wire			left_cterm_rst;
	wire			left_cterm_set;
	wire			left_cterm_oe = left_pterms[7];
	wire			right_cterm_rst;
	wire			right_cterm_set;
	wire			right_cterm_oe = right_pterms[7];

	BUFG bufg_left_ctr (.I(left_pterms[5]),  .O(left_cterm_rst));
	BUFG bufg_right_ctr(.I(right_pterms[5]), .O(right_cterm_rst));
	BUFG bufg_left_cts (.I(left_pterms[6]),  .O(left_cterm_set));
	BUFG bufg_right_cts(.I(right_pterms[6]), .O(right_cterm_set));

	genvar g;
	generate
		for(g=0; g<16; g=g+1) begin : mcells

			XC2CMacrocell left(
				.config_bits(left_mc_config[g*27 +: 27]),
				.pterm_a(left_pterms[g*3 + 8]),
				.pterm_b(left_pterms[g*3 + 9]),
				.pterm_c(left_pterms[g*3 + 10]),
				.or_term(left_orterms[g]),
				.raw_ibuf(iob_in[g + 16]),

				.cterm_clk(left_cterm_clk),
				.global_clk(global_clk),

				.global_sr(global_sr),
				.cterm_reset(left_cterm_rst),
				.cterm_set(left_cterm_set),
				.cterm_oe(left_cterm_oe),
				.global_tris(global_tris),

				.config_done_rst(config_done_rst),

				.mc_to_zia(macrocell_to_zia[g + 16]),
				.ibuf_to_zia(ibuf_to_zia[g + 16]),

				.mc_to_obuf(iob_out[g + 16]),
				.tristate_to_obuf(iob_t[g + 16])
			);

			XC2CMacrocell right(
				.config_bits(right_mc_config[g*27 +: 27]),
				.pterm_a(right_pterms[g*3 + 8]),
				.pterm_b(right_pterms[g*3 + 9]),
				.pterm_c(right_pterms[g*3 + 10]),
				.or_term(right_orterms[g]),
				.raw_ibuf(iob_in[g]),

				.cterm_clk(right_cterm_clk),
				.global_clk(global_clk),

				.global_sr(global_sr),
				.cterm_reset(right_cterm_rst),
				.cterm_set(right_cterm_set),
				.cterm_oe(right_cterm_oe),
				.global_tris(global_tris),

				.config_done_rst(config_done_rst),

				.mc_to_zia(macrocell_to_zia[g]),
				.ibuf_to_zia(ibuf_to_zia[g]),

				.mc_to_obuf(iob_out[g]),
				.tristate_to_obuf(iob_t[g])
			);

		end
	endgenerate

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Debug stuff

	//Helper to keep stuff from getting optimized out
	assign dbgout = ^macrocell_to_zia ^ ^iob_out ^ ^iob_t;

endmodule
