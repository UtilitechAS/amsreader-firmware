# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

AMS Reader (`ams2mqtt`) is PlatformIO/Arduino firmware for ESP8266 and ESP32 devices that reads data from European IEC-62056 compliant smart electricity meters via M-Bus/HAN port, decodes it, and publishes to MQTT. It also serves a Svelte SPA web interface.

## Prerequisites

- Python 3.9+ with `platformio` and `css_html_js_minify` (`pip install -U platformio css_html_js_minify`)
- Node.js 22.x (for Svelte 5 UI; CI uses 22.x, Vite 8 needs ≥ 20.19)

## Setup (first time)

```bash
# Required: copy and customize local build config
cp platformio-user.ini-example platformio-user.ini

# Build Svelte frontend FIRST (the firmware build aborts if ui/dist is missing)
cd ui && npm ci && npm run build && cd -

# Install PlatformIO library dependencies
pio pkg install
```

## Build Commands

```bash
pio run -e esp32s2            # Build for a specific target (esp8266, esp32, esp32s2, esp32s2psram, esp32s3, esp32c3, esp32solo)
pio run                       # Build all envs defined in platformio-user.ini default_envs
pio run -e esp32s2 -t upload  # Build and flash to connected device
pio device monitor            # Serial monitor at 115200 baud
pio test -e native            # Decoder unit tests on host (no device needed)
pio test -e native_config     # AmsConfiguration tests against the stub EEPROM
pio test -e native_watchdog   # NetworkWatchdog policy tests
pio test -e native_localday   # LocalDay hour/price-point positions on DST change days
```

### Svelte UI local development

```bash
cd ui
cp vite.config.local.example.js vite.config.local.js   # once; gitignored, set your device's address
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
  webserver/json/     — REST API JSON response templates (data, sysinfo, tariff, response; configuration JSON is generated in code, see below)
ui/                   — Svelte 5 app (built output embedded in firmware; uses mount()/unmount(), not `new Component()`)
custom-example/       — template for build customization (copy to custom/)
test/
  stubs/              — minimal native shims (Arduino.h, EEPROM.h, WString.h, Timezone.h, lwip/def.h)
  payloads/           — real meter payload corpus (per-vendor dirs + manifest.json; keys/ maps fixtures to key names, key values are not in git)
  test_decoder/       — Unity tests for the decoder module (encrypted/unencrypted/plaintext-with-key)
  test_configuration/ — Unity tests for AmsConfiguration (env native_config)
  test_network_watchdog/ — Unity tests for NetworkWatchdog (env native_watchdog)
  test_local_day/     — Unity tests for LocalDay.h on 25 and 23 hour days (env native_localday)
scripts/
  addversion.py       — writes src/generated_version.h before compile
  generate_includes.py — embeds ui/dist + json templates as C PROGMEM arrays
```

## Architecture

### Data Flow

```
Smart Meter (UART/HAN)
  --> MeterCommunicator (Passive/KMP/Pulse)
      --> PassiveMeterCommunicator::unwrapData (tag-driven src/decoder chain: HDLC/M-Bus/GBT -> GCM decrypt -> LLC -> DLMS/DSMR)
          --> IEC6205675 / IEC6205621, or LNG / LNG2 / LNG3 (src/) for Landis+Gyr proprietary push formats (extract AmsData fields)
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

If the dist directory is missing the script exits with an error instead of skipping: the generated headers are gitignored and would otherwise survive from an older build, so the firmware would build but the UI would 404 on stale asset URLs.

`ui/dist/` is committed (force-added despite `.gitignore`). PRs that change `ui/src/` rebuild it with `npm run build` and commit the result alongside.

### Pre-build Scripts

- `scripts/addversion.py` — writes `src/generated_version.h` from git hash or `$GITHUB_TAG`
- `scripts/generate_includes.py` — unified script replacing three old per-library scripts

### Configuration JSON and MQTT Control

`/configuration.json` and the MQTT `getconfig` reply are both produced by `AmsJsonGenerator::generateConfigurationJson()`, which emits section by section into a `JsonSink` (HTTP streams it with chunked transfer; MQTT collects it in a `BufferJsonSink`). There are no `conf_*.json` templates any more. A new config field has to be added in the generator (output), `AmsWebServer::handleSave()` (web form input), `JsonMqttHandler::handleConfigMessage()` (MQTT input, same short keys as the JSON output) and the UI.

Commands arrive on the subscribe topic (`<publish topic>/command` when none is configured), as a plain string or `{"action":...}`. `AmsMqttHandler::handleCommand()` is shared by the JSON, Home Assistant and Raw handlers (Domoticz and Passthrough ignore incoming messages):
- `fwupgrade` — optional `"version"` pins the target, otherwise the updater's next version
- `dayplot` / `monthplot` — publish plot JSON to `<publish topic>/dayplot|monthplot`
- `reboot`, `factoryreset` — only when `MqttConfig.allowDestructiveCommands` is set (default off; `q.dc` in the config JSON); `factoryreset` also needs `"confirm":true`

`JsonMqttHandler` additionally handles `{"action":"getconfig"}` (publishes to `<publish topic>/config`) and `{"action":"setconfig","config":{...}}`, which applies, calls `config->save()`, and reboots if network or web config changed. Both are ESP32-only.

The services array (HAN, MQTT, price, NTP, cloud, …) comes from `AmsJsonGenerator::generateServicesJson()` for both `sysinfo.json` and the MQTT services announcement, so the two cannot disagree.

### Network Resilience

- `NetworkWatchdog` (policy only, native-testable) + `GatewayPing` (non-blocking ICMP), driven by `handleNetworkWatchdog()` in `AmsToMqttBridge.cpp`, WiFi client mode only. Pings the gateway every 60 s; arms only after the gateway has answered once. Without replies it restarts WiFi after 3 min with doubling backoff (max 24 min) and reboots after 1 h (`REBOOT_CAUSE_NETWORK_UNREACHABLE`). Outage stats live in `ResetDataContainer`.
- `DnsGuard` (ESP32 only) keeps lwIP's shared DNS table from losing the DHCP IPv4 servers to IPv6 RDNSS/DHCPv6 entries. It intercepts `dns_setserver` via the linker: `[esp32] build_flags` in `platformio.ini` (`-Wl,--wrap=dns_setserver`, `-D AMS_WRAP_DNS_SETSERVER`) must be included in every ESP32 env.
- `NtpStatus` records the last SNTP sync (`ntpLastSyncMillis()`); the services array reports NTP as state 2 (connecting) when no sync has happened since boot or the last one is older than 3 h (`NTP_STALE_AFTER_SECONDS`).

### Decoder Test Fixtures

`test/test_decoder/` runs the real captured frames in `test/payloads/` through the decoder. After adding or changing a fixture, regenerate the generated headers (see `test/test_decoder/README.md` for full detail):

```bash
python3 scripts/build_payload_manifest.py     # rebuild manifest.json from per-vendor README tables
python3 scripts/gen_fixtures_header.py         # regenerate fixtures_generated.h
pio test -e native                             # run; review any golden-master diff
.pio/build/native/program gen > test/test_decoder/expected_unencrypted.h   # refresh golden snapshot
```

`test_unencrypted_golden` is a golden-master sweep: any change in how a real frame decodes flips the test for review in the diff. Encrypted fixtures need `libmbedtls-dev` installed and their keys (`AMS_TEST_KEY_*` environment variables, or the gitignored `test/payloads/keys/keys.local.json`), else they are skipped.

## Key Conventions

### Decoder Module is Arduino-Free

`src/decoder/` compiles on native (Linux/macOS) without Arduino. This is what enables `pio test -e native`. Key portability files:
- `src/decoder/include/byteorder.h` — portable `ntohl`/`ntohs` (`lwip/def.h` on device, `arpa/inet.h` on native)
- `src/decoder/include/DebugPrint.h` — portable `Print`/`Stream` (`#if ARDUINO` uses real class, else minimal stub)
- `GcmParser.cpp` decrypts with the ESP crypto lib on device; on native it uses system **mbedTLS** (2.x or 3.x) when `NATIVE_TEST` + `HAVE_MBEDTLS` are set (so encrypted fixtures can be tested), else falls back to `return GCM_DECRYPT_FAILED`. `scripts/native_crypto.py` probes for the mbedTLS headers and sets the flags/links `libmbedcrypto` automatically

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

Config is EEPROM-backed with fixed byte offsets (`CONFIG_*_START`) defined in `src/AmsConfiguration.h`. `EEPROM_EXPECTED_VERSION = 104` (formerly `EEPROM_CHECK_SUM`) identifies the stored layout; changing it needs a relocation path in `AmsConfiguration::load()` (see `relocateConfig103()`). Fields appended to an existing struct are instead migrated in its getter using the struct's `magic` byte (see `getMqttConfig()`).

`EEPROM.begin()` is called once in `load()`. The `setXxxConfig()` setters only write the RAM buffer and set change flags; nothing is persisted until `config.save()` commits.

### Build Customization

Copy `custom-example/` to `custom/` (gitignored) to override defaults:
- `custom/config.h` — `#define DEFAULT_AP_SSID`, `FIRMWARE_UPGRADE_URL`, `DEFAULT_LANGUAGE`, plus optional `CUSTOM_MQTT_*` overrides for the second MQTT connector (see `custom-example/config.h` for the full list)
- `custom/baudrates.h` — replace the autodetect baud/parity list
- `custom/lang/default.json` — embedded default language file
- `custom/ui/` — alternative Svelte app (`custom/ui/dist/` takes precedence over `ui/dist/`)

`scripts/generate_includes.py` emits `src/CustomDefaults.h` before each build — empty if `custom/config.h` is absent, otherwise a wrapper that includes it. Source files include `CustomDefaults.h` to pick up the `#define`s.

### Translations

Translations are managed externally in [Localazy](https://localazy.com/p/amsleser). The repo does not embed translated text; the device serves `translations.json?lang=XX` from LittleFS (`/translations-XX.json`), which are produced from Localazy and distributed via S3 by `.github/workflows/localazy.yml`.

```
localazy/
  localazy.json          — tracked: upload + download config
  localazy-keys.json     — gitignored: { writeKey, readKey }
  source/en.json         — tracked: canonical English source-of-truth (single file)
  language/              — gitignored: build output of `localazy download`
```

**To add a new UI string:**
1. Edit `localazy/source/en.json` in the PR that introduces the string. Use proper nested objects — never flat-dotted keys at the root (`"errors.mqtt.-1"` was a historical mistake; the UI lookup `translations.errors?.mqtt?.[code]` only sees nested entries).
2. After merge, push to Localazy: `cd localazy && localazy upload -k localazy-keys.json -w $LOCALAZY_WRITE_KEY`
3. Translators do the rest on Localazy. The next run of `.github/workflows/localazy.yml` (manual `workflow_dispatch`) propagates translations to S3 → device.

**Critical upload-config rules** (learned the hard way):
- Each entry in `upload.files` MUST set `"file": "translations.json"`. Default is `${autodetectFileWithFallback}` which creates a parallel `file.json` keyset and **doubles every key**.
- Use the array form with explicit `pattern` — `"files": "source/en.json"` works but `pattern` is required when using the object form.
- Never set `deprecateMissing: true` in the tracked `localazy.json`. If you need a one-time cleanup, use a temp config file (`-c /tmp/cleanup.json`) so it can't be triggered accidentally.
- Only English (`source/en.json`) is uploaded from this repo. Non-English translations belong on Localazy and must NOT be pushed from here — uploading without a per-entry `lang` field would interpret the file as a new English source.

### Releases

Pushing a `vX.Y.Z` tag runs `.github/workflows/release.yml`; `vX.Y.Z-rcN` runs `prerelease.yml`. Both generate GitHub release notes and need the full history (`fetch-depth: 0` on checkout):
- A final release compares against the previous *stable* tag (`git describe --tags --abbrev=0 --exclude='*-rc*' HEAD^`), so it lists everything since the last non-RC release.
- An RC compares against the previous tag of any kind (`git describe --tags --abbrev=0 HEAD^`): `rc1` against the last stable release, later RCs only against the RC before them.

### Local Dev Overrides

`platformio-user.ini` (gitignored) is loaded via `extra_configs = platformio-user.ini` in `platformio.ini`. Never commit personal `upload_port`, secrets, or dev-only build flags to `platformio.ini`.

### API Keys / Secrets

`platformio.ini` contains `NO_*` placeholder values for API keys. CI injects real keys via `sed` at build time. Do not add real keys to tracked files.

`ENERGY_SPEEDOMETER_USER`/`_PASS` are the credentials for the Flexpartner.Energy MQTT connector (ESP32 only, compiled in only when the password is defined). The code still calls it `energySpeedometer`; the UI shows the brand name hardcoded, not through a translation key.

### ESP8266 vs ESP32 Differences

`src/cloud/` (CloudConnector, ZmartCharge) is excluded from ESP8266 builds via `build_src_filter = -<cloud/>` — these use mbedTLS headers that don't exist on ESP8266. KmpTalker (Kamstrup protocol) is ESP32-only, requires precompiled `.a` in `precompiled/<env>/`. Configuration over MQTT (`getconfig`/`setconfig`) and `DnsGuard` are `#if defined(ESP32)` only.

### Positions Within a Local Day

A local day has 25 hours when DST ends and 23 when it starts. Hour of day and price point indexes are counted from local midnight in elapsed time with `src/LocalDay.h` (`localMidnightUtc()`, `hoursSinceLocalMidnight()`), never from the wall clock's `tm.Hour`, which repeats or skips an hour. `EnergyAccounting`'s `currentHour` and `PriceService`'s price point index follow this.

### 64-bit Uptime

Use `millis64()` from `src/Uptime.h` instead of `millis()` to avoid the 49-day rollover.

## Behavioral Guidelines (Karpathy)

**Tradeoff:** These guidelines bias toward caution over speed. For trivial tasks, use judgment.

### 1. Think Before Coding

**Don't assume. Don't hide confusion. Surface tradeoffs.**

Before implementing:
- State assumptions explicitly. If uncertain, ask.
- If multiple interpretations exist, present them — don't pick silently.
- If a simpler approach exists, say so. Push back when warranted.
- If something is unclear, stop. Name what's confusing. Ask.

### 2. Simplicity First

**Minimum code that solves the problem. Nothing speculative.**

- No features beyond what was asked.
- No abstractions for single-use code.
- No "flexibility" or "configurability" that wasn't requested.
- No error handling for impossible scenarios.
- If you write 200 lines and it could be 50, rewrite it.

Ask: "Would a senior engineer say this is overcomplicated?" If yes, simplify.

### 3. Surgical Changes

**Touch only what you must. Clean up only your own mess.**

When editing existing code:
- Don't "improve" adjacent code, comments, or formatting.
- Don't refactor things that aren't broken.
- Match existing style, even if you'd do it differently.
- If you notice unrelated dead code, mention it — don't delete it.

When your changes create orphans:
- Remove imports/variables/functions that YOUR changes made unused.
- Don't remove pre-existing dead code unless asked.

Every changed line should trace directly to the user's request.

### 4. Goal-Driven Execution

**Define success criteria. Loop until verified.**

- For `src/decoder/` changes: write or update a `test/test_decoder/` Unity test first, then make it pass. Verify with `pio test -e native`.
- For all other code (firmware, MQTT, web server, Svelte UI): define an explicit, verifiable "done" criteria before starting (e.g., "device reboots after save", "MQTT reconnects on cert change"), then confirm it is met.

For multi-step tasks, state a brief plan:
```
1. [Step] → verify: [check]
2. [Step] → verify: [check]
3. [Step] → verify: [check]
```
