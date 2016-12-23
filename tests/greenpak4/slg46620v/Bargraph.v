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
	INPUTS:
		Analog "vin" on pin 6

	OUTPUTS:
		Bandgap OK on pin 20 (should be high after reset)
		800 mV reference on pin 19
		600 mV reference on pin 18

		Comparator output on pin 12, true if vin > 200 mV
		Comparator output on pin 13, true if vin > 400 mV
		Comparator output on pin 14, true if vin > 600 mV
		Comparator output on pin 15, true if vin > 800 mV
		Comparator output on pin 16, true if vin > 1000 mV

		Comparator output on pin 17, true if vin2 > 1000 mV

	TEST PROCEDURE:

		Pin 6: < 200 mV

		Expected:
			Pin 20: digital high
			Pin 19: 800 mV
			Pin 18: 600 mV

			Pin 17: digital low
			Pin 16: digital low
			Pin 15: digital low
			Pin 14: digital low
			Pin 13: digital low
			Pin 12: digital low

		Sweep pin 6 up to 1100 mV and back.
		Pins 12-16 should light up in a bargraph display with 200 mV per step.

		Pin 17 should be high iff pin 4 is > 1000 mV
 */
module Bargraph(bg_ok, vref_800, vref_600, vin, vin2, cout1, cout2, cout3, cout4, cout5, cout6);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// I/O declarations

	(* LOC = "P20" *)
	output wire bg_ok;

	(* LOC = "P19" *)
	(* IBUF_TYPE = "ANALOG" *)
	output wire vref_800;

	(* LOC = "P18" *)
	(* IBUF_TYPE = "ANALOG" *)
	output wire vref_600;

	(* LOC = "P6" *)
	(* IBUF_TYPE = "ANALOG" *)
	input wire vin;

	(* LOC = "P4" *)
	(* IBUF_TYPE = "ANALOG" *)
	input wire vin2;

	(* LOC = "P12" *)
	output wire cout1;

	(* LOC = "P13" *)
	output wire cout2;

	(* LOC = "P14" *)
	output wire cout3;

	(* LOC = "P15" *)
	output wire cout4;

	(* LOC = "P16" *)
	output wire cout5;

	(* LOC = "P17" *)
	output wire cout6;

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
	// Analog buffer on the input voltage to reduce loading since we feed it to a couple of comparators

	wire vin_buf;
	GP_ABUF #(
		.BANDWIDTH_KHZ(5)
	) abuf (
		.IN(vin),
		.OUT(vin_buf)
	);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// 1.0V bandgap voltage reference (used by a lot of the mixed signal IP)

	GP_BANDGAP #(
		.AUTO_PWRDN(0),
		.CHOPPER_EN(1),
		.OUT_DELAY(550)
	) bandgap (
		.OK(bg_ok)
	);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Voltage references for the five bargraph steps

	wire vref_200;
	wire vref_400;
	wire vref_1000;

	GP_VREF #( .VIN_DIV(4'd1), .VREF(16'd200)  ) vr200  ( .VIN(1'b0), .VOUT(vref_200)  );
	GP_VREF #( .VIN_DIV(4'd1), .VREF(16'd400)  ) vr400  ( .VIN(1'b0), .VOUT(vref_400)  );
	GP_VREF #( .VIN_DIV(4'd1), .VREF(16'd600)  ) vr600  ( .VIN(1'b0), .VOUT(vref_600)  );
	GP_VREF #( .VIN_DIV(4'd1), .VREF(16'd800)  ) vr800  ( .VIN(1'b0), .VOUT(vref_800)  );
	GP_VREF #( .VIN_DIV(4'd1), .VREF(16'd1000) ) vr1000 ( .VIN(1'b0), .VOUT(vref_1000) );

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Comparator bank

	GP_ACMP #(.BANDWIDTH("LOW"), .VIN_ATTEN(4'd1), .VIN_ISRC_EN(1'b0), .HYSTERESIS(8'd25) )
		cmp_200 (.PWREN(por_done), .OUT(cout1), .VIN(vin_buf), .VREF(vref_200) );

	GP_ACMP #(.BANDWIDTH("LOW"), .VIN_ATTEN(4'd1), .VIN_ISRC_EN(1'b0), .HYSTERESIS(8'd25) )
		cmp_400 (.PWREN(por_done), .OUT(cout2), .VIN(vin_buf), .VREF(vref_400) );

	GP_ACMP #(.BANDWIDTH("LOW"), .VIN_ATTEN(4'd1), .VIN_ISRC_EN(1'b0), .HYSTERESIS(8'd25) )
		cmp_600 (.PWREN(por_done), .OUT(cout3), .VIN(vin_buf), .VREF(vref_600) );

	GP_ACMP #(.BANDWIDTH("LOW"), .VIN_ATTEN(4'd1), .VIN_ISRC_EN(1'b0), .HYSTERESIS(8'd25) )
		cmp_800 (.PWREN(por_done), .OUT(cout4), .VIN(vin_buf), .VREF(vref_800) );

	GP_ACMP #(.BANDWIDTH("LOW"), .VIN_ATTEN(4'd1), .VIN_ISRC_EN(1'b0), .HYSTERESIS(8'd25) )
		cmp_1000a (.PWREN(por_done), .OUT(cout5), .VIN(vin_buf), .VREF(vref_1000) );

	//Last comparator has to have different input
	//as acmp0 input is not routable elsewhere
	GP_ACMP #(.BANDWIDTH("LOW"), .VIN_ATTEN(4'd1), .VIN_ISRC_EN(1'b0), .HYSTERESIS(8'd25) )
		cmp_1000b (.PWREN(por_done), .OUT(cout6), .VIN(vin2), .VREF(vref_1000) );

endmodule
