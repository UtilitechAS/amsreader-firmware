// Kamstrup.h

#ifndef _KAMSTRUP_h
#define _KAMSTRUP_h


enum class Kamstrup
{
	List1 = 0x19,
	List2 = 0x23
};

enum class Kamstrup_List1
{
	Time,
	ListSize,
	ListVersionIdentifier,
	MeterID_OBIS,
	MeterID,
	MeterType_OBIS,
	MeterType,
	ActiveImportPower_OBIS,
	ActiveImportPower,
	ActiveExportPower_OBIS,
	ActiveExportPower,
	ReactiveImportPower_OBIS,
	ReactiveImportPower,
	ReactiveExportPower_OBIS,
	ReactiveExportPower,
	CurrentL1_OBIS,
	CurrentL1,
	CurrentL2_OBIS,
	CurrentL2,
	CurrentL3_OBIS,
	CurrentL3,
	VoltageL1_OBIS,
	VoltageL1,
	VoltageL2_OBIS,
	VoltageL2,
	VoltageL3_OBIS,
	VoltageL3
};


enum class Kamstrup_List2
{
	Time,
	ListSize,
	ListVersionIdentifier,
	MeterID_OBIS,
	MeterID,
	MeterType_OBIS,
	MeterType,
	ActiveImportPower_OBIS,
	ActiveImportPower,
	ActiveExportPower_OBIS,
	ActiveExportPower,
	ReactiveImportPower_OBIS,
	ReactiveImportPower,
	ReactiveExportPower_OBIS,
	ReactiveExportPower,
	CurrentL1_OBIS,
	CurrentL1,
	CurrentL2_OBIS,
	CurrentL2,
	CurrentL3_OBIS,
	CurrentL3,
	VoltageL1_OBIS,
	VoltageL1,
	VoltageL2_OBIS,
	VoltageL2,
	VoltageL3_OBIS,
	VoltageL3,
	MeterClock_OBIS,
	MeterClock,
	CumulativeActiveImportEnergy_OBIS,
	CumulativeActiveImportEnergy,
	CumulativeActiveExportEnergy_OBIS,
	CumulativeActiveExportEnergy,
	CumulativeReactiveImportEnergy_OBIS,
	CumulativeReactiveImportEnergy,
	CumulativeReactiveExportEnergy_OBIS,
	CumulativeReactiveExportEnergy
};


#endif

