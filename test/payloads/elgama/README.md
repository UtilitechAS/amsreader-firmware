# Elgama (GAMA) payloads

Elgama GAMA 150/350 (Poland, Stoen/E.ON). DSMR identification line (`/EGM5...`) followed by an AES-128-GCM encrypted DLMS block. Frames can exceed the ESP8266 buffer.

**1 payload file(s)** — 1 encrypted.

| File | Source | Model | Country | Protocol | Enc | Notes |
|------|--------|-------|---------|----------|-----|-------|
| `gh1177-1.txt` | [#1177](https://github.com/UtilitechAS/amsreader-firmware/issues/1177) | GAMA 150 G15 | Poland | DSMR+DLMS | 🔒 | Decoded/decrypted DSMR P1 ASCII telegram (EGM5G15) from GAMA 150 G15. Meter ID redacted by reporter. Original encrypted HDLC hex dump in issue was truncated (....) so only the decoded ASCII telegram captured. Transport was encrypted DSMR. |
