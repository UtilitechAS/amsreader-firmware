#pragma once

// ----------------------------------------------------------------------------
// Firmware update defaults
//
// Update these values to point the device towards the firmware feed that serves
// your customised builds. The defaults target the Neas firmware distribution.
// ----------------------------------------------------------------------------

#ifndef FIRMWARE_UPDATE_BASE_URL
#define FIRMWARE_UPDATE_BASE_URL "http://firmware.neas.no"
#endif

#ifndef FIRMWARE_UPDATE_CHANNEL
#define FIRMWARE_UPDATE_CHANNEL "stable"
#endif

#ifndef FIRMWARE_UPDATE_USER_AGENT
#define FIRMWARE_UPDATE_USER_AGENT "NEAS-Firmware-Updater"
#endif
