# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

AMS Reader (`ams2mqtt`) is PlatformIO/Arduino firmware for ESP8266 and ESP32 devices that reads data from European IEC-62056 compliant smart electricity meters via M-Bus/HAN port, decodes it, and publishes to MQTT. It also serves a Svelte SPA web interface.

## Prerequisites

- Python 3.9+ with `platformio` and `css_html_js_minify` (`pip install -U platformio css_html_js_minify`)
- Node.js 19.x (for Svelte UI)

## Setup (first time)

```bash
# Required: copy and customize local build config
cp platformio-user.ini-example platformio-user.ini

# Build Svelte frontend FIRST (must exist before firmware build)
cd lib/SvelteUi/app && npm ci && npm run build && cd -

# Install PlatformIO library dependencies
pio pkg install
```

## Build Commands

```bash
pio run -e esp32s2          # Build for a specific target (esp8266, esp32, esp32s2, esp32s3, esp32c3, esp32solo)
pio run                     # Build all envs defined in platformio-user.ini default_envs
pio run -e esp32s2 -t upload  # Build and flash to connected device
pio device monitor          # Serial monitor at 115200 baud
pio test                    # Run Unity tests (requires device or native env)
```

### Svelte UI local development

```bash
cd lib/SvelteUi/app
npm run local   # Vite dev server with proxy to a real device (vite.config.local.js)
```

## Architecture

### Data Flow

```
Smart Meter (UART/HAN)
  --> MeterCommunicator (Passive/KMP/Pulse)
      --> AmsDecoder (HDLC -> LLC -> GCM decrypt -> DLMS/DSMR/COSEM parse)
          --> AmsData (in-memory state)
              --> AmsDataStorage (LittleFS hourly/daily history)
              --> EnergyAccounting (peak tracking + cost calc via PriceService)
              --> RealtimePlot (circular buffer for live wattage)
              --> AmsMqttHandler (MQTT publish: JSON/HA/Domoticz/Raw/Passthrough)
              --> AmsWebServer (REST API + embedded Svelte UI)
```

### Entry Point

`src/AmsToMqttBridge.cpp` — the main Arduino sketch (`setup()` / `loop()`) with all global state. Complex logic is in `lib/` PlatformIO libraries.

### Web UI Compilation

The Svelte build output is minified, gzip-compressed, and embedded as C byte arrays in `lib/SvelteUi/include/html/*.h` by `lib/SvelteUi/scripts/generate_includes.py` (a pre-build step). The web server serves these embedded arrays directly — LittleFS is not used for UI assets.

### Pre-build Scripts

Two Python scripts run automatically before compilation:
- `scripts/addversion.py` — writes `lib/FirmwareVersion/src/generated_version.h` from git hash or `$GITHUB_TAG`
- `lib/SvelteUi/scripts/generate_includes.py` — embeds minified/compressed Svelte dist into C headers

## Key Conventions

### Platform Abstraction

The same codebase targets ESP8266 and multiple ESP32 variants. Debug output is gated throughout all libraries:
```cpp
#if defined(AMS_REMOTE_DEBUG)
  // uses RemoteDebug* (telnet debug)
#else
  // uses Stream*/Print*
#endif
```

### PROGMEM Strings

All string literals in library code use `PSTR()` + `printf_P()` to store strings in flash — critical for ESP8266 RAM constraints.

### Configuration Layout

Config is EEPROM-backed with fixed byte offsets defined in `lib/AmsConfiguration/`. The constant `EEPROM_CHECK_SUM = 104` must be incremented whenever any config struct layout changes. Migration from older config versions is handled in `AmsConfiguration.cpp`.

### Local Dev Overrides

`platformio-user.ini` (gitignored) is loaded via `extra_configs = platformio-user.ini` in `platformio.ini`. Never commit personal `upload_port`, secrets, or dev-only build flags to `platformio.ini`. Development environments with `AMS_REMOTE_DEBUG` and optional features (`AMS_CLOUD`, `AMS_KMP`) are defined there.

### API Keys / Secrets

`platformio.ini` contains `NO_*` placeholder values for API keys (price service, cloud connectors). CI injects real keys via `sed` at build time. Do not add real keys to tracked files.

### KmpTalker (Kamstrup)

Kamstrup KMP protocol support requires a precompiled closed-source static library in `precompiled/<env>/libKmpTalker.a`. It is only available for ESP32 variants and enabled with `AMS_KMP=1` build flag.

### 64-bit Uptime

Use `millis64()` from `lib/Uptime/` instead of `millis()` to avoid the 49-day rollover.
