# Captured meter payloads

Real smart-meter frames captured from users' HAN/P1/M-Bus ports, collected from
**GitHub issues** on `UtilitechAS/amsreader-firmware` and from the maintainer's
**email archive**. They are intended as fixtures for the decoder unit tests
(`test/test_decoder/`, run with `pio test -e native`).

## Layout

```
test/payloads/
  README.md                 — this file
  <manufacturer>/           — one folder per meter manufacturer
    README.md               — table describing every payload from that maker
    *.hex                   — raw frame bytes as hex (whitespace ignored)
    *.txt                   — DSMR/P1 ASCII telegrams, saved verbatim
  keys/
    README.md               — decryption keys: source, secret name, what they decrypt
    keymap.json             — payload -> secret-name mapping (tracked)
    keys.local.json         — actual key values (GITIGNORED)
  tools/
    verify_keys.py          — decrypts each keyed payload and checks it is valid DLMS
```

### File naming / provenance

- `ghNNN-K.*` — from GitHub issue `#NNN`, payload `K` within that issue.
- `emNNN-K.*` — from the email archive, message `NNN`, payload `K`.

The per-manufacturer `README.md` tables carry the full provenance: source link or
email date, meter model, country/DSO, protocol, whether the frame is encrypted
(🔒), whether a key exists (🔑), and notes on what makes each frame interesting
(format quirks, decrypt/plaintext pairs, deliberate corrupt/edge cases).

## Payload formats

| Form | Looks like | Notes |
|------|-----------|-------|
| HDLC | `7E A0 .. .. 7E` | DLMS/COSEM wrapped in HDLC; the common Nordic HAN form |
| M-Bus | `68 LL LL 68 .. 16` | DLMS over wired M-Bus long frames (Sagemcom, some Kaifa AT) |
| DSMR/P1 | `/XXX5...` ... `!CRC` | ASCII telegram (Dutch P1 companion standard); `.txt` files |
| Raw DLMS | starts `0F ..` / `E6 E7 00 ..` | LLC/APDU with no HDLC framing (Raw-bytes MQTT dumps) |

Encrypted frames carry an AES-128-GCM header (`DB` tag, system title, security
byte). See `keys/` for the keys we have.

## Counts

| Manufacturer | Files | Notes |
|--------------|------:|-------|
| Kamstrup     | 57 | Omnipower/Omnia — NO unencrypted, DK/SE/CH encrypted; decrypt/plaintext pairs |
| Iskraemeco   | 37 | AM550 + IE.5 — SI/AT/CH/HR, several proprietary non-OBIS formats |
| Kaifa        | 19 | MA304/MA309 — NO plain, AT/PL M-Bus encrypted |
| Sagemcom     | 15 | T210-D — AT, DLMS over M-Bus, mostly encrypted |
| Landis+Gyr   | 12 | E360/E450 — CH/AT/SE/DK, GBT pushes + encrypted |
| Aidon        | 19 | NO/SE — incl. deliberately mis-sampled negative fixtures |
| Elgama (GAMA)| 4  | PL — DSMR header + encrypted DLMS, oversized for ESP8266 |
| NES          | 1  | DK — DSMR/P1, `NES5` id |

## Decryption keys

Eight keys/keypairs are known (`keys/README.md`). Four sets decrypt payloads that
are present here and are **verified** (`tools/verify_keys.py`) — 12 frames total
(L&G #501 ×2, Kaifa #905, and **Iskraemeco #787 ×9**); the rest are keys-only
(no clean frame was published with them). Keys came from public GitHub issues and
from the maintainer's email (the #787 Austrian key and three Danish Kamstrup
keypairs were recovered from the Thunderbird mailbox).

Raw key values live only in `keys/keys.local.json` (gitignored). For CI, create
the GitHub Actions secrets named in `keys/README.md` and the test/verifier read
them from the environment.

```bash
# Verify keys against their payloads (uses keys.local.json locally, env in CI):
python3 test/payloads/tools/verify_keys.py
```

> Note: many encrypted frames here have **no** key (the reporter kept it). They are
> still useful for testing the HDLC/M-Bus framing and GCM-header parsing up to the
> decrypt step, and as negative cases. Frames marked corrupt/truncated in the
> tables are intentional edge-case fixtures (wrong parity, buffer overflow, etc.).
