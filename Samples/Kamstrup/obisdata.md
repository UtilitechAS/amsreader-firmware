# One sample of each list on the Kamstrup ??? meter

For the data parts, I have found they follow a pattern where the first byte dictates the data type and size:
02 - one byte following
06 - long (4 bytes / 32 bits)
12 - int (2 bytes / 16 bit)
09 - string, first byte is length (could it be a byte array?)
0A - string, first byte is length

## List #1 (0x19)
Data | Explaination | Value
---- | ------------ | -----
E6 E7 00 0F 00 00 00 00 | Uknown | 
09 0C 07 E1 0A 14 05 03 2B 1E FF 80 00 00 | Date and time | 2017-10-20 (fre) 03:43:30
02 19 | List ID | 25 (List 1)
0A 0E 4B 61 6D 73 74 72 75 70 5F 56 30 30 30 31 | OBIS List Version Identifier | Kamstrup_V0001
09 06 01 01 00 00 05 FF | OBIS for Meter ID | 1.1.0.0.5.255
0A 10 35 37 30 36 35 36 37 32 37 34 33 38 39 37 30 32 | Meter ID | 5706567274389702
09 06 01 01 60 01 01 FF | OBIS for Meter Type | 1.1.96.1.1.255
0A 12 36 38 34 31 31 32 31 42 4E 32 34 33 31 30 31 30 34 30 | Meter Type | 6841121BN243101040
09 06 01 01 01 07 00 FF | OBIS for Active Power + | 1.1.1.7.0.255
06 00 00 05 BC | Active Power + | 1468 W
09 06 01 01 02 07 00 FF | OBIS for Active Power - | 1.1.2.7.0.255
06 00 00 00 00 | Active Power - | 0 W
09 06 01 01 03 07 00 FF | OBIS for Reactive Power + | 1.1.3.7.0.255
06 00 00 00 00 | Reactive Power + | 0W
09 06 01 01 04 07 00 FF | OBIS for Reactive Power - | 1.1.4.7.0.255
06 00 00 01 CE | Reactive Power - | 462W
09 06 01 01 1F 07 00 FF | OBIS for L1 Current | 1.1.31.7.0.255
06 00 00 02 34 | L1 Current | 5.64 A
09 06 01 01 33 07 00 FF | OBIS for L2 Current | 1.1.51.7.0.255
06 00 00 00 CA | L2 Current | 2.02 A
09 06 01 01 47 07 00 FF | OBIS for L3 Current | 1.1.71.7.0.255
06 00 00 01 FF | L3 Current | 5.11 A
09 06 01 01 20 07 00 FF | OBIS for L1 Voltage | 1.1.32.7.0.255
12 00 E8 | L1 Voltage | 232 V
09 06 01 01 34 07 00 FF | OBIS for L2 Voltage | 1.1.52.7.0.255
12 00 E4 | L1 Voltage | 228 V
09 06 01 01 48 07 00 FF | OBIS for L3 Voltage | 1.1.72.7.0.255
12 00 E9 | L1 Voltage | 233 V
