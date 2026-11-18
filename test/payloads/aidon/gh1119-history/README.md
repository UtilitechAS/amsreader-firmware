# Aidon: stale energy registers at the whole hour (#1119) — a year of captures

This folder holds every stale whole-hour reading one Aidon meter produced
between 2025-12-01 and 2026-11-18, together with the readings around each one.
`test_stale_history_replay` (`test/test_decoder/test_stale_counter.cpp`)
replays them through the detector on every `pio test -e native`.

## Summary

* The meter published a **stale active energy register in 45 hourly readings**
  (43 events, two of them lasting two hours), on average about once every
  eight days, at any time of day, in every month of the year.
* In **41 of the 43 events the meter clock in the stale reading is current**.
  Only the first two events (2025-12-14 and 2026-01-05) repeat the previous
  hour's clock as well. The detector shipped in v2.6.0-rc1..rc3 required a
  repeated clock and therefore missed every event after 2026-01-05, including
  the one on 2026-11-18 that prompted this investigation.
* In 4 events the **reactive registers moved while active import stood still**,
  so "all registers identical" is not a reliable signature either.
* In 2 events active import stood still for **two consecutive hours**.
* The energy is never lost: the next reading that moves catches up exactly.
  The defect only misplaces it in time — a 0 kWh hour followed by a double one.
* Comparing every hour of the year against the energy integrated from the
  meter's own instantaneous power shows **no other kind of anomaly**: outside
  these 45 readings the register deltas match the integrated power with a
  median ratio of 1.0002.

## Source

* Meter: Aidon 6534 (list id `AIDON_V0001`), HAN-NVE List 3 every hour at
  hh:00:10, unencrypted HDLC, Norway.
* Every raw frame from the meter (List 1 every 2.5 s, List 2 every 10 s,
  List 3 every hour) was logged with a timestamp by a home-automation system
  next to an AMS reader. 24.3 million frames, 8,425 of them List 3, covering
  2025-12-01 14:00 to 2026-11-18 19:00 with no gaps in the List 3 series.
* The energy per hour was integrated from the logged List 1/2 import power
  (`1.7.0`), independently of the energy register.

Times below are the logger's local wall clock (CET/CEST). The meter clock is
shown as transmitted. This meter sends standard time all year, so during
summer time it reads one hour behind the logger.

## What a stale reading looks like

Two consecutive readings from 2026-11-18 (registers in units of 10 Wh, as
transmitted):

| Logged | Meter clock | A+ (1.8.0) | R+ (3.8.0) | R- (4.8.0) | P+ (1.7.0) |
|---|---|---|---|---|---|
| 15:00:11 | 15:00:00 | 12567470 | 14528 | 1727073 | 3416 W |
| 16:00:11 | **16:00:00** | **12567470** | 14528 | 1727073 | 1843 W |
| 17:00:11 | 17:00:00 | 12568089 | 14528 | 1727172 | 4499 W |

The 16:00 reading has a current clock and current instantaneous values, but
its energy registers are the 15:00 ones, although the logged power integrates
to 3.08 kWh over that hour. At 17:00 the register jumps 6.19 kWh: both hours.

In December 2025 and early January 2026 the same defect also repeated the
clock (see the fixtures `../gh1119-1.hex` … `gh1119-4.hex`). On 2026-01-10
02:00 the clock's deviation/status bytes changed from `0000/00` to `8000/FF`,
probably a meter firmware or configuration change. Every event since then has
a current clock.

## Effect on an AMS reader without detection

`AmsDataStorage` sees roughly 3600 s between the readings and takes its normal
branch twice. The hour that just ended is stored as 0, and the next hour gets
the two-hour delta. `EnergyAccounting` then takes that doubled hour as the
day's peak. On a tariff with power steps this can raise the month's step: on
2026-11-18 the doubled 6.18 kWh hour pushed the average of the three highest
days from 4.98 to 5.4 kWh. Days 02-21, 03-09 and 10-31 would have shown peaks
of about 16 kWh.

Because the register catches up at the next reading, daily totals are only
affected when the two hours fall on different days (2026-09-22 23:00 →
2026-09-23 00:00).

## Detection

A List 3 reading is now treated as stale when

* the meter has an observed register cadence of at least 30 minutes (learned
  from the clock of accepted readings, as before), **and**
* the active import or active export register equals the value of the last
  accepted reading, **and**
* the power integrated since that reading, from the meter's own List 1/2
  power values, amounts to at least `STALE_COUNTER_MIN_ESTIMATE_KWH` (0.1 kWh)
  in that direction.

The clock is not part of the decision. The threshold is ten steps of the
coarsest register resolution seen (10 Wh) and a third of the smallest stale
hour in this data (0.31 kWh). An unmoved register without that evidence (an
idle installation, or no sub-hour traffic received) is accepted as before.
After two consecutive rejections the reading is accepted regardless.

Replayed over all 8,425 hourly readings of the year, with the logged energy
fed in as sub-hour traffic, the rule flags exactly the 45 stale readings and
nothing else. The previous clock-based rule flags 2 of them.

How a flagged hour is stored and published is unchanged from #1119: the
integrated estimate is stored for the hour when the device has been up for an
hour, otherwise the next reading's delta is split across both hours. The MQTT
handlers withhold the stale registers and announce `hourly_data_substituted`.

## All occurrences

"Energy in the frozen hour(s)" is the integrated power over the hour (or hours)
the register stood still. "Register jump" is the A+ delta from the last good
reading to the next reading that moves. It covers the frozen hour(s) plus the
following hour.

| # | Stale reading logged | Meter clock in it | Clock repeated | Reactive registers | Energy in the frozen hour(s), integrated | Register jump at next moving reading |
|---|---|---|---|---|---|---|
| 1 | 2025-12-14 18:00 | 17:00 | yes | frozen | 3.96 kWh | 8.05 kWh |
| 2 | 2026-01-05 16:00 | 15:00 | yes | frozen | 2.16 kWh | 4.40 kWh |
| 3 | 2026-01-20 09:00 | 09:00 | no | frozen | 0.96 kWh | 1.92 kWh |
| 4 | 2026-01-22 01:00 | 01:00 | no | frozen | 3.53 kWh | 5.15 kWh |
| 5 | 2026-01-28 10:00, 11:00 | 10:00, 11:00 | no | **moved** | 3.11 kWh (2 h) | 3.11 kWh |
| 6 | 2026-02-07 10:00 | 10:00 | no | frozen | 1.48 kWh | 2.53 kWh |
| 7 | 2026-02-07 19:00 | 19:00 | no | frozen | 4.96 kWh | 9.92 kWh |
| 8 | 2026-02-21 19:00 | 19:00 | no | **moved** | 10.01 kWh | 16.63 kWh |
| 9 | 2026-03-06 12:00 | 12:00 | no | frozen | 0.54 kWh | 2.19 kWh |
| 10 | 2026-03-07 14:00 | 14:00 | no | frozen | 0.70 kWh | 5.67 kWh |
| 11 | 2026-03-09 23:00 | 23:00 | no | frozen | 9.96 kWh | 15.52 kWh |
| 12 | 2026-03-15 06:00 | 06:00 | no | frozen | 1.02 kWh | 3.33 kWh |
| 13 | 2026-03-21 05:00 | 05:00 | no | frozen | 4.03 kWh | 7.36 kWh |
| 14 | 2026-03-26 06:00 | 06:00 | no | frozen | 1.71 kWh | 2.64 kWh |
| 15 | 2026-04-09 19:00 | 18:00 | no | frozen | 0.88 kWh | 1.97 kWh |
| 16 | 2026-05-05 19:00 | 18:00 | no | frozen | 1.64 kWh | 2.35 kWh |
| 17 | 2026-05-12 20:00 | 19:00 | no | frozen | 0.61 kWh | 1.24 kWh |
| 18 | 2026-05-15 03:00 | 02:00 | no | frozen | 3.10 kWh | 7.96 kWh |
| 19 | 2026-05-17 03:00 | 02:00 | no | frozen | 4.94 kWh | 9.74 kWh |
| 20 | 2026-05-22 05:00 | 04:00 | no | frozen | 2.82 kWh | 5.85 kWh |
| 21 | 2026-05-24 16:00 | 15:00 | no | frozen | 0.38 kWh | 1.99 kWh |
| 22 | 2026-05-30 04:00 | 03:00 | no | frozen | 0.42 kWh | 0.42 kWh |
| 23 | 2026-06-04 16:00 | 15:00 | no | frozen | 0.33 kWh | 1.55 kWh |
| 24 | 2026-06-05 08:00 | 07:00 | no | frozen | 1.53 kWh | 2.13 kWh |
| 25 | 2026-06-24 14:00 | 13:00 | no | frozen | 1.21 kWh | 1.57 kWh |
| 26 | 2026-07-02 07:00 | 06:00 | no | frozen | 0.47 kWh | 1.08 kWh |
| 27 | 2026-07-05 23:00 | 22:00 | no | frozen | 0.82 kWh | 3.52 kWh |
| 28 | 2026-07-08 18:00 | 17:00 | no | frozen | 1.21 kWh | 1.61 kWh |
| 29 | 2026-07-15 11:00 | 10:00 | no | frozen | 0.87 kWh | 2.84 kWh |
| 30 | 2026-08-10 02:00 | 01:00 | no | frozen | 0.31 kWh | 3.44 kWh |
| 31 | 2026-08-30 15:00 | 14:00 | no | frozen | 0.80 kWh | 1.83 kWh |
| 32 | 2026-08-31 06:00 | 05:00 | no | **moved** | 4.01 kWh | 8.04 kWh |
| 33 | 2026-09-02 13:00, 14:00 | 12:00, 13:00 | no | **moved** | 1.84 kWh (2 h) | 2.47 kWh |
| 34 | 2026-09-09 15:00 | 14:00 | no | frozen | 4.32 kWh | 7.30 kWh |
| 35 | 2026-09-21 21:00 | 20:00 | no | frozen | 0.50 kWh | 1.61 kWh |
| 36 | 2026-09-23 00:00 | 23:00 | no | frozen | 4.09 kWh | 8.48 kWh |
| 37 | 2026-09-26 06:00 | 05:00 | no | frozen | 3.28 kWh | 6.46 kWh |
| 38 | 2026-09-28 11:00 | 10:00 | no | frozen | 0.71 kWh | 1.88 kWh |
| 39 | 2026-09-29 23:00 | 22:00 | no | frozen | 2.86 kWh | 4.47 kWh |
| 40 | 2026-10-06 09:00 | 08:00 | no | frozen | 1.54 kWh | 2.05 kWh |
| 41 | 2026-10-12 02:00 | 01:00 | no | frozen | 0.72 kWh | 1.47 kWh |
| 42 | 2026-10-31 02:00 | 02:00 | no | frozen | 9.97 kWh | 16.28 kWh |
| 43 | 2026-11-18 16:00 | 16:00 | no | frozen | 3.08 kWh | 6.19 kWh |

## `stale-events.tsv`

One row per List 3 reading, tab separated, `#` starts a comment:

| Column | Content |
|---|---|
| `event` | Event number (1–43). The replay starts a fresh `AmsData` per event. |
| `role` | `pre` (two readings before, used to learn the cadence), `stale`, or `post` (the reading after). |
| `logged_local` | Logger's local wall-clock time of the frame. |
| `integrated_kwh` | Import energy integrated from the logged power over the hour **before** this reading. |
| `frame_hex` | The complete raw HDLC frame (`7E … 7E`) as received from the HAN port. |

`AMS_GH1119_HISTORY=<path> pio test -e native` replays another file in the
same format instead, for example every hourly reading of the year with
`event` = 0 throughout and `role` `ok`/`stale`. That full-year file (3.4 MB)
is not committed. The frames include the meter's serial number, which the
meter owner agreed to publish.
