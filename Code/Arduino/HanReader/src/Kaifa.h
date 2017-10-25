#ifndef _KAIFA_h
#define _KAIFA_h

enum class Kaifa : byte {
	List1 = 0x01,
	List2 = 0x0D,
	List3 = 0x12
};

enum class Kaifa_List1 {
	Time,
	ListSize,
	ActivePowerImported
};

enum class Kaifa_List2 {
	Time,
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

enum class Kaifa_List3 {
	Time,
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

#endif
