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

/**
	@brief Bitstream memory and read/write logic, plus parsing to shuffle it off to various peripherals
 */
module XC2CBitstream(
	jtag_tck,
	config_erase,
	config_read_en, config_read_addr, config_read_data,
	config_write_en, config_write_addr, config_write_data,
	left_zia_config, right_zia_config,
	left_and_config, right_and_config,
	left_or_config, right_or_config,
	left_mc_config, right_mc_config
	);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// I/O declarations

	parameter ADDR_BITS	= 0;
	parameter MEM_DEPTH = 0;
	parameter SHREG_WIDTH = 0;

	input wire						jtag_tck;

	input wire						config_erase;

	input wire						config_read_en;
	input wire[ADDR_BITS-1:0]		config_read_addr;
	output reg[SHREG_WIDTH-1:0]		config_read_data = 0;

	input wire						config_write_en;
	input wire[ADDR_BITS-1:0]		config_write_addr;
	input wire[SHREG_WIDTH-1:0]		config_write_data;

	output reg[40*8-1:0]			left_zia_config;
	output reg[40*8-1:0]			right_zia_config;

	output reg[80*56-1:0]			left_and_config;
	output reg[80*56-1:0]			right_and_config;

	output reg[16*56-1:0]			left_or_config;
	output reg[16*56-1:0]			right_or_config;

	output reg[27*16-1:0]			left_mc_config;
	output reg[27*16-1:0]			right_mc_config;

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// The SRAM copy of the config bitstream (directly drives device behavior)

	reg[SHREG_WIDTH-1:0] ram_bitstream[MEM_DEPTH-1:0];

	/*
		Row configuration, left to right:
		1		259			Transfer bit (ignored)
		9		258:250		FB2 macrocells
		112		249:138		FB2 PLA
		16		137:122		ZIA (interleaved)
		112		121:10		FB1 PLA
		9		9:1			FB1 macrocells
		1		0			Transfer bit (ignored)
	 */

	integer row;
	initial begin
		for(row=0; row<MEM_DEPTH; row=row+1)
			ram_bitstream[row] <= {SHREG_WIDTH{1'b1}};	//copied from blank EEPROM = all 1s
	end

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// The EEPROM copy of the config bitstream (used to configure ram_bitstream at startup)

	//TODO

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// JTAG access

	//Read/write the EEPROM
	//TODO: add read enable?
	always @(posedge jtag_tck) begin

		if(config_read_en)
			config_read_data <= ram_bitstream[config_read_addr];

		if(config_write_en)
			ram_bitstream[config_write_addr]	<= config_write_data;

		//Wipe the config memory
		//TODO: go multicycle?
		//If we go multicycle, how do we handle this with no clock? Real chip is self-timed internally
		if(config_erase) begin
			for(row=0; row<MEM_DEPTH; row=row+1)
				ram_bitstream[row] <= {SHREG_WIDTH{1'b1}};
		end

	end

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Shuffle the bitstream out to various IP blocks

	integer nbit;
	integer nterm;
	integer toprow;
	integer orow;
	integer mcell;
	integer mcblock;

	always @(*) begin
		for(row=0; row<48; row=row+1) begin

			toprow = row - 8;
			orow = row - 20;
			mcell = row / 3;
			mcblock = row % 3;

			//Rows 0..19: 	MC-AND-ZIA-AND-MC
			//Rows 20...27: MC-OR--GLB-OR--MC
			//Rows 28..47: 	MC-AND-ZIA-AND-MC

			//The ZIA is bits 137:122
			//MSB is FB1, next is FB2.
			//We have stuff at the top and bottom of array, with global config in the middle
			if(row > 27) begin
				for(nbit=0; nbit<8; nbit=nbit+1) begin
					right_zia_config[toprow*8 + nbit]			<= ram_bitstream[toprow][123 + nbit*2];
					left_zia_config[toprow*8 + nbit]			<= ram_bitstream[toprow][122 + nbit*2];
				end
			end
			else if(row < 20) begin
				for(nbit=0; nbit<8; nbit=nbit+1) begin
					right_zia_config[row*8 + nbit]				<= ram_bitstream[row][123 + nbit*2];
					left_zia_config[row*8 + nbit]				<= ram_bitstream[row][122 + nbit*2];
				end
			end

			//We have PLA AND stuff at the top and bottom of array, with OR array in the middle
			//Each row is two bits from PT0, two from PT1, two from PT2, etc
			//Right side: 249:138 (mirrored)
			//Left side: 121:10
			if(row > 27) begin
				for(nterm=0; nterm<56; nterm=nterm+1) begin
					right_and_config[nterm*80 + toprow*2 + 0] 	<= ram_bitstream[toprow][249 - nterm*2 - 1];
					right_and_config[nterm*80 + toprow*2 + 1] 	<= ram_bitstream[toprow][249 - nterm*2 - 0];

					left_and_config[nterm*80 + toprow*2 + 0]	 <= ram_bitstream[toprow][10 + nterm*2 + 0];
					left_and_config[nterm*80 + toprow*2 + 1] 	<= ram_bitstream[toprow][10 + nterm*2 + 1];
				end
			end

			else if(row < 20) begin
				for(nterm=0; nterm<56; nterm=nterm+1) begin
					right_and_config[nterm*80 + row*2 + 0]		<= ram_bitstream[row][249 - nterm*2 - 1];
					right_and_config[nterm*80 + row*2 + 1] 		<= ram_bitstream[row][249 - nterm*2 - 0];

					left_and_config[nterm*80 + row*2 + 0] 		<= ram_bitstream[row][10 + nterm*2 + 0];
					left_and_config[nterm*80 + row*2 + 1] 		<= ram_bitstream[row][10 + nterm*2 + 1];
				end
			end

			//PLA OR array
			//One bit per product term, two OR terms per row
			if( (row >= 20) && (row <= 27) ) begin
				for(nterm=0; nterm<56; nterm=nterm+1) begin
					right_or_config[(orow*2)*56 + nterm]		<= ram_bitstream[orow][249 - nterm*2 + 0];
					right_or_config[(orow*2+1)*56 + nterm]		<= ram_bitstream[orow][249 - nterm*2 + 1];

					left_or_config[(orow*2)*56 + nterm]			<= ram_bitstream[orow][249 - nterm*2 + 0];
					left_or_config[(orow*2+1)*56 + nterm]		<= ram_bitstream[orow][249 - nterm*2 + 1];
				end
			end

			//Macrocells
			//9 bits per row, takes 3 rows to provision one macrocell
			//Right side: 258:250 (mirrored)
			//Left side: 9:1
			for(nbit=0; nbit<9; nbit=nbit+1) begin
				left_mc_config[mcell*27 + (2 - mcblock)*9 + nbit]	<= ram_bitstream[row][9 - nbit];
				right_mc_config[mcell*27 + (2 - mcblock)*9 + nbit]	<= ram_bitstream[row][250 + nbit];
			end

		end

		//TODO: read row 48 (SEC/done) and 49 (usercode)
	end

endmodule
