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
	@brief Global routing fabric - the "Zero Power Interconnect Array"
 */
module XC2CZIA(
	dedicated_in, ibuf_in, macrocell_in,
	zia_out,
	config_bits);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Device configuration

	parameter MACROCELLS = 32;		//A variant implied for 32/64, no support for base version

	initial begin
		if(MACROCELLS != 32) begin
			$display("ZIA not implemented for other device densities");
			$finish;
		end
	end

	localparam NUM_IBUFS	= 32;	//TODO: function or something
	localparam NUM_MCELLS	= 32;

	localparam BITS_PER_ROW	= 8;

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Inputs

	input wire 							dedicated_in;		//only present in 32a

	input wire[NUM_IBUFS-1:0]			ibuf_in;			//Inputs (from flipflop or input buffer)
															//{fb2, fb1}
	input wire[NUM_MCELLS-1:0]			macrocell_in;		//Inputs (from XOR gate or flipflop)
															//{fb2, fb1}

	output reg[39:0]					zia_out;			//40 outputs to the function block

	input wire[40 * BITS_PER_ROW-1 : 0]	config_bits;		//The actual config bitstream for this ZIA block

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Reshuffle the bitstream into proper rows

	reg[BITS_PER_ROW-1:0] row_config[39:0];

	integer i;
	always @(*) begin
		for(i=0; i<40; i=i+1)
			row_config[i]	<= config_bits[i*BITS_PER_ROW +: BITS_PER_ROW];
	end

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Inputs to the ROM muxes

	/*
		64:54	First group (left side, next to VCCINT rail)
		53:43	Second group
		42:32	Third group
		31:21	Fourth group
		20:10	Fifth group
		9:0		Sixth group (rightmost)
	 */
	wire[65:0] zbus =
	{
		macrocell_in,
		ibuf_in[31:16],
		dedicated_in,
		ibuf_in[15:0]
	};

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// The ROM muxes (M3-M4 vias)

	//This is for the XC2C32A only
	//TODO: do other densities
	wire[5:0] zia_row_inputs[39:0];
	assign zia_row_inputs[0]	=	{zbus[58], zbus[46], zbus[34], zbus[22], zbus[10], zbus[0]};
	assign zia_row_inputs[1]	=	{zbus[61], zbus[48], zbus[41], zbus[23], zbus[11], zbus[1]};
	assign zia_row_inputs[2]	=	{zbus[60], zbus[53], zbus[35], zbus[30], zbus[12], zbus[2]};
	assign zia_row_inputs[3]	=	{zbus[55], zbus[47], zbus[42], zbus[26], zbus[13], zbus[3]};
	assign zia_row_inputs[4]	=	{zbus[59], zbus[44], zbus[38], zbus[28], zbus[14], zbus[4]};
	assign zia_row_inputs[5]	=	{zbus[56], zbus[50], zbus[40], zbus[31], zbus[15], zbus[5]};
	assign zia_row_inputs[6]	=	{zbus[62], zbus[52], zbus[33], zbus[21], zbus[16], zbus[6]};
	assign zia_row_inputs[7]	=	{zbus[64], zbus[45], zbus[32], zbus[27], zbus[17], zbus[7]};
	assign zia_row_inputs[8]	=	{zbus[57], zbus[43], zbus[39], zbus[25], zbus[18], zbus[8]};
	assign zia_row_inputs[9]	=	{zbus[54], zbus[51], zbus[37], zbus[24], zbus[19], zbus[9]};
	assign zia_row_inputs[10]	=	{zbus[63], zbus[49], zbus[36], zbus[29], zbus[20], zbus[7]};
	assign zia_row_inputs[11]	=	{zbus[59], zbus[47], zbus[35], zbus[23], zbus[11], zbus[0]};
	assign zia_row_inputs[12]	=	{zbus[64], zbus[50], zbus[37], zbus[30], zbus[12], zbus[1]};
	assign zia_row_inputs[13]	=	{zbus[62], zbus[49], zbus[42], zbus[24], zbus[19], zbus[2]};
	assign zia_row_inputs[14]	=	{zbus[61], zbus[44], zbus[36], zbus[31], zbus[15], zbus[3]};
	assign zia_row_inputs[15]	=	{zbus[56], zbus[48], zbus[33], zbus[27], zbus[17], zbus[4]};
	assign zia_row_inputs[16]	=	{zbus[60], zbus[45], zbus[39], zbus[29], zbus[20], zbus[5]};
	assign zia_row_inputs[17]	=	{zbus[57], zbus[51], zbus[41], zbus[22], zbus[10], zbus[6]};
	assign zia_row_inputs[18]	=	{zbus[63], zbus[53], zbus[34], zbus[21], zbus[16], zbus[7]};
	assign zia_row_inputs[19]	=	{zbus[55], zbus[46], zbus[32], zbus[28], zbus[14], zbus[8]};
	assign zia_row_inputs[20]	=	{zbus[58], zbus[43], zbus[40], zbus[26], zbus[13], zbus[9]};
	assign zia_row_inputs[21]	=	{zbus[54], zbus[52], zbus[38], zbus[25], zbus[18], zbus[8]};
	assign zia_row_inputs[22]	=	{zbus[60], zbus[48], zbus[36], zbus[24], zbus[12], zbus[0]};
	assign zia_row_inputs[23]	=	{zbus[54], zbus[53], zbus[39], zbus[26], zbus[19], zbus[1]};
	assign zia_row_inputs[24]	=	{zbus[55], zbus[51], zbus[38], zbus[31], zbus[13], zbus[2]};
	assign zia_row_inputs[25]	=	{zbus[63], zbus[50], zbus[33], zbus[25], zbus[20], zbus[3]};
	assign zia_row_inputs[26]	=	{zbus[62], zbus[45], zbus[37], zbus[22], zbus[16], zbus[4]};
	assign zia_row_inputs[27]	=	{zbus[57], zbus[49], zbus[34], zbus[28], zbus[18], zbus[5]};
	assign zia_row_inputs[28]	=	{zbus[61], zbus[46], zbus[40], zbus[30], zbus[11], zbus[6]};
	assign zia_row_inputs[29]	=	{zbus[58], zbus[52], zbus[42], zbus[23], zbus[10], zbus[7]};
	assign zia_row_inputs[30]	=	{zbus[64], zbus[44], zbus[35], zbus[21], zbus[17], zbus[8]};
	assign zia_row_inputs[31]	=	{zbus[56], zbus[47], zbus[32], zbus[29], zbus[15], zbus[9]};
	assign zia_row_inputs[32]	=	{zbus[59], zbus[43], zbus[41], zbus[27], zbus[14], zbus[9]};
	assign zia_row_inputs[33]	=	{zbus[61], zbus[49], zbus[37], zbus[25], zbus[13], zbus[0]};
	assign zia_row_inputs[34]	=	{zbus[60], zbus[43], zbus[42], zbus[28], zbus[15], zbus[1]};
	assign zia_row_inputs[35]	=	{zbus[54], zbus[44], zbus[40], zbus[27], zbus[20], zbus[2]};
	assign zia_row_inputs[36]	=	{zbus[56], zbus[52], zbus[39], zbus[22], zbus[14], zbus[3]};
	assign zia_row_inputs[37]	=	{zbus[64], zbus[51], zbus[34], zbus[26], zbus[11], zbus[4]};
	assign zia_row_inputs[38]	=	{zbus[63], zbus[46], zbus[38], zbus[23], zbus[17], zbus[5]};
	assign zia_row_inputs[39]	=	{zbus[58], zbus[50], zbus[35], zbus[29], zbus[19], zbus[6]};

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// The SRAM muxes

	always @(*) begin

		for(i=0; i<40; i=i+1) begin

			//The real silicon uses a tristate mux so bus fights are possible if multiple bits are asserted.
			//For the FPGA model, use an if/else cascade to resolve ambiguity (rather than bus fight)
			//Note that bit 7 is active high but the rest are active low.
			//This is done to ensure that a blank bitstream (all 1s) puts the chip in a sane state.
			//TODO: model OGATE properly during the config step?
			if(row_config[i][7])
				zia_out[i]	<= 1'b1;
			else if(!row_config[i][6])
				zia_out[i]	<= 1'b0;
			else if(!row_config[i][5])
				zia_out[i]	<= zia_row_inputs[i][5];
			else if(!row_config[i][4])
				zia_out[i]	<= zia_row_inputs[i][4];
			else if(!row_config[i][3])
				zia_out[i]	<= zia_row_inputs[i][3];
			else if(!row_config[i][2])
				zia_out[i]	<= zia_row_inputs[i][2];
			else if(!row_config[i][1])
				zia_out[i]	<= zia_row_inputs[i][1];
			else if(!row_config[i][0])
				zia_out[i]	<= zia_row_inputs[i][0];

			//Default to 0 if no bits are active.
			//The real silicon has a weak keeper circuit for this
			else
				zia_out[i]	<= 0;

		end

	end

endmodule
