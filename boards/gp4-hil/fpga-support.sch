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
Sheet 7 7
Title ""
Date "2016-05-12"
Rev ""
Comp ""
Comment1 ""
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
$Comp
L XC7AxT-xFTG256x U2
U 1 1 5732CCFA
P 1900 6200
F 0 "U2" H 1900 6000 60  0000 L CNN
F 1 "XC7A100T-1FTG256C" H 1900 6100 60  0000 L CNN
F 2 "" H 1900 6200 60  0000 C CNN
F 3 "" H 1900 6200 60  0000 C CNN
	1    1900 6200
	1    0    0    -1  
$EndComp
$Comp
L XC7AxT-xFTG256x U2
U 2 1 5732CEE7
P 5300 6200
F 0 "U2" H 5300 6000 60  0000 L CNN
F 1 "XC7A100T-1FTG256C" H 5300 6100 60  0000 L CNN
F 2 "" H 5300 6200 60  0000 C CNN
F 3 "" H 5300 6200 60  0000 C CNN
	2    5300 6200
	1    0    0    -1  
$EndComp
$Comp
L W25Q80BV U6
U 1 1 5732D74D
P 12400 4150
F 0 "U6" H 12400 4837 60  0000 C CNN
F 1 "W25Q32FW" H 12400 4731 60  0000 C CNN
F 2 "" H 12400 4150 60  0000 C CNN
F 3 "" H 12400 4150 60  0000 C CNN
	1    12400 4150
	1    0    0    -1  
$EndComp
Text Notes 7450 9100 0    60   ~ 0
Recommended decoupling for 7a100t (largest FTG256 device):\n\nVCCINT\n* 1x 330 uF\n* 6x 4.7 uF\n* 8x 0.47 uF\n\nVCCBRAM\n* 1x 100 uF\n* 2x 0.47 uF\n\nVCCAUX\n* 1x 47 uF \n* 2x 4.7 uF \n* 3x 0.47 uF\n\nVCCO_0\n* 1x 47 uF\n\nVCCO (per bank):\n* 1x 47/100 uF per 4 banks\n* 2x 4.7 uF\n* 4x 0.47 uF
Wire Wire Line
	5100 1500 5000 1500
Wire Wire Line
	5000 1300 5000 4400
Wire Wire Line
	5000 1600 5100 1600
Wire Wire Line
	5000 1700 5100 1700
Connection ~ 5000 1600
Wire Wire Line
	5000 1800 5100 1800
Connection ~ 5000 1700
Wire Wire Line
	5000 1900 5100 1900
Connection ~ 5000 1800
Wire Wire Line
	5000 2000 5100 2000
Connection ~ 5000 1900
Wire Wire Line
	5000 2100 5100 2100
Connection ~ 5000 2000
Wire Wire Line
	5000 2200 5100 2200
Connection ~ 5000 2100
Wire Wire Line
	5000 2300 5100 2300
Connection ~ 5000 2200
Wire Wire Line
	5000 2400 5100 2400
Connection ~ 5000 2300
Wire Wire Line
	5000 2500 5100 2500
Connection ~ 5000 2400
Wire Wire Line
	5000 2600 5100 2600
Connection ~ 5000 2500
Wire Wire Line
	5000 2700 5100 2700
Connection ~ 5000 2600
Wire Wire Line
	5000 2800 5100 2800
Connection ~ 5000 2700
Wire Wire Line
	5000 2900 5100 2900
Connection ~ 5000 2800
Wire Wire Line
	5000 3000 5100 3000
Connection ~ 5000 2900
Wire Wire Line
	5000 3100 5100 3100
Connection ~ 5000 3000
Wire Wire Line
	5000 3200 5100 3200
Connection ~ 5000 3100
Wire Wire Line
	5000 3300 5100 3300
Connection ~ 5000 3200
Wire Wire Line
	5000 3400 5100 3400
Connection ~ 5000 3300
Wire Wire Line
	5000 3500 5100 3500
Connection ~ 5000 3400
Wire Wire Line
	5000 3600 5100 3600
Connection ~ 5000 3500
Wire Wire Line
	5000 3700 5100 3700
Connection ~ 5000 3600
Wire Wire Line
	5000 3800 5100 3800
Connection ~ 5000 3700
Wire Wire Line
	5000 3900 5100 3900
Connection ~ 5000 3800
Wire Wire Line
	5000 4000 5100 4000
Connection ~ 5000 3900
Wire Wire Line
	5000 4100 5100 4100
Connection ~ 5000 4000
Wire Wire Line
	5000 4200 5100 4200
Connection ~ 5000 4100
Wire Wire Line
	5000 4300 5100 4300
Connection ~ 5000 4200
Wire Wire Line
	5000 4400 5100 4400
Connection ~ 5000 4300
Wire Wire Line
	4850 1300 5100 1300
Connection ~ 5000 1500
Connection ~ 5000 1300
Wire Wire Line
	6850 1650 6950 1650
Wire Wire Line
	6950 1650 6950 2250
Wire Wire Line
	6950 1750 6850 1750
Wire Wire Line
	6950 1850 6850 1850
Connection ~ 6950 1750
Wire Wire Line
	6950 1950 6850 1950
Connection ~ 6950 1850
Wire Wire Line
	6950 2050 6850 2050
Connection ~ 6950 1950
Wire Wire Line
	6950 2150 6850 2150
Connection ~ 6950 2050
Wire Wire Line
	6950 2250 6850 2250
Connection ~ 6950 2150
Wire Wire Line
	6850 2450 6950 2450
Wire Wire Line
	6950 2450 6950 2550
Wire Wire Line
	6950 2550 6850 2550
Wire Wire Line
	6850 2750 6950 2750
Wire Wire Line
	6950 2750 6950 3050
Wire Wire Line
	6950 2850 6850 2850
Wire Wire Line
	6950 2950 6850 2950
Connection ~ 6950 2850
Wire Wire Line
	6950 3050 6850 3050
Connection ~ 6950 2950
Wire Wire Line
	6850 3450 6950 3450
Wire Wire Line
	6950 3450 6950 3950
Wire Wire Line
	6950 3550 6850 3550
Wire Wire Line
	6950 3650 6850 3650
Connection ~ 6950 3550
Wire Wire Line
	6950 3750 6850 3750
Connection ~ 6950 3650
Wire Wire Line
	6950 3850 6850 3850
Connection ~ 6950 3750
Wire Wire Line
	6950 3950 6850 3950
Connection ~ 6950 3850
Wire Wire Line
	6850 4150 6950 4150
Wire Wire Line
	6950 4150 6950 4650
Wire Wire Line
	6950 4250 6850 4250
Wire Wire Line
	6950 4350 6850 4350
Connection ~ 6950 4250
Wire Wire Line
	6950 4450 6850 4450
Connection ~ 6950 4350
Wire Wire Line
	6950 4550 6850 4550
Connection ~ 6950 4450
Wire Wire Line
	6950 4650 6850 4650
Connection ~ 6950 4550
Wire Wire Line
	6850 4850 6950 4850
Wire Wire Line
	6950 4850 6950 5050
Wire Wire Line
	6950 4950 6850 4950
Wire Wire Line
	6950 5050 6850 5050
Connection ~ 6950 4950
Wire Wire Line
	6850 5250 6950 5250
Wire Wire Line
	6950 5250 6950 5750
Wire Wire Line
	6950 5350 6850 5350
Wire Wire Line
	6950 5450 6850 5450
Connection ~ 6950 5350
Wire Wire Line
	6950 5550 6850 5550
Connection ~ 6950 5450
Wire Wire Line
	6950 5650 6850 5650
Connection ~ 6950 5550
Wire Wire Line
	6950 5750 6850 5750
Connection ~ 6950 5650
Text HLabel 4850 1300 0    60   Input ~ 0
GND
$EndSCHEMATC
