// Arduino Timezone Library Copyright (C) 2018 by Jack Christensen and
// licensed under GNU GPL v3.0, https://www.gnu.org/licenses/gpl.html
//
// Arduino Timezone Library example sketch.
// Self-adjusting clock for one time zone using an external real-time
// clock, either a DS1307 or DS3231 (e.g. Chronodot).
// Assumes the RTC is set to UTC.
// TimeChangeRules can be hard-coded or read from EEPROM, see comments.
// Check out the Chronodot at http://www.macetech.com/store/
//
// Jack Christensen Aug 2012

#include <DS1307RTC.h>   // https://github.com/PaulStoffregen/DS1307RTC
#include <Timezone.h>    // https://github.com/JChristensen/Timezone

// US Eastern Time Zone (New York, Detroit)
TimeChangeRule myDST = {"EDT", Second, Sun, Mar, 2, -240};    //Daylight time = UTC - 4 hours
TimeChangeRule mySTD = {"EST", First, Sun, Nov, 2, -300};     //Standard time = UTC - 5 hours
Timezone myTZ(myDST, mySTD);

// If TimeChangeRules are already stored in EEPROM, comment out the three
// lines above and uncomment the line below.
//Timezone myTZ(100);       //assumes rules stored at EEPROM address 100

TimeChangeRule *tcr;        //pointer to the time change rule, use to get TZ abbrev

void setup()
{
    Serial.begin(115200);
    setSyncProvider(RTC.get);   // the function to get the time from the RTC
    if(timeStatus()!= timeSet)
        Serial.println("Unable to sync with the RTC");
    else
        Serial.println("RTC has set the system time");
}

void loop()
{
    time_t utc = now();
    time_t local = myTZ.toLocal(utc, &tcr);
    Serial.println();
    printDateTime(utc, "UTC");
    printDateTime(local, tcr -> abbrev);
    delay(10000);
}

// format and print a time_t value, with a time zone appended.
void printDateTime(time_t t, const char *tz)
{
    char buf[32];
    char m[4];    // temporary storage for month string (DateStrings.cpp uses shared buffer)
    strcpy(m, monthShortStr(month(t)));
    sprintf(buf, "%.2d:%.2d:%.2d %s %.2d %s %d %s",
        hour(t), minute(t), second(t), dayShortStr(weekday(t)), day(t), m, year(t), tz);
    Serial.println(buf);
}

