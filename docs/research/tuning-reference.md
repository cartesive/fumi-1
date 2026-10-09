# Tuning reference (implementation tables)

This file is the authoritative statement of the pitch maths. It supersedes the one-line formula sketch in `04-firmware-design.md`, which is garbled.

## Formula

All pitch is carried in cents relative to A4, as a float.

```
cents_from_A4 = 100 × (low_tonic_midi[hon] − 69)
              + 1200 × (octave_shift + key_octave)
              + scale[degree]
              + tuning_offset[tuning][degree]
              + fine_tune
              + bend + vibrato + trill + attack_scoop      // all smoothed, see 04

freq_hz = a4_ref × 2^(cents_from_A4 / 1200)                // a4_ref = 440.0 default
```

`bend`, `vibrato`, `trill` and `attack_scoop` are per-sample (or interpolated) signals. Everything above them changes only on key or setting events.

## 本 (hon) → tonic

| 本 | Note | Tonic pitch class (MIDI, A4 = 69) | `low_tonic_midi` (proposed) | Hz at A = 440 | Status |
|---|---|---|---|---|---|
| 1 | A | 69 | 57 (A3) | 220.00 | sourced (measured 440 Hz for 1本) |
| 2 | A♯ | 70 | 58 | 233.08 | interpolated |
| 3 | B | 71 | 59 | 246.94 | interpolated |
| 4 | C | 72 | 60 (C4) | 261.63 | sourced |
| 5 | C♯ | 73 | 61 | 277.18 | interpolated |
| 6 | D | 74 | 62 | 293.66 | sourced |
| 7 | D♯ | 75 | 63 | 311.13 | interpolated |
| 8 | E | 76 | 64 | 329.63 | sourced |
| 9 | F | 77 | 53 (F3) | 174.61 | interpolated; register from one demo |
| 10 | F♯ | 78 | 54 | 185.00 | interpolated |
| 11 | G | 79 | 55 | 196.00 | interpolated |
| 12 | G♯ | 80 | 56 | 207.65 | interpolated |

水 numbers: 水n本 has the pitch class of (13 − n)本, one octave lower (水1本 = G♯ … 水5本 = E). 13–15本 = 1–3本.

`low_tonic_midi` is the pitch of white key 1 with no octave shift. Which octave each 本 sits in on the real ST-50 is not known; the wrap at 9本 is a guess. Keep it as a table.

## Scale tables (cents above tonic)

| Name | Degree names | Equal temperament | WA 和 provisional (pure fifths/fourths) |
|---|---|---|---|
| IN 陰 | mi fa la ti do | 0, 100, 500, 700, 800 | 0.0, 90.2, 498.0, 702.0, 792.2 |
| YO 陽 | mi sol la ti re | 0, 300, 500, 700, 1000 | 0.0, 294.1, 498.0, 702.0, 996.1 |
| Upper row | fa♯ sol la♭ do♯ re | 200, 300, 400, 900, 1000 | 203.9, 294.1, 407.8, 905.9, 996.1 |

WA ratios: fa 256/243, la 4/3, ti 3/2, do 128/81; fa♯ 9/8, sol 32/27, la♭ 81/64, do♯ 27/16, re 16/9.

The WA column is a proposal from how a koto is tuned by ear, **not** a measurement of the ST-50. Replace it with measured offsets once recordings have been pitch-tracked. Traditional practice often takes the semitones narrower still, so expect fa and do to end up at or below these values.

Stored as offsets from equal temperament (what the USER tuning edits):

| Degree (IN) | mi | fa | la | ti | do |
|---|---|---|---|---|---|
| ET | 0 | 0 | 0 | 0 | 0 |
| WA provisional | 0 | −9.8 | −2.0 | +2.0 | −7.8 |

## Key tables

White keys (index 0–15, F3…G5 on the panel): `degree = i mod 5`, `key_octave = i div 5`. So keys 0, 5, 10, 15 are tonics.

Black keys, option A (index 0–10, F♯3…F♯5 on the panel): `degree = i mod 5` into the upper-row table, `key_octave = i div 5`.

## Checks to automate

- Every white key × every 本 × {ET, WA}: rendered pitch within 0.1 cent of the table (YIN on a host render; allow 1 cent for the measurement itself).
- 1本, white key 1, ET, A = 440 → 220.00 Hz.
- 4本, white key 6 → C5 = 523.25 Hz.
- 9本, white keys 1–5, ET → F3, G♭3, B♭3, C4, D♭4 (matches the notes read off the ST-50 demo).
- Switching ET ↔ WA while a note is held produces no click (the offset change goes through the bend smoother).
