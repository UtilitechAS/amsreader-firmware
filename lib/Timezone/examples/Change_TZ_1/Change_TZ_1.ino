// Arduino Timezone Library Copyright (C) 2018 by Jack Christensen and
// licensed under GNU GPL v3.0, https://www.gnu.org/licenses/gpl.html
//
// Arduino Timezone Library example sketch.
// Demonstrates changing timezone "on the fly".
// Uses a pushbutton switch to change between the four continental US time zones.
// The current timezone setting is saved in EEPROM so it is remembered if
// the power is cycled.
// Tested with Arduino 1.8.5 and an Arduino Uno.
//
// Jack Christensen 02Jan2018

#include <avr/eeprom.h>
#include <JC_Button.h>          // http://github.com/JChristensen/JC_Button
#include <Streaming.h>          // http://arduiniana.org/libraries/streaming/
#include <Timezone.h>           // http://github.com/JChristensen/Timezone

const uint8_t BUTTON_PIN(8);    // connect a button from this pin to ground
Button btn(BUTTON_PIN);

uint8_t tzIndex;            //index to the arrays below
EEMEM uint8_t ee_tzIndex;   //copy of tzIndex persisted in EEPROM
const char* dstNames[] = {"EDT", "CDT", "MDT", "PDT"};
const char* stdNames[] = {"EST", "CST", "MST", "PST"};
const int dstOffsets[] = {-240, -300, -360, -420};
const int stdOffsets[] = {-300, -360, -420, -480};

TimeChangeRule dstRule = {"EDT", Second, Sun, Mar, 2, -240};
TimeChangeRule stdRule = {"EST", First, Sun, Nov, 2, -300};
Timezone tz(dstRule, stdRule);

void setup()
{
    // set the system time to UTC
    // warning: assumes that compileTime() returns US EST
    // adjust the following line accordingly if you're in another time zone
    setTime(compileTime() + 300 * 60);

    // get tzIndex from eeprom and ensure that it's valid
    tzIndex = eeprom_read_byte( &ee_tzIndex );
    if ( tzIndex >= sizeof(stdOffsets) / sizeof(stdOffsets[0]) )
    {
        tzIndex = 0;
        eeprom_write_byte( &ee_tzIndex, tzIndex);
    }

    btn.begin();
    Serial.begin(115200);
    changeTZ();
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
        printDateTime(tz.toLocal(t, &tcr));
        Serial << " " << tcr -> abbrev;
        Serial << endl;
    }

    // change the time zone if button pressed
    btn.read();
    if (btn.wasPressed())
    {
        if ( ++tzIndex >= sizeof(stdOffsets) / sizeof(stdOffsets[0]) ) tzIndex = 0;
        changeTZ();
    }
}

void changeTZ()
{
    Serial << "tzIndex " << tzIndex << endl;
    eeprom_update_byte( &ee_tzIndex, tzIndex );
    dstRule.offset = dstOffsets[tzIndex];
    stdRule.offset = stdOffsets[tzIndex];
    strcpy(dstRule.abbrev, dstNames[tzIndex]);
    strcpy(stdRule.abbrev, stdNames[tzIndex]);
    tz.setRules(dstRule, stdRule);
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

