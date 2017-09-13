SAMPLE Data:

[2017-09-12 23.18.43.731 - Received 41 (0x29) bytes]
7E A0 27 01 02 01 10 5A  87 E6 E7 00 0F 40 00 00  00 09 0C 07 E1 09 0C 02
17 12 2A FF 80 00 00 02  01 06 00 00 05 28 B8 0C  7E


Breakdown:
---------
7E: 	Frame Start flag

A: 	4 bits, A = Frame Format Type 3 (0b1010)

0
27:	11 bits, Frame size: 0x27 (39 bytes, excluding start/end flags)

01:	Destination Address
	(Address can be 1-4 bytes. Terminated by LSB=1)
02
01:	Source Address
	(Address can be 1-4 bytes. Terminated by LSB=1)

10:	Control Field

5A
87:	Header check sequence (HCS)
	(Check sum calculated from the address bytes, according to RFC 1662)

--------
Data:

		LLC PDU Format (GB, 8.3 Fig 19)
E6		Destination LSAP
E7		Source LSAP
00		LLC Quality
0F 		Information, n*8 bits?

		(UNCERTAIN)
40		Push setup, class_id=40
00		version=0

00 00 09 0C 	(UNKNOWN, 2316?)

07 E1	2017 (year)
09 		09 (sep)
0C		12 (date)
02		tuesday 

17		23 (hour)
12		18 (min)
2A		42 (sec)
FF 		(fff not specified)
80 		(deviation not specified)
00 
00 		(clock_status)

02 		(UNKNOWN)
01 06 	(UNCERTAIN. 0106=List1, 0D09=List2, 1209=List3)
00 		(UNKNOWN)
00 		(UNKNOWN)

05 28	1320 Watt
--------

B8 
0C:	Frame check sequence (FCS)
	(Check sum calculated from full frame, excluding flags and FCS, according to RFC 1662)

7E:	Frame End flag
