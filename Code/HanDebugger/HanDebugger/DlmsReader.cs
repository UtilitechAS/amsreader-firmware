using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace HanDebugger
{
    public class DlmsReader
    {
        private byte[] buffer;
        private int position;
        private int dataLength;
        private byte frameFormatType;
        private byte[] destinationAddress;
        private byte[] sourceAddress;

        public DlmsReader()
        {
            Clear();
        }

        private void Clear()
        {
            buffer = new byte[256];
            position = 0;
            dataLength = 0;
            destinationAddress = null;
            sourceAddress = null;
            frameFormatType = 0;
        }

        public bool Read(byte data)
        {
            if (position == 0 && data != 0x7E)
            {
                // we haven't started yet, wait for the start flag (no need to capture any data yet)
                return false;
            }
            else
            {
                // We have completed reading of one package, so clear and be ready for the next
                if (dataLength > 0 && position >= dataLength + 2)
                    Clear();
                
                // Check if we're about to run into a buffer overflow
                if (position >= buffer.Length)
                    Clear();

                // Check if this is a second start flag, which indicates the previous one was a stop from the last package
                if (position == 1 && data == 0x7E)
                {
                    // just return, we can keep the one byte we had in the buffer
                    return false;
                }

                // We have started, so capture every byte
                buffer[position++] = data;

                if (position == 1)
                {
                    // This was the start flag, we're not done yet
                    return false;
                }
                else if (position == 2)
                {
                    // Capture the Frame Format Type
                    frameFormatType = (byte)(data & 0xF0);
                    if (!IsValidFrameFormat(frameFormatType))
                        Clear();
                    return false;
                }
                else if (position == 3)
                {
                    // Capture the length of the data package
                    dataLength = ((buffer[1] & 0x0F) << 8) | buffer[2];
                    return false;
                }
                else if (destinationAddress == null)
                {
                    // Capture the destination address
                    destinationAddress = GetAddress(3);
                    if (destinationAddress?.Length > 3)
                        Clear();
                    return false;
                }
                else if (sourceAddress == null)
                {
                    // Capture the source address
                    sourceAddress = GetAddress(3 + destinationAddress.Length);
                    if (sourceAddress?.Length > 3)
                        Clear();
                    return false;
                }
                else if (position == 4 + destinationAddress.Length + sourceAddress.Length + 2)
                {
                    // Verify the header checksum
                    var headerChecksum = GetChecksum(position - 3);
                    if (headerChecksum != Crc16.ComputeChecksum(buffer, 1, position - 3))
                        Clear();
                    return false;
                }
                else if (position == dataLength + 1)
                {
                    // Verify the data package checksum
                    var checksum = GetChecksum(position - 3);
                    if (checksum != Crc16.ComputeChecksum(buffer, 1, position - 3))
                        Clear();
                    return false;
                }
                else if (position == dataLength + 2)
                {
                    // We're done, check the stop flag and signal we're done
                    if (data == 0x7E)
                        return true;
                    else
                    {
                        Clear();
                        return false;
                    }
                }
            }
            return false;
        }

        private bool IsValidFrameFormat(byte frameFormatType)
        {
            return frameFormatType == 0xA0;
        }

        public byte[] GetRawData()
        {
            if (dataLength > 0 && position == dataLength + 2)
            {
                var headerLength = 3 + destinationAddress.Length + sourceAddress.Length + 2;
                return buffer.Skip(headerLength + 1).Take(dataLength - headerLength - 2).ToArray();
            }
            else
                return null;
        }

        private byte[] GetAddress(int addressPosition)
        {
            List<byte> address = new List<byte>();
            for (int i = addressPosition; i < position; i++)
            {
                address.Add(buffer[i]);
                
                // LSB=1 means this was the last address byte
                if ((buffer[i] & 0x01) == 0x01)  
                    break;

                // See if we've reached last byte, try again when we've got more data
                else if (i == position - 1)
                    return null;
            }
            return address.ToArray();
        }

        private ushort GetChecksum(int checksumPosition)
        {
            return (ushort)(buffer[checksumPosition + 2] << 8 |
                buffer[checksumPosition + 1]);
        }
    }
}
