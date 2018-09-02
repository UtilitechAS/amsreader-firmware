#ifndef _KAIFA_h
#define _KAIFA_h

enum class Kaifa : byte {
	List1  = 0x01,
	List21 = 0x09, // list 2 for 1 fase
	List2  = 0x0D, // list 2 for 3 fase
	List31 = 0x0E, // list 3 for 1 fase
	List3  = 0x12  // list 3 for 3 fase
};

enum class Kaifa_List1 {
	ListSize,
	ActivePowerImported
};

enum class Kaifa_List21 {
	ListSize,
	ListVersionIdentifier,
	MeterID,
	MeterType,
	ActiveImportPower,
	ActiveExportPower,
	ReactiveImportPower,
	ReactiveExportPower,
	CurrentL1,
	VoltageL1
};

enum class Kaifa_List2 {
	ListSize,
	ListVersionIdentifier,
	MeterID,
	MeterType,
	ActiveImportPower,
	ActiveExportPower,
	ReactiveImportPower,
	ReactiveExportPower,
	CurrentL1,
	CurrentL2,
	CurrentL3,
	VoltageL1,
	VoltageL2,
	VoltageL3
};

enum class Kaifa_List31 {
	ListSize,
	ListVersionIdentifier,
	MeterID,
	MeterType,
	ActiveImportPower,
	ActiveExportPower,
	ReactiveImportPower,
	ReactiveExportPower,
	CurrentL1,
	VoltageL1,
	MeterClock,
	CumulativeActiveImportEnergy,
	CumulativeActiveExportEnergy,
	CumulativeReactiveImportEnergy,
	CumulativeReactiveExportEnergy
};

enum class Kaifa_List3 {
	ListSize,
	ListVersionIdentifier,
	MeterID,
	MeterType,
	ActiveImportPower,
	ActiveExportPower,
	ReactiveImportPower,
	ReactiveExportPower,
	CurrentL1,
	CurrentL2,
	CurrentL3,
	VoltageL1,
	VoltageL2,
	VoltageL3,
	MeterClock,
	CumulativeActiveImportEnergy,
	CumulativeActiveExportEnergy,
	CumulativeReactiveImportEnergy,
	CumulativeReactiveExportEnergy
};

enum class Kaifa_List4 { // TODO: Stop using list size like this? Only really need a single long list.
	ListSize,
	ListVersionIdentifier,
	MeterID,
	MeterType,
	ActiveImportPower,
	ActiveExportPower,
	ReactiveImportPower,
	ReactiveExportPower,
	CurrentL1,
	CurrentL2,
};

#endif
