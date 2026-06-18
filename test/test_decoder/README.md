# Decoder unit tests (`pio test -e native`)

Native (host) tests that feed the real captured frames in `test/payloads/`
through the decoder and assert the parsed `AmsData`.

```bash
pio test -e native
```

## Files

| File | Purpose |
|------|---------|
| `test_main.cpp` | Unity `main()`, low-level parser guards, explicit readable tests |
| `test_unencrypted.cpp` | golden-master sweep + per-meter documentation tests |
| `decoder_harness.{h,cpp}` | loads a fixture and drives HDLC→LLC/MBUS/GBT→DLMS/DSMR→`IEC6205675`/`LNG`/`IEC6205621`, mirroring `PassiveMeterCommunicator`. Provides a native `millis64()` and a `NullStream`. |
| `fixtures_generated.h` | **generated** — fixture lists (`UNENC_OK`, `UNENC_EDGE`, `ENC_KEYED`) from `test/payloads/manifest.json` |
| `expected_unencrypted.h` | **generated** — golden decode of every unencrypted fixture |

## Golden-master sweep

`test_unencrypted_golden` decodes every unencrypted fixture and compares the
result (listType, meterType, power, per-phase V/A, import counter, meter id)
against `expected_unencrypted.h`. Any change to how a real frame decodes flips
the test so it can be reviewed in the diff. Known meter quirks (e.g. the L&G
integer-voltage frames, multi-segment frames that don't decode standalone) are
captured as-is — see the per-manufacturer notes in `test/payloads/*/README.md`.

## Regenerating after adding/changing fixtures

```bash
# 1. rebuild the fixture manifest from the payload README tables
python3 scripts/build_payload_manifest.py
# 2. regenerate the C++ fixture lists
python3 scripts/gen_fixtures_header.py
# 3. rebuild, then refresh the golden snapshot (review the diff!)
pio test -e native
.pio/build/native/program gen > test/test_decoder/expected_unencrypted.h
```

## Native build notes

The decoder needs a small Arduino surface on host, provided by `test/stubs/`:
`Arduino.h`, `WString.h` (a `std::string`-backed `String`), `EEPROM.h`,
`Timezone.h` (+ `tmElements_t`/`makeTime`), `lwip/def.h` (+ `PROGMEM`).
`[env:native]` sets `test_build_src = yes` so `build_src_filter` objects link
into the test.

## Encrypted fixtures (GCM on native)

`GcmParser` has a native branch using system **mbedTLS** (3.x). `scripts/native_crypto.py`
probes for the headers and, when present, defines `HAVE_MBEDTLS` and links
`libmbedcrypto`; otherwise the encrypted tests self-ignore.

```bash
sudo apt-get install -y libmbedtls-dev   # Debian/Ubuntu
```

Keys are resolved per fixture (`ENC_KEYED` in `fixtures_generated.h`) by secret
name — from the environment first (GitHub Actions secrets in CI), then from the
gitignored `test/payloads/keys/keys.local.json` for local runs. Set the secrets
listed in `test/payloads/keys/README.md` (e.g. `AMS_TEST_KEY_GH787_EK`) to run
the decrypt tests in CI; without them those tests are skipped, not failed.

### Encrypted frames without a key

The encrypted fixtures we hold no key for (`ENC_NOKEY`) can't be decrypted, but
`test_encrypted_framing_no_key` still exercises them: it probes each through the
framing (HDLC / M-Bus / LLC) and the GCM-header parse with a dummy key (the
system title is read before any decryption), asserting the decoder never crashes
and that every frame reaching the GCM layer yields a system title. This is what
caught the `GcmParser` ciphertext-length underflow. It needs no mbedTLS.
