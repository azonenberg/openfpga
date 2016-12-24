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

/**
	OUTPUTS:
		TODO

	TEST PROCEDURE:
		TODO
 */
module SPIToDCMP(spi_sck, spi_cs_n, spi_mosi, spi_int, dcmp_greater, dcmp_equal);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// I/O declarations

	(* LOC = "P8" *)
	input wire spi_sck;

	(* LOC = "P9" *)
	input wire spi_cs_n;

	(* LOC = "P10" *)
	input wire spi_mosi;

	(* LOC = "P7" *)
	output wire spi_int;

	(* LOC = "P20" *)
	output wire dcmp_greater;

	(* LOC = "P19" *)
	output wire dcmp_equal;

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// System reset stuff

	//Power-on reset
	wire por_done;
	GP_POR #(
		.POR_TIME(500)
	) por (
		.RST_DONE(por_done)
	);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// RC oscillator clock and buffers

	wire clk_2mhz;
	GP_RCOSC #(
		.PWRDN_EN(0),
		.AUTO_PWRDN(0),
		.OSC_FREQ("2M"),
		.HARDIP_DIV(1),
		.FABRIC_DIV(1)
	) rcosc (
		.PWRDN(1'b0),
		.CLKOUT_HARDIP(clk_2mhz),
		.CLKOUT_FABRIC()
	);

	wire clk_2mhz_buf;
	GP_CLKBUF clkbuf_rcosc (
		.IN(clk_2mhz),
		.OUT(clk_2mhz_buf));

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// SPI interface

	wire spi_sck_buf;
	GP_CLKBUF clkbuf_sck(
		.IN(spi_sck),
		.OUT(spi_sck_buf));

	wire[7:0] rxd_low;
	wire[7:0] rxd_high;
	GP_SPI #(
		.DATA_WIDTH(16),
		.SPI_CPHA(0),
		.SPI_CPOL(0),
		.DIRECTION("INPUT")
	) spi (
		.SCK(spi_sck_buf),
		.SDAT(spi_mosi),
		.CSN(spi_cs_n),
		.TXD_HIGH(8'h00),
		.TXD_LOW(8'h00),
		.RXD_HIGH(rxd_high),
		.RXD_LOW(rxd_low),
		.INT(spi_int)
	);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// The DCMP

	wire[7:0] ref0;
	GP_DCMPREF #(
		.REF_VAL(8'h40)
	) dcref (
		.OUT(ref0)
	);

	GP_DCMP #(
		.GREATER_OR_EQUAL(1'b0),
		.CLK_EDGE("RISING"),
		.PWRDN_SYNC(1'b1)
	) dcmp(
		.INP(rxd_high),
		.INN(ref0),
		.CLK(clk_2mhz_buf),
		.PWRDN(1'b0),
		.GREATER(dcmp_greater),
		.EQUAL(dcmp_equal)
	);

endmodule
