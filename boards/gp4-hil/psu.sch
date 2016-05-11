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
Sheet 2 7
Title "GreenPak Hardware-In-Loop Test Platform"
Date "2016-05-09"
Rev "0.1"
Comp "Andrew Zonenberg"
Comment1 ""
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
Text Notes 5200 3450 0    60   ~ 0
Low current rails:\n* 1V25 precision reference for XADC\n\nHigh current rails (LTC3374):\n* 2V5 PHY analog\n* 1V8 FPGA VCCAUX/VCCXADC, PHY I/O\n* 1V2 PHY analog/digital core\n* 1V0 FPGA VCCINT/VCCBRAM\n\nVariable rails:\n* 7V5 Vpp\n* 0-3.4V Vdd/VCCO1\n* 0-3.4V VCCO2\n\nInput: Unregulated 5V DC
$EndSCHEMATC
