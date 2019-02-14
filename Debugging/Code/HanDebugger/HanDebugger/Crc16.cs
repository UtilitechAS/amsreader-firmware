using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace HanDebugger
{
    public static class Crc16
    {
        private const ushort polynomial = 0x8408;
        private static ushort[] table = new ushort[256];

        static Crc16()
        {
            ushort value;
            ushort temp;
            for (ushort i = 0; i < table.Length; ++i)
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

        public static ushort ComputeChecksum(byte[] data)
        {
            return ComputeChecksum(data, 0, data.Length);
        }

        public static ushort ComputeChecksum(byte[] data, int start, int length)
        {
            ushort fcs = 0xffff;
            for (int i = start; i < (start + length); i++)
            {
                var index = (fcs ^ data[i]) & 0xff;
                fcs = (ushort)((fcs >> 8) ^ table[index]);
            }
            fcs ^= 0xffff;
            return fcs;
        }
    }
}

