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
	@brief JTAG stuff for an XC2C-series device
 */
module XC2CJTAG(tdi, tms, tck, tdo);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Device configuration

	parameter MACROCELLS = 32;	//A variant implied for 32/64, no support for base version

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// I/Os

	input wire					tdi;
	input wire					tms;
	input wire					tck;
	output wire					tdo;

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// The core JTAG state machine

	localparam STATE_TEST_LOGIC_RESET	= 4'h0;
	localparam STATE_RUN_TEST_IDLE		= 4'h1;
	localparam STATE_SELECT_DR_SCAN		= 4'h2;
	localparam STATE_SELECT_IR_SCAN		= 4'h3;
	localparam STATE_CAPTURE_DR			= 4'h4;
	localparam STATE_CAPTURE_IR			= 4'h5;
	localparam STATE_SHIFT_DR			= 4'h6;
	localparam STATE_SHIFT_IR			= 4'h7;
	localparam STATE_EXIT1_DR			= 4'h8;
	localparam STATE_EXIT1_IR			= 4'h9;
	localparam STATE_PAUSE_DR			= 4'ha;
	localparam STATE_PAUSE_IR			= 4'hb;
	localparam STATE_EXIT2_DR			= 4'hc;
	localparam STATE_EXIT2_IR			= 4'hd;
	localparam STATE_UPDATE_DR			= 4'he;
	localparam STATE_UPDATE_IR			= 4'hf;

	reg[3:0] state						= STATE_TEST_LOGIC_RESET;

	always @(posedge tck) begin

		case(state)

			STATE_TEST_LOGIC_RESET: begin
				if(tms)
					state			<= STATE_RUN_TEST_IDLE;
			end	//end STATE_TEST_LOGIC_RESET

			STATE_RUN_TEST_IDLE: begin
				if(tms)
					state			<= STATE_SELECT_DR_SCAN;
			end	//end STATE_RUN_TEST_IDLE

			STATE_SELECT_DR_SCAN: begin
				if(tms)
					state			<= STATE_SELECT_IR_SCAN;
				else
					state			<= STATE_CAPTURE_DR;
			end	//end STATE_SELECT_DR_SCAN

			STATE_SELECT_IR_SCAN: begin
				if(tms)
					state			<= STATE_TEST_LOGIC_RESET;
				else
					state			<= STATE_CAPTURE_IR;
			end	//end STATE_SELECT_IR_SCAN

			STATE_CAPTURE_DR: begin
				if(tms)
					state			<= STATE_EXIT1_DR;
				else
					state			<= STATE_SHIFT_DR;
			end	//end STATE_CAPTURE_DR

			STATE_CAPTURE_IR: begin
				if(tms)
					state			<= STATE_EXIT1_IR;
				else
					state			<= STATE_SHIFT_IR;
			end	//end STATE_CAPTURE_IR

			STATE_SHIFT_DR: begin
				if(tms)
					state			<= STATE_EXIT1_DR;
			end	//end STATE_SHIFT_DR

			STATE_SHIFT_IR: begin
				if(tms)
					state			<= STATE_EXIT1_IR;
			end	//end STATE_SHIFT_IR

			STATE_EXIT1_DR: begin
				if(tms)
					state			<= STATE_UPDATE_DR;
				else
					state			<= STATE_PAUSE_DR;
			end	//end STATE_EXIT1_DR

			STATE_EXIT1_IR: begin
				if(tms)
					state			<= STATE_UPDATE_IR;
				else
					state			<= STATE_PAUSE_IR;
			end	//end STATE_EXIT1_IR

			STATE_PAUSE_DR: begin
				if(tms)
					state			<= STATE_EXIT2_DR;
			end	//end STATE_PAUSE_DR

			STATE_PAUSE_IR: begin
				if(tms)
					state			<= STATE_EXIT2_IR;
			end	//end STATE_PAUSE_IR

			STATE_EXIT2_DR: begin
				if(tms)
					state			<= STATE_UPDATE_DR;
				else
					state			<= STATE_SHIFT_DR;
			end	//end STATE_EXIT2_DR

			STATE_EXIT2_IR: begin
				if(tms)
					state			<= STATE_UPDATE_IR;
				else
					state			<= STATE_SHIFT_IR;
			end	//end STATE_EXIT2_IR

			STATE_UPDATE_DR: begin
				if(tms)
					state			<= STATE_SELECT_DR_SCAN;
				else
					state			<= STATE_RUN_TEST_IDLE;
			end	//end STATE_UPDATE_DR

			STATE_UPDATE_IR: begin
				if(tms)
					state			<= STATE_SELECT_IR_SCAN;
				else
					state			<= STATE_RUN_TEST_IDLE;
			end	//end STATE_UPDATE_IR

		endcase

	end

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// The instruction register

	localparam INST_BYPASS			= 8'hff;
	localparam INST_ISC_ENABLE		= 8'he8;
	localparam INST_ISC_ENABLEOTF	= 8'he4;
	localparam INST_ISC_SRAM_READ	= 8'he7;
	localparam INST_ISC_SRAM_WRITE	= 8'he6;
	localparam INST_ISC_ERASE		= 8'hed;
	localparam INST_ISC_PROGRAM		= 8'hea;
	localparam INST_ISC_INIT		= 8'hf0;
	localparam INST_ISC_DISABLE		= 8'hc0;
	localparam INST_ISC_READ		= 8'hee;

	/*
		//TODO: implement some of these?

		"INTEST         (00000010)," &
		"SAMPLE         (00000011)," &
		"EXTEST         (00000000)," &
		"IDCODE         (00000001)," &
		"USERCODE       (11111101)," &
		"HIGHZ          (11111100)," &
		"ISC_ENABLE_CLAMP (11101001)," &
		"TEST_ENABLE    (00010001)," &
		"BULKPROG       (00010010)," &
		"ERASE_ALL      (00010100)," &
		"MVERIFY        (00010011)," &
		"TEST_DISABLE   (00010101)," &
		"ISC_NOOP       (11100000)";
	*/

	reg[7:0] ir = INST_BYPASS;

	//Capture stuff

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// IDCODE DR

endmodule
