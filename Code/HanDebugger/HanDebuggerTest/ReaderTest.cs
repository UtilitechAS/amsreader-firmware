using System;
using System.Linq;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using System.IO;
using HanDebugger;

namespace HanDebuggerTest
{
    [TestClass]
    [DeploymentItem(@"SampleData.txt")]
    [DeploymentItem(@"Kamstrup228.txt")]
    [DeploymentItem("Microsoft.VisualStudio.TestPlatform.TestFramework.Extensions.dll")]
    public class ReaderTest
    {
        [TestMethod]
        public void TestParser()
        {
            var lines = File.ReadAllLines("SampleData.txt");
            var sample = lines[0].Trim();
            byte[] bytes = sample.Split(' ').Select(v => (byte)int.Parse(v, System.Globalization.NumberStyles.HexNumber)).ToArray();
            var reader = new Reader(bytes);
            Assert.IsTrue(reader.IsValid(), "Data is not valid");
        }

        [TestMethod]
        public void TestAllLines()
        {
            var lines = File.ReadAllLines("SampleData.txt");
            for (int i=0; i<lines.Length; i++)
            {
                var sample = lines[i].Trim();
                byte[] bytes = sample.Split(' ').Select(v => (byte)int.Parse(v, System.Globalization.NumberStyles.HexNumber)).ToArray();
                var reader = new Reader(bytes);
                Assert.IsTrue(reader.IsValid(), $"Data is not valid (Line #{i + 1}): {sample}");
            }
        }


        [TestMethod]
        public void TestKamstrup228()
        {
            var text = File.ReadAllText("Kamstrup228.txt");
            byte[] bytes = text.Trim().Split(' ').Select(v => (byte)int.Parse(v, System.Globalization.NumberStyles.HexNumber)).ToArray();
            var reader = new Reader(bytes);
            Assert.IsTrue(reader.IsValid(), "Data is not valid");
        }

    }
}
