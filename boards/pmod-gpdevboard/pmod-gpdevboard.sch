EESchema Schematic File Version 2
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
LIBS:pmod-gpdevboard-cache
EELAYER 26 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 1 1
Title ""
Date ""
Rev ""
Comp ""
Comment1 ""
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
$Comp
L GREENPAK_DEVKIT_HDR J1
U 1 1 58C9C127
P 2350 3250
F 0 "J1" H 2878 4358 60  0000 L CNN
F 1 "GREENPAK_DEVKIT_HDR" H 2878 4252 60  0000 L CNN
F 2 "azonenberg_pcb:CONN_HEADER_2.54MM_2x10_RA_FEMALE" H 2350 3200 60  0001 C CNN
F 3 "" H 2350 3200 60  0001 C CNN
	1    2350 3250
	-1   0    0    -1  
$EndComp
$Comp
L PMOD_DEVICE_DIFF_PWRIN J2
U 1 1 58C9C219
P 4400 2700
F 0 "J2" H 4778 3533 60  0000 L CNN
F 1 "PMOD_DEVICE_DIFF_PWRIN" H 4778 3427 60  0000 L CNN
F 2 "azonenberg_pcb:CONN_HEADER_2.54MM_2x6_RA_PMOD_MODULE" H 4400 2700 60  0001 C CNN
F 3 "" H 4400 2700 60  0001 C CNN
	1    4400 2700
	1    0    0    -1  
$EndComp
NoConn ~ 4200 1200
NoConn ~ 4200 1300
NoConn ~ 2550 1200
Wire Wire Line
	2550 1300 4000 1300
Wire Wire Line
	4000 1300 4000 1400
Wire Wire Line
	4000 1400 4200 1400
Wire Wire Line
	4200 1500 4100 1500
Wire Wire Line
	4100 1500 4100 1400
Connection ~ 4100 1400
NoConn ~ 2550 1900
NoConn ~ 2550 2000
NoConn ~ 2550 2100
NoConn ~ 2550 2200
NoConn ~ 2550 2300
NoConn ~ 2550 2800
NoConn ~ 2550 2900
NoConn ~ 2550 3000
NoConn ~ 2550 3100
NoConn ~ 2550 3200
NoConn ~ 3300 4300
Text Label 2750 1500 0    60   ~ 0
DQ0
Text Label 2750 1600 0    60   ~ 0
DQ1
Text Label 2750 1700 0    60   ~ 0
DQ2
Text Label 2750 1800 0    60   ~ 0
DQ3
Text Label 2750 2400 0    60   ~ 0
DQ4
Text Label 2750 2500 0    60   ~ 0
DQ5
Text Label 2750 2600 0    60   ~ 0
DQ6
Text Label 2750 2700 0    60   ~ 0
DQ7
Wire Wire Line
	2750 1500 2550 1500
Wire Wire Line
	2550 1600 2750 1600
Wire Wire Line
	2750 1700 2550 1700
Wire Wire Line
	2550 1800 2750 1800
Wire Wire Line
	2750 2400 2550 2400
Wire Wire Line
	2550 2500 2750 2500
Wire Wire Line
	2750 2600 2550 2600
Wire Wire Line
	2750 2700 2550 2700
Text Label 4000 2250 2    60   ~ 0
DQ0
Wire Wire Line
	4000 2250 4200 2250
Text Label 4000 1700 2    60   ~ 0
DQ1
Wire Wire Line
	4000 1700 4200 1700
Text Label 4000 2350 2    60   ~ 0
DQ2
Text Label 4000 1800 2    60   ~ 0
DQ3
Wire Wire Line
	4000 1800 4200 1800
Text Label 4000 2550 2    60   ~ 0
DQ5
Wire Wire Line
	4000 1950 4200 1950
Text Label 4000 2650 2    60   ~ 0
DQ7
Wire Wire Line
	4000 2050 4200 2050
Wire Wire Line
	4000 2350 4200 2350
Text Label 4000 1950 2    60   ~ 0
DQ4
Text Label 4000 2050 2    60   ~ 0
DQ6
Wire Wire Line
	4000 2650 4200 2650
Wire Wire Line
	4200 2550 4000 2550
Text Label 2750 1300 0    60   ~ 0
GND
$EndSCHEMATC
