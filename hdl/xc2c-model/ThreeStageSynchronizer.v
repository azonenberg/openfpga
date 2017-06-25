`timescale 1ns / 1ps
/***********************************************************************************************************************
*                                                                                                                      *
* ANTIKERNEL v0.1                                                                                                      *
*                                                                                                                      *
* Copyright (c) 2012-2017 Andrew D. Zonenberg                                                                          *
* All rights reserved.                                                                                                 *
*                                                                                                                      *
* Redistribution and use in source and binary forms, with or without modification, are permitted provided that the     *
* following conditions are met:                                                                                        *
*                                                                                                                      *
*    * Redistributions of source code must retain the above copyright notice, this list of conditions, and the         *
*      following disclaimer.                                                                                           *
*                                                                                                                      *
*    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the       *
*      following disclaimer in the documentation and/or other materials provided with the distribution.                *
*                                                                                                                      *
*    * Neither the name of the author nor the names of any contributors may be used to endorse or promote products     *
*      derived from this software without specific prior written permission.                                           *
*                                                                                                                      *
* THIS SOFTWARE IS PROVIDED BY THE AUTHORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED   *
* TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL *
* THE AUTHORS BE HELD LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES        *
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR       *
* BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT *
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE       *
* POSSIBILITY OF SUCH DAMAGE.                                                                                          *
*                                                                                                                      *
***********************************************************************************************************************/

/**
	@file
	@author Andrew D. Zonenberg
	@brief Three-stage flipflop-based synchronizer
 */
module ThreeStageSynchronizer(
	input wire clk_in,
	input wire din,
	input wire clk_out,
	output wire dout
    );

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// The flipflops

	//Set false to skip the first FF stage (if input is already registered, or totally async)
	parameter IN_REG = 1;

	wire dout0;			//The cross-clock path
	wire dout1;
	wire dout2;

	//First stage: either a FF in the transmitting clock domain, or nothing
	generate

		if(!IN_REG)
			assign dout0 = din;
		else
			FDCE stage0 (.Q(dout0), .C(clk_in),  .CE(1'b1), .CLR(1'b0), .D(din));

	endgenerate

	//Three stages in the receiving clock domain
	(* RLOC="X0Y0" *) FDCE stage1 (.Q(dout1), .C(clk_out), .CE(1'b1), .CLR(1'b0), .D(dout0));
	(* RLOC="X0Y0" *) FDCE stage2 (.Q(dout2), .C(clk_out), .CE(1'b1), .CLR(1'b0), .D(dout1));
	(* RLOC="X0Y0" *) FDCE stage3 (.Q(dout ), .C(clk_out), .CE(1'b1), .CLR(1'b0), .D(dout2));

endmodule
