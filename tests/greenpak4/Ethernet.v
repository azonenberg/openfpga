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
module Ethernet(rst_done, txd, lcw, pulse_start, burst_start);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// I/O declarations

	(* LOC = "P20" *)
	output reg rst_done = 0;

	(* LOC = "P19" *)
	output wire txd;

	(* LOC = "P18" *)
	output wire lcw;

	(* LOC = "P17" *)
	output wire pulse_start;

	(* LOC = "P16" *)
	output burst_start;

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

	//TODO: Bit ordering fix
	//We should send the LSB first on the wire... is this correct?

	//0x4000 = acknowledgement (always send this since we don't have enough FFs to actually check the LCW)
	//0x0001 = 10baseT half duplex
	//0x0002 = 10baseT full duplex
	reg pgen_reset	= 1;				//TODO
	reg lcw_advance	= 0;
	GP_PGEN #(
		//.PATTERN_DATA(16'h4003),
		.PATTERN_DATA(16'h5555),
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
	/*
	localparam BURST_MAX = 32;
	reg[7:0] burst_count = BURST_MAX;
	wire burst_done = (burst_count == 0);
	always @(posedge pulse_start) begin

		if(burst_count == 0)
			burst_count <= BURST_MAX;

		else
			burst_count <= burst_count - 1'd1;

	end
	*/

	//When a burst starts send 16 FLPs, with a LCW pulse in between each
	//Send one more pulse at the end
	reg next_pulse_is_lcw = 0;
	reg burst_active = 0;
	always @(posedge clk_fabric) begin

		//Default to not sending a pulse or advancing the LCW
		lcw_advance		<= 0;
		pulse_en		<= 0;

		//Start a new burst every 16 ms.
		//Stop it after 33 pulses.
		if(burst_start)
			burst_active	<= 1;
		//else if(burst_done)
		//	burst_active	<= 0;

		//DEBUG: always send a pulse
		pulse_en <= burst_active;

		/*
		//Send a pulse
		if(pulse_start) begin

			//Sending a bit from the LCW
			if(next_pulse_is_lcw) begin
				pulse_en			<= lcw;
				lcw_advance			<= 1;
			end

			//Not sending LCW bit, send a FLP instead
			else
				pulse_en			<= 1'b1;

			//Alternate LCW and FLP bits
			next_pulse_is_lcw	<= ~next_pulse_is_lcw;

		end
		*/

	end

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Debug outputs

	//Detect when the system reset has completed
	always @(posedge clk_fabric)
		rst_done <= 1;

endmodule
