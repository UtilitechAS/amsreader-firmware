#pragma once

// ----------------------------------------------------------------------------
// Firmware update defaults
//
// Update these values to point the device towards the firmware feed that serves
// your customised builds. The defaults target the Neas firmware distribution.
// ----------------------------------------------------------------------------

#ifndef FIRMWARE_UPDATE_BASE_URL
#define FIRMWARE_UPDATE_BASE_URL "https://eivindh06.github.io/neas-amsreader-firmware-test"
#endif

#ifndef FIRMWARE_UPDATE_CHANNEL
#define FIRMWARE_UPDATE_CHANNEL "stable"
#endif

#ifndef FIRMWARE_UPDATE_USER_AGENT
#define FIRMWARE_UPDATE_USER_AGENT "NEAS-Firmware-Updater"
#endif

#ifndef FIRMWARE_UPDATE_USE_MANIFEST
#define FIRMWARE_UPDATE_USE_MANIFEST 1
#endif

#ifndef FIRMWARE_UPDATE_MANIFEST_NAME
#define FIRMWARE_UPDATE_MANIFEST_NAME "manifest.json"
#endif
