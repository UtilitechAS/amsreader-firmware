#include "Crc16.h"

Crc16Class::Crc16Class()
{
    unsigned short value;
    unsigned short temp;
    for (unsigned short i = 0; i < 256; ++i)
    {
        value = 0;
        temp = i;
        for (byte j = 0; j < 8; ++j)
        {
            if (((value ^ temp) & 0x0001) != 0)
            {
                value = (ushort)((value >> 1) ^ polynomial);
            }
            else
            {
                value >>= 1;
            }
            temp >>= 1;
        }
        table[i] = value;
    }
}

unsigned short Crc16Class::ComputeChecksum(byte *data, int start, int length)
{
    ushort fcs = 0xffff;
    for (int i = start; i < (start + length); i++)
    {
        byte index = (fcs ^ data[i]) & 0xff;
        fcs = (ushort)((fcs >> 8) ^ table[index]);
    }
    fcs ^= 0xffff;
    return fcs;
}
