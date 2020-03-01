// Kamstrup.h

#ifndef _KAMSTRUP_h
#define _KAMSTRUP_h

enum class Kamstrup
{
	List3PhaseShort = 0x19,
	List3PhaseLong  = 0x23,
	List1PhaseShort = 0x11,
	List1PhaseLong  = 0x1B
};

enum class Kamstrup_List3Phase
{
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

enum class Kamstrup_List1Phase
{
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
	VoltageL1_OBIS,
	VoltageL1,
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

