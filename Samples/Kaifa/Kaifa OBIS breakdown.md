# One sample of each list on the Kaifa MA304H3E meter
(Samples are captured from my meter and the breakdown and values are from ![Kaifa HAN OBIS codes](Documentation/Kaifa%20HAN%20OBIS%20codes%20KFM_001.pdf). Please note that units and/or data types in this document doesn't seem to be right. E.g. my total meter reading is 403 kWh, not the tenfold. Hopefully, the current is also closer to 1A than 1000A in each wire)

For the data parts, I have found they follow a pattern where the first byte dictates the data type and size:
02 - one byte following
06 - long (4 bytes / 32 bits)
09 - string, first byte is length

## List #1 (0x01) (time is 1506275242) consumption is 601 Watts
Data | Explaination | Value
---- | ------------ | -----
E6 E7 00 0F 40 00 00 00 | Uknown | 
09 0C 07 E1 09 18 07 11 2F 16 FF 80 00 00 | Date and time | 2017-09-24 17:47:22
02 01 | List (02) of 1 elements (01) following | 0x01 = one element
06 00 00 02 59 | Active power+ (Q1+Q4) | 0.601 kW

## List #2 (0x0D) (time is 1506275240)
Data | Explaination | Value
---- | ------------ | -----
E6 E7 00 0F 40 00 00 00  | Unknown |
09 0C 07 E1 09 18 07 11 2F 14 FF 80 00 00 | Date and time | 2017-09-24 17:47:20 (FF=hundreds not available, 8000=deviation, 00=status)
02 0D | List (02) of 13 elements (0D) following | 0x0D = 13 elements
09 07 4B 46 4D 5F 30 30 31 | OBIL List version identifier | KFM_001
09 10 36 39 37 30 36 33 31 34 30 31 37 35 33 39 38 35 | Meter -ID (GIAI GS1 -16 digit ) | 6970631401753985
09 08 4D 41 33 30 34 48 33 45 | Meter type | MA304H3E
06 00 00 02 5C | Active power+ (Q1+Q4) | 0.604 kW
06 00 00 00 00 | Active power - (Q2+Q3) | 0.000 kW
06 00 00 00 00 | Reactive power + (Q1+Q2) | 0.000 kVAr
06 00 00 01 19 | Reactive power - (Q3+Q4) | 0.281 kVAr
06 00 00 06 BD | IL1 Current phase L1 | 1725 A
06 00 00 05 BA | IL2 Current phase L2 | 1466 A
06 00 00 08 86 | IL3 Current phase L3 | 2182 A
06 00 00 09 60 | ULN1 Phase voltage 4W meter, Line voltage 3W meter | 2400 V
06 00 00 00 00 | ULN2 Phase voltage 4W meter, Line voltage 3W meter | 0 V
06 00 00 09 78 | ULN3 Phase voltage 4W meter, Line voltage 3W meter | 2424 V

## List #3 (0x12) (time is 1506247210)
Data | Explaination | Value
---- | ------------ | -----
E6 E7 00 0F 40 00 00 00  | Unknown |
09 0C 07 E1 09 18 07 0A 00 0A FF 80 00 00 | Date and time | 2017-09-24 10:00:10
02 12 | List (02) of 18 elements (12) following | 0x12 = 18 elements
09 07 4B 46 4D 5F 30 30 31 | OBIL List version identifier | KFM_001
09 10 36 39 37 30 36 33 31 34 30 31 37 35 33 39 38 35 | Meter -ID (GIAI GS1 -16 digit ) | 6970631401753985
09 08 4D 41 33 30 34 48 33 45 | Meter type | MA304H3E
06 00 00 01 BA | Active power+ (Q1+Q4) | 0.442 kW
06 00 00 00 00 | Active power - (Q2+Q3) | 0.000 kW
06 00 00 00 00 | Reactive power + (Q1+Q2) | 0.000 kVAr
06 00 00 01 01 | Reactive power - (Q3+Q4) | 0.257 kVAr
06 00 00 04 92 | IL1 Current phase L1 | 1170 A
06 00 00 05 94 | IL2 Current phase L2 | 1428 A
06 00 00 05 A7 | IL3 Current phase L3 | 1447 A
06 00 00 09 62 | ULN1 Phase voltage 4W meter, Line voltage 3W meter | 2402 V
06 00 00 00 00 | ULN2 Phase voltage 4W meter, Line voltage 3W meter | 0 V
06 00 00 09 73 | ULN3 Phase voltage 4W meter, Line voltage 3W meter | 2419 V
09 0C 07 E1 09 18 07 0A 00 0A FF 80 00 00 | Clock and date in meter | 2017-09-24 10:00:10
06 00 06 27 21 | Cumulative hourly active import energy (A+) (Q1+Q4) | 403 233 kWh
06 00 00 00 00 | Cumulative hourly active export energy (A-) (Q2+Q3) | 0 kWh
06 00 00 03 5A | Cumulative hourly reactive import energy (R+) (Q1+Q2) | 858 kWh
06 00 00 DA AE | Cumulative hourly reactive export energy (R-) (Q3+Q4) | 55 982 kWh
