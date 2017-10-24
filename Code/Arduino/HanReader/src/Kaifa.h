#ifndef _KAIFA_h
#define _KAIFA_h

enum class Kaifa : byte {
	List1 = 0x01,
	List2 = 0x0D,
	List3 = 0x12
};

enum class Kaifa_List1_ObisObjects {
	ActivePowerImported
};

enum class Kaifa_List2_ObisObjects {
	Time,
	ListID,
	ObisListVersionIdentifier,
	MeterID,
	MeterType,
	ActivePowerImported,
	ActivePowerExported,
	ReactivePowerImported,
	ReactivePowerExported,
	CurrentPhaseL1,
	CurrentPhaseL2,
	CurrentPhaseL3,
	VoltagePhaseL1,
	VoltagePhaseL2,
	VoltagePhaseL3
};

enum class Kaifa_List3_ObisObjects {
	Time,
	ListID,
	ObisListVersionIdentifier,
	MeterID,
	MeterType,
	ActivePowerImported,
	ActivePowerExported,
	ReactivePowerImported,
	ReactivePowerExported,
	CurrentPhaseL1,
	CurrentPhaseL2,
	CurrentPhaseL3,
	VoltagePhaseL1,
	VoltagePhaseL2,
	VoltagePhaseL3,
	ClockAndDate,
	TotalActiveEnergyImported,
	TotalActiveEnergyExported,
	TotalReactiveEnergyImported,
	TotalReactiveEnergyExported
};

#endif
