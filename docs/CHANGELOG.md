# FuMi-1 releases

Identities are `FM-1_7` + major, minor and patch as two digits each; the FM-1 reports the identity
(`python3 tools/fm1_install.py --info`).

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
