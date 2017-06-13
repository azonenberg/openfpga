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
	@brief Top level module for an XC2C-series device
 */
module XC2CDevice(
	jtag_tdi, jtag_tms, jtag_tck, jtag_tdo,
	dedicated_input, macrocell_io);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Device configuration

	parameter MACROCELLS = 32;		//A variant implied for 32/64, no support for base version

	parameter PACKAGE = "QFG32";	//Package code (lead-free G assumed)

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Helpers for figuring out dimensions etc of the device

	function integer ConfigMemoryWidth(integer cells)
		case(cells)
			32:			ConfigMemoryWidth <= 260;
			64:			ConfigMemoryWidth <= 274;
			128:		ConfigMemoryWidth <= 752;
			256:		ConfigMemoryWidth <= 1364;
			384:		ConfigMemoryWidth <= 1868;
			512:		ConfigMemoryWidth <= 1980;
			default:	ConfigMemoryWidth <= 0;
		endcase
	endfunction

	function integer ConfigMemoryDepth(integer cells)
		case(cells)
			32:			ConfigMemoryDepth <= 49;
			64:			ConfigMemoryDepth <= 97;
			128:		ConfigMemoryDepth <= 81;
			256:		ConfigMemoryDepth <= 97;
			384:		ConfigMemoryDepth <= 121;
			512:		ConfigMemoryDepth <= 161;
			default:	ConfigMemoryDepth <= 0;
		endcase
	endfunction

	localparam SHREG_WIDTH	= ConfigMemoryWidth(MACROCELLS);
	localparam MEM_DEPTH	= ConfigMemoryDepth(MACROCELLS);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// I/Os

	input wire					jtag_tdi;
	input wire					jtag_tms;
	input wire					jtag_tck;
	output wire					jtag_tdo;

	input wire					dedicated_input;	//only present in 32a

	inout wire[MACROCELLS-1:0]	macrocell_io;		//The actual device I/O pins.
													//Note that not all of these are broken out to bond pads;
													//buried macrocells drive a constant 0 here

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// The SRAM copy of the config bitstream (directly drives device behavior)

	reg[SHREG_WIDTH-1:0] ram_bitstream[MEM_DEPTH-1:0];

	integer i;
	initial begin
		for(i=0; i<MEM_DEPTH; i++)
			ram_bitstream[i] <= {SHREG_WIDTH{1'b1}};	//copied from blank EEPROM = all 1s
	end

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// The EEPROM copy of the config bitstream (used to configure ram_bitstream at startup)

	//TODO

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// JTAG stuff

	XC2CJTAG #(
		.MACROCELLS(MACROCELLS),
		.PACKAGE(PACKAGE)
	) jtag (
		.tdi(jtag_tdi),
		.tdo(jtag_tdo),
		.tms(jtag_tms),
		.tck(jtag_tck)
	);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// The actual CPLD function blocks

endmodule
