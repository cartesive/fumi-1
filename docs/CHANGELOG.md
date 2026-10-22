# FuMi-1 releases

Identities are `FM-1_7` + major, minor and patch as two digits each; the FM-1 reports the identity
(`python3 tools/fm1_install.py --info`).

## 1.0.8 (22 Oct 2026) — `FM-1_7010008`

- **Seven more voices**, the owner's picks from community banks on patches.fm: Chimes チャイム (CHIMES 1), Rin 鈴
  (TBTAN BELL by Tim Conrardy), Bell ベル (MICHEL 2 by Tim Conrardy), Ice Bells 氷鈴 (Ice.Bells1 by Tim Garrett),
  Clarinet クラリネット (Clarinet12 by Tim Garrett), Strings 弦楽 (STRINGS 2 by Tim Conrardy) and Fairytale 御伽
  (Fairlyte1a). Voices written an octave up or down have their ratios doubled or halved, as the engine ignores a
  patch's transpose (`tools/patchfm_import.py`); four were trimmed into the others' level band (Rin −4 dB, Bell
  −1.5, Ice Bells −8, Fairytale −3, measured at key 三).
- **Twenty-one instruments in the ST-50 strip's families**, not appended: the kotos (Koto, Koto II, Koto Pluck, Koto
  Ring, Koto Warm, Harp), the bells, the winds (Shakuhachi, Bamboo Flt, Dragon Flt, Sho), the sustained voices (Kokyu,
  Clarinet, Strings, Fairytale), percussion (Taiko, Hyoshigi, Mokugyo). Koto is still the boot voice. A setup saved
  by 1.0.6 or 1.0.7 keeps its instrument by name (PROJ_FORMAT 4, the slot renumbered on load).
- **`FuMi-1.0.8.sf2` on the release** (22 Oct 2026, 114 MB): the twenty-one instruments as a SoundFont in this
  version's order, rendered as in 1.0.7 (`docs/soundfont.md`). The three ringing bells are a kind of their own in
  the tools, trimmed as plucks but their pitch only reported: Ice Bells has no carrier at the key (a strike tone a
  pitch estimate reads about 48 cents sharp) and Rin keeps its author's +7 detune. The 1.0.7 file stays on the
  1.0.7 release in 1.0.7's numbering, as a released file does.
- **The screen's keys are the panel's**: two rows of round caps, sixteen below with the ST-50's tan caps on ラ乙 シ一
  ミ三 ラ五 シ六, eleven above sitting between them, each lit with its voice; under the lower row the conductor's
  names 水 乙 一 二 三 三′ 五 六 七 八 as printed on the panel, 三 marked. Beneath them a **voice slider**, the 音色
  strip: a notch per instrument in PRESETS order, a wider gap where the family changes, the one playing filled; the
  name in large type says which. Names of four characters or more take a 24 px face so the Latin name keeps its
  room. The bench's on-screen keys follow the same layout.
- The firmware grows by the glyphs (each size holds only what it draws: 51 glyphs, 43 KB) and the seven voices.

## 1.0.7 (12 Oct 2026) — `FM-1_7010007`

- **Fixed: the trill on the hyōshigi.** The trill clock gave up when the held key's voice was gone, and a
  hyōshigi crack is over in under 200 ms: at the trill rate it managed two strokes, with oroshi one, the
  mokugyo one; the slow trill rates gave a single clap. The held key is now struck again whatever became of
  its last sound, on a fresh voice. Measured: 20 strokes in 3 s plain, 6 on oroshi Long, as the taiko.
- **The koto's left hand on the three empty ornament keys** (SEQ on). Black 5, 押し手 oshide: the sounding
  notes rise a whole tone over about 80 ms while the key is held, and fall back on release. Black 10, 引き色
  hikiiro: while held, each new note dips 40 cents just after the pluck and comes back over 150 ms. Black
  11, 後押し atooshi: while held, each new note is pushed up a semitone 200 ms after it sounds. They ride the
  bend's pitch glide (no click), a trill stroke starts them again, and the loop records them like the others.
- **MIDI in carries velocity.** A note from a MIDI keyboard plays at its own velocity, through the
  instrument's velocity response (FuMi's own six respond; the community patches as their authors wrote
  them): on Koto Pluck, velocity 64 is 5 dB under 100. Before, every MIDI note played at 100.
- **The screen.** In ornament mode the header shows 颪 while oroshi is set, so the player knows the trill
  key rolls. On the Trill page, pair gap and ta level read as words, Even / Loose / Paired / Tight and
  Same / Softer / Light / Faint, the bar beneath keeping the exact position. Header tags never run into
  the page title: those that would not fit are dropped.
- **Kokyū 胡弓 without the bow tremolo** (the owner's call): the slowed attack stays, the amplitude LFO is
  gone, and the level swing over a held note falls from 4.7 dB to 2.9 dB (what is left is the ensemble
  patch's own beating).
- **Fixed: a MIDI note-off that never came.** A key whose sound ends by itself with the key still down (the
  hyōshigi, the mokugyo) sent its note-on and no note-off, so a synth on MIDI out held the note; the note-off
  now goes out when the sound ends. Found in review, with four smaller faults in the new code (a swoop on
  notes plucked under 押し手, the trill losing the held key after another was released or after a retune).
- The setup format is unchanged; 1.0.6 setups load as they are.
- **13 Oct 2026: `FuMi-1.0.7.sf2` on the release**, the fourteen instruments as a SoundFont rendered from this
  version (the owner's design, `docs/soundfont.md`): white keys F3–G5 at 1本, the Suiko tuning in the
  samples, dry, CC BY 4.0. Made by `tools/sf_render.py`, `sf_bank.py`, `sf_check.py`; the host simulator
  gained `wavgain` for it. The firmware is unchanged.

## 1.0.6 (11 Oct 2026) — `FM-1_7010006`

- **A trill that sounds played.** Strokes come in pairs, *ka-ta*, the short gap 42 % of the pair; the first
  stroke of a trill is the hardest (velocity 127, +1.5 dB), each *ta* light (64, −4 dB), each later *ka*
  normal, and no two strokes alike. A key pressed while the trill is on is a hard *KA* and restarts the pair,
  so the strokes line up with the hand. Jitter sits mostly on the long gap. **Trill feel** (the old Trill
  var) is one knob from 0, the ST-50's even machine, to 100, the full pairs and accents. The strong-pluck
  ornament plays the hard stroke too, and is now audible. MIDI out carries the velocities.
- **Oroshi 颪.** A setting for the trill ornament key (the second black key in ornament mode): None, Short,
  Medium or Long, the time the trill takes from slow to the trill rate (about 1.5, 3 and 5 s). It starts at
  a quarter of the rate, each gap shorter than the last by a constant ratio, pairs and the light *ta*
  arriving as it speeds up; it restarts when the ornament key is pressed again and carries on across new
  notes while the key is held. The ARP button's trill stays plain.
- **The Trill pages on GLO.** Hold GLO: Trill feel, pair gap, ta level, oroshi; hold again: first stroke,
  jitter. Saved with the setup. A tap on GLO is still mono / poly.
- **Kokyū 胡弓**, the ST-50's bowed voice, placed after the winds and before Taiko as on its slider: Tim
  Garrett's Viola.Ens (patches.fm, TX7-80B), the attack slowed so the note swells in (the first 60 ms about
  30 dB under the sustain) and a bow tremolo added; an octave corrected (the source relied on a transpose
  the engine does not read). Fourteen instruments.
- **FuMi's own instruments respond to velocity**, measured: a light stroke on Koto Pluck is 5 dB quieter and
  a third duller, a hard one a third brighter and 1 dB louder; Koto Ring, Koto Warm, Harp, Hyoshigi and
  Mokugyo likewise, each in its own measure. The community patches are untouched.
- **The loop's ornaments act on the loop's notes.** A trill, vibrato or damp recorded in a layer now works
  on that layer's note; before, a recorded ornament reached for whatever the hands held.
- **Fixed: a note could drone.** Two keys held in poly, GLO tapped to mono, the top key released: the key
  still held got a second voice and one of them never let go (since 1.0.0). Found in review.
- The saved setup is migrated from 1.0.4 / 1.0.5: instruments from Taiko on move up one slot so the setup
  keeps its instrument; Trill feel starts at its new default.
- The site's About page and the browser instrument's help now carry the instruments, what comes from the
  ST-50, the ornament keys, the trill and the looper.

## 1.0.5 (10 Oct 2026) — `FM-1_7010005`

- **Each loop layer keeps its instrument.** Loop a koto phrase, turn PRESETS to the shakuhachi and play
  over it: the koto stays a koto, the overdub is a shakuhachi layer, and so on for up to eight instruments
  in one loop (a ninth follows PRESETS). A layer's instrument is the one playing at its first note. In
  1.0.4 the whole loop changed with PRESETS. The eight voices are shared between the loop and the hands,
  as before.
- **Louder, by 9 dB.** The output trim is now 21 dB below 0.1 (`OUT_TRIM` 0.0891), half way between
  1.0.2's 12 dB and 1.0.3's 30 dB. MASTER and Level still work above it.

## 1.0.4 (10 Oct 2026) — `FM-1_7010004`

- **A looper on REC and PLAY.** Tap REC to arm; the first key you play starts the loop; tap REC again to
  close it where you are (or it closes itself at the ruler's length, 32 beats at 60 BPM by default) and
  it plays; REC while it plays overdubs a new layer, REC again ends the layer. PLAY stops and starts from
  the top; hold PLAY a second to undo the top layer; hold REC a second to clear. Keys, the ornament keys
  and the bend buttons are recorded with their timing; knobs and MASTER are not, so the left hand stays
  free. The loop stores keys, not pitches: change 本数 or 調律 and the loop follows. Notes ringing across
  the join keep ringing. ENV is the Loop page: BPM (30–120), length in beats (8–64) and a quiet click
  (off by default). REC lights red while recording and blinks while armed; PLAY lights while the loop
  plays; a thin bar under the keys shows where the loop is. One instrument at a time; the loop lives in
  memory until the FM-1 is switched off. Nothing autosaves while a loop runs.
- **Two wood instruments, after Taiko:** Hyoshigi 拍子木, two hardwood sticks struck together, a dry crack
  the same on every key; and Mokugyo 木魚, the wooden fish, a round pitched tok that follows the keys.
  Both FuMi's own. The trill button rolls them.
- **Shakuhachi replaced.** The owner's pick, PAN FL T A (AAAHGOOD.SYX, patches.fm), for the AirFltMal1
  whose every note opened with a bright blast.
- **Fixed: the wrong name on the screen.** Turning PRESETS to an instrument whose name starts like the
  last one's (Koto → Koto II, Sho → Shakuhachi) could leave the old name beside the new kanji.
- **Fixed: the host simulator did not build on Linux** (`clock_gettime` under `-std=c99`).
- The saved setup has three new values, so the format changed: the first start of 1.0.4 begins from the
  defaults (余韻 80, vibrato depth 80, reverb 55, Koto, 1本, SUIKO).

## 1.0.3 (10 Oct 2026) — `FM-1_7010003`

- **Quieter again, by 18 dB.** The output is now 30 dB below 0.1 (`OUT_TRIM` 0.03125). Suiko is a gentle
  instrument; MASTER and Level still work above this, so there is room to go up.
- **The instrument fills the screen.** Its name in the largest face that fits, and its kanji at 48 pixels,
  across the middle of the HOME screen; the key lights are a short strip beneath; the tuning and scale sit
  top right. New screens on the site.

## 1.0.2 (10 Oct 2026) — `FM-1_7010002`

The first version played on hardware, with the two things the first flash showed:

- **Much quieter.** 0.1 was far too loud on the FM-1's own speaker even at the lowest MASTER setting. The
  output is now 12 dB down across every instrument (one constant, `OUT_TRIM` in `firmware/src/dsp/fumi.c`);
  the MASTER pot, the Level knob and the instruments' own levels are unchanged, so turn MASTER up.
- **PRESETS no longer skips an instrument.** A single click sometimes arrived as two counts and jumped
  from Koto to Sho, or from Koto II to Dragon Flt. SELECT, PRESETS and ALGORITHM now take one step at a
  time, at most one every 60 ms; a deliberate turn is never slower than that, and a double count inside
  the window is dropped.

Everything else is 0.1.

## 0.1 (10 Oct 2026) — `FM-1_7000100`

The first release: the pitch core (本数, the ST-50's measured 純正律, pure fifths, a user table, depth and
微調), the FM6 engine with eleven instruments and their Japanese names on the screen, bends on the OCT
buttons, ornaments on the black keys, vibrato, trill, 余韻, mono with slide, low and high cut, character,
reverb, MIDI in and out, the host simulator, the browser emulator, the audition bench and the web
installer. Not played on hardware before release.
