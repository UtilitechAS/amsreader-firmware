#include "AmsWebServer.h"

AmsWebServer::AmsWebServer(uint8_t* buf, RemoteDebug* Debug, HwTools* hw) {
	this->debugger = Debug;
	this->hw = hw;
	this->buf = (char*) buf;
}

void AmsWebServer::setup(AmsConfiguration* config, GpioConfig* gpioConfig, MeterConfig* meterConfig, AmsData* meterState, AmsDataStorage* ds, EnergyAccounting* ea) {
    this->config = config;
	this->gpioConfig = gpioConfig;
	this->meterConfig = meterConfig;
	this->meterState = meterState;
	this->ds = ds;
	this->ea = ea;
}