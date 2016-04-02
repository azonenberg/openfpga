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
	out_lfosc_ff, out_lfosc_count, sys_rst, count_rst, dbg1, dbg2);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// I/O declarations
	
	(* LOC = "P20" *)
	output reg out_lfosc_ff = 0;
	
	(* LOC = "P19" *)
	output reg out_lfosc_count = 0;
	
	(* LOC = "P2" *)
	(* PULLDOWN = "10k" *)
	(* SCHMITT_TRIGGER *)
	input wire sys_rst;			//Full chip reset.
	
	(* LOC = "P3" *)
	(* PULLDOWN = "10k" *)
	(* SCHMITT_TRIGGER *)
	input wire count_rst;		//logic reset
	
	(* LOC = "P18" *)
	output wire dbg1;
	
	(* LOC = "P17" *)
	output wire dbg2;
	
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// System reset

	//Level-triggered asynchronous system reset
	GP_SYSRESET #(
		.RESET_MODE("LEVEL")
	) reset_ctrl (
		.RST(sys_rst)
	);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Oscillators
	
	//The 1730 Hz oscillator
	wire clk_108hz;
	GP_LFOSC #(
		.PWRDN_EN(0),
		.AUTO_PWRDN(0),
		.OUT_DIV(16)
	) lfosc (
		.PWRDN(1'b0),
		.CLKOUT(clk_108hz)
	);
	
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Counter configuration
	
	localparam COUNT_MAX = 31;
	
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Low-frequency oscillator and post-divider in behavioral logic, extracted to a hard IP block by synthesis
	
	//NOTE: GP_SYSRESET reset causes un-extracted RTL counters to clear to zero and glitch! No obvious workaround.
	//The un-extracted counter behaves identically to hard IP in POR or count_rst modes.

	//Fabric post-divider
	reg[4:0] count = COUNT_MAX;
	wire out_fabric_raw = (count == 0);
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
	
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Low-frequency oscillator and post-divider in hard counter
	
	//Hard IP post-divider
	wire out_lfosc_raw;
	GP_COUNT8 #(
		.RESET_MODE("LEVEL"),
		.COUNT_TO(COUNT_MAX),
		.CLKIN_DIVIDE(1)
	) hard_counter (
		.CLK(clk_108hz),
		.RST(count_rst),
		.OUT(out_lfosc_raw)
	);
	
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// LED toggling
	
	//Toggle the output every time the counters underflow
	always @(posedge clk_108hz) begin
	
		if(out_fabric_raw)
			out_lfosc_ff	<= ~out_lfosc_ff;
		if(out_lfosc_raw)
			out_lfosc_count <= ~out_lfosc_count;

	end
	
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Test stuff
	
	assign dbg1 = out_fabric_raw;
	assign dbg2 = out_lfosc_raw;
	
endmodule
