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
$Descr A2 23386 16535
encoding utf-8
Sheet 2 7
Title "GreenPak Hardware-In-Loop Test Platform"
Date "2016-05-13"
Rev "0.1"
Comp "Andrew Zonenberg"
Comment1 "Power regulation"
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
Text Notes 5200 3450 0    60   ~ 0
Low current rails:\n* 1V25 precision reference for XADC\n* 7V5 Vpp\n* 3V3_DUT from LDO\n* 1V8_DUT from LDO\n\nHigh current rails (LTC3374):\n* 2V5 PHY analog\n* 1V8 FPGA VCCAUX/VCCXADC, PHY I/O\n* 1V2 PHY analog/digital core\n* 1V0 FPGA VCCINT/VCCBRAM\n\nVariable rails:\n* DUT_VDD1/DUT_VDD2 muxed from 1V8/3V3_DUT\n\nInput: Unregulated 5V DC
$Comp
L LTC3374-QFN U3
U 1 1 5732D550
P 2550 4200
F 0 "U3" H 3477 4511 60  0000 L CNN
F 1 "LTC3374-QFN" H 3477 4405 60  0000 L CNN
F 2 "" H 2550 4200 60  0000 C CNN
F 3 "" H 2550 4200 60  0000 C CNN
	1    2550 4200
	1    0    0    -1  
$EndComp
Text HLabel 8000 4650 0    60   Output ~ 0
DUT_VPP
Text HLabel 8000 4000 0    60   Output ~ 0
2V5
Text HLabel 8000 4100 0    60   Output ~ 0
1V8
Text HLabel 8000 4200 0    60   Output ~ 0
1V2
Text HLabel 8000 4300 0    60   Output ~ 0
1V0
Text HLabel 8000 5150 0    60   Output ~ 0
GND
Text HLabel 8000 4750 0    60   Output ~ 0
DUT_VDD1
Text HLabel 8000 4850 0    60   Output ~ 0
DUT_VDD2
Text HLabel 8000 3900 0    60   Output ~ 0
3V3
$EndSCHEMATC
