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
	led_lfosc_ff, led_lfosc_count, led_lfosc_shreg1, led_lfosc_shreg1a, led_lfosc_shreg2, led_lfosc_shreg2a,
		led_rosc_ff, led_rcosc_ff,
	sys_rst, count_rst, osc_pwrdn);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// I/O declarations

	// Put outputs all together on the 11-20 side of the device

	(* LOC = "P20" *)
	output reg led_lfosc_ff = 0;

	(* LOC = "P19" *)
	output reg led_lfosc_count = 0;

	(* LOC = "P18" *)
	output wire led_lfosc_shreg1;

	(* LOC = "P17" *)
	output wire led_lfosc_shreg1a;

	(* LOC = "P16" *)
	output wire led_lfosc_shreg2;

	(* LOC = "P15" *)
	output wire led_lfosc_shreg2a;

	(* LOC = "P14" *)
	output reg led_rosc_ff = 0;

	(* LOC = "P13" *)
	output reg led_rcosc_ff = 0;

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
		.CLKOUT_HARDIP(clk_1687khz_cnt),
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
		.CLKOUT_HARDIP(clk_6khz_cnt),
		.CLKOUT_FABRIC(clk_6khz)
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
	wire led_fabric_raw = (count == 0);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Low-frequency oscillator and post-divider in hard counter

	//Hard IP post-divider
	wire led_lfosc_raw;
	(* LOC = "COUNT8_7" *)		//device is pretty full, PAR fails without this constraint
								//TODO: better optimizer so we can figure this out without help?
	GP_COUNT8 #(
		.RESET_MODE("LEVEL"),
		.COUNT_TO(COUNT_MAX),
		.CLKIN_DIVIDE(1)
	) lfosc_cnt (
		.CLK(clk_108hz),
		.RST(count_rst),
		.OUT(led_lfosc_raw)
	);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Ring oscillator and post-divider in hard counter

	//Hard IP post-divider
	wire led_rosc_raw;
	GP_COUNT14 #(
		.RESET_MODE("LEVEL"),
		.COUNT_TO(16383),
		.CLKIN_DIVIDE(1)
	) ringosc_cnt (
		.CLK(clk_1687khz_cnt),
		.RST(count_rst),
		.OUT(led_rosc_raw)
	);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// RC oscillator and post-divider in hard counter

	//Hard IP post-divider
	wire led_rcosc_raw;
	GP_COUNT14 #(
		.RESET_MODE("LEVEL"),
		.COUNT_TO(1023),
		.CLKIN_DIVIDE(1)
	) rcosc_cnt (
		.CLK(clk_6khz_cnt),
		.RST(count_rst),
		.OUT(led_rcosc_raw)
	);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// LF oscillator LED toggling

	//Toggle the output every time the counters underflow
	always @(posedge clk_108hz) begin

		//Gate toggle signals with POR to prevent glitches
		//caused by blocks resetting at different times during boot
		if(por_done) begin

			if(led_fabric_raw)
				led_lfosc_ff	<= ~led_lfosc_ff;
			if(led_lfosc_raw)
				led_lfosc_count <= ~led_lfosc_count;

		end

	end

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Ring oscillator LED toggling

	//Slow it down with a few DFFs to make it readable
	reg[3:0] pdiv = 0;
	always @(posedge clk_1687khz) begin
		if(led_rosc_raw) begin
			pdiv				<= pdiv + 1'd1;
			if(pdiv == 0)
				led_rosc_ff		<= ~led_rosc_ff;
		end
	end

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Ring oscillator LED toggling

	always @(posedge clk_6khz) begin
		if(led_rcosc_raw)
			led_rcosc_ff		<= ~led_rcosc_ff;
	end

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Shift register to TAP RC oscillator outputs

	GP_SHREG #(
		.OUTA_TAP(8),
		.OUTA_INVERT(0),
		.OUTB_TAP(16)
	) shreg (
		.nRST(1'b1),
		.CLK(clk_108hz),
		.IN(led_lfosc_ff),
		.OUTA(led_lfosc_shreg1),
		.OUTB(led_lfosc_shreg2)
	);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Same shift register, but written using behavioral logic and extracted to a shreg cell by synthesis

	reg[15:0] led_lfosc_infreg = 0;
	assign led_lfosc_shreg1a = led_lfosc_infreg[7];
	assign led_lfosc_shreg2a = led_lfosc_infreg[15];

	always @(posedge clk_108hz) begin
		led_lfosc_infreg	<= {led_lfosc_infreg[14:0], led_lfosc_ff};
	end

endmodule
