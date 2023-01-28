#include <Timezone.h>

#define JULY1970 15634800

TimeChangeRule TC_GMT  = {"GMT",  Last, Sun, Jan, 0,   0};
TimeChangeRule TC_WET  = {"WET",  Last, Sun, Oct, 2,   0};
TimeChangeRule TC_WEST = {"WEST", Last, Sun, Mar, 1,  60};
TimeChangeRule TC_CET  = {"CET",  Last, Sun, Oct, 3,  60};
TimeChangeRule TC_CEST = {"CEST", Last, Sun, Mar, 2, 120};
TimeChangeRule TC_EET  = {"EET",  Last, Sun, Oct, 4, 120};
TimeChangeRule TC_EEST = {"EEST", Last, Sun, Mar, 3, 180};

Timezone GMT                   = Timezone(TC_GMT);
Timezone WesterEuropean        = Timezone(TC_WET, TC_WEST);
Timezone CentralEuropean       = Timezone(TC_CET, TC_CEST);
Timezone EasternEuropean       = Timezone(TC_EET, TC_EEST);

Timezone* resolveTimezone(char* name) {
    if(strncmp_P(name, PSTR("Europe/"), 7) == 0) {
        if(strncmp_P(name+7, PSTR("Amsterdam"), 9) == 0)
            return &CentralEuropean;
        if(strncmp_P(name+7, PSTR("Athens"), 6) == 0)
            return &EasternEuropean;
        if(strncmp_P(name+7, PSTR("Belfast"), 7) == 0)
            return &WesterEuropean;
        if(strncmp_P(name+7, PSTR("Berlin"), 6) == 0)
            return &CentralEuropean;
        if(strncmp_P(name+7, PSTR("Bratislava"), 10) == 0)
            return &CentralEuropean;
        if(strncmp_P(name+7, PSTR("Brussels"), 8) == 0)
            return &CentralEuropean;
        if(strncmp_P(name+7, PSTR("Bucharest"), 9) == 0)
            return &EasternEuropean;
        if(strncmp_P(name+7, PSTR("Budapest"), 8) == 0)
            return &CentralEuropean;
        if(strncmp_P(name+7, PSTR("Copenhagen"), 10) == 0)
            return &CentralEuropean;
        if(strncmp_P(name+7, PSTR("Dublin"), 6) == 0)
            return &WesterEuropean;
        if(strncmp_P(name+7, PSTR("Helsinki"), 8) == 0)
            return &EasternEuropean;
        if(strncmp_P(name+7, PSTR("Lisbon"), 6) == 0)
            return &WesterEuropean;
        if(strncmp_P(name+7, PSTR("Ljubljana"), 9) == 0)
            return &CentralEuropean;
        if(strncmp_P(name+7, PSTR("London"), 6) == 0)
            return &WesterEuropean;
        if(strncmp_P(name+7, PSTR("Luxembourg"), 10) == 0)
            return &CentralEuropean;
        if(strncmp_P(name+7, PSTR("Madrid"), 6) == 0)
            return &CentralEuropean;
        if(strncmp_P(name+7, PSTR("Malta"), 5) == 0)
            return &CentralEuropean;
        if(strncmp_P(name+7, PSTR("Nicosia"), 7) == 0)
            return &EasternEuropean;
        if(strncmp_P(name+7, PSTR("Oslo"), 4) == 0)
            return &CentralEuropean;
        if(strncmp_P(name+7, PSTR("Paris"), 5) == 0)
            return &CentralEuropean;
        if(strncmp_P(name+7, PSTR("Podgorica"), 9) == 0)
            return &CentralEuropean;
        if(strncmp_P(name+7, PSTR("Prague"), 6) == 0)
            return &CentralEuropean;
        if(strncmp_P(name+7, PSTR("Riga"), 4) == 0)
            return &EasternEuropean;
        if(strncmp_P(name+7, PSTR("Rome"), 4) == 0)
            return &CentralEuropean;
        if(strncmp_P(name+7, PSTR("Sofia"), 5) == 0)
            return &EasternEuropean;
        if(strncmp_P(name+7, PSTR("Stockholm"), 9) == 0)
            return &CentralEuropean;
        if(strncmp_P(name+7, PSTR("Tallinn"), 7) == 0)
            return &EasternEuropean;
        if(strncmp_P(name+7, PSTR("Vienna"), 6) == 0)
            return &CentralEuropean;
        if(strncmp_P(name+7, PSTR("Vilnius"), 7) == 0)
            return &EasternEuropean;
        if(strncmp_P(name+7, PSTR("Warsaw"), 6) == 0)
            return &CentralEuropean;
        if(strncmp_P(name+7, PSTR("Zagreb"), 6) == 0)
            return &CentralEuropean;
        if(strncmp_P(name+7, PSTR("Zurich"), 6) == 0)
            return &CentralEuropean;
    }
    return &GMT;
}
