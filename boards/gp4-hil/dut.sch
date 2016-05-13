EESchema Schematic File Version 2
LIBS:analog-azonenberg
LIBS:cmos
LIBS:cypress-azonenberg
LIBS:hirose-azonenberg
LIBS:memory-azonenberg
LIBS:microchip-azonenberg
LIBS:osc-azonenberg
LIBS:passive-azonenberg
LIBS:power-azonenberg
LIBS:special-azonenberg
LIBS:xilinx-azonenberg
LIBS:conn
LIBS:device
LIBS:gp4-hil-cache
EELAYER 25 0
EELAYER END
$Descr A3 16535 11693
encoding utf-8
Sheet 5 7
Title "GreenPak Hardware-In-Loop Test Platform"
Date "2016-05-11"
Rev "0.1"
Comp "Andrew Zonenberg"
Comment1 ""
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
$Comp
L CONN_01X20 P1
U 1 1 5731797F
P 1400 1900
F 0 "P1" H 1478 1938 50  0000 L CNN
F 1 "CONN_01X20" H 1478 1846 50  0000 L CNN
F 2 "" H 1400 1900 60  0000 C CNN
F 3 "" H 1400 1900 60  0000 C CNN
	1    1400 1900
	-1   0    0    -1  
$EndComp
Text HLabel 1900 950  2    60   Input ~ 0
DUT_VDD1
Wire Wire Line
	1900 950  1600 950 
Wire Wire Line
	1600 1050 2400 1050
Text HLabel 2400 4850 2    60   Input ~ 0
DUT_VPP
Text HLabel 2350 4600 2    60   Input ~ 0
DUT_VDD2
Text HLabel 1900 1950 2    60   Input ~ 0
GND
Text Notes 2150 4200 0    60   ~ 0
TODO: Pass transistor to enable DUT_VDD2
Text Notes 2150 4300 0    60   ~ 0
TODO: Buffered indicator LEDs
Wire Wire Line
	1900 1950 1600 1950
Wire Wire Line
	1600 1150 2400 1150
Wire Wire Line
	2400 1250 1600 1250
Wire Wire Line
	1600 1350 2400 1350
Wire Wire Line
	2400 1450 1600 1450
Wire Wire Line
	1600 1550 2400 1550
Wire Wire Line
	2400 1650 1600 1650
Wire Wire Line
	1600 1750 2400 1750
Wire Wire Line
	2400 1850 1600 1850
Wire Wire Line
	2400 2050 1600 2050
Wire Wire Line
	1600 2150 2400 2150
Wire Wire Line
	2400 2250 1600 2250
Wire Wire Line
	1600 2350 2400 2350
Wire Wire Line
	2400 2450 1600 2450
Wire Wire Line
	1600 2550 2400 2550
Wire Wire Line
	2400 2650 1600 2650
Wire Wire Line
	2400 2750 1600 2750
Wire Wire Line
	1600 2850 2400 2850
Text Notes 2150 4000 0    60   ~ 0
Total number of switches needed:\n* 18 GPIO -> FPGA\n* 15 GPIO -> DAC\n* 2 Vdd -> regulator\n\nADC is always connected\n\nNeed >8V compatible switch (in open state) for FPGA->GPIO2
$Comp
L TS3A4751 U1
U 1 1 5732B5B9
P 1700 6000
F 0 "U1" H 2300 6050 60  0000 C CNN
F 1 "TS3A4751" H 1900 5950 60  0000 C CNN
F 2 "" H 1700 6000 60  0000 C CNN
F 3 "" H 1700 6000 60  0000 C CNN
	1    1700 6000
	1    0    0    -1  
$EndComp
$Comp
L TS3A4751 U1
U 2 1 5732C1D7
P 1700 6600
F 0 "U1" H 2300 6650 60  0000 C CNN
F 1 "TS3A4751" H 1900 6550 60  0000 C CNN
F 2 "" H 1700 6600 60  0000 C CNN
F 3 "" H 1700 6600 60  0000 C CNN
	2    1700 6600
	1    0    0    -1  
$EndComp
$Comp
L TS3A4751 U1
U 3 1 5732C21D
P 3200 6000
F 0 "U1" H 3800 6050 60  0000 C CNN
F 1 "TS3A4751" H 3400 5950 60  0000 C CNN
F 2 "" H 3200 6000 60  0000 C CNN
F 3 "" H 3200 6000 60  0000 C CNN
	3    3200 6000
	1    0    0    -1  
$EndComp
$Comp
L TS3A4751 U1
U 4 1 5732C2B5
P 3200 6600
F 0 "U1" H 3800 6650 60  0000 C CNN
F 1 "TS3A4751" H 3400 6550 60  0000 C CNN
F 2 "" H 3200 6600 60  0000 C CNN
F 3 "" H 3200 6600 60  0000 C CNN
	4    3200 6600
	1    0    0    -1  
$EndComp
$Comp
L TS3A4751 U4
U 4 1 5732C31D
P 4300 6600
F 0 "U4" H 4900 6650 60  0000 C CNN
F 1 "TS3A4751" H 4500 6550 60  0000 C CNN
F 2 "" H 4300 6600 60  0000 C CNN
F 3 "" H 4300 6600 60  0000 C CNN
	4    4300 6600
	1    0    0    -1  
$EndComp
Text HLabel 2400 1050 2    60   BiDi ~ 0
DUT_GPIO2
Text HLabel 2400 1150 2    60   BiDi ~ 0
DUT_GPIO3
Text HLabel 2400 1250 2    60   BiDi ~ 0
DUT_GPIO4
Text HLabel 2400 1350 2    60   BiDi ~ 0
DUT_GPIO5
Text HLabel 2400 1450 2    60   BiDi ~ 0
DUT_GPIO6
Text HLabel 2400 1550 2    60   BiDi ~ 0
DUT_GPIO7
Text HLabel 2400 1650 2    60   BiDi ~ 0
DUT_GPIO8
Text HLabel 2400 1750 2    60   BiDi ~ 0
DUT_GPIO9
Text HLabel 2400 1850 2    60   BiDi ~ 0
DUT_GPIO10
Text HLabel 2400 2050 2    60   BiDi ~ 0
DUT_GPIO12
Text HLabel 2400 2150 2    60   BiDi ~ 0
DUT_GPIO13
Text HLabel 2400 2250 2    60   BiDi ~ 0
DUT_GPIO14
Text HLabel 2400 2350 2    60   BiDi ~ 0
DUT_GPIO15
Text HLabel 2400 2450 2    60   BiDi ~ 0
DUT_GPIO16
Text HLabel 2400 2550 2    60   BiDi ~ 0
DUT_GPIO17
Text HLabel 2400 2650 2    60   BiDi ~ 0
DUT_GPIO18
Text HLabel 2400 2750 2    60   BiDi ~ 0
DUT_GPIO19
Text HLabel 2400 2850 2    60   BiDi ~ 0
DUT_GPIO20
$EndSCHEMATC
