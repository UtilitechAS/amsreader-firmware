using HanDebugger;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace HanDebuggerTest
{
    [TestClass]
    public class DlmsReaderTest
    {
        [TestMethod]
        public void TestDlmsReader()
        {
            var text = File.ReadAllText("SampleData.txt").Replace("\r\n", " ").Replace("  ", " ");
            byte[] bytes = text.Trim().Split(' ').Select(v => (byte)int.Parse(v, System.Globalization.NumberStyles.HexNumber)).ToArray();
            var reader = new DlmsReader();
            int packages = 0;
            for (int i=0; i<bytes.Length; i++)
            {
                if (reader.Read(bytes[i]))
                {
                    packages++;
                    //byte[] data = reader.GetRawData();
                }
            }
            Assert.IsTrue(packages == 559, $"There should be 559 packages. Was: {packages}");

        }


        [TestMethod]
        public void TestDlmsReaderWithError()
        {
            var text = File.ReadAllText("SampleData.txt").Replace("\r\n", " ").Replace("  ", " ");
            byte[] bytes = text.Trim().Split(' ').Select(v => (byte)int.Parse(v, System.Globalization.NumberStyles.HexNumber)).ToArray();
            bytes = bytes.Skip(10).ToArray();
            var reader = new DlmsReader();
            int packages = 0;
            for (int i = 0; i < bytes.Length; i++)
            {
                if (reader.Read(bytes[i]))
                {
                    packages++;
                    //byte[] data = reader.GetRawData();
                }
            }
            Assert.IsTrue(packages == 558, $"There should be 558 packages. Was: {packages}");

        }
    }
}
