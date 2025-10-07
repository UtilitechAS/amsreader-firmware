#pragma once

//-----------------------------------------------------------------------------//
// MQTT factory defaults
//
// Update these values before building the firmware to automatically provision
// new devices with a pre-configured MQTT connection. Leave any field empty to
// keep the existing behaviour (value will be cleared on first boot).
//
// For secrets, prefer supplying overrides via `build_flags` in a private
// PlatformIO config (for example `platformio-user.ini`) so credentials never
// live in version control:
//
//   build_flags = ... -D MQTT_DEFAULT_USERNAME=\"user\" -D MQTT_DEFAULT_PASSWORD=\"pass\"
//-----------------------------------------------------------------------------//

// Broker hostname or IP address (leave empty to disable automatic MQTT setup)
#ifndef MQTT_DEFAULT_HOST
#define MQTT_DEFAULT_HOST "mqtt.neas.no"
#endif

// Broker port (1883 for plain MQTT, 8883 for TLS)
#ifndef MQTT_DEFAULT_PORT
#define MQTT_DEFAULT_PORT 1883
#endif

// Optional username/password credentials
#ifndef MQTT_DEFAULT_USERNAME
#define MQTT_DEFAULT_USERNAME ""
#endif

#ifndef MQTT_DEFAULT_PASSWORD
#define MQTT_DEFAULT_PASSWORD ""
#endif

// Default client identifier (leave empty to auto-generate from device ID)
#ifndef MQTT_DEFAULT_CLIENT_ID
#define MQTT_DEFAULT_CLIENT_ID ""
#endif

// Default publish and subscribe topics
#ifndef MQTT_DEFAULT_PUBLISH_TOPIC
#define MQTT_DEFAULT_PUBLISH_TOPIC "amsreader/telemetry"
#endif

#ifndef MQTT_DEFAULT_SUBSCRIBE_TOPIC
#define MQTT_DEFAULT_SUBSCRIBE_TOPIC "amsreader/command"
#endif

// Payload format (0: JSON classic, 5: JSON multi-topic, etc.)
#ifndef MQTT_DEFAULT_PAYLOAD_FORMAT
#define MQTT_DEFAULT_PAYLOAD_FORMAT 0
#endif

// Whether to use TLS when connecting (false/true)
#ifndef MQTT_DEFAULT_SSL
#define MQTT_DEFAULT_SSL false
#endif

// Enable periodic state updates (set to true and pick an interval below)
#ifndef MQTT_DEFAULT_STATE_UPDATE
#define MQTT_DEFAULT_STATE_UPDATE false
#endif

// Interval for state updates in seconds (only used if STATE_UPDATE is true)
#ifndef MQTT_DEFAULT_STATE_UPDATE_INTERVAL
#define MQTT_DEFAULT_STATE_UPDATE_INTERVAL 60
#endif

// Connection timeout (ms) and keepalive (seconds)
#ifndef MQTT_DEFAULT_TIMEOUT
#define MQTT_DEFAULT_TIMEOUT 1000
#endif

#ifndef MQTT_DEFAULT_KEEPALIVE
#define MQTT_DEFAULT_KEEPALIVE 60
#endif
