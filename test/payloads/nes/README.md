# NES (Networked Energy Services) payloads

NES (Networked Energy Services) meters (Denmark). DSMR/P1 ASCII telegrams; the `NES5` manufacturer id was historically unrecognised by the firmware.

**1 payload file(s)** — 0 encrypted.

| File | Source | Model | Country | Protocol | Enc | Notes |
|------|--------|-------|---------|----------|-----|-------|
| `gh712-1.hex` | [#712](https://github.com/UtilitechAS/amsreader-firmware/issues/712) | 83335-X | Denmark (DK) | DSMR+DLMS | — | Raw DSMR/P1 ASCII telegram as hex bytes. Header /NES5:83335-3E. Unencrypted, 115200 8E1. Full 3-phase set incl reactive 3.8.0/4.8.0 and per-phase reactive 23/24/43/44/63/64.7.0, voltages, currents. CRC !F302. Issue: NES manufacturer id not recognized by firmware. |
