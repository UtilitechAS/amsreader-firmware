#include "KaifaHan.h"


byte KaifaHan::GetListID(byte *buffer, int start, int length)
{
  if (length > 23)
  {
    byte list = buffer[start + 23];
    if (list == List1) return List1;
    if (list == List2) return List2;
    if (list == List3) return List3;
  }
  return ListUnknown;
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

