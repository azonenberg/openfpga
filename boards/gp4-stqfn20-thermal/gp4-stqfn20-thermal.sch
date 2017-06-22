EESchema Schematic File Version 2
LIBS:conn
LIBS:device
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
LIBS:silego-azonenberg
LIBS:gp4-stqfn20-thermal-cache
EELAYER 26 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 1 1
Title "GreenPAK4 STQFN-20 Thermal Characterization Board"
Date "2017-06-21"
Rev "0.1"
Comp "Andrew Zonenberg"
Comment1 ""
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
$Comp
L SI705x U1
U 1 1 594B5883
P 1850 4650
F 0 "U1" H 2100 5047 60  0000 C CNN
F 1 "SI7050" H 2100 4941 60  0000 C CNN
F 2 "azonenberg_pcb:DFN_6_1MM_3x3MM" H 2000 4550 60  0001 C CNN
F 3 "" H 2000 4550 60  0001 C CNN
	1    1850 4650
	1    0    0    -1  
$EndComp
$Comp
L GREENPAK_ZIF_HDR J1
U 1 1 594B5D69
P 1950 2950
F 0 "J1" H 2450 2800 60  0000 R CNN
F 1 "GREENPAK_ZIF_HDR" H 2450 2900 60  0000 R CNN
F 2 "azonenberg_pcb:CONN_HEADER_2.54MM_2x10_RA_FEMALE" H 1950 2900 60  0001 C CNN
F 3 "" H 1950 2900 60  0001 C CNN
	1    1950 2950
	-1   0    0    -1  
$EndComp
Text Label 2250 1000 0    60   ~ 0
GND
$Comp
L CONN_01X04 P1
U 1 1 594B5F65
P 1500 3600
F 0 "P1" H 1550 3250 50  0000 R CNN
F 1 "CONN_01X04" H 1550 3350 50  0000 R CNN
F 2 "azonenberg_pcb:CONN_HEADER_2.54MM_1x4" H 1500 3600 50  0001 C CNN
F 3 "" H 1500 3600 50  0000 C CNN
	1    1500 3600
	-1   0    0    -1  
$EndComp
Text Label 1850 3450 0    60   ~ 0
3V3
Text Label 1850 3550 0    60   ~ 0
I2C_SDA
Text Label 1850 3650 0    60   ~ 0
I2C_SCL
Text Label 1850 3750 0    60   ~ 0
GND
$Comp
L C C4
U 1 1 594B6084
P 4450 750
F 0 "C4" H 4565 796 50  0000 L CNN
F 1 "4.7 uF" H 4565 705 50  0000 L CNN
F 2 "azonenberg_pcb:EIA_0603_CAP_NOSILK" H 4488 600 50  0001 C CNN
F 3 "" H 4450 750 50  0000 C CNN
	1    4450 750 
	1    0    0    -1  
$EndComp
$Comp
L C C5
U 1 1 594B6136
P 4950 750
F 0 "C5" H 5065 796 50  0000 L CNN
F 1 "0.47 uF" H 5065 705 50  0000 L CNN
F 2 "azonenberg_pcb:EIA_0402_CAP_NOSILK" H 4988 600 50  0001 C CNN
F 3 "" H 4950 750 50  0000 C CNN
	1    4950 750 
	1    0    0    -1  
$EndComp
$Comp
L INA226 U2
U 1 1 594B64E0
P 2350 5700
F 0 "U2" H 2325 6187 60  0000 C CNN
F 1 "INA226" H 2325 6081 60  0000 C CNN
F 2 "azonenberg_pcb:SOP_10_0.5MM_3MM" H 2350 5700 60  0001 C CNN
F 3 "" H 2350 5700 60  0000 C CNN
	1    2350 5700
	1    0    0    -1  
$EndComp
Text Label 2650 4500 0    60   ~ 0
I2C_SDA
Text Label 2650 4600 0    60   ~ 0
I2C_SCL
Text Label 1400 5800 2    60   ~ 0
I2C_SDA
NoConn ~ 1550 5700
Text Label 1400 5900 2    60   ~ 0
I2C_SCL
Text Notes 1850 4750 0    60   ~ 0
Addr = 8'h80 (fixed)
Text Notes 1850 6100 0    60   ~ 0
Addr = 8'h8A
Text Label 3250 5900 0    60   ~ 0
3V3
Text Label 1400 5600 2    60   ~ 0
3V3
Text Label 1400 5500 2    60   ~ 0
3V3
Text Label 3250 5800 0    60   ~ 0
GND
Text Label 1500 4500 2    60   ~ 0
3V3
Text Label 1500 4600 2    60   ~ 0
GND
$Comp
L C C1
U 1 1 594B7189
P 2500 3600
F 0 "C1" H 2615 3646 50  0000 L CNN
F 1 "4.7 uF" H 2615 3555 50  0000 L CNN
F 2 "azonenberg_pcb:EIA_0603_CAP_NOSILK" H 2538 3450 50  0001 C CNN
F 3 "" H 2500 3600 50  0000 C CNN
	1    2500 3600
	1    0    0    -1  
$EndComp
$Comp
L C C2
U 1 1 594B718F
P 3000 3600
F 0 "C2" H 3115 3646 50  0000 L CNN
F 1 "0.47 uF" H 3115 3555 50  0000 L CNN
F 2 "azonenberg_pcb:EIA_0402_CAP_NOSILK" H 3038 3450 50  0001 C CNN
F 3 "" H 3000 3600 50  0000 C CNN
	1    3000 3600
	1    0    0    -1  
$EndComp
$Comp
L C C3
U 1 1 594B724F
P 3550 3600
F 0 "C3" H 3665 3646 50  0000 L CNN
F 1 "0.47 uF" H 3665 3555 50  0000 L CNN
F 2 "azonenberg_pcb:EIA_0402_CAP_NOSILK" H 3588 3450 50  0001 C CNN
F 3 "" H 3550 3600 50  0000 C CNN
	1    3550 3600
	1    0    0    -1  
$EndComp
Text Label 3250 5700 0    60   ~ 0
VDD_DUT
Text Label 3250 5600 0    60   ~ 0
VDD_DUT
Text Label 3250 5500 0    60   ~ 0
VDD_IN
Text Label 2250 900  0    60   ~ 0
VDD_IN
Text Label 3500 900  0    60   ~ 0
VDD_DUT
Text Label 4250 600  2    60   ~ 0
GND
Wire Wire Line
	1700 3750 3550 3750
Wire Wire Line
	1700 3650 1850 3650
Wire Wire Line
	1850 3550 1700 3550
Wire Wire Line
	1700 3450 3550 3450
Wire Wire Line
	3200 900  5700 900 
Connection ~ 4450 900 
Wire Wire Line
	2650 4600 2550 4600
Wire Wire Line
	2650 4500 2550 4500
Wire Wire Line
	3100 5900 3250 5900
Wire Wire Line
	1400 5500 1550 5500
Wire Wire Line
	1550 5600 1400 5600
Wire Wire Line
	1400 5800 1550 5800
Wire Wire Line
	1550 5900 1400 5900
Wire Wire Line
	3250 5800 3100 5800
Wire Wire Line
	1500 4500 1650 4500
Wire Wire Line
	1500 4600 1650 4600
Connection ~ 2500 3450
Connection ~ 3000 3450
Connection ~ 2500 3750
Connection ~ 3000 3750
Wire Wire Line
	3250 5600 3100 5600
Wire Wire Line
	3250 5700 3100 5700
Wire Wire Line
	3250 5500 3100 5500
Wire Wire Line
	2150 900  2900 900 
Wire Wire Line
	2250 1000 2150 1000
$Comp
L SLG46620V U3
U 1 1 594B7FAE
P 5900 2950
F 0 "U3" H 5900 2900 60  0000 L CNN
F 1 "SLG46620V" H 5900 2800 60  0000 L CNN
F 2 "azonenberg_pcb:QFN_20_0.4MM_3x2MM_SILEGO_STQFN" H 5900 2950 60  0001 C CNN
F 3 "" H 5900 2950 60  0001 C CNN
	1    5900 2950
	1    0    0    -1  
$EndComp
Wire Wire Line
	4250 600  4950 600 
Connection ~ 4450 600 
Connection ~ 4950 900 
Text Label 5550 1000 2    60   ~ 0
GND
Wire Wire Line
	5550 1000 5700 1000
Wire Wire Line
	5700 1200 2150 1200
Wire Wire Line
	2150 1300 5700 1300
Wire Wire Line
	5700 1400 2150 1400
Wire Wire Line
	2150 1500 5700 1500
Wire Wire Line
	5700 1600 2150 1600
Wire Wire Line
	2150 1700 5700 1700
Wire Wire Line
	5700 1800 2150 1800
Wire Wire Line
	2150 1900 5700 1900
Wire Wire Line
	5700 2000 2150 2000
Wire Wire Line
	2150 2100 5700 2100
Wire Wire Line
	5700 2200 2150 2200
Wire Wire Line
	2150 2300 5700 2300
Wire Wire Line
	5700 2400 2150 2400
Wire Wire Line
	2150 2500 5700 2500
Wire Wire Line
	5700 2600 2150 2600
Wire Wire Line
	2150 2700 5700 2700
Wire Wire Line
	5700 2800 2150 2800
Wire Wire Line
	2150 2900 5700 2900
Text Label 2250 1200 0    60   ~ 0
DUT_P2
Text Label 2250 1300 0    60   ~ 0
DUT_P3
Text Label 2250 1400 0    60   ~ 0
DUT_P4
Text Label 2250 1500 0    60   ~ 0
DUT_P5
Text Label 2250 1600 0    60   ~ 0
DUT_P6
Text Label 2250 1700 0    60   ~ 0
DUT_P7
Text Label 2250 1800 0    60   ~ 0
DUT_P8
Text Label 2250 1900 0    60   ~ 0
DUT_P9
Text Label 2250 2000 0    60   ~ 0
DUT_P10
Text Label 2250 2100 0    60   ~ 0
DUT_P12
Text Label 2250 2200 0    60   ~ 0
DUT_P13
Text Label 2250 2300 0    60   ~ 0
DUT_P14
Text Label 2250 2400 0    60   ~ 0
DUT_P15
Text Label 2250 2500 0    60   ~ 0
DUT_P16
Text Label 2250 2600 0    60   ~ 0
DUT_P17
Text Label 2250 2700 0    60   ~ 0
DUT_P18
Text Label 2250 2800 0    60   ~ 0
DUT_P19
Text Label 2250 2900 0    60   ~ 0
DUT_P20
Text Notes 5900 4150 0    60   ~ 0
SHUNT RESISTOR\n5 ohm\nScale = 5 mV / 1 mA\n1 LSB = 2.5 uV = 0.5 uA\nFull scale = 81.92 mV = 16.384 mA\n\n1 ohm\nScale = 1 mV / 1 mA\n1 LSB = 2.5 uV = 2.5 uA\nFull scale = 81.92 mV = 81.92 mA
$Comp
L R R1
U 1 1 594B9CAE
P 3050 900
F 0 "R1" V 2950 900 50  0000 C CNN
F 1 "1" V 3050 900 50  0000 C CNN
F 2 "azonenberg_pcb:EIA_0805_CAP_NOSILK" V 2980 900 50  0001 C CNN
F 3 "" H 3050 900 50  0000 C CNN
	1    3050 900 
	0    1    1    0   
$EndComp
$EndSCHEMATC
