# FuMi-1 releases

Identities are `FM-1_7` + major, minor and patch as two digits each; the FM-1 reports the identity
(`python3 tools/fm1_install.py --info`).

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
