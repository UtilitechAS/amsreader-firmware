using System;
using System.Collections.Generic;
using System.IO.Ports;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

namespace HanDebugger
{
    class Program
    {
        static List<byte> gBuffer = new List<byte>();

        static void Main(string[] args)
        {
            SerialPort vPort = new SerialPort("COM3", 2400, Parity.Even, 8, StopBits.One);
            vPort.DataReceived += VPort_DataReceived;
            vPort.Open();

            while (true)
                Thread.Sleep(100);
        }

        private static void VPort_DataReceived(object sender, SerialDataReceivedEventArgs e)
        {
            var vPort = sender as SerialPort;
            byte[] vBuffer = new byte[1000];
            int vBytesRead = vPort.Read(vBuffer, 0, vBuffer.Length);
            for (int i = 0; i < vBytesRead; i++)
            {
                gBuffer.Add(vBuffer[i]);
                
                // If we're catching a '7E' and it's not the beginning, it must be the end
                if (gBuffer.Count > 1 && vBuffer[i] == 0x7e)
                    WriteAndEmptyBuffer();
            }
        }

        private static void WriteAndEmptyBuffer()
        {
            Console.WriteLine();
            Console.WriteLine($"[{DateTime.Now:yyyy-MM-dd HH:mm:ss.fff} - Received {gBuffer.Count} (0x{gBuffer.Count:X2}) bytes]");

            int j = 0;
            foreach (var vByte in gBuffer)
            {
                Console.Write(string.Format("{0:X2} ", (int)vByte));

                if (++j % 8 == 0)
                    Console.Write(" ");

                if (j % 24 == 0)
                    Console.WriteLine();
            }

            Console.WriteLine();
            Console.WriteLine();

            gBuffer.Clear();
        }
    }
}
