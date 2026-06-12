# Aidon payloads

Aidon meters (Norway, Sweden). Unencrypted DLMS over HDLC (HAN-NVE list formats). Several Swedish captures here are deliberately mis-sampled (wrong parity/inversion) and serve as negative fixtures.

**9 payload file(s)** — 0 encrypted.

| File | Source | Model | Country | Protocol | Enc | Notes |
|------|--------|-------|---------|----------|-----|-------|
| `em003-2.hex` | email Sun, 14 Nov 2021 15:43:49 +0100 |  | Sweden | DLMS | — | Swedish Aidon HAN capture (115200 8N1 inverted) via AMS reader telnet 'Frame dump'. Begins with a valid HDLC header (7E A2 A8 21 B0 A2 D8 CF ...) but the second half degrades into mis-sampled bytes (recurring F0 EC 60 pattern); firmware reported Invalid HDLC. Partially-valid frame, good edge-case fixture. |
| `em004-1.hex` | email Sun, 14 Nov 2021 16:12:57 +0100 |  | Sweden | DLMS | — | TRUNCATED 32-byte prefix of the Swedish Aidon frame in em003-2 (same capture, quoted by amsleser.no/Egil). Clean HDLC header 7E A2 A8 21 B0 A2 D8 CF ... only; rest not included. Partial fixture; duplicate-source of em003-2 head. |
| `gh1119-1.hex` | [#1119](https://github.com/UtilitechAS/amsreader-firmware/issues/1119) | AIDON_V0001 | Norway | HDLC | — | Aidon HAN-NVE list2 hourly frame 16:00 (#1119 accumulated-values bug). AIDON_V0001, unencrypted DLMS. |
| `gh1119-2.hex` | [#1119](https://github.com/UtilitechAS/amsreader-firmware/issues/1119) | AIDON_V0001 | Norway | HDLC | — | Aidon HAN-NVE list2 hourly frame 17:00 - clock shows 0x11 but A+ counters identical to 18:00 (the bug). |
| `gh1119-3.hex` | [#1119](https://github.com/UtilitechAS/amsreader-firmware/issues/1119) | AIDON_V0001 | Norway | HDLC | — | Aidon HAN-NVE list2 hourly frame 18:00 - duplicate accumulated values vs 17:00. |
| `gh1119-4.hex` | [#1119](https://github.com/UtilitechAS/amsreader-firmware/issues/1119) | AIDON_V0001 | Norway | HDLC | — | Aidon HAN-NVE list2 hourly frame 19:00. |
| `gh143-2.hex` | [#143](https://github.com/UtilitechAS/amsreader-firmware/issues/143) |  | Sweden | HDLC | — | Swedish Aidon RJ45 27-item long list, unencrypted. First of 1509 frames from attachment HAN.Readings.HEX.Dump.txt (logic-analyzer capture by Isaksson). Frame starts 7E A2 43. Full dump 2.6MB not stored; representative frame only. |
| `gh146-1.hex` | [#146](https://github.com/UtilitechAS/amsreader-firmware/issues/146) |  | Sweden | HDLC | — | HDLC frame starting 7E A2 43, LLC E6 E7 00, plaintext DLMS (0F 40 00 00). ~510 bytes but TRUNCATED: captured after a 'Buffer overflow' debug message, no closing 7E (ends 02 0F 00). Older pre-NVE Swedish Aidon meter on M-Bus/RJ45, 27-item list. Partial. |
| `gh156-1.hex` | [#156](https://github.com/UtilitechAS/amsreader-firmware/issues/156) |  | Sweden | HDLC | — | 581-byte HDLC frame (header 0xA243), telnet dump from comment 0. Plaintext DLMS structure with scaler+unit fields (02 02 0F ...). Used to implement scale/unit field interpretation; Swedish Aidon showed 10x without scaling. Valid HDLC, payload starts at byte 18. Byte count matches header. |
