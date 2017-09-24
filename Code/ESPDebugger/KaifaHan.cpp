#include "KaifaHan.h"


byte KaifaHan::GetListID(byte *buffer, int start, int length)
{
  if (length > 23)
  {
    byte list = buffer[start + 23];
    if (list == (byte)List::List1) return (byte)List::List1;
    if (list == (byte)List::List2) return (byte)List::List2;
    if (list == (byte)List::List3) return (byte)List::List3;
  }
  return (byte)List::ListUnknown;
}

long KaifaHan::GetPackageTime(byte *buffer, int start, int length)
{
  const int timeStart = 10;
  int year = buffer[start + timeStart] << 8 |
      buffer[start + timeStart + 1];
  
  int month = buffer[start + timeStart + 2];
  int day = buffer[start + timeStart + 3];
  int hour = buffer[start + timeStart + 5];
  int minute = buffer[start + timeStart + 6];
  int second = buffer[start + timeStart + 7];

  return toUnixTime(year, month, day, hour, minute, second);
}

int KaifaHan::GetInt(int dataPosition, byte *buffer, int start, int length)
{
  int valuePosition = findValuePosition(dataPosition, buffer, start, length);
  if (valuePosition > 0)
  {
    int value = 0;
    for (int i = valuePosition + 1; i < valuePosition + 5; i++)
    {
        value = value << 8 | buffer[i];
    }
    return value;
  }
  return 0;
}

int KaifaHan::findValuePosition(int dataPosition, byte *buffer, int start, int length)
{
  const int dataStart = 24;
  for (int i=start + dataStart; i<length; i++)
  {
      if (dataPosition-- == 0)
        return i;
       else if (buffer[i] == 0x09) // string value
        i += buffer[i+1] + 1;
       else if (buffer[i] == 0x06) // integer value
        i += 4;
       else 
        return 0; // unknown data type found
  }
  return 0;
}

String KaifaHan::GetString(int dataPosition, byte *buffer, int start, int length)
{
  int valuePosition = findValuePosition(dataPosition, buffer, start, length);
  if (valuePosition > 0)
  {
    String value = String("");
    for (int i = valuePosition + 2; i < valuePosition + buffer[valuePosition + 1]; i++)
    {
        value += String((char)buffer[i]);
    }
    return value;
  }
  return String("");
}

time_t KaifaHan::toUnixTime(int year, int month, int day, int hour, int minute, int second)
{
  byte daysInMonth[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
  long secondsPerMinute = 60;
  long secondsPerHour = secondsPerMinute * 60;
  long secondsPerDay = secondsPerHour * 24;

  long time = (year - 1970) * secondsPerDay * 365L;

  for (int yearCounter = 1970; yearCounter<year; yearCounter++)
    if ((yearCounter % 4 == 0) && ((yearCounter % 100 != 0) || (yearCounter % 400 == 0)))
      time += secondsPerDay;

  if (month > 2 && (year % 4 == 0) && ((year % 100 != 0) || (year % 400 == 0)))
    time += secondsPerDay;

  for (int monthCounter = 1; monthCounter<month; monthCounter++)
    time += daysInMonth[monthCounter - 1] * secondsPerDay;

  time += (day - 1) * secondsPerDay;
  time += hour * secondsPerHour;
  time += minute * secondsPerMinute;
  time += second;

  return (time_t)time;
}

