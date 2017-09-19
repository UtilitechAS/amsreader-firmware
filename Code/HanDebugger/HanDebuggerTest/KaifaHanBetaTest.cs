using System;
using System.Linq;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using System.IO;
using System.Collections.Generic;
using HanDebugger;

namespace HanDebuggerTest
{
    [TestClass]
    [DeploymentItem(@"ESP 20170918 Raw.txt")]
    [DeploymentItem("Microsoft.VisualStudio.TestPlatform.TestFramework.Extensions.dll")]
    public class KaifaHanBetaTest
    {
        [TestMethod]
        public void TestGetPackageID()
        {
            Dictionary<byte, int> packageCount = new Dictionary<byte, int>();
            var packages = File.ReadAllLines("ESP 20170918 Raw.txt");
            var lines = packages.Select(line => line.Trim().Split(' ').Select(v => (byte)int.Parse(v, System.Globalization.NumberStyles.HexNumber)).ToArray()).ToArray();
            foreach (var line in lines)
            {
                byte list = KaifaHanBeta.GetPackageID(line, 0, line.Length);
                if (packageCount.ContainsKey(list))
                    packageCount[list]++;
                else
                    packageCount.Add(list, 1);
            }
            var startTime = new DateTime(1970, 1, 1, 0, 0, 0, 0).AddSeconds(KaifaHanBeta.GetPackageTime(lines[0], 0, lines[0].Length));
            var finishTime = new DateTime(1970, 1, 1, 0, 0, 0, 0).AddSeconds(KaifaHanBeta.GetPackageTime(lines[packages.Length - 1], 0, packages[packages.Length - 1].Length));
            var durationInSeconds = finishTime.Subtract(startTime).TotalSeconds;

            Assert.IsTrue(durationInSeconds > 60 * 60 * 4, $"There should be more than 4 hours of recording. Was: {durationInSeconds / 60 * 60 * 4}");

            double list3PerSecond = (1.0 / 3600.0);
            double list2PerSecond = (1.0 / 10.0) - list3PerSecond;
            double list1PerSecond = (1.0 / 2.0) - list2PerSecond;

            Assert.AreEqual(false, packageCount.ContainsKey(KaifaHanBeta.ListUnknown), "There should be no unknown packages");
            Assert.AreEqual(durationInSeconds * list1PerSecond, packageCount[KaifaHanBeta.List1], 1 + 0.01 * packageCount[KaifaHanBeta.List1], "There should be one 'List1' every 2.5s");
            Assert.AreEqual(durationInSeconds * list2PerSecond, packageCount[KaifaHanBeta.List2], 1 + 0.01 * packageCount[KaifaHanBeta.List2], "There should be one 'List2' every 10s");
            Assert.AreEqual(durationInSeconds * list3PerSecond, packageCount[KaifaHanBeta.List3], 1 + 0.01 * packageCount[KaifaHanBeta.List3], "There should be one 'List3' every 1h");

            double targetList1To2Ratio = 4.0;
            double actualList1To2Ratio = (double)packageCount[KaifaHanBeta.List1] / (double)packageCount[KaifaHanBeta.List2];
            Assert.AreEqual(targetList1To2Ratio, actualList1To2Ratio, 0.01, "There should be a ratio of List1:List2 of 4");
        }

        [TestMethod]
        public void TestGetConsumption()
        {
            List<int> consumption = new List<int>();
            var packages = File.ReadAllLines("ESP 20170918 Raw.txt");
            var lines = packages.Select(line => line.Trim().Split(' ').Select(v => (byte)int.Parse(v, System.Globalization.NumberStyles.HexNumber)).ToArray()).ToArray();
            foreach (var line in lines)
            {
                if (KaifaHanBeta.GetPackageID(line, 0, line.Length) == KaifaHanBeta.List1)
                {
                    consumption.Add(KaifaHanBeta.GetInt(0, line, 0, line.Length));
                }
            }
            Assert.AreEqual(1500.0, consumption.Average(), 500.0, "Consumption should be between 1000 and 2000 watts");
        }
    }
}
