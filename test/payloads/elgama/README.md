# Elgama (GAMA) payloads

Elgama GAMA 150/350 (Poland, Stoen/E.ON). DSMR identification line (`/EGM5...`) followed by an AES-128-GCM encrypted DLMS block. Frames can exceed the ESP8266 buffer.

**4 payload file(s)** — 4 encrypted.

| File | Source | Model | Country | Protocol | Enc | Notes |
|------|--------|-------|---------|----------|-----|-------|
| `gh1177-1.txt` | [#1177](https://github.com/UtilitechAS/amsreader-firmware/issues/1177) | GAMA 150 G15 | Poland | DSMR+DLMS | 🔒 | Decoded/decrypted DSMR P1 ASCII telegram (EGM5G15) from GAMA 150 G15. Meter ID redacted by reporter. Original encrypted HDLC hex dump in issue was truncated (....) so only the decoded ASCII telegram captured. Transport was encrypted DSMR. |
| `gh1198-1.hex` | [#1198](https://github.com/UtilitechAS/amsreader-firmware/issues/1198) | GAMA 350 (G35) | Poland | DSMR+DLMS | 🔒 | Truncated capture (94 bytes) on ESP8266 (serial buffer overflow). Leading 0xFF then ASCII '/EGM5G35\r\n\r\n' DSMR ident, then 00 82 02 30 long-form length (~560B), then encrypted DLMS. Frame counter region '8D C2'. Trailing FF FE FC F8 is overflow garbage. Full telegram ~580B, too big for ESP8266 256B buffer. No keys provided (EK+AK from DSO). |
| `gh1198-2.hex` | [#1198](https://github.com/UtilitechAS/amsreader-firmware/issues/1198) | GAMA 350 (G35) | Poland | DSMR+DLMS | 🔒 | Second consecutive telegram fragment (48 bytes). Same structure as gh1198-1 but frame counter incremented '8D C3' (vs '8D C2'). Starts at ASCII 'EGM5G35'. |
| `gh1198-3.hex` | [#1198](https://github.com/UtilitechAS/amsreader-firmware/issues/1198) | GAMA 350 (G35) | Poland | DSMR+DLMS | 🔒 | Third telegram fragment (92 bytes), partially overwritten/misaligned (leading C0 1D, frame counter region '88 C0'). Encrypted DLMS. Trailing FF FE FC F8 overflow garbage. |
