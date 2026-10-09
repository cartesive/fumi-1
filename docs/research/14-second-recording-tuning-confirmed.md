# 14 — Second recording: tuning confirmed, and the 本数 slider decoded

Written 9 Oct 2026 from the second MP3 you supplied (Hainbach, "The rarest Japanese synthesizer? | Suiko ST-50", 18 min 38 s, 101 kbit/s). The opening run up the lower row (0.45–4.4 s) is as good as you hoped: fifteen notes, one after another, all left ringing, with nothing else playing.

## Method for this clip

The notes go by at about 60 ms each, too fast for a pitch tracker. But because they all ring on, one long spectrum over the whole four seconds shows every note as a sharp peak, at a resolution of a fraction of a cent in the mid range. Peak positions were read with interpolation. This is a different method from file 13 and a different recording, so agreement between the two is a real cross-check.

## The notes

F, G♭, B♭, C, D♭ in three octaves, from **F2 to D♭5**. That is the in scale with **mi = F**, which is the "mi = F" demo the forum answer in file 01 was describing.

Offsets from equal temperament at A = 440:

| Degree | Note | Octave 1 | Octave 2 | Octave 3 |
|---|---|---|---|---|
| mi | F | +4.7 (F2) | +4.0 (F3) | +5.7 (F4) |
| fa | G♭ | −16.0 | −15.6 | −16.6 |
| la | B♭ | +3.4 | +4.3 | +4.1 |
| ti | C | +6.6 | +4.4 | +4.7 |
| do | D♭ | −12.1 | −10.9 | −11.5 |

The whole instrument is about 4.6 cents sharp in this recording (2.7 in the first). The lowest octave is the least precise, because a cent is a much smaller number of hertz down there.

## Result: the same table

Each degree relative to the mi of its own octave, averaged over the three octaves:

| Degree | mi | fa | la | ti | do |
|---|---|---|---|---|---|
| **Recording 2** (mi = F, spectral peaks) | 0 | **79.1** | 499.1 | 700.4 | **783.7** |
| **Recording 1** (mi = A, pitch tracker) | 0 | **79.3** | 499.5 | 700.3 | **784.5** |
| Difference | | 0.2 | 0.4 | 0.1 | 0.8 |

Two recordings, two keys eight semitones apart, two measuring methods, agreement inside one cent on every note. The conclusions of file 13 stand, now firmly:

- 純正律 on the ST-50 is **equal temperament with fa lowered about 21 cents and do lowered about 16 cents**. Semitones of 79 and 84 cents.
- La and ti are **not** moved to pure intervals.
- The table belongs to the **scale degrees**, not to fixed note names: it moves with the 本数 setting. (It could in principle have been a fixed table per note name; this rules that out.)

**Firmware default for 純正律, as offsets from equal temperament: fa −20.8, do −16.0, ti♭ about −25 (weak), others 0.** This supersedes the figures in 13 by a fraction of a cent.

Hainbach's unit must also have had the switch on 純正律, since it produces the same table.

## The 本数 slider decoded

The two recordings together explain how the key slider maps to pitch.

- Recording 2: the row runs F2 … D♭5, so key 1 (水) = F2, key 6 (三) = **F3**, key 11 (八) = F4. This matches the forum note that key 六 was middle C.
- Recording 1 (set to mi = A): the plucked voice's lowest notes are A2 and B♭2 and it reaches F5. So key 1 = A2, key 6 (三) = **A3 = 220 Hz**.
- The panel photo, which has the same marker annotations as this video, shows the 本数 slider at its far left: **水4**.

Put together: the slider is simply sixteen consecutive semitones, left to right.

| Slider | 水4 | 水3 | 水2 | 水1 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| 三 (white key 6) | F3 | F♯3 | G3 | G♯3 | **A3** | A♯3 | B3 | C4 | C♯4 | D4 | D♯4 | E4 | F4 | F♯4 | G4 | G♯4 |
| 水 (white key 1) | F2 | F♯2 | G2 | G♯2 | A2 | A♯2 | B2 | C3 | C♯3 | D3 | D♯3 | E3 | F3 | F♯3 | G3 | G♯3 |

So **三 = MIDI note 53 + slider position (0–15)**, 1本 puts 三 on A3 = 220 Hz, and the 水 settings continue straight down below it. No octave wrap anywhere.

This replaces the 本 table in `tuning-reference.md`, where I had guessed a wrap at 9本. Confidence: good but not certain. It rests on the photo and the audio being the same session, and on neither player having the octave-down button engaged. The alternative reading is that Hainbach was at 9本 with octave-down on, which gives the same sounding pitches, so the table above is right for 1本 and for whichever of 水4 / 9本-down he used, and the straight-line slider is the simplest thing that fits both.

## What else is in this recording

Only the first four seconds are clean. After that there is speech and layered, processed playing, and the stereo image is wide (left and right correlate at 0.8), so effects are in the chain. I did not attempt timbre measurements from it. The opening run itself is too fast for per-note attack and decay figures.

## The transcript

I could not get it: YouTube blocked the fetch, and a transcript mirror reported the same block. If you open the video's transcript panel and paste the text, I will go through it for anything about the tuning switch, the two rockers, 余韻, the voices and the drums.

## Still unmeasured

- The other four upper-row notes (fa♯, sol, do♯, re) in 純正律.
- The same run with the switch on 平均律.
- Vibrato rate and depth range, trill speed, pitch-rocker range.
