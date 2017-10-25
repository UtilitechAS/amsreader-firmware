## Sample for Kamstrup

This sample shows a full data package, including start and stop bits.
Please check the [data breakdown](./obisdata.md) for a more detailed description of each OBIS code value.

A good portion of raw sample data is also available in [HAN 20171019 xibriz4.txt](./HAN%2020171019%20xibriz4.txt)

### SAMPLE Data:

> **[2017-10-20 04.43.32.368 - Received 229 (0xE5) bytes]**
> 7E A0 E3 2B 21 13 98 86 E6 E7 00 0F 00 00 00 00 09 0C 07 E1 0A 14 05 03 2B 1E FF 80 00 00 02 19 0A 0E 4B 61 6D 73 74 72 75 70 5F 56 30 30 30 31 09 06 01 01 00 00 05 FF 0A 10 35 37 30 36 35 36 37 32 37 34 33 38 39 37 30 32 09 06 01 01 60 01 01 FF 0A 12 36 38 34 31 31 32 31 42 4E 32 34 33 31 30 31 30 34 30 09 06  01 01 01 07 00 FF 06 00 00 05 BC 09 06 01 01 02
> 07 00 FF 06 00 00 00 00  09 06 01 01 03 07 00 FF  06 00 00 00 00 09 06 01
> 01 04 07 00 FF 06 00 00  01 CE 09 06 01 01 1F 07  00 FF 06 00 00 02 34 09
> 06 01 01 33 07 00 FF 06  00 00 00 CA 09 06 01 01  47 07 00 FF 06 00 00 01
> FF 09 06 01 01 20 07 00  FF 12 00 E8 09 06 01 01  34 07 00 FF 12 00 E4 09
> 06 01 01 48 07 00 FF 12  00 E9 A1 A5 7E


### Breakdown:
Data	|	Explaination
--------|-------------
7E 	|	Frame Start flag
A	|	4 bits, A = Frame Format Type 3 (0b1010)
0 E3	|	12 bits, Frame size: 0xE3 (227 bytes, excluding start/end flags)
2B	|	Destination Address (Address can be 1-4 bytes. Terminated by LSB=1)
21 	|	Source Address (Address can be 1-4 bytes. Terminated by LSB=1)
13	|	Control Field
98 86	|	Header check sequence (HCS) (Check sum calculated from the address bytes, according to RFC 1662)
**Begin Data**|	LLC PDU Format (GB, 8.3 Fig 19)||
E6	|	Destination LSAP
E7	|	Source LSAP
00	|	LLC Quality
0F 	|	Information, n*8 bits?
00 00 00 00	|	(UNCERTAIN)
09 | String following (really, it's a date!)
0C | 12 bytes (length of the string)
07 E1	|	2017 (year)
0A	|	10 (oct)
14	|	20 (date)
05	|	friday 
03	|	03 (hour)
2B	|	43 (min)
1E	|	30 (sec)
FF 	|	(fff not specified)
80 	|	(deviation not specified?)
(...) | (...)
**End Data**|
A1 A5	|	Frame check sequence (FCS) (Check sum calculated from full frame, excluding flags and FCS, according to RFC 1662)
7E	|	Frame End flag
