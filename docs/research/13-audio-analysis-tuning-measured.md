# 13 — Audio analysis of the ST-50 demo video: the tuning, measured

Written 9 Oct 2026 from the MP3 you supplied ("Suiko ST-50 Koto Synthesizer: How Does it Sound… Worth the Hype", 10 min 18 s, 68 kbit/s, mono content). You confirmed the 調律 switch is in the **lower position, 純正律 (邦楽)**, for the whole video.

**Scope decision recorded:** back to the original plan. Drum mode stays parked; file 12's proposal to promote it is withdrawn.

## Result: what 純正律 does on the ST-50

The melodic sections all use the same five notes: A, B♭, D, E, F. That is the in scale with **mi = A, so the unit is set to 1本**.

Pitch of each note, measured frame by frame with a pitch tracker over the sustained parts of notes, relative to equal temperament at A = 440:

| Note | Degree | Offset from ET (cents) | Spread | Frames measured |
|---|---|---|---|---|
| A | mi | +2.7 | under 1 | 460+ |
| B♭ | fa | **−18.0** | 1 to 4 | 300+ |
| D | la | +2.2 | under 1 | 2,000+ |
| E | ti | +3.0 | about 1 | 500+ |
| F | do | **−12.8** | under 1 | 400+ |
| E♭ | ti♭ (upper row) | **−22 to −24** | about 1 | 31, two sections only |

The same values come out of five separate sections played on different voices and in different octaves (51–73 s, 77–104 s, 172–202 s, 208–224 s, 353–379 s), which is what gives me confidence in them. Everything sits about 2.7 cents sharp overall, i.e. A ≈ 440.7 Hz; that could be the unit, its fine-tune slider or the recording chain.

Expressed from the tonic:

| Degree | mi | fa | la | ti | do | ti♭ |
|---|---|---|---|---|---|---|
| **ST-50 純正律, measured** | 0 | **79** | 499.5 | 700 | **784.5** | **≈ 575** |
| Equal temperament | 0 | 100 | 500 | 700 | 800 | 600 |
| Pure fifths (file 07 default) | 0 | 90 | 498 | 702 | 792 | — |
| Textbook just intonation | 0 | 112 | 498 | 702 | 814 | — |

## What this settles

1. **Narrow, not wide.** The open question in file 10 is answered: 純正律 on the ST-50 lowers the semitone notes. Textbook just intonation is ruled out.
2. **Deeper than pure fifths.** Fa is about 21 cents flat and do about 15.5 flat. The semitones are 79 cents (mi–fa) and 84 cents (ti–do). That is very close to the "about 20 cents narrower" the ear-tuned koto demonstration reported in file 07, and deeper than the school tuner guide's 10.
3. **The frame stays equal-tempered.** La and ti do not move to pure 498 and 702; they stay at 500 and 700 within my measuring error. So the ST-50 does not retune the whole scale. It takes equal temperament and **drops only the "dark" notes**. Simple, and audibly Japanese.
4. **The two semitones are not identical**, and the upper-row ti♭ is lowered most of all (about 25). So it is a per-note table, not one depth applied uniformly.

## Change to the firmware tuning

The default 純正律 becomes the measured table, as offsets from equal temperament:

| Degree | mi | fa | la | ti | do | fa♯ | sol | ti♭ | do♯ | re |
|---|---|---|---|---|---|---|---|---|---|---|
| **SUIKO (default)** | 0 | −20.7 | 0 | 0 | −15.5 | ? | ? | −25 | ? | ? |

- Tuning presets are now: **平均律** (equal), **SUIKO** (measured, default for 純正律), **KOTO** (pure fifths: 90 / 498 / 702 / 792), **USER**.
- The single "depth" knob from file 07 is kept as a scaling of the SUIKO table (0 % = equal, 100 % = measured, beyond for deeper), since the offsets are not uniform.
- The four `?` upper-row notes are not measurable from this video: they barely occur in clean sustained form. Until measured, leave fa♯, sol, do♯ and re at 0. If you find a clip where the upper row is played slowly, I can fill them in.

## The other half: timbre

This is a much weaker result, for reasons worth knowing:

- **Notes overlap.** With long 余韻, several notes ring at once, and a pitch tracker then reports phantom low notes (two notes a fifth apart look like one note an octave or two below). That does not disturb the tuning figures above, but it wrecks per-note harmonic measurements in the densest section, which is the first one.
- **I cannot tell which section is which voice.** I cannot listen; I can only describe each section's shape. The video is not strictly one phrase per preset: there are 26 sections separated by silences.
- **The audio is mono** (left and right identical until the last 45 seconds), so it says nothing about stereo width or chorus.
- 68 kbit/s MP3 has nothing above 16 kHz and smears very short transients, so "click" measurements are indicative only.

### Sections, for you to label

| # | Time | Character from the analysis |
|---|---|---|
| 0 | 0:00.6–0:23 | Plucked, steady notes three per second, bright attack going darker (centroid about 1150 → 510 Hz), heavy overlap. **Koto, if the video starts with preset 1** |
| 1–3 | 0:28–0:48 | Three short plucked phrases, very low and dark (centroid 170–450 Hz). 17-string koto? |
| 4 | 0:51–1:13 | Plucked, brighter (about 1700 → 1000 Hz), fast clean attack with a click |
| 5 | 1:17–1:44 | Sustained, slow attack (about 85 ms), high notes |
| 6 | 1:49–1:59 | Struck, very short, bright, off-scale pitches. Bell or percussion |
| 7–8 | 2:07–2:45 | Plucked/struck, medium decay |
| 9–10 | 2:52–3:44 | Sustained with fast attack; section 9 has trills |
| 11 | 3:49–4:00 | Sustained, bright, different notes (B, C, D♯, F♯, G). Likely a bell tone confusing the tracker, or another key |
| 12–16 | 4:03–6:19 | Sustained voices, increasingly bright (flutes, reeds, strings?) |
| 17–22 | 6:25–8:16 | Struck sounds. Section 22 (7:44–8:16) is the darkest and lowest: the big drums |
| 23–25 | 8:18–10:18 | Many more pitch classes, not the in scale: looks like a demo song or outro music |

If you note the start time of each preset as you listen, I can attach the measurements to names.

### Plucked-voice measurements (section 4, the cleanest isolated notes)

Eleven notes that sound alone for at least 0.45 s:

- **Attack:** peak within about 5 ms on the mid and high notes. Effectively instant.
- **Click:** in the first 12 ms the energy above 5 kHz is 7–9 dB higher than in the following tenth of a second, on the mid and high notes. A distinct transient, as you heard on Harp 2.
- **Two-stage decay:** about −28 dB/s for the first 200 ms, easing to about −15 to −18 dB/s after. A fast initial drop, then a longer tail.
- **Harmonics, A4 at the start:** partials 1, 2 and 3 nearly equal (0, 0, −2 dB), then 4 at −9, 5 at −6, and −14 dB or lower from the 6th up.
- **Harmonics, same note 300 ms later:** 1 strongest; 2 down to −6; 3 and 4 at −2; **everything from the 5th up has fallen to −20 dB or below.**
- **Pitch at onset:** no consistent scoop or drop (median about −1 cent). The pitch is stable from the first instant.

Reading: a full, fundamental-heavy tone with strong low partials, an upper spectrum that collapses within a few hundred milliseconds, and a separate click on top. That matches "Harp 2 is closer than the thin Koto patch": the body is in partials 1–4, not in a bright wiry upper spectrum. It also suggests your filter hunch is describing this fast darkening after the attack. Whether section 4 is the koto or the neighbouring pluck voice needs your label.

## What would improve this

1. **Timestamps per preset** from you.
2. **A cleaner source.** If the original video can be downloaded at higher audio quality, the click and upper partials will be measurable properly.
3. **Single notes with short 余韻**, if any video has them. Overlap is the main obstacle.
4. **The 調律 switch in the other position** on the same phrase, to confirm that 平均律 really is plain equal temperament.
5. **Upper-row notes held**, to complete the tuning table.

## Method, briefly

Decoded to 44.1 kHz mono. Sections split at silences longer than 0.4 s below −50 dB. Pitch by the YIN method on 90 ms frames every 20 ms, keeping only frames with a strong periodicity score; offsets taken as the median per pitch class per section. Timbre from spectra at 0, 100 and 300 ms after each isolated onset, reading the level at each multiple of the fundamental. The analysis scripts are in this session's scratch space and can be rebuilt as proper tools in the bench (M2.5).
