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
	IGN_1,
	MeterID_OBIS,
	MeterID,
	IGN_2,
	MeterType_OBIS,
	MeterType,
	IGN_3,
	ActiveImportPower_OBIS,
	ActiveImportPower,
	IGN_4,
	ActiveImportPowerInt8,
	ActiveImportPowerEnum,
	IGN_5,
	ActiveExportPower_OBIS,
	ActiveExportPower,
	IGN_6,
	ActiveExportPowerInt8,
	ActiveExportPowerEnum,
	IGN_7,
	ReactiveImportPower_OBIS,
	ReactiveImportPower,
	IGN_8,
	ReactiveImportPowerInt8,
	ReactiveImportPowerEnum,
	IGN_9,
	ReactiveExportPower_OBIS,
	ReactiveExportPower,
	IGN_10,
	ReactiveExportPowerInt8,
	ReactiveExportPowerEnum,
	IGN_11,
	CurrentL1_OBIS,
	CurrentL1,
	IGN_12,
	CurrentL1Int8,
	CurrentL1Enum,
	IGN_13,
	CurrentL2_OBIS,
	CurrentL2,
	IGN_14,
	CurrentL2Int8,
	CurrentL2Enum,
	IGN_15,
	CurrentL3_OBIS,
	CurrentL3,
	IGN_16,
	CurrentL3Int8,
	CurrentL3Enum,
	IGN_17,
	VoltageL1_OBIS,
	VoltageL1,
	IGN_18,
	VoltageL1Int8,
	VoltageL1Enum,
	IGN_19,
	VoltageL2_OBIS,
	VoltageL2,
	IGN_20,
	VoltageL2Int8,
	VoltageL2Enum,
	IGN_21,
	VoltageL3_OBIS,
	VoltageL3,
	IGN_22,
	VoltageL3Int8,
	VoltageL3Enum
};


#endif

