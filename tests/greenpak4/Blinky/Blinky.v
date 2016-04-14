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

module Blinky(
	out_lfosc_ff, out_lfosc_count, out_rosc_ff, out_rcosc_ff,
	sys_rst, count_rst, bg_ok, osc_pwrdn);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// I/O declarations
	
	// Put outputs all together on the 11-20 side of the device
	
	(* LOC = "P20" *)
	output reg out_lfosc_ff = 0;
	
	(* LOC = "P19" *)
	output reg out_lfosc_count = 0;
	
	(* LOC = "P18" *)
	output reg out_rosc_ff = 0;
	
	(* LOC = "P17" *)
	output reg out_rcosc_ff = 0;
		
	(* LOC = "P16" *)
	output wire bg_ok;
	
	// Put inputs all together on the 1-10 side of the device
	
	(* LOC = "P2" *)
	(* PULLDOWN = "10k" *)
	(* SCHMITT_TRIGGER *)
	input wire sys_rst;			//Full chip reset
	
	(* LOC = "P3" *)
	(* PULLDOWN = "10k" *)
	(* SCHMITT_TRIGGER *)
	input wire count_rst;		//Logic reset
	
	(* LOC = "P4" *)
	(* PULLDOWN = "10k" *)
	(* SCHMITT_TRIGGER *)
	input wire osc_pwrdn;		//Power-gate input to the oscillators

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// System reset stuff

	//Level-triggered asynchronous system reset input
	GP_SYSRESET #(
		.RESET_MODE("LEVEL")
	) reset_ctrl (
		.RST(sys_rst)
	);
	
	//Power-on reset
	wire por_done;
	GP_POR #(
		.POR_TIME(500)
	) por (
		.RST_DONE(por_done)
	);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Oscillators
	
	//The 1730 Hz oscillator
	wire clk_108hz;
	GP_LFOSC #(
		.PWRDN_EN(1),
		.AUTO_PWRDN(0),
		.OUT_DIV(16)
	) lfosc (
		.PWRDN(osc_pwrdn),
		.CLKOUT(clk_108hz)
	);
	
	//The 27 MHz ring oscillator
	wire clk_1687khz_cnt;		//dedicated output to hard IP only
	wire clk_1687khz;			//general fabric output (used to toggle the LED)
	GP_RINGOSC #(
		.PWRDN_EN(1),
		.AUTO_PWRDN(0),
		.PRE_DIV(16),
		.FABRIC_DIV(1)
	) ringosc (
		.PWRDN(osc_pwrdn),
		.CLKOUT_PREDIV(clk_1687khz_cnt),
		.CLKOUT_FABRIC(clk_1687khz)
	);
	
	//The 25 kHz RC oscillator
	wire clk_6khz_cnt;			//dedicated output to hard IP only
	wire clk_6khz;				//general fabric output (used to toggle the LED)
	GP_RCOSC #(
		.PWRDN_EN(1),
		.AUTO_PWRDN(0),
		.OSC_FREQ("25k"),		//osc can run at 25 kHz or 2 MHz
		.PRE_DIV(4),
		.FABRIC_DIV(1)
	) rcosc (
		.PWRDN(osc_pwrdn),
		.CLKOUT_PREDIV(clk_6khz_cnt),
		.CLKOUT_FABRIC(clk_6khz)
	);
	
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Bandgap voltage reference (used by a lot of the mixed signal IP)
	
	wire bandgap_vout;
	GP_BANDGAP #(
		.AUTO_PWRDN(0),
		.CHOPPER_EN(1),
		.OUT_DELAY(550)
	) bandgap (
		.OK(bg_ok),
		.VOUT(bandgap_vout)
	);
		
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Low-frequency oscillator and post-divider in behavioral logic, extracted to a hard IP block by synthesis

	localparam COUNT_MAX = 31;

	//Fabric post-divider
	reg[7:0] count = COUNT_MAX;
	always @(posedge clk_108hz, posedge count_rst) begin
		
		//level triggered reset
		if(count_rst)
			count			<= 0;
		
		//counter
		else begin

			if(count == 0)
				count		<= COUNT_MAX;
			else
				count		<= count - 1'd1;

		end
		
	end
	
	//Output bit
	wire out_fabric_raw = (count == 0);
	
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Low-frequency oscillator and post-divider in hard counter
	
	//Hard IP post-divider
	wire out_lfosc_raw;
	GP_COUNT8 #(
		.RESET_MODE("LEVEL"),
		.COUNT_TO(COUNT_MAX),
		.CLKIN_DIVIDE(1)
	) lfosc_cnt (
		.CLK(clk_108hz),
		.RST(count_rst),
		.OUT(out_lfosc_raw)
	);
	
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Ring oscillator and post-divider in hard counter
	
	//Hard IP post-divider
	wire out_rosc_raw;
	GP_COUNT14 #(
		.RESET_MODE("LEVEL"),
		.COUNT_TO(16383),
		.CLKIN_DIVIDE(1)
	) ringosc_cnt (
		.CLK(clk_1687khz_cnt),
		.RST(count_rst),
		.OUT(out_rosc_raw)
	);
	
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// RC oscillator and post-divider in hard counter
	
	//Hard IP post-divider
	wire out_rcosc_raw;
	GP_COUNT14 #(
		.RESET_MODE("LEVEL"),
		.COUNT_TO(1023),
		.CLKIN_DIVIDE(1)
	) rcosc_cnt (
		.CLK(clk_6khz_cnt),
		.RST(count_rst),
		.OUT(out_rcosc_raw)
	);
	
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// LF oscillator LED toggling
	
	//Toggle the output every time the counters underflow
	always @(posedge clk_108hz) begin
	
		//Gate toggle signals with POR to prevent glitches
		//caused by blocks resetting at different times during boot
		if(por_done) begin
		
			if(out_fabric_raw)
				out_lfosc_ff	<= ~out_lfosc_ff;
			if(out_lfosc_raw)
				out_lfosc_count <= ~out_lfosc_count;
				
		end

	end
	
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Ring oscillator LED toggling
	
	//Slow it down with a few DFFs to make it readable
	reg[3:0] pdiv = 0;
	always @(posedge clk_1687khz) begin
		if(out_rosc_raw) begin
			pdiv				<= pdiv + 1'd1;
			if(pdiv == 0)
				out_rosc_ff		<= ~out_rosc_ff;
		end
	end
	
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Ring oscillator LED toggling
	
	always @(posedge clk_6khz) begin
		if(out_rcosc_raw)
			out_rcosc_ff		<= ~out_rcosc_ff;
	end
	
endmodule
