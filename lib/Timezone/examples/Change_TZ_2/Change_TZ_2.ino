// Arduino Timezone Library Copyright (C) 2018 by Jack Christensen and
// licensed under GNU GPL v3.0, https://www.gnu.org/licenses/gpl.html
//
// Arduino Timezone Library example sketch.
// Demonstrates changing time zones using an array of Timezone objects.
// Uses a pushbutton switch to change between the four US continental time zones.
// Tested with Arduino 1.8.5 and an Arduino Uno.
//
// Jack Christensen 02Jan2018

#include <JC_Button.h>          // http://github.com/JChristensen/JC_Button
#include <Streaming.h>          // http://arduiniana.org/libraries/streaming/
#include <Timezone.h>           // http://github.com/JChristensen/Timezone

const uint8_t BUTTON_PIN(8);    // connect a button from this pin to ground
Button btn(BUTTON_PIN);

//Continental US Time Zones
TimeChangeRule EDT = { "EDT", Second, Sun, Mar, 2, -240 };    //Daylight time = UTC - 4 hours
TimeChangeRule EST = { "EST", First, Sun, Nov, 2, -300 };     //Standard time = UTC - 5 hours
Timezone Eastern(EDT, EST);
TimeChangeRule CDT = { "CDT", Second, Sun, Mar, 2, -300 };    //Daylight time = UTC - 5 hours
TimeChangeRule CST = { "CST", First, Sun, Nov, 2, -360 };     //Standard time = UTC - 6 hours
Timezone Central(CDT, CST);
TimeChangeRule MDT = { "MDT", Second, Sun, Mar, 2, -360 };    //Daylight time = UTC - 6 hours
TimeChangeRule MST = { "MST", First, Sun, Nov, 2, -420 };     //Standard time = UTC - 7 hours
Timezone Mountain(MDT, MST);
TimeChangeRule PDT = { "PDT", Second, Sun, Mar, 2, -420 };    //Daylight time = UTC - 7 hours
TimeChangeRule PST = { "PST", First, Sun, Nov, 2, -480 };     //Standard time = UTC - 8 hours
Timezone Pacific(PDT, PST);
Timezone* timezones[] = { &Eastern, &Central, &Mountain, &Pacific };
Timezone* tz;                   //pointer to the time zone
uint8_t tzIndex;                //indexes the timezones[] array
TimeChangeRule* tcr;            //pointer to the time change rule, use to get TZ abbrev

void setup()
{
    // set the system time to UTC
    // warning: assumes that compileTime() returns US EST
    // adjust the following line accordingly if you're in another time zone
    setTime(compileTime() + 300 * 60);

    btn.begin();
    Serial.begin(115200);
    tz = timezones[tzIndex];
}

void loop()
{
    // print the time if it's changed
    static time_t tLast;
    time_t t = now();
    if (t != tLast)
    {
        tLast = t;
        printDateTime(t);
        Serial << " UTC  ";
        TimeChangeRule* tcr;    //pointer to current time change rule, used to get TZ abbrev
        printDateTime((*tz).toLocal(t, &tcr));
        Serial << " " << tcr -> abbrev;
        Serial << endl;
    }

    // change the time zone if button pressed
    btn.read();
    if (btn.wasPressed())
    {
        if ( ++tzIndex >= sizeof(timezones) / sizeof(timezones[0]) ) tzIndex = 0;
        Serial << "tzIndex " << tzIndex << endl;
        tz = timezones[tzIndex];
    }
}

void printDateTime(time_t t)
{
    Serial << ((day(t)<10) ? "0" : "") << _DEC(day(t));
    Serial << monthShortStr(month(t)) << _DEC(year(t)) << ' ';
    Serial << ((hour(t)<10) ? "0" : "") << _DEC(hour(t)) << ':';
    Serial << ((minute(t)<10) ? "0" : "") << _DEC(minute(t)) << ':';
    Serial << ((second(t)<10) ? "0" : "") << _DEC(second(t));
}

// function to return the compile date and time as a time_t value
time_t compileTime()
{
    const time_t FUDGE(10);    //fudge factor to allow for upload time, etc. (seconds, YMMV)
    const char *compDate = __DATE__, *compTime = __TIME__, *months = "JanFebMarAprMayJunJulAugSepOctNovDec";
    char compMon[4], *m;

    strncpy(compMon, compDate, 3);
    compMon[3] = '\0';
    m = strstr(months, compMon);

    tmElements_t tm;
    tm.Month = ((m - months) / 3 + 1);
    tm.Day = atoi(compDate + 4);
    tm.Year = atoi(compDate + 7) - 1970;
    tm.Hour = atoi(compTime);
    tm.Minute = atoi(compTime + 3);
    tm.Second = atoi(compTime + 6);

    time_t t = makeTime(tm);
    return t + FUDGE;        //add fudge factor to allow for compile time
}

