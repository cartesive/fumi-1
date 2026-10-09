# 06 — Decisions, bend gestures and the voice plan

Written 9 Oct 2026 after your answers. Where this file disagrees with 04 or 05, this file wins.

## Decisions taken

| Question | Decision |
|---|---|
| Black keys | **Switchable**: ST-50 upper row ↔ ornament keys |
| First release | **Just the instrument.** No drum mode, recorder or preludes. M6 in the build plan is parked |
| Dev machine | **macOS, M4 mini** |
| Recovery | Accept the risk on one unit; build with care. No dongle |
| Licence | GPL-3.0, public release is fine |
| Success test | **Feel, not function.** A beautiful koto and the ST-50's character. Everything below is organised around that |

### What macOS on an M4 means

- The JieLi toolchain is Linux x86-64 only, so firmware builds run in Docker (`linux/amd64`, through Rosetta). FoMni's `build.sh` already does this. Keep the source tree under `/Users` so Docker can share it.
- The host simulator, host tests and browser emulator build natively and fast. That is where nearly all voice work happens.
- The no-hardware rescue script (`tools/fm1_rescue.py`) is Mac-hosted, so you have the software rung of the recovery ladder even without the dongle. It only helps if the unit reaches the chip's own USB mode (`WL80UBOOT`), which is where a boot-looping Felucca-family firmware ends up after four failed boots.
- "Accept the risk" makes the M1 step more important, not less: flash your own build of *unmodified* FoMni and do a full round trip to V15 before any ST50 code goes on.

## A name

FoMni is FM + Omni. Candidates in the same spirit:

- **FuMi-1** — 文 (*fumi*) means writing, a letter, a composed text. FM with the vowels filled in, and it is a poetry word. My pick.
- **FoeM-1** — FM + poem. The most direct echo of FoMni.
- **FuMiKoto** — the two above ideas plus the instrument. Longer; reads as a name.
- **GinFM** — 吟 (*gin*), the chanting in shigin.
- **UtaFuMi** — 歌 (*uta*, song/poem) + 文.

Whatever the name, the public description says "inspired by the Suiko ST-50".

## Bend: octave buttons and key slides

Both of your ideas are better than the knob as the primary gesture, and they solve the click problem at the root: a button or a key gives the engine a start, a target and a time, so the pitch curve is drawn by the engine and is smooth by construction. KNOB 1 stops being the bender and becomes "how far and how fast".

### OCT− / OCT+ as bend buttons

- **Role is decided at the moment of the press.** If at least one note key is held, the button is a bender until it is released. If no key is held, it shifts the octave as now. A press never changes role half way.
- **OCT+ held:** the pitch rises to the bend target over the bend time and stays there. **Released:** it falls back. That is a spring-return lever, which an encoder cannot be.
- **Quick tap** = up and straight back: the koto's *tsuki-iro*.
- **Press after the pluck** = *ato-oshi* (the note blooms upward as it rings).
- **OCT−:** a small drop and return, the *hiki-iro* (a real koto can only lower a string slightly). Default about −40 cents; settable.
- **Bend target** (KNOB 1 while a bend button is held, or on the bend page): semitone (*yowa-oshi*, weak press), whole tone (*tsuyo-oshi*, strong press), or "next scale note up".
- **Bend time:** default about 90 ms up, 120 ms down, with a slightly S-shaped curve. A finger pressing a string does not move linearly.
- **Which notes bend:** only the most recently played note by default, as on a koto where the left hand presses one string. A held chord stays put while one voice bends. Option: all notes.
- **Pre-bent notes** (press the string, then pluck) cannot be done this way, because with no key held the button is an octave shift. The black keys in upper-row mode are those notes.
- **Both buttons together stay reserved** for update mode (countdown appears after 2 s, enters at 5 s, cancels on release). Bend logic ignores the both-held state.

### Slide between keys

- **SLIDE** is a per-voice setting. With it on, pressing a second key while the first is still held glides the sounding note to the new pitch without a new attack; releasing the second key glides back if the first is still down.
- **Shakuhachi:** on by default. This is how the instrument is played and it will sound right immediately.
- **Koto:** off by default (a new key is a new pluck, so glissandi across the white keys work). With it on, a held note plus a neighbouring key gives a press-bend to exactly that scale note. Less faithful to the ST-50 panel, quite faithful to the instrument.
- Slide time on a knob; short slides for small intervals, longer for wide ones.

### Black keys, switchable

- **Upper-row mode:** fa♯ sol la♭ do♯ re, as on the ST-50.
- **Ornament mode:** momentary techniques on the sounding note. With bend now on the OCT buttons, the eleven keys are free for the rest: vibrato while held (*yuri*), trill while held, tremolo re-pluck (*sararin*-style), scrape attack (*chirashi*), octave-doubled pluck (*awase-zume*), damp/mute, and one-key rising and falling glissandi (*nagashi-zume*).
- Switch with SEQ (now free, since drum mode is out). The screen shows which mode is active and labels the keys.

The click-free smoother from 04 stays: it still covers KNOB adjustments, the level control, tuning switches and MIDI bend. The click regression test stays too.

## The level lever

Honest status of the research: I know the lever exists and nothing about how it behaves. Three seller descriptions mention it, as "two bend levers, one for pitch and one for sound level" and as a "Pitch Wheel and Volume Wheel". None says which way it springs, whether it rests at full or at silence, how far it goes, or how players use it. I did not have "fade-in" before you said it.

If it is a lever you hold to silence the attack and release (or push) to let the note swell in, that matters a great deal for feel: it is how a plucked voice turns into a bowed or blown one, and how a shakuhachi phrase is shaped. So this needs your knowledge or a close watch of the videos. Things to pin down:

1. Rest position: full volume or silent?
2. Sprung or stays where put?
3. Does it reach true silence?
4. Is it used per note (swell each note in) or per phrase?

How it could map to the FM-1, once known:

- **MASTER pot as the lever.** It is the only genuinely continuous control on the panel, 10-bit, already smoothed in the platform code. In a LEVER mode it becomes the level lever and overall volume moves to a setting. This is the only mapping that gives a hand-shaped fade. No spring, though.
- **A swell button.** Hold a button while playing and each new note fades in over the swell time instead of plucking; release returns to normal attacks. A sprung lever in button form. Good for alternating plucked and swelled notes quickly.
- **KNOB 2** as in 04: stepped, so only suitable for setting a level, not performing a fade.

My guess is MASTER-as-lever plus a swell button is the right pair, but I would rather build it from how the real one behaves.

## Other things learned in this pass

- There appear to be **two ST-50 versions**. A 2010 listing describes four sounds (Koto, Shakuhachi, "Suiko" original sound, Strings) and mentions an "expanded ST-50" with more. A 2018 listing claims 15 selectable sounds (shakuhachi, koto, "various flutes", strings) and 15 percussion sounds. The five-voice list in 01 may be a third description of the same thing. The first release should nail koto, shakuhachi, strings and one "Suiko" voice before widening.
- Sellers describe the tone as "very analog and warm" and "realistic, warm and strange". That argues against a bright, glassy, physically perfect koto.
- The MetaFilter thread points to **Ando's paper on koto tuning** as a source for traditional frequencies. Worth finding for the WA tuning table before relying on my pure-fifths guess.

## Voice plan

### Two targets, not one

"A beautiful koto" and "the ST-50 feel" are different targets and can pull apart. A real koto is bright, percussive and complex. The ST-50 is a 1980s machine's idea of a koto: warmer, simpler, steadier, a little strange. The plan is to build the koto properly, then put it through a character stage that takes it toward the ST-50, with one control between them. Then you choose by ear where on that line it should live, per voice.

### Architecture: three models, many patches

A patch is a small table of numbers; a model is the code that turns it into sound. Three models cover everything the ST-50 does:

| Model | Voices | Core |
|---|---|---|
| **PLUCK** | Koto, later 17-string bass koto, shamisen, biwa | Excitation burst → tuned string loop (delay line with damping and dispersion filters) → body resonator |
| **BLOW** | Shakuhachi, oboe, later shinobue, nohkan | Sine/triangle core with a little FM, breath noise through a band-pass that tracks pitch, pitch and amplitude contours |
| **ENSEMBLE** | Strings, chorus, the "Suiko" voice | Two or three detuned oscillators → low-pass → formant filter (chorus) → ensemble chorus |

All three share the pitch core (tuning, bend, slide, vibrato, trill), the level control, the character stage and the reverb.

A patch is a `const` struct in flash, around 30 parameters. Patches live in one human-readable file that the build turns into C, so they can be edited without touching engine code, and later sent from the web editor.

### What makes the koto a koto

In rough order of how much each contributes:

1. **The attack.** A hard plectrum (*tsume*) striking close to the bridge: a sharp click plus a bright, slightly hollow start. Modelled as a short noise/impulse burst whose colour depends on pluck position (a comb filter), not as an envelope on a static wave.
2. **Two-stage decay.** Loud and bright for the first 100–200 ms, then a long, darker, quieter tail. High partials die much faster than low ones. This falls out of a string loop with frequency-dependent damping; it is the reason to prefer a string model over an FM pluck.
3. **Pitch settling.** A hard-plucked string starts a few cents sharp and relaxes. Tiny, and the ear notices when it is missing.
4. **Body.** A long hollow paulownia box: a few broad resonances, not much low end.
5. **Sympathetic ring.** Other strings answering. Felucca has a sympathetic-strings module (`phys_symp.c`) to borrow. With SUSTAIN on, notes ring over each other as on the instrument.
6. **No two plucks identical.** See the feel layer below.

Two prototypes (string model and FM pluck) are still worth rendering side by side in M3, because the FM pluck may turn out closer to the *ST-50's* koto even if the string model is closer to a real one.

### What makes the shakuhachi a shakuhachi

- The note starts flat and rises into pitch, with a breath burst (*muraiki* when strong).
- Breath noise is part of the tone throughout, more at the start and in loud notes.
- Vibrato arrives late and is as much amplitude and breath as pitch (*yuri*).
- Notes are joined by slides, and end by falling away in pitch and level rather than stopping.
- So BLOW's contours matter more than its oscillator. SLIDE and the level lever do most of the expressive work.

### The feel layer

The keys have no velocity, so dynamics and variation have to come from how you play. This is where "technically works" and "feels alive" part ways, and it is cheap to implement:

- **Gesture-derived dynamics.** Time since the previous note and the distance between keys tell the engine what is happening. Fast adjacent keys are a glissando: lighter, brighter, shorter plucks that build toward the last note. A note after a pause is a full, deliberate pluck.
- **Per-pluck variation.** Small random changes to pluck position, strength and burst colour, and a cent or two of detune. Repeated notes stop sounding like a machine gun.
- **Same string, same voice.** Re-plucking a key re-excites the same string loop instead of starting a new voice on top, with the brief damping thump a finger makes. Sixteen white keys behave like sixteen strings.
- **Release behaviour.** SUSTAIN on: strings ring until they die or are re-plucked. SUSTAIN off: gentle damping on release, not a gate.
- **Trill and vibrato with human timing.** Slightly uneven rate, depth that grows in. The one criticism found of the real ST-50 is that its trill is repetitive; this is the place to be better than the original.
- **A strong-pluck modifier** on an ornament key for accents.

### The character stage

Per voice, between the model and the reverb: gentle band-limiting, a soft low-mid warmth, optional reduced sample rate and bit depth for 1980s digital grain, and the stereo ensemble. One CHARACTER amount from "clean model" to "ST-50". Its settings are calibrated against reference recordings, since the ST-50's actual engine is unknown.

### How patches get made: the bench

Voicing by flashing hardware would be slow and would wear the one unit. Instead:

1. **`bench/`**, a small host program: takes a patch file and a phrase script (single notes across the range, a glissando, a bend, a trill, a slow melody), writes WAVs. A second or two per render on the M4.
2. **Reference analysis.** For each reference recording, measure what can be measured: attack time, decay time per partial, spectral centroid over time, inharmonicity, pitch contour at onset, vibrato rate and depth, noise-to-tone ratio. This gives starting values for a patch and catches gross errors. It does not decide whether it is beautiful.
3. **A/B by ear.** Level-matched reference and render, same note, played back to back. A short listening sheet per voice (attack, body, tail, bend, gliss, low/mid/high register) so judgements are written down rather than remembered.
4. **Play it.** The browser emulator for feel under the fingers, then the hardware. **Voice on the FM-1's own speaker early**: a tiny speaker changes everything, and a patch tuned on studio monitors will be wrong on it. Check headphones and line out as well; the platform has a speaker EQ setting for this trade.
5. **Iterate in that loop**, with Claude Code doing the rendering, measurement and parameter sweeps, and you doing the listening. Golden-file tests then lock a voice once you approve it, so later engine changes cannot quietly alter it.

References needed: the Aaron Horn ST-50 one-shots for the ST-50 target, and a few clean solo koto and shakuhachi recordings for the instrument target (any you own; they are for listening and measurement only, nothing is copied into the firmware).

### Order of voice work

1. Koto, to the point where you enjoy playing it. Nothing else until then.
2. Bend buttons, slide, the feel layer, reverb, on the koto.
3. Shakuhachi with slide and the level lever.
4. Strings and the "Suiko"/chorus voice.
5. Oboe and any flutes.

## Changes to the build plan

- **M2** adds the bend-button and slide logic to the pitch core, with tests for role-at-press and the both-buttons reservation.
- **M3 becomes "the koto"**: bench tool, reference analysis, two prototypes, feel layer, character stage, reverb. It is the longest milestone and is allowed to be.
- **M4** (first flash) happens when the koto is worth hearing on the speaker, not before.
- **M5** is the other voices, the black-key mode switch, tunings, level lever, save/load, MIDI.
- **M6 is parked.**
- **M7/M8** unchanged.

Addition to the Claude Code kickoff prompt: "Read `06-decisions-and-voice-plan.md` after 05; it overrides 04 and 05 where they differ. The priority is how the koto sounds and feels. Build the bench tool before any voice code."

## Sources added in this pass

- https://www.matrixsynth.com/2010/07/suiko-st-50.html (four sounds; expanded version; two bend levers)
- https://www.matrixsynth.com/2018/12/suiko-suiko-st-50-rare-portable.html (15 sounds, 15 percussion, "Pitch Wheel and Volume Wheel")
- https://ask.metafilter.com/361261/What-notes-do-the-keys-of-the-Suiko-ST-50-play-so-I-can-DIY-one-myself (pointer to Ando's koto tuning paper)
- Koto and shakuhachi technique names and acoustics are from general knowledge, not from a source fetched in this session; check them against a koto method book before they go into a manual.
