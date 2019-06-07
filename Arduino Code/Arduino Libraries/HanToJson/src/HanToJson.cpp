#include "HanToJson.h"
#include "Aidon.h"
#include "Kaifa.h"
#include "Kamstrup.h"


static void hanToJsonKaifa3phase(int listSize, JsonObject& data, HanReader& hanReader, Stream *debugger)
{
    if (listSize >= (int)Kaifa::List2)
    {
        data["lv"]   = hanReader.getString(  (int)Kaifa_List3::ListVersionIdentifier);
        data["id"]   = hanReader.getString(  (int)Kaifa_List3::MeterID);
        data["type"] = hanReader.getString(  (int)Kaifa_List3::MeterType);
        data["P"]    = hanReader.getInt(     (int)Kaifa_List3::ActiveImportPower);
        data["Q"]    = hanReader.getInt(     (int)Kaifa_List3::ReactiveImportPower);
        data["I1"]   = hanReader.getInt(     (int)Kaifa_List3::CurrentL1);
        data["I2"]   = hanReader.getInt(     (int)Kaifa_List3::CurrentL2);
        data["I3"]   = hanReader.getInt(     (int)Kaifa_List3::CurrentL3);
        data["U1"]   = hanReader.getInt(     (int)Kaifa_List3::VoltageL1);
        data["U2"]   = hanReader.getInt(     (int)Kaifa_List3::VoltageL2);
        data["U3"]   = hanReader.getInt(     (int)Kaifa_List3::VoltageL3);
    }

    if (listSize >=  (int)Kaifa::List3)
    {
        data["tPI"]  = hanReader.getInt(     (int)Kaifa_List3::CumulativeActiveImportEnergy);
        data["tPO"]  = hanReader.getInt(     (int)Kaifa_List3::CumulativeActiveExportEnergy);
        data["tQI"]  = hanReader.getInt(     (int)Kaifa_List3::CumulativeReactiveImportEnergy);
        data["tQO"]  = hanReader.getInt(     (int)Kaifa_List3::CumulativeReactiveExportEnergy);
    }
}

static void hanToJsonKaifa1phase(int listSize, JsonObject& data, HanReader& hanReader, Stream *debugger)
{
    if (listSize >=  (int)Kaifa::List21)
    {
        data["lv"]   = hanReader.getString(  (int)Kaifa_List31::ListVersionIdentifier);
        data["id"]   = hanReader.getString(  (int)Kaifa_List31::MeterID);
        data["type"] = hanReader.getString(  (int)Kaifa_List31::MeterType);
        data["P"]    = hanReader.getInt(     (int)Kaifa_List31::ActiveImportPower);
        data["Q"]    = hanReader.getInt(     (int)Kaifa_List31::ReactiveImportPower);
        data["I1"]   = hanReader.getInt(     (int)Kaifa_List31::CurrentL1);
        data["U1"]   = hanReader.getInt(     (int)Kaifa_List31::VoltageL1);
    }

    if (listSize >=  (int)Kaifa::List31)
    {
        data["tPI"]  = hanReader.getInt(     (int)Kaifa_List31::CumulativeActiveImportEnergy);
        data["tPO"]  = hanReader.getInt(     (int)Kaifa_List31::CumulativeActiveExportEnergy);
        data["tQI"]  = hanReader.getInt(     (int)Kaifa_List31::CumulativeReactiveImportEnergy);
        data["tQO"]  = hanReader.getInt(     (int)Kaifa_List31::CumulativeReactiveExportEnergy);
    }
}

static void hanToJsonKaifa(JsonObject& data, HanReader& hanReader, Stream *debugger)
{
    int listSize = hanReader.getListSize();

    if (listSize == (int)Kaifa::List1)
    {
        // Handle listSize == 1 specially
        data["P"]   = hanReader.getInt(     (int)Kaifa_List1::ActivePowerImported);
        return;
    }

    switch (listSize) {
        case (int)Kaifa::List2:
        case (int)Kaifa::List3:
            return hanToJsonKaifa3phase(listSize, data, hanReader, debugger);
        case (int)Kaifa::List21:
        case (int)Kaifa::List31:
            return hanToJsonKaifa1phase(listSize, data, hanReader, debugger);
		default:
			if (debugger) debugger->printf("Warning: Unknown listSize %d\n", listSize);
			return;
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

static void hanToJsonKamstrup3phase(int listSize, JsonObject& data, HanReader& hanReader, Stream *debugger)
{
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

static void hanToJsonKamstrup1phase(int listSize, JsonObject& data, HanReader& hanReader, Stream *debugger)
{
    if (listSize >= (int)Kamstrup::List3)
    {
        data["lv"]      = hanReader.getString(  (int)Kamstrup_List4::ListVersionIdentifier);
        data["id"]      = hanReader.getString(  (int)Kamstrup_List4::MeterID);
        data["type"]    = hanReader.getString(  (int)Kamstrup_List4::MeterType);
        data["P"]       = hanReader.getInt(     (int)Kamstrup_List4::ActiveImportPower);
        data["Q"]       = hanReader.getInt(     (int)Kamstrup_List4::ReactiveImportPower);
        data["I1"]      = hanReader.getInt(     (int)Kamstrup_List4::CurrentL1);
        data["U1"]      = hanReader.getInt(     (int)Kamstrup_List4::VoltageL1);
    }

    if (listSize >= (int)Kamstrup::List4)
    {
        data["tPI"]     = hanReader.getInt(     (int)Kamstrup_List4::CumulativeActiveImportEnergy);
        data["tPO"]     = hanReader.getInt(     (int)Kamstrup_List4::CumulativeActiveExportEnergy);
        data["tQI"]     = hanReader.getInt(     (int)Kamstrup_List4::CumulativeReactiveImportEnergy);
        data["tQO"]     = hanReader.getInt(     (int)Kamstrup_List4::CumulativeReactiveExportEnergy);
    }
}

static void hanToJsonKamstrup(JsonObject& data, HanReader& hanReader, Stream *debugger)
{
    int listSize = hanReader.getListSize();

    switch (listSize) {
        case (int)Kamstrup::List1:
        case (int)Kamstrup::List2:
            return hanToJsonKamstrup3phase(listSize, data, hanReader, debugger);
        case (int)Kamstrup::List3:
        case (int)Kamstrup::List4:
            return hanToJsonKamstrup1phase(listSize, data, hanReader, debugger);
		default:
			if (debugger) debugger->printf("Warning: Unknown listSize %d\n", listSize);
			return;
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
