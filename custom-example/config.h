/**
 * Custom build configuration overrides.
 *
 * Copy this file (and the rest of custom-example/) to custom/ and uncomment
 * the defines you want to override.  custom/ is gitignored — never commit it.
 *
 * Each define here takes effect before the firmware defaults are applied, so
 * your value wins.  Any define you leave commented out uses the built-in default.
 */

// Default Wi-Fi Access Point SSID shown during initial setup
//#define DEFAULT_AP_SSID "MyMeterReader"

// URL the firmware checks for OTA upgrades
//#define FIRMWARE_UPGRADE_URL "https://example.com/firmware/"

// Default UI language tag (must match a key in the translation files)
//#define DEFAULT_LANGUAGE "en"


// ---------------------------------------------------------------------------
// Custom MQTT connector
//
// When enabled, the firmware runs a second MQTT connector in parallel with
// the user-configurable broker (the one set via the web UI).  The custom
// connector is driven entirely by the macros below — it does not read from
// EEPROM and cannot be changed at runtime.
//
// To enable: uncomment CUSTOM_MQTT_HOST and any other fields you need to
// override.  Anything left commented falls back to the defaults shown.
//
// Supported payload formats: 0 / 5 / 6 (JSON variants), 1 / 2 (raw).
// Domoticz (3), Home Assistant (4) and Passthrough (255) are intentionally
// not supported here because they depend on additional runtime config that
// belongs to the user-facing broker slot.
// ---------------------------------------------------------------------------

//#define CUSTOM_MQTT_HOST "mqtt.example.com"
//#define CUSTOM_MQTT_PORT 1883
//#define CUSTOM_MQTT_USERNAME ""
//#define CUSTOM_MQTT_PASSWORD ""
//#define CUSTOM_MQTT_PUBLISH_TOPIC "ams"
//#define CUSTOM_MQTT_SUBSCRIBE_TOPIC ""   // empty -> "<publish-topic>/command"
//#define CUSTOM_MQTT_PAYLOAD_FORMAT 0     // 0 JSON, 1/2 raw, 5/6 JSON variants
//#define CUSTOM_MQTT_SSL false            // true requires CUSTOM_MQTT_PORT 8883 (typically)
//#define CUSTOM_MQTT_CA_VERIFY false      // true requires a CA cert provisioned via the normal flow
//#define CUSTOM_MQTT_KEEPALIVE 60         // seconds
//#define CUSTOM_MQTT_TIMEOUT 1000         // milliseconds
//#define CUSTOM_MQTT_STATE_UPDATE false   // periodic state publish
//#define CUSTOM_MQTT_STATE_UPDATE_INTERVAL 10  // seconds
