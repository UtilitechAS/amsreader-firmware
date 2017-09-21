using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace HanDebugger
{
    // As it seems Kaifa/Valider put some premature firmware on my AMS,
    // there's need for some dirty hacks to get any information
    public class KaifaHanBeta
    {
        public const byte ListUnknown = 0x00;
        public const byte List1 = 0x01;
        public const byte List2 = 0x0D;
        public const byte List3 = 0x12;

        public static byte GetListID(byte[] package, int start, int length)
        {
            switch (package[start + 23])
            {
                case List1:
                case List2:
                case List3:
                    return package[start + 23];
                default:
                    return 0x00;
            }
        }

        public static double GetPackageTime(byte[] package, int start, int length)
        {
            const int timeStart = 10;
            int year = package[start + timeStart] << 8 |
                package[start + timeStart + 1];

            int month = package[start + timeStart + 2];
            int day = package[start + timeStart + 3];
            int hour = package[start + timeStart + 5];
            int minute = package[start + timeStart + 6];
            int second = package[start + timeStart + 7];


            return new DateTime(year, month, day, hour, minute, second).Subtract(new DateTime(1970, 1, 1, 0, 0, 0, 0)).TotalSeconds;
        }

        public static int GetInt(int dataPosition, byte[] buffer, int start, int length)
        {
            const int dataStart = 24;
            int value = 0;
            int foundPosition = 0;
            for (int i = start + dataStart; i < start + length; i++)
            {
                if (foundPosition == 0)
                {
                    if (buffer[i] == 0x06)
                        foundPosition = i;
                }
                else
                {
                    value = value << 8 |
                        buffer[i];
                    if (i == foundPosition + 4)
                        return value;
                }
            }
            return 0;
        }
    }
}
