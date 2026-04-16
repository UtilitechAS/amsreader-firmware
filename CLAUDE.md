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
cd ui && npm ci && npm run build && cd -

# Install PlatformIO library dependencies
pio pkg install
```

## Build Commands

```bash
pio run -e esp32s2            # Build for a specific target (esp8266, esp32, esp32s2, esp32s3, esp32c3, esp32solo)
pio run                       # Build all envs defined in platformio-user.ini default_envs
pio run -e esp32s2 -t upload  # Build and flash to connected device
pio device monitor            # Serial monitor at 115200 baud
pio test -e native            # Run decoder unit tests on host (no device needed)
```

### Svelte UI local development

```bash
cd ui
npm run local   # Vite dev server with proxy to a real device (vite.config.local.js)
```

## Directory Layout

```
src/                  — all firmware C++ (flat)
  decoder/            — parser module (Arduino-free, compiles on native)
    include/          — HdlcParser.h, GcmParser.h, Cosem.h, IEC6205675.h, etc.
    src/              — corresponding .cpp files
  mqtt/               — all MQTT handler variants + json/ templates
  cloud/              — CloudConnector, ZmartCharge (ESP32 only, AMS_CLOUD flag)
  webserver/json/     — REST API JSON response templates
ui/                   — Svelte 4 app (built output embedded in firmware)
custom-example/       — template for build customization (copy to custom/)
test/
  stubs/              — minimal native shims (lwip/def.h, Timezone.h)
  test_decoder/       — Unity tests for the decoder module
scripts/
  addversion.py       — writes src/generated_version.h before compile
  generate_includes.py — embeds ui/dist + json templates as C PROGMEM arrays
```

## Architecture

### Data Flow

```
Smart Meter (UART/HAN)
  --> MeterCommunicator (Passive/KMP/Pulse)
      --> AmsDecoder (HDLC -> LLC -> GCM decrypt -> DLMS/DSMR/COSEM parse)
          --> IEC6205675 / IEC6205621 (extract AmsData fields)
          --> AmsData (in-memory state)
              --> AmsDataStorage (LittleFS hourly/daily history)
              --> EnergyAccounting (peak tracking + cost calc via PriceService)
              --> RealtimePlot (circular buffer for live wattage)
              --> AmsMqttHandler (MQTT publish: JSON/HA/Domoticz/Raw/Passthrough)
              --> AmsWebServer (REST API + embedded Svelte UI)
```

### Entry Point

`src/AmsToMqttBridge.cpp` — the main Arduino sketch (`setup()` / `loop()`) with all global state.

### Web UI Compilation

`scripts/generate_includes.py` (pre-build) minifies + gzip-compresses `ui/dist/` and embeds it as C PROGMEM byte arrays in `src/webserver/html/*.h`. The web server serves these directly — LittleFS is not used for UI assets. If `custom/ui/dist/` exists it is used instead of `ui/dist/`.

### Pre-build Scripts

- `scripts/addversion.py` — writes `src/generated_version.h` from git hash or `$GITHUB_TAG`
- `scripts/generate_includes.py` — unified script replacing three old per-library scripts

## Key Conventions

### Decoder Module is Arduino-Free

`src/decoder/` compiles on native (Linux/macOS) without Arduino. This is what enables `pio test -e native`. Key portability files:
- `src/decoder/include/byteorder.h` — portable `ntohl`/`ntohs` (`lwip/def.h` on device, `arpa/inet.h` on native)
- `src/decoder/include/DebugPrint.h` — portable `Print`/`Stream` (`#if ARDUINO` uses real class, else minimal stub)
- `GcmParser.cpp` has `#else return GCM_DECRYPT_FAILED` for native builds (no crypto library available)

### Platform Abstraction

Debug output is gated throughout the codebase:
```cpp
#if defined(AMS_REMOTE_DEBUG)
  // uses RemoteDebug* (telnet debug)
#else
  // uses Stream*/Print*
#endif
```

### Configuration Layout

Config is EEPROM-backed with fixed byte offsets defined in `src/AmsConfiguration.h`. The constant `EEPROM_CHECK_SUM = 104` must be incremented whenever any config struct layout changes. Migration from older config versions is handled in `AmsConfiguration.cpp`.

### Build Customization

Copy `custom-example/` to `custom/` (gitignored) to override defaults:
- `custom/config.h` — `#define DEFAULT_AP_SSID`, `FIRMWARE_UPGRADE_URL`, `DEFAULT_LANGUAGE`
- `custom/baudrates.h` — replace the autodetect baud/parity list
- `custom/lang/default.json` — embedded default language file
- `custom/ui/` — alternative Svelte app (`custom/ui/dist/` takes precedence over `ui/dist/`)

### Local Dev Overrides

`platformio-user.ini` (gitignored) is loaded via `extra_configs = platformio-user.ini` in `platformio.ini`. Never commit personal `upload_port`, secrets, or dev-only build flags to `platformio.ini`.

### API Keys / Secrets

`platformio.ini` contains `NO_*` placeholder values for API keys. CI injects real keys via `sed` at build time. Do not add real keys to tracked files.

### ESP8266 vs ESP32 Differences

`src/cloud/` (CloudConnector, ZmartCharge) is excluded from ESP8266 builds via `build_src_filter = -<cloud/>` — these use mbedTLS headers that don't exist on ESP8266. KmpTalker (Kamstrup protocol) is ESP32-only, requires precompiled `.a` in `precompiled/<env>/`.

### 64-bit Uptime

Use `millis64()` from `src/Uptime.h` instead of `millis()` to avoid the 49-day rollover.
