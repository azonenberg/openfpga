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
	cterm_clk, global_clk,
	mc_to_zia, ibuf_to_zia, mc_to_obuf,
	tristate_to_obuf,
	raw_ibuf,
	config_done_rst, global_sr, cterm_reset, cterm_set, cterm_oe, global_tris
	);

	/*
		UNIMPLEMENTED FEATURES (WONTFIX, the emulator doesn't have a pad ring so these make no sense)
		* Config bit 10: Schmitt trigger on input
		* Config bit 2: Termination
		* Config bit 1: Slew rate
	 */

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// I/O declarations

	input wire[26:0]	config_bits;

	input wire			pterm_a;
	input wire			pterm_b;
	input wire			pterm_c;

	input wire			or_term;

	input wire			cterm_clk;
	input wire[2:0]		global_clk;

	output reg			mc_to_zia;
	output reg			mc_to_obuf;
	output reg			ibuf_to_zia;

	output reg			tristate_to_obuf;

	input wire			raw_ibuf;

	input wire			config_done_rst;
	input wire			global_sr;
	input wire			cterm_reset;
	input wire			cterm_set;
	input wire			cterm_oe;
	input wire[3:0]		global_tris;

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
	// FF input muxing

	reg		dff_imux;

	reg		dff_in;
	always @(*) begin

		//Normal input source mux
		if(config_bits[11])
			dff_imux		<= xor_out;
		else
			dff_imux		<= raw_ibuf;

		//T flipflop has inverting feedback path
		if(config_bits[17:16] == 2) begin

			//Toggling?
			if(dff_imux)
				dff_in		<= ~mc_dff_muxed;

			//Nope, preserve it
			else
				dff_in		<= mc_dff_muxed;

		end

		//D flipflop just takes data from input
		else
			dff_in			<= dff_imux;


	end

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// FF clock enable

	reg		dff_ce;

	always @(*) begin

		//We have a clock enable
		if(config_bits[17:16] == 3)
			dff_ce			<= pterm_c;

		//No clock enable, always on
		else
			dff_ce			<= 1'b1;

	end

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// FF set/reset logic

	reg		dff_sr;
	reg		dff_srval;

	always @(*) begin

		//Power-on reset
		if(config_done_rst) begin
			dff_sr		<= 1;
			dff_srval	<= !config_bits[0];
		end

		//DFF reset
		else if(
			( (config_bits[21:20] == 2'b00) && pterm_a ) ||
			( (config_bits[21:20] == 2'b01) && global_sr ) ||
			( (config_bits[21:20] == 2'b00) && cterm_reset )
			) begin
			dff_sr		<= 1;
			dff_srval	<= 0;
		end

		//DFF set
		else if(
			( (config_bits[19:18] == 2'b00) && pterm_a ) ||
			( (config_bits[19:18] == 2'b01) && global_sr ) ||
			( (config_bits[19:18] == 2'b00) && cterm_set )
			) begin
			dff_sr		<= 1;
			dff_srval	<= 1;
		end

		//Nope, not setting or resetting
		else begin
			dff_sr		<= 0;
			dff_srval	<= 0;
		end

	end

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Macrocell flipflop

	wire	dff_rising_en	= !config_bits[25] || config_bits[22];
	wire	dff_falling_en	= config_bits[25] || config_bits[22];

	//FF is replicated a bunch of times b/c of complex clocking structure.
	//It's not too practical to have a FF with one of many possible clocks in a 7-series device.
	//It's way easier to just have one FF per clock, then mux the outputs combinatorially.

	wire mc_dff_ctc;
	ConfigurableEdgeFlipflop ff_ctc(
		.d(dff_in),
		.clk(cterm_clk),
		.ce(dff_ce),
		.sr(dff_sr),
		.srval(dff_srval),
		.q(mc_dff_ctc),
		.rising_en(dff_rising_en),
		.falling_en(dff_falling_en)
	);

	wire mc_dff_ptc;
	ConfigurableEdgeFlipflop ff_ptc(
		.d(dff_in),
		.clk(pterm_c),
		.ce(dff_ce),
		.sr(dff_sr),
		.srval(dff_srval),
		.q(mc_dff_ptc),
		.rising_en(dff_rising_en),
		.falling_en(dff_falling_en)
	);

	wire mc_dff_gck0;
	ConfigurableEdgeFlipflop ff_gck0(
		.d(dff_in),
		.clk(global_clk[0]),
		.ce(dff_ce),
		.sr(dff_sr),
		.srval(dff_srval),
		.q(mc_dff_gck0),
		.rising_en(dff_rising_en),
		.falling_en(dff_falling_en)
	);

	wire mc_dff_gck1;
	ConfigurableEdgeFlipflop ff_gck1(
		.d(dff_in),
		.clk(global_clk[1]),
		.ce(dff_ce),
		.sr(dff_sr),
		.srval(dff_srval),
		.q(mc_dff_gck1),
		.rising_en(dff_rising_en),
		.falling_en(dff_falling_en)
	);

	wire mc_dff_gck2;
	ConfigurableEdgeFlipflop ff_gck2(
		.d(dff_in),
		.clk(global_clk[2]),
		.ce(dff_ce),
		.sr(dff_sr),
		.srval(dff_srval),
		.q(mc_dff_gck2),
		.rising_en(dff_rising_en),
		.falling_en(dff_falling_en)
	);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Macrocell latch

	wire mc_latch_ctc;
	ConfigurableEdgeLatch l_ctc(
		.d(dff_in),
		.clk(cterm_clk),
		.sr(dff_sr),
		.srval(dff_srval),
		.q(mc_latch_ctc),
		.pass_high(!config_bits[25])
	);

	wire mc_latch_ptc;
	ConfigurableEdgeLatch l_ptc(
		.d(dff_in),
		.clk(pterm_c),
		.sr(dff_sr),
		.srval(dff_srval),
		.q(mc_latch_ptc),
		.pass_high(!config_bits[25])
	);

	wire mc_latch_gck0;
	ConfigurableEdgeLatch l_gck0(
		.d(dff_in),
		.clk(global_clk[0]),
		.sr(dff_sr),
		.srval(dff_srval),
		.q(mc_latch_gck0),
		.pass_high(!config_bits[25])
	);

	wire mc_latch_gck1;
	ConfigurableEdgeLatch l_gck1(
		.d(dff_in),
		.clk(global_clk[1]),
		.sr(dff_sr),
		.srval(dff_srval),
		.q(mc_latch_gck1),
		.pass_high(!config_bits[25])
	);

	wire mc_latch_gck2;
	ConfigurableEdgeLatch l_gck2(
		.d(dff_in),
		.clk(global_clk[2]),
		.sr(dff_sr),
		.srval(dff_srval),
		.q(mc_latch_gck2),
		.pass_high(!config_bits[25])
	);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Storage element output muxing

	//Final clock output mux
	reg							mc_dff_muxed;
	reg							mc_latch_muxed;
	reg							mc_storage_muxed;
	always @(*) begin

		case(config_bits[24:23])
			0:	begin
				mc_dff_muxed	<= mc_dff_gck0;
				mc_latch_muxed	<= mc_latch_gck0;
			end
			1: begin
				mc_dff_muxed	<= mc_dff_gck2;
				mc_latch_muxed	<= mc_latch_gck2;
			end
			2:	begin
				mc_dff_muxed	<= mc_dff_gck1;
				mc_latch_muxed	<= mc_latch_gck1;
			end
			3: begin
				if(config_bits[26]) begin
					mc_dff_muxed	<= mc_dff_ctc;
					mc_latch_muxed	<= mc_latch_ctc;
				end
				else begin
					mc_dff_muxed	<= mc_dff_ptc;
					mc_latch_muxed	<= mc_latch_ptc;
				end
			end
		endcase

		//Take output from latch
		if(config_bits[17:16] == 2'b01)
			mc_storage_muxed	<= mc_latch_muxed;

		//Nope, output comes from DFF
		else
			mc_storage_muxed	<= mc_dff_muxed;

	end

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Final output muxing

	always @(*) begin

		//Enable for macrocell->ZIA driver (active low so we're powered down when device is blank)
		if(!config_bits[12]) begin

			//See where macrocell->ZIA driver should come from
			if(config_bits[13])
				mc_to_zia			<= mc_storage_muxed;
			else
				mc_to_zia			<= xor_out;

		end

		//Output driver powered down, feed a constant zero into the ZIA to prevent spurious toggles
		else
			mc_to_zia				<= 1'b0;

		//Mux for CGND
		if(config_bits[6:3] == 4'b1110)
			mc_to_obuf				<= 1'b0;
		else begin

			//Mux for pad driver
			if(config_bits[7])
				mc_to_obuf			<= xor_out;
			else
				mc_to_obuf			<= mc_storage_muxed;

		end

		//Enable for input->ZIA driver (active low so we're powered down when device is blank)
		if(!config_bits[14]) begin

			//See where input->ZIA driver should come from
			if(config_bits[15])
				ibuf_to_zia			<= mc_storage_muxed;
			else
				ibuf_to_zia			<= raw_ibuf;

		end

		//Output driver powered down, feed a constant zero into the ZIA to prevent spurious toggles
		else
			ibuf_to_zia				<= 1'b0;

	end

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Tri-state control

	always @(*) begin

		case(config_bits[6:3])

			4'b0000:	tristate_to_obuf	<= 1'b0;			//Push-pull output, always enabled (not tristated)
			4'b0001:	tristate_to_obuf	<= mc_to_obuf;		//Open drain (tristating any time we drive high)
			4'b0010:	tristate_to_obuf	<= global_tris[1];	//Tri-state output (using GTS1)
			//4'b0011:											//Unknown
			4'b0100:	tristate_to_obuf	<= pterm_b;			//Tri-state output (using PTB)
			//4'b0101:											//Unknown
			4'b0110:	tristate_to_obuf	<= global_tris[3];	//Tri-state output (using GTS3)
			//4'b0111:											//Unknown
			4'b1000:	tristate_to_obuf	<= cterm_oe;		//Tri-state output (using CTE)
			//4'b1001:											//Unknown
			4'b1010:	tristate_to_obuf	<= global_tris[2];	//Tri-state output (using GTS2)
			//4'b1011:											//Unknown
			4'b1100:	tristate_to_obuf	<= global_tris[0];	//Tri-state output (using GTS0)
			//4'b1101:											//Unknown
			4'b1110:	tristate_to_obuf	<= 1'b0;			//CGND, always enabled (not tristated)
			4'b1111:	tristate_to_obuf	<= 1'b1;			//Input/unused (always tristated)

			default:	tristate_to_obuf	<= 1'b1;			//unknown, float the output

		endcase
	end

endmodule
