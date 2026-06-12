# Decryption keys

AES-128-GCM keys recovered alongside encrypted payloads. Each key is **16 bytes (32 hex chars)**.
The firmware uses `encryptionKey` (EK) for the cipher and `authenticationKey` (AK) for the GCM tag
(AK only when the security byte has bit `0x10` set).

Sources are linked below (public GitHub issues, or private email to the maintainer). The raw key
values are kept **out of git** in `keys.local.json` (gitignored) and are meant to be injected into
tests via **GitHub Actions secrets** using the suggested names below. Run `tools/verify_keys.py`
(see top-level README) to confirm a key still decrypts its payload.

## Keys

### Iskraemeco AM550 — Austria, KELAG/Kärnten Netz ([#787](https://github.com/UtilitechAS/amsreader-firmware/issues/787))

- Decrypts: `iskraemeco/gh787-1.hex` … `gh787-9.hex`
- Encryption key (EK) → secret `AMS_TEST_KEY_GH787_EK`
- No AK: frames use security byte `0x20` (encrypt-only, no GCM auth), so only the EK is needed.
- ✅ Verified: EK decrypts all 9 frames to valid DLMS (data-notification, 2024-05-07).
- Source: emailed to the maintainer by the reporter (2024-06-01) — the issue
  itself says the key was sent privately. Found in the maintainer's Thunderbird mailbox.

### Kamstrup Omnipower — Denmark ([#126](https://github.com/UtilitechAS/amsreader-firmware/issues/126))

- Decrypts: _(no payload frame published in source)_
- Encryption key (EK) → secret `AMS_TEST_KEY_GH126_EK`
- Authentication key (AK) → secret `AMS_TEST_KEY_GH126_AK`


### Landis+Gyr E450 — Austria ([#501](https://github.com/UtilitechAS/amsreader-firmware/issues/501))

- Decrypts: `gh501-1.hex`, `gh501-2.hex`
- Encryption key (EK) → secret `AMS_TEST_KEY_GH501_EK`
- Authentication key (AK) → secret `AMS_TEST_KEY_GH501_AK`
- ✅ Verified: EK decrypts the payload to valid DLMS.

### Kaifa MA309 (serial 1KFM0200169986/169990) — Poland ([#905](https://github.com/UtilitechAS/amsreader-firmware/issues/905))

- Decrypts: `gh905-1.hex`
- Encryption key (EK) → secret `AMS_TEST_KEY_GH905_EK`
- ✅ Verified: EK decrypts the payload to valid DLMS.

### Kamstrup Omnipower — Denmark (email, 2020-07-10)

- Decrypts: `kamstrup/gh73-1.hex`, `gh73-2.hex`, `gh73-3.hex`
- Encryption key (EK / Kamstrup obj 64 GUEK) → secret `AMS_TEST_KEY_EM20200710_EK`
- Authentication key (AK / Kamstrup obj 65 GAK) → secret `AMS_TEST_KEY_EM20200710_AK`
- ✅ Verified: decrypts + authenticates to valid DLMS (data-notification, 2020-05-12).
- This is the project's **first encrypted meter**: the keys were emailed by the reporter (issue [#73](https://github.com/UtilitechAS/amsreader-firmware/issues/73),
  "Add support for encrypted meters"); the frames are from that issue's comment 12.

### Kamstrup Omnipower — Denmark (email, 2023-10-25)

- Decrypts: an HDLC frame **inlined as text** in the email (system title `4B414D45013535D4`).
  ✅ Confirmed: this EK decrypts it to valid DLMS (data-notification dated 2023-10-25).
  **Not committed as a fixture** — the only capture is the firmware's verbose telnet
  "Frame dump", quoted/line-wrapped in the reply, so it can't be reconstructed
  byte-exact (the real decoder's HDLC FCS / GCM auth tag rejects it). A clean
  on-device telnet capture from this meter would make it a usable fixture.
- Encryption key (EK / obj 64) → secret `AMS_TEST_KEY_EM20231025_EK`
- Authentication key (AK / obj 65) → secret `AMS_TEST_KEY_EM20231025_AK`
- Reporter confirmed "the encryption keys are working". Found in Thunderbird mailbox.

### Kamstrup Omnipower — Denmark (email, 2025-04-04)

- Decrypts: _(no clean payload frame in-thread)_
- Encryption key (EK) → secret `AMS_TEST_KEY_EM20250404_EK`
- Authentication key (AK) → secret `AMS_TEST_KEY_EM20250404_AK`
- Found in Thunderbird mailbox.

