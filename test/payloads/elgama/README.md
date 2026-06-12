# Elgama (GAMA) payloads

Elgama GAMA 150/350 (Poland, Stoen/E.ON). DSMR identification line (`/EGM5...`) followed by an AES-128-GCM encrypted DLMS block. Frames can exceed the ESP8266 buffer.

**1 payload file(s)** — 0 encrypted.

| File | Source | Model | Country | Protocol | Enc | Notes |
|------|--------|-------|---------|----------|-----|-------|
| `gh1177-1.txt` | [#1177](https://github.com/UtilitechAS/amsreader-firmware/issues/1177) | GAMA 150 G15 | Poland | DSMR+DLMS | — | Decoded DSMR P1 ASCII telegram (EGM5G15) from GAMA 150 G15. This is the cleartext telegram (the meter's transport was encrypted DSMR, but only the decoded text was captured), so it decodes directly without a key. Meter ID redacted by reporter. |
