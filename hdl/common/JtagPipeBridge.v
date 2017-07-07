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
module JtagPipeBridge(
	output wire		tck,
	output wire		tdi,
	output wire		tms,
	input wire		tdo
	);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Opcode values

	//The canonical copy of these is in jtaghal/jtagd_opcodes.yml but we're not building with Splash
	//For now, replicate by hand
	localparam JTAGD_OP_GET_NAME 		= 8'h00;
	localparam JTAGD_OP_GET_SERIAL		= 8'h01;
	localparam JTAGD_OP_GET_USERID		= 8'h02;
	localparam JTAGD_OP_GET_FREQ		= 8'h03;
	localparam JTAGD_OP_SHIFT_DATA_WO	= 8'h06;
	localparam JTAGD_OP_SHIFT_DATA		= 8'h07;
	localparam JTAGD_OP_DUMMY_CLOCK		= 8'h08;
	localparam JTAGD_OP_QUIT			= 8'h0e;
	localparam JTAGD_OP_TLR				= 8'h18;
	localparam JTAGD_OP_ENTER_SIR		= 8'h19;
	localparam JTAGD_OP_LEAVE_E1IR		= 8'h1a;
	localparam JTAGD_OP_ENTER_SDR		= 8'h1b;
	localparam JTAGD_OP_LEAVE_E1DR		= 8'h1c;
	localparam JTAGD_OP_RESET_IDLE		= 8'h1d;

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Adapter metadata

	localparam ADAPTER_NAME		= "JtagPipeBridge";
	localparam ADAPTER_SERIAL	= "NoSerialYet";
	localparam ADAPTER_USERID	= "NoUserIDYet";
	localparam ADAPTER_FREQ		= 25000000;

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// GSR wait

	reg ready = 0;
	initial begin
		#100;
		ready = 1;
	end

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Clock oscillator

	reg		clk = 0;
	always begin
		#5;
		clk = 1;
		#5;
		clk = 0;
	end

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Protocol parsing stuff

	`include "JtagMaster_states.vh"

	wire		jtag_done;

	reg			state_en	= 0;
	reg[2:0]	next_state	= OP_TEST_RESET;

	reg			shift_en		= 0;
	reg			packet_last_tms	= 0;
	reg			last_tms		= 0;
	reg[31:0]	packet_len		= 0;
	reg[5:0]	shift_len		= 0;
	reg[7:0]	shift_din		= 0;
	wire[31:0]	shift_dout;

	JtagMaster master(
		.clk(clk),
		.clkdiv(8'h1),

		.tck(tck),
		.tdi(tdi),
		.tms(tms),
		.tdo(tdo),

		.state_en(state_en),
		.next_state(next_state),

		.shift_en(shift_en),
		.len(shift_len),
		.last_tms(last_tms),
		.din({24'h0, shift_din}),
		.dout(shift_dout),
		.done(jtag_done)
	);

	wire[7:0]	shift_dout_byte	= shift_dout >> (32 - shift_len);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Protocol parsing stuff

	integer readpipe;
	integer writepipe;
	initial begin
		$display("Opening read pipe");
		readpipe = $fopen("/tmp/simreadpipe", "r");
		$display("Opening write pipe");
		writepipe = $fopen("/tmp/simwritepipe", "w");
	end

	localparam PROTO_STATE_BOOT			= 8'h00;
	localparam PROTO_STATE_IDLE			= 8'h01;
	localparam PROTO_STATE_WAIT			= 8'h02;
	localparam PROTO_STATE_DONE			= 8'h03;
	localparam PROTO_STATE_SHIFT		= 8'h04;
	localparam PROTO_STATE_SHIFT_WAIT	= 8'h05;

	reg[7:0] 	cmd_byte 		= 0;
	reg[7:0]	proto_state		= PROTO_STATE_BOOT;
	reg			want_response	= 0;

	always @(posedge clk) begin

		state_en	<= 0;
		shift_en	<= 0;

		case(proto_state)

			////////////////////////////////////////////////////////////////////////////////////////////////////////////
			// BOOT - sit around and wait for GSR

			PROTO_STATE_BOOT: begin
				if(ready)
					proto_state	<= PROTO_STATE_IDLE;
			end	//end PROTO_STATE_BOOT

			////////////////////////////////////////////////////////////////////////////////////////////////////////////
			// IDLE - read a command and execute it

			PROTO_STATE_IDLE: begin

				//Read a command byte from the pipe
				if(1 != $fscanf(readpipe, "%x", cmd_byte)) begin
					$display("Read failed");
					$finish;
				end

				//See what the command is. Info-query commands run in a single clock
				case(cmd_byte)

					JTAGD_OP_GET_NAME: begin
						$display("Reading adapter name");
						$fdisplay(writepipe, "%s", ADAPTER_NAME);
						$fflush(writepipe);
					end	//end JTAGD_OP_GET_NAME

					JTAGD_OP_GET_SERIAL: begin
						$display("Reading adapter serial");
						$fdisplay(writepipe, "%s", ADAPTER_SERIAL);
						$fflush(writepipe);
					end	//end JTAGD_OP_GET_SERIAL

					JTAGD_OP_GET_USERID: begin
						$display("Reading adapter userid");
						$fdisplay(writepipe, "%s", ADAPTER_USERID);
						$fflush(writepipe);
					end	//end JTAGD_OP_GET_USERID

					JTAGD_OP_GET_FREQ: begin
						$display("Reading adapter clock frequency");
						$fdisplay(writepipe, "%d", ADAPTER_FREQ);
						$fflush(writepipe);
					end	//end JTAGD_OP_GET_FREQ

					JTAGD_OP_SHIFT_DATA: begin
						$fscanf(readpipe, "%x", packet_last_tms);
						$fscanf(readpipe, "%x", packet_len);
						$display("Shifting %d bits of data (ending with tms=%d)",
							packet_len, packet_last_tms);
						proto_state		<= PROTO_STATE_SHIFT;
						want_response	<= 1;
					end	//end JTAGD_OP_SHIFT_DATA

					JTAGD_OP_SHIFT_DATA_WO: begin
						$fscanf(readpipe, "%x", packet_last_tms);
						$fscanf(readpipe, "%x", packet_len);
						$display("Shifting %d bits of data (ending with tms=%d, no reply required)",
							packet_len, packet_last_tms);
						proto_state		<= PROTO_STATE_SHIFT;
						want_response	<= 0;
					end	//end JTAGD_OP_SHIFT_DATA_WO

					JTAGD_OP_TLR: begin
						$display("Resetting to TLR");
						state_en		<= 1;
						next_state		<= OP_TEST_RESET;
						proto_state		<= PROTO_STATE_WAIT;
					end	//end JTAGD_OP_TLR

					JTAGD_OP_ENTER_SIR: begin
						$display("Entering Shift-IR");
						state_en		<= 1;
						next_state		<= OP_SELECT_IR;
						proto_state		<= PROTO_STATE_WAIT;
					end	//end JTAGD_OP_ENTER_SIR

					JTAGD_OP_LEAVE_E1IR: begin
						$display("Leaving Exit1-IR");
						state_en		<= 1;
						next_state		<= OP_LEAVE_IR;
						proto_state		<= PROTO_STATE_WAIT;
					end	//end JTAGD_OP_LEAVE_E1IR

					JTAGD_OP_LEAVE_E1DR: begin
						$display("Leaving Exit1-DR");
						state_en		<= 1;
						next_state		<= OP_LEAVE_DR;
						proto_state		<= PROTO_STATE_WAIT;
					end	//end JTAGD_OP_LEAVE_E1DR

					JTAGD_OP_ENTER_SDR: begin
						$display("Entering Shift-DR");
						state_en		<= 1;
						next_state		<= OP_SELECT_DR;
						proto_state		<= PROTO_STATE_WAIT;
					end	//end JTAGD_OP_ENTER_SDR

					JTAGD_OP_RESET_IDLE: begin
						$display("Resetting to RTI");
						state_en		<= 1;
						next_state		<= OP_RESET_IDLE;
						proto_state		<= PROTO_STATE_WAIT;
					end	//end JTAGD_OP_RESET_IDLE

					JTAGD_OP_DUMMY_CLOCK: begin
						$display("Sending dummy clocks");
						$display("WARNING: dummy clocks not implemented");
					end	//end JTAGD_OP_DUMMY_CLOCK

					JTAGD_OP_QUIT: begin
						$display("Client disconnected");
						$fclose(readpipe);
						$fclose(writepipe);
						proto_state		<= PROTO_STATE_DONE;
					end

					default: begin
						$display("Command value %x not implemented", cmd_byte);
					end

				endcase

			end	//end PROTO_STATE_IDLE

			////////////////////////////////////////////////////////////////////////////////////////////////////////////
			// WAIT - sit around until the pending command executes (no readback)

			PROTO_STATE_WAIT: begin
				if(jtag_done)
					proto_state			<= PROTO_STATE_IDLE;
			end	//end PROTO_STATE_WAIT

			////////////////////////////////////////////////////////////////////////////////////////////////////////////
			// SHIFT - push data out through the JTAG interface

			PROTO_STATE_SHIFT: begin

				//Read a byte of data and send it out
				$fscanf(readpipe, "%x", shift_din);
				shift_en	<= 1;

				//Last block? Send actual length and final TMS value
				if(packet_len <= 8) begin
					last_tms	<= packet_last_tms;
					shift_len	<= packet_len;
					packet_len	<= 0;
				end

				//Nope, more bytes coming
				else begin
					last_tms	<= 0;
					shift_len	<= 8;
					packet_len	<= packet_len - 8;
				end

				proto_state	<= PROTO_STATE_SHIFT_WAIT;

			end	//end PROTO_STATE_SHIFT

			PROTO_STATE_SHIFT_WAIT: begin
				if(jtag_done) begin

					//Send the output value
					//NOTE: it's left justified! 1 bit is in [31] etc
					if(want_response)
						$fdisplay(writepipe, "%x", shift_dout_byte);

					if(packet_len == 0) begin
						proto_state			<= PROTO_STATE_IDLE;
						if(want_response)
							$fflush(writepipe);
					end
					else
						proto_state			<= PROTO_STATE_SHIFT;
				end
			end	//end PROTO_STATE_SHIFT_WAIT

			////////////////////////////////////////////////////////////////////////////////////////////////////////////
			// DONE - permanent shutdown of the pipe logic

			PROTO_STATE_DONE: begin
			end	//end PROTO_STATE_DONE

		endcase

	end

endmodule
