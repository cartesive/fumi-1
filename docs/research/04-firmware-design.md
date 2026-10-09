# 04 — ST50 firmware design

Draft for discussion, 9 Oct 2026. Decisions marked **DECIDE** are yours; a default is given for each so work can start without waiting.

## Goal

Capture the essence of the Suiko ST-50 on the FM-1: an instrument whose keys *are* a Japanese scale, tuned the Japanese way, with a small set of traditional voices, expressive bend and vibrato, and a few switches rather than menus. Not a groovebox.

"Essence" in order of importance:

1. The key layout: in-scale front row you can gliss across, off-scale notes on a second row.
2. The 本 key system and a non-equal "Japanese" tuning.
3. Koto and shakuhachi voices that sit right, plus the dark strings/chorus.
4. Bend, vibrato, trill, sustain: the ornaments.
5. Reverb, because every owner adds it.
6. Auto-chord, drum mode, the recorder. Nice, later.

## Control mapping

The FM-1 has 16 white keys, 11 black keys, 14 lit buttons, 7 clicky encoders and one real pot (MASTER). No velocity.

### Keys

**White keys = the ST-50 front row.** Sixteen white keys hold exactly three octaves of a five-note scale plus the top tonic.

| White key | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 | 13 | 14 | 15 | 16 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| Degree | mi | fa | la | ti | do | mi | fa | la | ti | do | mi | fa | la | ti | do | mi |
| Cents (ET) | 0 | 100 | 500 | 700 | 800 | 1200 | 1300 | 1700 | 1900 | 2000 | 2400 | 2500 | 2900 | 3100 | 3200 | 3600 |

- Tonic keys (1, 6, 11, 16) glow as landmarks, like SLOOP's row markers.
- Sliding a finger along the whites is the koto glissando. The key scan runs at about 900 Hz, so fast slides register cleanly.
- The layout is a table in code, so an alternative start point is one line. The ST-50's own number labels (1 2 3 3' 5 6 7 8) suggest its row may start two notes below the tonic. Check against photos or video in the first session and switch if so.

**Black keys = the ST-50 upper row. DECIDE (default A).**

- **A. Faithful.** The eleven black keys play the off-scale notes fa♯ sol la♭ do♯ re (200, 300, 400, 900, 1000 cents above the tonic). A piano octave has five black keys and the upper row has five notes per octave, so they map one group to one octave: two full octaves plus one extra fa♯. The catch is alignment: a black-key octave spans seven white keys but a white-key octave is five, so the rows drift apart physically. On the real ST-50 both rows are five per octave and line up.
- **B. Ornament keys.** Black keys become momentary koto left-hand techniques applied to whatever white key is sounding: press-bend up a semitone (oshi-de), up a whole tone, bend-after-pluck (ato-oshi), quick up-and-back (tsuki-iro), slight drop (hiki-iro), vibrato while held (yuri), trill while held. This is less faithful to the panel and arguably more faithful to how the music is played, and it solves bend without touching a knob.
- **C. Both**, switched by a button (suggest SEQ, see below). This is where it probably ends up; A is the simpler thing to build first.

### Buttons

The ST-50 has switches, so buttons are latching toggles with their LED showing state. Tap toggles; hold shows that function's page on the screen with KNOB 1–4 as its settings.

| Button | Function | Hold + knobs |
|---|---|---|
| ENV | **SUSTAIN** on/off | decay / release time |
| LFO | **VIBRATO** on/off | rate, depth, onset delay |
| ARP | **TRILL** on/off | interval (next scale note / semitone), speed |
| SEL | **TUNING**: Japanese / equal | per-degree offsets, reference pitch |
| GLO | **CHORD**: keys play the "Japanese chord" | chord shape |
| FX | Effects page | reverb, size, ensemble, tone |
| EDIT | Voice edit pages | per-voice parameters |
| SEQ | **DRUM** mode (keys become percussion); or black-key mode switch if option C | kit levels |
| HOME | Home screen; MONO/POLY lives here | |
| SAVE | Save the current setup | |
| PLAY / REC | Performance recorder: record, play, overdub with another voice | |
| OCT− / OCT+ | Octave shift | |
| **OCT− + OCT+ held 5 s** | **Update mode. Reserved. Never reassign.** | |

### Encoders

| Encoder | Function |
|---|---|
| SELECT | **本** (key): 1–12, shown large on the home screen with its note name; continue below 1 into 水 numbers |
| ALGORITHM | **Voice**: shakuhachi, koto, oboe, strings, chorus |
| PRESETS | Scale: IN (default), YO; later, saved setups |
| KNOB 1 | **Pitch bend** (see below) |
| KNOB 2 | **Level** swell (the ST-50's second lever) |
| KNOB 3 | Vibrato depth |
| KNOB 4 | Reverb amount |
| MASTER | Volume |

### Screen

Home shows what a player needs at a glance: 本 number and note name, voice, tuning (和 / ET), and the row of sixteen keys with their degree names lighting as they sound. A small bend indicator shows the current offset and its return to centre. Japanese labels (本, 陰, 陽, 水) would look right but need a font with those glyphs; Felucca's emulator already bundles DotGothic16 (OFL), which has them.

## Tuning

Everything is computed in **cents**, in floating point, then converted to a frequency ratio once. No MIDI-note integer anywhere in the pitch path.

```
pitch_cents = tonic(本) + 1200 × octave
            + scale_table[degree] + tuning_offset[degree]
            + fine_tune + bend + vibrato + trill + attack_scoop
freq = 440 × 2^((pitch_cents − 5700 … ) / 1200)     // A4 reference, adjustable
```

### 本 → tonic

One semitone per 本, 1本 = A (measured 440 Hz on a real conductor; 4 = C, 6 = D, 8 = E are directly sourced).

| 本 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| Tonic | A | A♯ | B | C | C♯ | D | D♯ | E | F | F♯ | G | G♯ |
| Lowest white key (proposed) | A3 | A♯3 | B3 | C4 | C♯4 | D4 | D♯4 | E4 | F3 | F♯3 | G3 | G♯3 |

The wrap at 9本 is a guess from one data point (in a demo with mi = F, the tonic below middle C was F3). The real unit's register per 本 is unknown; it is a table, so it is easy to correct. OCT± moves everything by octaves.

### Scales

| Scale | Degrees (cents from tonic, equal temperament) | Use |
|---|---|---|
| IN 陰 (miyako-bushi) | 0, 100, 500, 700, 800 | Standard shigin. Default |
| YO 陽 | 0, 300, 500, 700, 1000 | Haiku |
| Upper row | 200, 300, 400, 900, 1000 | Black keys (option A) |

### "Japanese tuning"

The ST-50 has a switch for it and the smaller models have a quarter-tone switch, but **no source gives the actual offsets**. Plan:

1. Ship three tunings selectable on the SEL page:
   - **ET**: offsets all zero.
   - **WA 和 (provisional)**: tuned by pure fifths and fourths the way a koto is tuned by ear, which makes the two semitones narrow. In scale: **0, 90, 498, 702, 792**. Upper row by the same method: 204, 294, 408, 906, 996. This is my proposal from general koto practice, not a measurement of the ST-50.
   - **USER**: per-degree offset, ±50 cents, edited with the knobs or the web editor.
2. **Measure the real thing.** Run a pitch tracker over ST-50 recordings (the Aaron Horn one-shots; Hainbach's video where he flips the tuning switch, if he does) and replace WA with measured values. Felucca's `tests/pitch.py` (YIN) is already there for this.
3. Global fine tune ±50 cents and an A = 440/442 choice, since ensembles differ.

A unit test asserts every key × 本 × tuning against a reference table to 0.1 cent.

## Pitch bend without clicks

The risk is real: the knobs are detented encoders, so a naive bend is a staircase, and every stair is a small discontinuity you hear as zipper noise or a tick.

There are four separate causes of clicks here and each needs its own fix.

| Cause | Fix |
|---|---|
| **Stepped control value.** One detent = one jump in pitch | The encoder only moves a *target*. The audio engine chases it with a smoother in the cents domain: a one-pole filter (time constant about 25–40 ms) followed by a slew limit, updated every sample or every 8–16 samples with linear interpolation of the oscillator increment between updates. Felucca already does this for MIDI bend (`voice.c`, a quarter-of-the-gap-per-block chase), so there is working precedent |
| **Phase jumps.** Recomputing an oscillator from a new frequency | Phase-accumulator oscillators only: change the increment, never the phase. FM operators the same |
| **Delay-line jumps** (if the koto is a plucked-string model) | Never change the delay length abruptly. Slew the fractional delay and interpolate it (allpass or Lagrange). Unit-test this specifically; it is the usual source of bend clicks in string models |
| **Gain jumps.** The level lever, voice stealing, note-off in mono mode | Every gain change is a ramp of at least 3–5 ms. A stolen voice fades over about 2 ms before restarting |

Making an encoder feel like a lever:

- **Resolution and acceleration.** About 10 cents per detent turned slowly, more when turned fast (the platform's KNOB ACCEL idea), so one comfortable twist covers a whole tone. Range selectable: semitone, whole tone, minor third.
- **Spring return.** A lever returns to centre; an encoder does not. After about 150 ms without movement the bend target glides back to zero (return time adjustable, or off for "sticky" bend). It also resets on the next note in mono mode.
- **Screen feedback** shows the offset, since the knob has no physical centre.

Alternatives that avoid the knob entirely, worth having alongside it:

- **Ornament keys** (option B above): a held black key bends the sounding note up by a scale step or semitone over about 80 ms and releases back. This is how a koto player actually bends, and there is nothing stepped about it.
- **Mono legato glide**: in MONO, overlapping keys slide between pitches (portamento time on a knob). Very shakuhachi.
- **MIDI pitch bend in** over USB or TRS, 14-bit, through the same smoother.
- The **MASTER pot** is the only truly continuous control on the panel. An option to make it the level lever (expression) instead of volume is possible; not a default, since losing the volume knob is a nuisance.

**Regression test for clicks:** the host simulator renders a held note while a script applies bend steps, level steps, voice steals and tuning switches. The test fails if any sample-to-sample jump exceeds what the waveform's own slope allows, or if broadband energy spikes in the 5 ms after a control event. Add it early and keep it; clicks are easy to reintroduce.

## Voices

All synthesised. No ST-50 samples can be shipped, and flash is too small for good multisamples anyway. The ST-50's own engine is unknown, so the targets are the instruments it imitates, checked by ear against ST-50 recordings.

| Voice | Approach | Details that carry the character |
|---|---|---|
| **KOTO** | Plucked-string model (Felucca's PHYS strings, MIT-derived) or 2-operator FM pluck. Prototype both on the host and pick by ear | Sharp plectrum attack, pick-position comb colour, long decay with SUSTAIN on and damped with it off, slight inharmonic "sawari" brightness |
| **SHAKUHACHI** | Sine/triangle core with light FM, plus band-passed breath noise | Pitch scoop into the note (starts 30–80 cents flat, rises in about 80 ms), breath burst on attack, slow delayed vibrato, legato glide in mono |
| **OBOE** | Narrow pulse or 2-op FM through a fixed formant | Nasal, steady, close to hichiriki |
| **STRINGS** | Two detuned saws, low-pass, ensemble chorus | Dark and slow, "Mellotron-like" per owners |
| **CHORUS** | Formant "aah" (Felucca's VOICE engine is a starting point) through the same ensemble | Dark, slightly unstable pitch |
| **DRUM mode** | Synthesised: taiko (pitched membrane + noise), shime-daiko, tsuzumi (pitch-drop "pon"), atarigane (metallic FM), hyōshigi (wood clack) | One sound per white key; later |

Shared: vibrato LFO with onset delay, trill (alternates with the next scale degree up, as a koto trill does), hall reverb (Felucca's HALL), ensemble chorus, stereo width. Polyphony target 8 in POLY; MONO is last-note priority with legato.

No velocity on the keys, so dynamics come from KNOB 2 (level), from MIDI velocity in, and optionally a "hard hit" modifier (SLOOP uses OCT− / OCT+ held for ghost and hard hits; here those are octave buttons, so a held black key in ornament mode could do it).

## Chord, recorder, cards

- **Auto-chord:** what the ST-50 plays is unknown. Start with tonic + fifth + octave drawn from the in scale (an open, koto-like sonority), make the shape a table, correct after listening.
- **Recorder:** one performance, record / play / overdub with a different voice, stored through the platform's storage objects (3,840 bytes each, so events are packed and a recording spans several objects). Later milestone.
- **IC accompaniment cards:** not reproducible (contents unknown). The equivalent is a few built-in preludes (前奏) in the public domain, played by the recorder engine. Optional.

## MIDI

- Out: notes as the nearest equal-tempered note plus pitch bend for the tuning offset, or plain notes with tuning ignored (setting). Per-note bend only works in MONO; say so in the manual.
- In: notes play the current voice. Two modes: chromatic (a normal keyboard), or white-keys-as-scale so an external keyboard behaves like the front row.
- Clock: not needed in v1.

## Web

- **Installer:** FoMni's `omni_installer.html` rebranded. Includes "return to official V15".
- **Emulator:** FoMni's `web/emu/` builds the same C to WebAssembly. Useful for trying layouts before flashing, and for sharing.
- **Editor (later):** SLOOP/Felucca's SysEx editor protocol (`F0 7D 46 4C …`), cut down to what an ST50 needs: tuning tables, voice parameters, key layout, setups backup. Use a different two-byte sub-ID from Felucca's "FL" so the editors cannot talk to the wrong firmware.

## Open design questions

1. Black keys: faithful upper row, ornament keys, or switchable? (Default: build A, add B, end at C.)
2. How much beyond the instrument itself do you want in the first release: drum mode, recorder, preludes?
3. Japanese labels on screen, English, or both?
4. A working name. "ST50" is fine privately; a public release needs a name of its own with "inspired by the Suiko ST-50".
