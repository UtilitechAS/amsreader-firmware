#include "HanToJson.h"
#include "Aidon.h"
#include "Kaifa.h"
#include "Kamstrup.h"


static void hanToJsonKaifa(JsonObject& data, HanReader& hanReader, Stream *debugger)
{
    int listSize = hanReader.getListSize();

    if (listSize == (int)Kaifa::List1)
    {
        // Handle listSize == 1 specially
        data["P"]   = hanReader.getInt(     (int)Kaifa_List1::ActivePowerImported);
        return;
    }

    if (listSize >=  (int)Kaifa::List21)
    {
        data["lv"]   = hanReader.getString(  (int)Kaifa_List3::ListVersionIdentifier);
        data["id"]   = hanReader.getString(  (int)Kaifa_List3::MeterID);
        data["type"] = hanReader.getString(  (int)Kaifa_List3::MeterType);
        data["P"]    = hanReader.getInt(     (int)Kaifa_List3::ActiveImportPower);
        data["Q"]    = hanReader.getInt(     (int)Kaifa_List3::ReactiveImportPower);
        data["I1"]   = hanReader.getInt(     (int)Kaifa_List3::CurrentL1);
        data["I2"]   = hanReader.getInt(     (int)Kaifa_List3::CurrentL2);
    }

    if (listSize >= (int)Kaifa::List2)
    {
        data["I3"]   = hanReader.getInt(     (int)Kaifa_List3::CurrentL3);
        data["U1"]   = hanReader.getInt(     (int)Kaifa_List3::VoltageL1);
        data["U2"]   = hanReader.getInt(     (int)Kaifa_List3::VoltageL2);
        data["U3"]   = hanReader.getInt(     (int)Kaifa_List3::VoltageL3);
    }

    if (listSize >  (int)Kaifa::List3) // Note: Bug in Kaifa? (Should have been '>=')
    {
        data["tPI"]  = hanReader.getInt(     (int)Kaifa_List3::CumulativeActiveImportEnergy);
        data["tPO"]  = hanReader.getInt(     (int)Kaifa_List3::CumulativeActiveExportEnergy);
        data["tQI"]  = hanReader.getInt(     (int)Kaifa_List3::CumulativeReactiveImportEnergy);
        data["tQO"]  = hanReader.getInt(     (int)Kaifa_List3::CumulativeReactiveExportEnergy);
    }
}

static void hanToJsonAidon(JsonObject& data, HanReader& hanReader, Stream *debugger)
{
    int listSize = hanReader.getListSize();

    // Based on the list number, get all details
    // according to OBIS specifications for the meter
    if (listSize == (int)Aidon::List1)
    {
        // Handle listSize == 1 specially
        data["P"] = hanReader.getInt((int)Aidon_List1::ActiveImportPower);
        return;
    }

    if (listSize >= (int)Aidon::List21)
    {
        data["lv"]      = hanReader.getString(          (int)Aidon_List3::ListVersionIdentifier);
        data["id"]      = hanReader.getString(          (int)Aidon_List3::MeterID);
        data["type"]    = hanReader.getString(          (int)Aidon_List3::MeterType);
        data["P"]       = hanReader.getInt(             (int)Aidon_List3::ActiveImportPower);
        data["Q"]       = hanReader.getInt(             (int)Aidon_List3::ReactiveExportPower);
        data["I1"]      = ((double) hanReader.getInt(   (int)Aidon_List3::CurrentL1)) / 10;
        data["I2"]      = ((double) hanReader.getInt(   (int)Aidon_List3::CurrentL2)) / 10;
    }

    if (listSize >= (int)Aidon::List2)
    {
        data["I3"]      = ((double) hanReader.getInt(   (int)Aidon_List3::CurrentL3)) / 10;
        data["U1"]      = ((double) hanReader.getInt(   (int)Aidon_List3::VoltageL1)) / 10;
        data["U2"]      = ((double) hanReader.getInt(   (int)Aidon_List3::VoltageL2)) / 10;
        data["U3"]      = ((double) hanReader.getInt(   (int)Aidon_List3::VoltageL3)) / 10;
    }

    if (listSize >= (int)Aidon::List3)
    {
        data["tPI"]     = hanReader.getInt(             (int)Aidon_List3::CumulativeActiveImportEnergy);
        data["tPO"]     = hanReader.getInt(             (int)Aidon_List3::CumulativeActiveExportEnergy);
        data["tQI"]     = hanReader.getInt(             (int)Aidon_List3::CumulativeReactiveImportEnergy);
        data["tQO"]     = hanReader.getInt(             (int)Aidon_List3::CumulativeReactiveExportEnergy);
    }
}

static void hanToJsonKamstrup(JsonObject& data, HanReader& hanReader, Stream *debugger)
{
    int listSize = hanReader.getListSize();

    if (listSize >= (int)Kamstrup::List1)
    {
        data["lv"]      = hanReader.getString(  (int)Kamstrup_List2::ListVersionIdentifier);
        data["id"]      = hanReader.getString(  (int)Kamstrup_List2::MeterID);
        data["type"]    = hanReader.getString(  (int)Kamstrup_List2::MeterType);
        data["P"]       = hanReader.getInt(     (int)Kamstrup_List2::ActiveImportPower);
        data["Q"]       = hanReader.getInt(     (int)Kamstrup_List2::ReactiveImportPower);
        data["I1"]      = hanReader.getInt(     (int)Kamstrup_List2::CurrentL1);
        data["I2"]      = hanReader.getInt(     (int)Kamstrup_List2::CurrentL2);
        data["I3"]      = hanReader.getInt(     (int)Kamstrup_List2::CurrentL3);
        data["U1"]      = hanReader.getInt(     (int)Kamstrup_List2::VoltageL1);
        data["U2"]      = hanReader.getInt(     (int)Kamstrup_List2::VoltageL2);
        data["U3"]      = hanReader.getInt(     (int)Kamstrup_List2::VoltageL3);
    }

    if (listSize >= (int)Kamstrup::List2)
    {
        data["tPI"]     = hanReader.getInt(     (int)Kamstrup_List2::CumulativeActiveImportEnergy);
        data["tPO"]     = hanReader.getInt(     (int)Kamstrup_List2::CumulativeActiveExportEnergy);
        data["tQI"]     = hanReader.getInt(     (int)Kamstrup_List2::CumulativeReactiveImportEnergy);
        data["tQO"]     = hanReader.getInt(     (int)Kamstrup_List2::CumulativeReactiveExportEnergy);
    }
}

void hanToJson(JsonObject& data, byte meterType, HanReader& hanReader, Stream *debugger)
{
    // Based on the list number, get all details
    // according to OBIS specifications for the meter
    switch (meterType)
    {
        case 1: // Kaifa
            return hanToJsonKaifa(data, hanReader, debugger);
        case 2: // Aidon
            return hanToJsonAidon(data, hanReader, debugger);
        case 3: // Kamstrup
            return hanToJsonKamstrup(data, hanReader, debugger);
        default:
            if (debugger) {
                debugger->print("Meter type ");
                debugger->print(meterType, HEX);
                debugger->println(" is unknown");
            }
            break;
    }
}

void hanToJson(JsonObject& data, byte meterType, HanReader& hanReader)
{
    return hanToJson(data, meterType, hanReader, NULL);
}
