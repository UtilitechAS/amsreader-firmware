// Aidon.h

#ifndef _AIDON_h
#define _AIDON_h


enum class Aidon
{
	List1 = 0x01,
	List2 = 0x0D
};

enum class Aidon_List1
{
	ListSize,
	IGN_0,
	ActiveImportPower_OBIS,
	ActiveImportPower,
	IGN_1,
	ActiveImportPowerInt8,
	ActiveImportPowerEnum
};


enum class Aidon_List2
{
	ListSize,
	IGN_0,
	ListVersionIdentifier_OBIS,
	ListVersionIdentifier,
	IGN_0,
	MeterID_OBIS,
	MeterID,
	IGN_0,
	MeterType_OBIS,
	MeterType,
	IGN_0,
	ActiveImportPower_OBIS,
	ActiveImportPower,
	IGN_0,
	ActiveImportPowerInt8,
	ActiveImportPowerEnum,
	IGN_0,
	ActiveExportPower_OBIS,
	ActiveExportPower,
	IGN_0,
	ActiveExportPowerInt8,
	ActiveExportPowerEnum,
	IGN_0,
	ReactiveImportPower_OBIS,
	ReactiveImportPower,
	IGN_0,
	ReactiveImportPowerInt8,
	ReactiveImportPowerEnum,
	IGN_0,
	ReactiveExportPower_OBIS,
	ReactiveExportPower,
	IGN_0,
	ReactiveExportPowerInt8,
	ReactiveExportPowerEnum,
	IGN_0,
	CurrentL1_OBIS,
	CurrentL1,
	IGN_0,
	CurrentL1Int8,
	CurrentL1Enum,
	IGN_0,
	CurrentL2_OBIS,
	CurrentL2,
	IGN_0,
	CurrentL2Int8,
	CurrentL2Enum,
	IGN_0,
	CurrentL3_OBIS,
	CurrentL3,
	IGN_0,
	CurrentL3Int8,
	CurrentL3Enum,
	IGN_0,
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

