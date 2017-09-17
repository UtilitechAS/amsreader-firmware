using System;
using System.Linq;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using HanDebugger;

namespace HanDebuggerTest
{
    [TestClass]
    public class Crc16Test
    {
        [TestMethod]
        public void Crc16ChecksumTest()
        {
            TestChecksum(new byte[] { 0x80, 0x93 }, "7E A0 79 01 02 01 10", 1, 6);
            TestChecksum(new byte[] { 0x80, 0x93 }, "7E A0 79 01 02 01 10 80 93 E6 E7 00 0F 40 00 00 00 09 0C 07 E1 09 0E 04 15 11 00 FF 80 00 00 02 0D 09 07 4B 46 4D 5F 30 30 31 09 10 36 39 37 30 36 33 31 34 30 31 37 35 33 39 38 35 09 08 4D 41 33 30 34 48 33 45 06 00 00 02 FC 06 00 00 00 00 06 00 00 00 00 06 00 00 00 8C 06 00 00 08 1C 06 00 00 07 97 06 00 00 0A CA 06 00 00 09 5E 06 00 00 00 00 06 00 00 09 66 FD 27 7E", 1, 6);
            TestChecksum(new byte[] { 0x80, 0x93 }, "A0 79 01 02 01 10");
            TestChecksum(new byte[] { 0xA7, 0x44 }, "A0 79 01 02 01 10 80 93 E6 E7 00 0F 40 00 00 00 09 0C 07 E1 09 0E 04 15 11 0A FF 80 00 00 02 0D 09 07 4B 46 4D 5F 30 30 31 09 10 36 39 37 30 36 33 31 34 30 31 37 35 33 39 38 35 09 08 4D 41 33 30 34 48 33 45 06 00 00 02 FF 06 00 00 00 00 06 00 00 00 00 06 00 00 00 8A 06 00 00 08 22 06 00 00 07 9A 06 00 00 0A D4 06 00 00 09 5B 06 00 00 00 00 06 00 00 09 66");
            //string bytesAsString = "A0 79 01 02 01 10 80 93 E6 E7 00 0F 40 00 00 00 09 0C 07 E1 09 0E 04 15 11 0A FF 80 00 00 02 0D 09 07 4B 46 4D 5F 30 30 31 09 10 36 39 37 30 36 33 31 34 30 31 37 35 33 39 38 35 09 08 4D 41 33 30 34 48 33 45 06 00 00 02 FF 06 00 00 00 00 06 00 00 00 00 06 00 00 00 8A 06 00 00 08 22 06 00 00 07 9A 06 00 00 0A D4 06 00 00 09 5B 06 00 00 00 00 06 00 00 09 66";
            //byte[] bytes = bytesAsString.Split(' ').Select(v => (byte)int.Parse(v, System.Globalization.NumberStyles.HexNumber)).ToArray();
            //var checksum = Crc16.ComputeChecksum(bytes);
            //var checksumBytes = ConvertToBytes(checksum);
            //Assert.IsTrue(Equals(new byte[] { 0xa7, 0x44 }, checksumBytes), "Checksum did not match");
        }

        private void TestChecksum(byte[] validChecksum, string dataAsString)
        {
            byte[] bytes = dataAsString.Trim().Split(' ').Select(v => (byte)int.Parse(v, System.Globalization.NumberStyles.HexNumber)).ToArray();
            var checksum = Crc16.ComputeChecksum(bytes);
            var checksumBytes = ConvertToBytes(checksum);
            Assert.IsTrue(Equals(validChecksum, checksumBytes), $"Checksum did not match. Was: {Format(checksumBytes)}, Expected: {Format(validChecksum)}");
        }

        private void TestChecksum(byte[] validChecksum, string dataAsString, int start, int length)
        {
            // "A0 79 01 02 01 10"
            byte[] bytes = dataAsString.Trim().Split(' ').Select(v => (byte)int.Parse(v, System.Globalization.NumberStyles.HexNumber)).ToArray();
            var checksum = Crc16.ComputeChecksum(bytes, start, length);
            var checksumBytes = ConvertToBytes(checksum);
            Assert.IsTrue(Equals(validChecksum, checksumBytes), $"Checksum did not match. Was: {Format(checksumBytes)}, Expected: {Format(validChecksum)}");
        }

        private string Format(byte[] buffer)
        {
            return string.Join(" ", buffer.Select(v => $"{v:X}").ToArray());
        }

        private static byte[] ConvertToBytes(ushort checksum)
        {
            return new byte[]
            {
                (byte)(checksum & 0xff),
                (byte)((checksum >> 8) & 0xff)
            };
        }

        private static bool Equals(byte[] a1, byte[] a2)
        {
            if (a1 == null || a2 == null) return false;
            if (a1?.Length != a2?.Length) return false;
            for (int i = 0; i < a1.Length; i++)
                if (a1[i] != a2[i])
                    return false;
            return true;
        }
    }
}
