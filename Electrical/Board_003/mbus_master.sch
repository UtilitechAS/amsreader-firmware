EESchema Schematic File Version 2
LIBS:power
LIBS:device
LIBS:switches
LIBS:relays
LIBS:motors
LIBS:transistors
LIBS:conn
LIBS:linear
LIBS:regul
LIBS:74xx
LIBS:cmos4000
LIBS:adc-dac
LIBS:memory
LIBS:xilinx
LIBS:microcontrollers
LIBS:dsp
LIBS:microchip
LIBS:analog_switches
LIBS:motorola
LIBS:texas
LIBS:intel
LIBS:audio
LIBS:interface
LIBS:digital-audio
LIBS:philips
LIBS:display
LIBS:cypress
LIBS:siliconi
LIBS:opto
LIBS:atmel
LIBS:contrib
LIBS:valves
LIBS:mbus_master-cache
EELAYER 25 0
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
L Conn_01x02 J2
U 1 1 5A6C5520
P 2900 3300
F 0 "J2" H 2900 3400 50  0000 C CNN
F 1 "Conn_01x02" H 2900 3100 50  0000 C CNN
F 2 "Pin_Headers:Pin_Header_Straight_1x02_Pitch1.27mm" H 2900 3300 50  0001 C CNN
F 3 "" H 2900 3300 50  0001 C CNN
	1    2900 3300
	-1   0    0    1   
$EndComp
$Comp
L GND #PWR01
U 1 1 5A6C5572
P 3600 3400
F 0 "#PWR01" H 3600 3150 50  0001 C CNN
F 1 "GND" H 3600 3250 50  0000 C CNN
F 2 "" H 3600 3400 50  0001 C CNN
F 3 "" H 3600 3400 50  0001 C CNN
	1    3600 3400
	1    0    0    -1  
$EndComp
$Comp
L PWR_FLAG #FLG02
U 1 1 5A6C558D
P 3250 3400
F 0 "#FLG02" H 3250 3475 50  0001 C CNN
F 1 "PWR_FLAG" H 3250 3550 50  0000 C CNN
F 2 "" H 3250 3400 50  0001 C CNN
F 3 "" H 3250 3400 50  0001 C CNN
	1    3250 3400
	-1   0    0    1   
$EndComp
Wire Wire Line
	3100 3200 3250 3200
Wire Wire Line
	3250 3200 4200 3200
Wire Wire Line
	4200 3200 5300 3200
Wire Wire Line
	5300 3200 6700 3200
Wire Wire Line
	6700 3200 7550 3200
Wire Wire Line
	7550 3200 8450 3200
Wire Wire Line
	3100 3300 3250 3300
Wire Wire Line
	3250 3300 3600 3300
Wire Wire Line
	3600 3300 3600 3400
Wire Wire Line
	3250 3400 3250 3300
Connection ~ 3250 3300
$Comp
L PWR_FLAG #FLG03
U 1 1 5A6C5614
P 3250 3050
F 0 "#FLG03" H 3250 3125 50  0001 C CNN
F 1 "PWR_FLAG" H 3250 3200 50  0000 C CNN
F 2 "" H 3250 3050 50  0001 C CNN
F 3 "" H 3250 3050 50  0001 C CNN
	1    3250 3050
	1    0    0    -1  
$EndComp
Wire Wire Line
	3250 3050 3250 3200
Connection ~ 3250 3200
Text Label 3600 3200 1    60   ~ 0
EXT_PWR
Text Notes 2050 3350 0    60   ~ 0
External power\n21-42V
$Comp
L Conn_01x02 J1
U 1 1 5A6C56AB
P 1950 6800
F 0 "J1" H 1950 6900 50  0000 C CNN
F 1 "Conn_01x02" H 1950 6600 50  0000 C CNN
F 2 "Pin_Headers:Pin_Header_Straight_1x02_Pitch1.27mm" H 1950 6800 50  0001 C CNN
F 3 "" H 1950 6800 50  0001 C CNN
	1    1950 6800
	-1   0    0    1   
$EndComp
Text Notes 950  6800 0    60   ~ 0
Input signal, TTL\nSendt to m-bus
$Comp
L GND #PWR04
U 1 1 5A6C5767
P 2300 6900
F 0 "#PWR04" H 2300 6650 50  0001 C CNN
F 1 "GND" H 2300 6750 50  0000 C CNN
F 2 "" H 2300 6900 50  0001 C CNN
F 3 "" H 2300 6900 50  0001 C CNN
	1    2300 6900
	1    0    0    -1  
$EndComp
Wire Wire Line
	2150 6800 2300 6800
Wire Wire Line
	2300 6800 2300 6900
$Comp
L BC337 Q1
U 1 1 5A6C58F3
P 3200 6700
F 0 "Q1" H 3400 6775 50  0000 L CNN
F 1 "BC337" H 3400 6700 50  0000 L CNN
F 2 "TO_SOT_Packages_THT:TO-92_Molded_Narrow" H 3400 6625 50  0001 L CIN
F 3 "" H 3200 6700 50  0001 L CNN
	1    3200 6700
	1    0    0    -1  
$EndComp
$Comp
L R R1
U 1 1 5A6C5956
P 2700 6700
F 0 "R1" V 2780 6700 50  0000 C CNN
F 1 "10k" V 2700 6700 50  0000 C CNN
F 2 "Resistors_THT:R_Axial_DIN0207_L6.3mm_D2.5mm_P10.16mm_Horizontal" V 2630 6700 50  0001 C CNN
F 3 "" H 2700 6700 50  0001 C CNN
	1    2700 6700
	0    1    1    0   
$EndComp
Wire Wire Line
	2150 6700 2400 6700
Wire Wire Line
	2400 6700 2550 6700
Wire Wire Line
	2850 6700 3000 6700
$Comp
L GND #PWR05
U 1 1 5A6C59DD
P 3300 7100
F 0 "#PWR05" H 3300 6850 50  0001 C CNN
F 1 "GND" H 3300 6950 50  0000 C CNN
F 2 "" H 3300 7100 50  0001 C CNN
F 3 "" H 3300 7100 50  0001 C CNN
	1    3300 7100
	1    0    0    -1  
$EndComp
Wire Wire Line
	3300 6900 3300 7100
$Comp
L D_Zener_Small_ALT D1
U 1 1 5A6C5A7F
P 6700 3550
F 0 "D1" V 6700 3650 50  0000 C CNN
F 1 "D_Zener_Small_ALT" H 6700 3460 50  0001 C CNN
F 2 "Diodes_THT:D_A-405_P7.62mm_Horizontal" V 6700 3550 50  0001 C CNN
F 3 "" V 6700 3550 50  0001 C CNN
	1    6700 3550
	0    1    1    0   
$EndComp
Wire Wire Line
	6700 3200 6700 3450
Wire Wire Line
	6700 4350 7250 4350
$Comp
L Conn_01x02 J3
U 1 1 5A6C5F4D
P 9000 3450
F 0 "J3" H 9000 3550 50  0000 C CNN
F 1 "Conn_01x02" H 9000 3250 50  0000 C CNN
F 2 "Pin_Headers:Pin_Header_Straight_1x02_Pitch1.27mm" H 9000 3450 50  0001 C CNN
F 3 "" H 9000 3450 50  0001 C CNN
	1    9000 3450
	1    0    0    1   
$EndComp
Wire Wire Line
	8800 3350 8450 3350
Wire Wire Line
	8450 3350 8450 3200
Connection ~ 6700 3200
Text Label 8100 3200 0    60   ~ 0
MBus+
Text Label 8100 3750 0    60   ~ 0
MBus-
$Comp
L BD136 Q4
U 1 1 5A6C614C
P 7450 4350
F 0 "Q4" H 7650 4425 50  0000 L CNN
F 1 "BD136" H 7650 4350 50  0000 L CNN
F 2 "TO_SOT_Packages_THT:TO-126_Vertical" H 7650 4275 50  0001 L CIN
F 3 "" H 7450 4350 50  0001 L CNN
	1    7450 4350
	1    0    0    1   
$EndComp
Wire Wire Line
	7550 3200 7550 3350
Connection ~ 7550 3200
$Comp
L GND #PWR07
U 1 1 5A6C640E
P 7550 5250
F 0 "#PWR07" H 7550 5000 50  0001 C CNN
F 1 "GND" H 7550 5100 50  0000 C CNN
F 2 "" H 7550 5250 50  0001 C CNN
F 3 "" H 7550 5250 50  0001 C CNN
	1    7550 5250
	1    0    0    -1  
$EndComp
Wire Wire Line
	7550 4550 7550 5250
Wire Wire Line
	7550 3750 8450 3750
Wire Wire Line
	8450 3750 8450 3450
Wire Wire Line
	8450 3450 8800 3450
Connection ~ 7550 3750
Text Notes 9250 3450 0    60   ~ 0
M-bus output signal
Wire Wire Line
	7550 3650 7550 3750
Wire Wire Line
	7550 3750 7550 4150
Text Notes 3000 1650 0    60   ~ 0
Schematic based on https://github.com/rscada/libmbus/blob/master/hardware/MBus_USB.pdf\nmentioned in https://electronics.stackexchange.com/a/214477/568.\n\nRemoved all Rx support. Made more robust by using a zener diode so that the voltage drop is exact and that supply voltage does not matter.
Wire Notes Line
	2900 1250 8750 1250
Wire Notes Line
	8750 1250 8750 1800
Wire Notes Line
	8750 1800 2900 1800
Wire Notes Line
	2900 1800 2900 1250
$Comp
L R R6
U 1 1 5A70E602
P 7550 3500
F 0 "R6" H 7650 3500 50  0000 C CNN
F 1 "220k" V 7550 3500 50  0000 C CNN
F 2 "Resistors_THT:R_Axial_DIN0207_L6.3mm_D2.5mm_P10.16mm_Horizontal" V 7480 3500 50  0001 C CNN
F 3 "" H 7550 3500 50  0001 C CNN
	1    7550 3500
	1    0    0    -1  
$EndComp
$Comp
L R R?
U 1 1 5A92EF05
P 3300 6000
F 0 "R?" V 3380 6000 50  0000 C CNN
F 1 "8.6k" V 3300 6000 50  0000 C CNN
F 2 "" V 3230 6000 50  0001 C CNN
F 3 "" H 3300 6000 50  0001 C CNN
	1    3300 6000
	1    0    0    -1  
$EndComp
Wire Wire Line
	3300 6150 3300 6500
Wire Wire Line
	6700 3650 6700 4350
Wire Wire Line
	6700 4350 6700 4350
Wire Wire Line
	6700 4350 6700 5850
Connection ~ 6700 4350
Wire Wire Line
	6700 5850 3300 5850
$Comp
L BC337 Q?
U 1 1 5A92F34B
P 4100 4650
F 0 "Q?" H 4300 4725 50  0000 L CNN
F 1 "BC337" H 4300 4650 50  0000 L CNN
F 2 "TO_SOT_Packages_THT:TO-92_Molded_Narrow" H 4300 4575 50  0001 L CIN
F 3 "" H 4100 4650 50  0001 L CNN
	1    4100 4650
	1    0    0    -1  
$EndComp
$Comp
L GND #PWR?
U 1 1 5A92F352
P 4200 5050
F 0 "#PWR?" H 4200 4800 50  0001 C CNN
F 1 "GND" H 4200 4900 50  0000 C CNN
F 2 "" H 4200 5050 50  0001 C CNN
F 3 "" H 4200 5050 50  0001 C CNN
	1    4200 5050
	1    0    0    -1  
$EndComp
Wire Wire Line
	4200 4850 4200 5050
$Comp
L R R?
U 1 1 5A92F359
P 4200 3500
F 0 "R?" V 4280 3500 50  0000 C CNN
F 1 "10k" V 4200 3500 50  0000 C CNN
F 2 "" V 4130 3500 50  0001 C CNN
F 3 "" H 4200 3500 50  0001 C CNN
	1    4200 3500
	1    0    0    -1  
$EndComp
Wire Wire Line
	4200 3650 4200 4000
Wire Wire Line
	4200 4000 4200 4450
$Comp
L R R?
U 1 1 5A92F5C3
P 3600 4650
F 0 "R?" V 3680 4650 50  0000 C CNN
F 1 "10k" V 3600 4650 50  0000 C CNN
F 2 "" V 3530 4650 50  0001 C CNN
F 3 "" H 3600 4650 50  0001 C CNN
	1    3600 4650
	0    1    1    0   
$EndComp
Wire Wire Line
	2400 6700 2400 4650
Wire Wire Line
	2400 4650 3450 4650
Connection ~ 2400 6700
Wire Wire Line
	3750 4650 3900 4650
Wire Wire Line
	4200 3350 4200 3200
Connection ~ 4200 3200
$Comp
L BC337 Q?
U 1 1 5A92F825
P 5200 4000
F 0 "Q?" H 5400 4075 50  0000 L CNN
F 1 "BC337" H 5400 4000 50  0000 L CNN
F 2 "TO_SOT_Packages_THT:TO-92_Molded_Narrow" H 5400 3925 50  0001 L CIN
F 3 "" H 5200 4000 50  0001 L CNN
	1    5200 4000
	1    0    0    -1  
$EndComp
$Comp
L R R?
U 1 1 5A92F832
P 5300 3500
F 0 "R?" V 5380 3500 50  0000 C CNN
F 1 "10k" V 5300 3500 50  0000 C CNN
F 2 "" V 5230 3500 50  0001 C CNN
F 3 "" H 5300 3500 50  0001 C CNN
	1    5300 3500
	1    0    0    -1  
$EndComp
Wire Wire Line
	5300 3650 5300 3800
Wire Wire Line
	5300 3350 5300 3200
Connection ~ 5300 3200
Wire Wire Line
	5000 4000 4200 4000
Connection ~ 4200 4000
Wire Wire Line
	5300 4200 5300 4350
Wire Wire Line
	5300 4350 6700 4350
Connection ~ 6700 4350
$EndSCHEMATC
