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
	output reg		tck = 0,
	output reg		tdi = 0,
	output reg		tms = 0,
	input wire		tdo
	);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Opcode values

	//The canonical copy of these is in jtaghal/jtagd_opcodes.yml but we're not building with Splash
	//For now, replicate by hand
	localparam JTAGD_OP_GET_NAME 	= 8'h00;
	localparam JTAGD_OP_GET_SERIAL	= 8'h01;
	localparam JTAGD_OP_GET_USERID	= 8'h02;

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Adapter metadata

	localparam ADAPTER_NAME = "JtagPipeBridge";
	localparam ADAPTER_SERIAL = "NoSerialYet";
	localparam ADAPTER_USERID = "NoUserIDYet";

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// GSR wait

	integer ready = 0;
	initial begin
		#100;
		ready = 1;
	end

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

	reg[7:0] cmd_byte;
	always begin

		if(ready) begin

			//Read a command byte from the pipe
			$display("Reading opcode...");
			if(1 != $fscanf(readpipe, "%x", cmd_byte)) begin
				$display("Read failed");
				$finish;
			end
			$display("Read command %x", cmd_byte);

			//See what the command is
			case(cmd_byte)

				JTAGD_OP_GET_NAME: begin
					$fdisplay(writepipe, "%s", ADAPTER_NAME);
					$fflush(writepipe);
				end	//end JTAGD_OP_GET_NAME

				JTAGD_OP_GET_SERIAL: begin
					$fdisplay(writepipe, "%s", ADAPTER_SERIAL);
					$fflush(writepipe);
				end	//end JTAGD_OP_GET_SERIAL

				JTAGD_OP_GET_USERID: begin
					$fdisplay(writepipe, "%s", ADAPTER_USERID);
					$fflush(writepipe);
				end	//end JTAGD_OP_GET_USERID

			endcase

		end

		//Wait 5 ns to force sim time to advance
		#5;

	end

	//Cleanup
	initial begin
		#1000;
		$fclose(readpipe);
		$fclose(writepipe);
		$finish;
	end

endmodule
