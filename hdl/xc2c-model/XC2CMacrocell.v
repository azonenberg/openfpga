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
	mc_to_zia, mc_to_obuf,
	raw_ibuf,
	config_done_rst
	);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// I/O declarations

	input wire[26:0]			config_bits;

	input wire					pterm_a;
	input wire					pterm_b;
	input wire					pterm_c;

	input wire					or_term;

	input wire					cterm_clk;
	input wire[2:0]				global_clk;

	output reg					mc_to_zia;
	output reg					mc_to_obuf;

	input wire					raw_ibuf;

	input wire					config_done_rst;

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

	//TODO: implement latch mode

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

		//TODO: set/reset inputs
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
	// FF output muxing

	//Final clock output mux
	reg							mc_dff_muxed;
	always @(*) begin

		case(config_bits[24:23])
			0:	mc_dff_muxed	<= mc_dff_gck0;
			1:	mc_dff_muxed	<= mc_dff_gck2;
			2:	mc_dff_muxed	<= mc_dff_gck1;
			3: begin
				if(config_bits[26])
					mc_dff_muxed	<= mc_dff_ctc;
				else
					mc_dff_muxed	<= mc_dff_ptc;
			end
		endcase

	end

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Final output muxing

	//TODO: mux for bit 15 (ibuf/mc flipflop)

	always @(*) begin

		//Enable for macrocell->ZIA driver (active low so we're powered down when device is blank)
		if(!config_bits[12]) begin

			//See where macrocell->ZIA driver should go
			if(config_bits[13])
				mc_to_zia				<= mc_dff_muxed;
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
			mc_to_obuf					<= mc_dff_muxed;

	end

endmodule
