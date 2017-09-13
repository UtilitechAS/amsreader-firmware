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
87 E6 E7 00 0F 40 00 00  
00 09 0C 07 E1 09 0C 02
17 12 2A FF 80 00 00 02
01 06 00 00 05 28 
--------

B8 
0C:	Frame check sequence (FCS)
	(Check sum calculated from full frame, excluding flags and FCS, according to RFC 1662)

7E:	Frame End flag
