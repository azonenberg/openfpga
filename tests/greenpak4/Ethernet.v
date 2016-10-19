/***********************************************************************************************************************
 * Copyright (C) 2016 Andrew Zonenberg and contributors                                                                *
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
	@brief Minimal 10baseT autonegotiation implementation
 */
module Ethernet(rst_done, txd, lcw, burst_start, lcw_advance, pulse_start_gated);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// I/O declarations

	(* LOC = "P20" *)
	output reg rst_done = 0;

	(* LOC = "P19" *)
	(* DRIVE_STRENGTH = "2X" *)
	output wire txd;

	(* LOC = "P18" *)
	output wire lcw;

	(* LOC = "P17" *)
	output burst_start;

	(* LOC = "P16" *)
	output lcw_advance;

	(* LOC = "P15" *)
	output pulse_start_gated;

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Power-on-reset configuration

	wire por_done;
	GP_POR #(.POR_TIME(500)) por (.RST_DONE(por_done));

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Clock source - 2 MHz RC oscillator (500 ns per cycle)

	wire clk_hardip;
	wire clk_fabric;
	GP_RCOSC #(
		.PWRDN_EN(0),
		.AUTO_PWRDN(0),
		.OSC_FREQ("2M"),
		.PRE_DIV(1),
		.FABRIC_DIV(1)
	) rcosc (
		.PWRDN(1'b0),
		.CLKOUT_HARDIP(clk_hardip),
		.CLKOUT_FABRIC(clk_fabric)
	);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Edge detector for producing ~150 ns FLP/NLPs

	reg pulse_en = 0;
	GP_EDGEDET #(
		.DELAY_STEPS(1),
		.EDGE_DIRECTION("RISING"),
		.GLITCH_FILTER(0)
	) delay(
		.IN(pulse_en),
		.OUT(txd)
		);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Interval timer for pulses within a burst

	//Delay between successive pulses is 62.5 us (125 clocks)
	localparam PULSE_INTERVAL = 124;
	reg[7:0] pulse_count = PULSE_INTERVAL;
	wire pulse_start = (pulse_count == 0);
	always @(posedge clk_hardip) begin

		if(pulse_count == 0)
			pulse_count <= PULSE_INTERVAL;

		else
			pulse_count <= pulse_count - 1'd1;
	end

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Interval timer for bursts

	//Delay between successive bursts is ~16 ms (32000 clocks)
	//This is too big for one counter at our clock speed so do an 8ms counter plus one more stage in DFF logic
	//TODO: make gp4_counters pass infer cascaded counters and/or fabric post-dividers automatically?

	//8ms counter
	localparam BURST_INTERVAL = 15999;
	reg[13:0] interval_count = BURST_INTERVAL;
	wire burst_start_raw = (interval_count == 0);
	always @(posedge clk_hardip) begin

		if(interval_count == 0)
			interval_count <= BURST_INTERVAL;

		else
			interval_count <= interval_count - 1'd1;

	end

	//Post-divider to give one short pulse every 16 ms
	reg burst_start_t = 0;
	reg burst_start = 0;
	always @(posedge clk_fabric) begin
		burst_start <= 0;

		if(burst_start_raw)
			burst_start_t	<= !burst_start_t;

		if(burst_start_t && burst_start_raw)
			burst_start		<= 1;

	end

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Link codeword generation

	//Send the link codeword onto the wire.
	//Ethernet bit ordering is backwards so send MSB first in Verilog ordering
	//0x8000 = selector (802.3)
	//0x0400 = 10baseT half duplex
	//0x0200 = 10baseT full duplex
	//0x0002 = ACK (always send this since we don't have enough FFs to actually check the LCW)

	reg pgen_reset	= 1;
	reg lcw_advance	= 0;
	GP_PGEN #(
		.PATTERN_DATA(16'h8602),
		.PATTERN_LEN(5'd16)
	) pgen (
		.nRST(pgen_reset),
		.CLK(lcw_advance),
		.OUT(lcw)
	);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// FLP bursting logic

	//We have a total of 33 pulse times in our burst.
	//End the burst after the 33rd pulse.
	//TODO: Add inference support to greenpak4_counters pass so we can infer GP_COUNTx_ADV cells from behavioral logic
	wire burst_done;
	GP_COUNT8_ADV #(
		.CLKIN_DIVIDE(1),
		.COUNT_TO(33),
		.RESET_MODE("RISING"),
		.RESET_VALUE("COUNT_TO")
	) burst_count (
		.CLK(clk_hardip),
		.RST(burst_start),
		.UP(1'b0),
		.KEEP(!pulse_start),
		.OUT(burst_done)
	);

	//When a burst starts send 16 FLPs, with a LCW pulse in between each
	//Send one more pulse at the end
	reg next_pulse_is_lcw = 0;
	reg burst_active = 0;
	reg pulse_start_gated = 0;
	always @(posedge clk_fabric) begin

		//Default to not sending a pulse or advancing the LCW
		lcw_advance		<= 0;
		pulse_en		<= 0;
		pgen_reset		<= 1;

		//Start a new burst every 16 ms.
		//Always start the burst with a clock pulse, then the first LCW bit.
		//Whack the pgen with a clock now to get the first output bit ready.
		if(burst_start) begin
			burst_active		<= 1;
			next_pulse_is_lcw	<= 0;
			lcw_advance			<= 1;
		end

		//Stop the burst after 33 pulses.
		else if(burst_done) begin
			burst_active	<= 0;
			pgen_reset		<= 0;
		end

		//Indicates a new pulse is coming
		//Move this up one clock to reduce fan-in on the pulse generation logic
		pulse_start_gated <= burst_active && pulse_start;

		if(pulse_en && next_pulse_is_lcw)
			lcw_advance				<= 1;

		//Send a pulse if we're currently in a burst
		if(pulse_start_gated) begin

			//Sending a bit from the LCW
			if(next_pulse_is_lcw)
				pulse_en			<= lcw;

			//Not sending LCW bit, send a FLP instead
			else
				pulse_en			<= 1'b1;

			//Alternate LCW and FLP bits
			next_pulse_is_lcw		<= ~next_pulse_is_lcw;

		end

	end

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Debug outputs

	//Detect when the system reset has completed
	always @(posedge clk_fabric)
		rst_done <= 1;

endmodule
