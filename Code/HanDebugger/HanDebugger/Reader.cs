using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace HanDebugger
{
    public class Reader
    {
        private int position;
        private int dataLength;
        private byte[] buffer;

        public Reader(byte[] buffer)
        {
            this.buffer = buffer;
            position = 0;
            dataLength = ((buffer[1] & 0x0F) << 8) | buffer[2];
        }

        public bool IsValid()
        {
            return IsValidStart() &&
                IsValidLength() &&
                IsValidHeaderChecksum() &&
                IsValidChecksum();
        }

        private bool IsValidChecksum()
        {
            ushort checkSum = GetChecksum(dataLength - 2);
            return checkSum == Crc16.ComputeChecksum(buffer, 1, dataLength - 2);
        }

        private bool IsValidHeaderChecksum()
        {
            int headerLength = GetHeaderLength();
            ushort checkSum = GetChecksum(headerLength);
            return checkSum == Crc16.ComputeChecksum(buffer, 1, headerLength);
        }

        private ushort GetChecksum(int checksumPosition)
        {
            return (ushort)(buffer[checksumPosition + 2] << 8 |
                buffer[checksumPosition + 1]);
        }

        private int GetHeaderLength()
        {
            var pos = position + 3; // Dest address
            while ((buffer[pos] & 0x01) == 0x00)
                pos++;
            pos++; // source address
            while ((buffer[pos] & 0x01) == 0x00)
                pos++;
            pos++; // control field
            return pos;
        }

        private bool IsValidLength()
        {
            return buffer.Length >= dataLength + 2;
        }

        public bool IsValidStart()
        {
            return (buffer[0] == 0x7E);
        }

    }
}
