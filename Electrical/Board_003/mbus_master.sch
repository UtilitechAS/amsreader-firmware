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
	3100 3200 8450 3200
Wire Wire Line
	3100 3300 3600 3300
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
External power\n25-40V
$Comp
L Conn_01x02 J1
U 1 1 5A6C56AB
P 2300 6800
F 0 "J1" H 2300 6900 50  0000 C CNN
F 1 "Conn_01x02" H 2300 6600 50  0000 C CNN
F 2 "Pin_Headers:Pin_Header_Straight_1x02_Pitch1.27mm" H 2300 6800 50  0001 C CNN
F 3 "" H 2300 6800 50  0001 C CNN
	1    2300 6800
	-1   0    0    1   
$EndComp
Text Notes 1300 6800 0    60   ~ 0
Input signal, TTL\nSendt to m-bus
$Comp
L GND #PWR04
U 1 1 5A6C5767
P 2650 6900
F 0 "#PWR04" H 2650 6650 50  0001 C CNN
F 1 "GND" H 2650 6750 50  0000 C CNN
F 2 "" H 2650 6900 50  0001 C CNN
F 3 "" H 2650 6900 50  0001 C CNN
	1    2650 6900
	1    0    0    -1  
$EndComp
Wire Wire Line
	2500 6800 2650 6800
Wire Wire Line
	2650 6800 2650 6900
$Comp
L BC337 Q1
U 1 1 5A6C58F3
P 3750 6700
F 0 "Q1" H 3950 6775 50  0000 L CNN
F 1 "BC337" H 3950 6700 50  0000 L CNN
F 2 "TO_SOT_Packages_THT:TO-92_Molded_Narrow" H 3950 6625 50  0001 L CIN
F 3 "" H 3750 6700 50  0001 L CNN
	1    3750 6700
	1    0    0    -1  
$EndComp
$Comp
L R R1
U 1 1 5A6C5956
P 3200 6700
F 0 "R1" V 3280 6700 50  0000 C CNN
F 1 "1k" V 3200 6700 50  0000 C CNN
F 2 "Resistors_THT:R_Axial_DIN0207_L6.3mm_D2.5mm_P10.16mm_Horizontal" V 3130 6700 50  0001 C CNN
F 3 "" H 3200 6700 50  0001 C CNN
	1    3200 6700
	0    1    1    0   
$EndComp
Wire Wire Line
	2500 6700 3050 6700
Wire Wire Line
	3350 6700 3550 6700
$Comp
L GND #PWR05
U 1 1 5A6C59DD
P 3850 7100
F 0 "#PWR05" H 3850 6850 50  0001 C CNN
F 1 "GND" H 3850 6950 50  0000 C CNN
F 2 "" H 3850 7100 50  0001 C CNN
F 3 "" H 3850 7100 50  0001 C CNN
	1    3850 7100
	1    0    0    -1  
$EndComp
Wire Wire Line
	3850 6900 3850 7100
$Comp
L D_Zener_Small_ALT D1
U 1 1 5A6C5A7F
P 5750 3500
F 0 "D1" V 5750 3600 50  0000 C CNN
F 1 "D_Zener_Small_ALT" H 5750 3410 50  0001 C CNN
F 2 "Diodes_THT:D_A-405_P7.62mm_Horizontal" V 5750 3500 50  0001 C CNN
F 3 "" V 5750 3500 50  0001 C CNN
	1    5750 3500
	0    1    1    0   
$EndComp
$Comp
L GND #PWR06
U 1 1 5A6C5BFD
P 5750 4800
F 0 "#PWR06" H 5750 4550 50  0001 C CNN
F 1 "GND" H 5750 4650 50  0000 C CNN
F 2 "" H 5750 4800 50  0001 C CNN
F 3 "" H 5750 4800 50  0001 C CNN
	1    5750 4800
	1    0    0    -1  
$EndComp
Wire Wire Line
	5750 4600 5750 4800
Wire Wire Line
	5750 3200 5750 3400
Wire Wire Line
	5750 3600 5750 4300
Wire Wire Line
	4700 6500 3850 6500
Wire Wire Line
	4700 4000 7250 4000
Connection ~ 5750 4000
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
Connection ~ 5750 3200
Text Label 8100 3200 0    60   ~ 0
MBus+
Text Label 8100 3750 0    60   ~ 0
MBus-
$Comp
L BD136 Q4
U 1 1 5A6C614C
P 7450 4000
F 0 "Q4" H 7650 4075 50  0000 L CNN
F 1 "BD136" H 7650 4000 50  0000 L CNN
F 2 "TO_SOT_Packages_THT:TO-126_Vertical" H 7650 3925 50  0001 L CIN
F 3 "" H 7450 4000 50  0001 L CNN
	1    7450 4000
	1    0    0    1   
$EndComp
Wire Wire Line
	7550 3200 7550 3350
Connection ~ 7550 3200
$Comp
L GND #PWR07
U 1 1 5A6C640E
P 7550 4800
F 0 "#PWR07" H 7550 4550 50  0001 C CNN
F 1 "GND" H 7550 4650 50  0000 C CNN
F 2 "" H 7550 4800 50  0001 C CNN
F 3 "" H 7550 4800 50  0001 C CNN
	1    7550 4800
	1    0    0    -1  
$EndComp
Wire Wire Line
	7550 4200 7550 4800
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
	7550 3650 7550 3800
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
L R R5
U 1 1 5A70EB33
P 5750 4450
F 0 "R5" H 5850 4450 50  0000 C CNN
F 1 "22k" V 5750 4450 50  0000 C CNN
F 2 "Resistors_THT:R_Axial_DIN0207_L6.3mm_D2.5mm_P10.16mm_Horizontal" V 5680 4450 50  0001 C CNN
F 3 "" H 5750 4450 50  0001 C CNN
	1    5750 4450
	1    0    0    -1  
$EndComp
$Comp
L BC337 Q3
U 1 1 5A70ED17
P 4900 4800
F 0 "Q3" H 5100 4875 50  0000 L CNN
F 1 "BC337" H 5100 4800 50  0000 L CNN
F 2 "TO_SOT_Packages_THT:TO-92_Molded_Narrow" H 5100 4725 50  0001 L CIN
F 3 "" H 4900 4800 50  0001 L CNN
	1    4900 4800
	1    0    0    -1  
$EndComp
$Comp
L BC337 Q2
U 1 1 5A70EDBB
P 4200 5100
F 0 "Q2" H 4400 5175 50  0000 L CNN
F 1 "BC337" H 4400 5100 50  0000 L CNN
F 2 "TO_SOT_Packages_THT:TO-92_Molded_Narrow" H 4400 5025 50  0001 L CIN
F 3 "" H 4200 5100 50  0001 L CNN
	1    4200 5100
	-1   0    0    -1  
$EndComp
$Comp
L R R4
U 1 1 5A70EE1E
P 5000 5400
F 0 "R4" H 5100 5400 50  0000 C CNN
F 1 "150" V 5000 5400 50  0000 C CNN
F 2 "Resistors_THT:R_Axial_DIN0207_L6.3mm_D2.5mm_P10.16mm_Horizontal" V 4930 5400 50  0001 C CNN
F 3 "" H 5000 5400 50  0001 C CNN
	1    5000 5400
	1    0    0    -1  
$EndComp
$Comp
L R R2
U 1 1 5A70EEBF
P 4100 4500
F 0 "R2" H 4200 4500 50  0000 C CNN
F 1 "6k8" V 4100 4500 50  0000 C CNN
F 2 "Resistors_THT:R_Axial_DIN0207_L6.3mm_D2.5mm_P10.16mm_Horizontal" V 4030 4500 50  0001 C CNN
F 3 "" H 4100 4500 50  0001 C CNN
	1    4100 4500
	1    0    0    -1  
$EndComp
$Comp
L R R3
U 1 1 5A70EF3B
P 4650 5100
F 0 "R3" V 4730 5100 50  0000 C CNN
F 1 "1" V 4650 5100 50  0000 C CNN
F 2 "Resistors_THT:R_Axial_DIN0207_L6.3mm_D2.5mm_P10.16mm_Horizontal" V 4580 5100 50  0001 C CNN
F 3 "" H 4650 5100 50  0001 C CNN
	1    4650 5100
	0    1    1    0   
$EndComp
Wire Wire Line
	5000 5000 5000 5250
Wire Wire Line
	4800 5100 5000 5100
Connection ~ 5000 5100
Wire Wire Line
	4400 5100 4500 5100
Wire Wire Line
	4100 4650 4100 4900
Wire Wire Line
	4700 4800 4100 4800
Connection ~ 4100 4800
Wire Wire Line
	4100 5300 4100 5700
Wire Wire Line
	4100 5700 5000 5700
Wire Wire Line
	5000 5700 5000 5550
Connection ~ 4700 5700
Wire Wire Line
	4100 4350 4100 4250
Wire Wire Line
	4100 4250 5000 4250
Wire Wire Line
	5000 4250 5000 4600
Wire Wire Line
	4700 4000 4700 4250
Connection ~ 4700 4250
Wire Notes Line
	3700 4150 5300 4150
Wire Notes Line
	5300 4150 5300 6200
Wire Notes Line
	5300 6200 3700 6200
Wire Notes Line
	3700 6200 3700 4150
Text Notes 4100 6000 0    60   ~ 0
Current limit, ca 8mA
Wire Wire Line
	4700 6500 4700 5700
$EndSCHEMATC
