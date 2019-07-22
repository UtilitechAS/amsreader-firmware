#ifndef _KAIFA_h
#define _KAIFA_h

enum class Kaifa : byte {
	List1			= 0x01,
	List1PhaseShort	= 0x09,
	List3PhaseShort	= 0x0D,
	List1PhaseLong	= 0x0E,
	List3PhaseLong	= 0x12
};

enum class Kaifa_List1 {
	ListSize,
	ActivePowerImported
};

enum class Kaifa_List3Phase {
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

enum class Kaifa_List1Phase {
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

#endif
