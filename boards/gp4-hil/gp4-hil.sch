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
LIBS:gp4-hil-cache
EELAYER 25 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 1 7
Title "GreenPak Hardware-In-Loop Test Platform"
Date "2016-05-10"
Rev "0.1"
Comp "Andrew Zonenberg"
Comment1 "Top level"
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
$Sheet
S 1500 1200 1050 1250
U 57316A38
F0 "Power supply" 60
F1 "psu.sch" 60
F2 "DUT_VPP" O R 2550 1350 60 
F3 "2V5" O R 2550 1750 60 
F4 "1V8" O R 2550 1850 60 
F5 "1V2" O R 2550 1950 60 
F6 "1V0" O R 2550 2050 60 
F7 "GND" O R 2550 2250 60 
F8 "DUT_VDD1" O R 2550 1450 60 
F9 "DUT_VDD2" O R 2550 1550 60 
$EndSheet
$Sheet
S 3000 1200 900  1250
U 57316A40
F0 "Ethernet" 60
F1 "ethernet.sch" 60
$EndSheet
$Sheet
S 4600 1200 1100 1250
U 57316A4B
F0 "JTAG masters" 60
F1 "jtag-masters.sch" 60
$EndSheet
$Sheet
S 6600 1200 950  1250
U 57316A58
F0 "DUT" 60
F1 "dut.sch" 60
F2 "DUT_VDD1" I L 6600 1450 60 
F3 "DUT_VPP" I L 6600 1350 60 
F4 "DUT_VDD2" I L 6600 1550 60 
F5 "GND" I L 6600 2250 60 
$EndSheet
$Sheet
S 8300 1200 900  1250
U 57316A68
F0 "Analog IO" 60
F1 "analog-io.sch" 60
$EndSheet
$Sheet
S 8300 3100 900  1350
U 57316B0C
F0 "FPGA support" 60
F1 "fpga-support.sch" 60
$EndSheet
$EndSCHEMATC
