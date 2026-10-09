# ST50 on the FM-1 — research and plan

Custom firmware for the M-VAVE FM-1 that captures the essence of the Suiko ST-50 Japanese poetry synth. Research and planning done 9 Oct 2026, ahead of the first Claude Code session.

## Files

| File | What it holds |
|---|---|
| `01-ST50-research.md` | The ST-50: purpose, panel, voices, key layout, the shigin scale and 本 key system, sample packs, what is still unknown |
| `02-FM1-platform-and-firmware-scene.md` | FM-1 hardware as the firmware sees it, flash layout, how installs work, the firmwares, why to fork FoMni |
| `03-flashing-safety.md` | The V14/V15 answer, design rules that prevent bricks, flashing checklist, recovery ladder |
| `04-firmware-design.md` | Control mapping, tuning, click-free bend, voices, web tools |
| `tuning-reference.md` | Pitch formula and tables for implementation. **Authoritative for the maths** |
| `05-build-plan.md` | Milestones M0–M8, risks, and a kickoff prompt to paste into Claude Code |

## Headline findings

1. **No ST-50 emulator exists.** There is one direct sample pack (Aaron Horn, £9.99) usable as a listening reference only. Nothing can legally be embedded, and the FM-1 has about 581 KB for code and data together, so the voices will be synthesised.
2. **The ST-50's layout fits the FM-1 neatly.** Its front row is the in scale (mi fa la ti do) over three octaves: 15 notes plus a top tonic is exactly the FM-1's 16 white keys.
3. **Tuning is half known.** The scale and the 本 system (1本 = A, one semitone per 本) are well sourced. The offsets of the ST-50's "Japanese tuning" switch are not published anywhere; the plan ships a provisional pure-fifths tuning and measures the real one from recordings.
4. **The FM-1's knobs are clicky encoders, not pots.** That is the root of the bend "clicking" worry. The fix is in the audio engine: the knob moves a target, a smoother chases it, oscillators stay phase-continuous, and a regression test listens for clicks. Koto-style "ornament keys" give a bend that needs no knob at all.
5. **V15 first: yes, treat it as mandatory.** Every guide says so and V15 changed the flash layout that the community tools assume. No one has documented a V14-specific brick, but no one has tested that path either. Check the device identity (`FM-1_015`), not the updater's label.
6. **Bricks are avoidable by construction.** Fork a firmware that already has the safety net (watchdog, boot-loop guard, safe mode, update-mode key combo), never modify its loader/update/HAL code, and keep a cheap RP2040 recovery dongle on the desk.
7. **Fork FoMni (the Omnichord clone), not SLOOP.** Same shape as an ST50, about 6,400 lines, with a host simulator, browser emulator and web installer. Take the web editor protocol and DSP parts from SLOOP/Felucca later.

## Questions for you

1. **Black keys:** the ST-50's upper row of off-scale notes (faithful), koto ornament keys (press-bends, trill, vibrato while held), or switchable between the two?
2. **First-release scope:** just the instrument (voices, tuning, ornaments, reverb), or also drum mode and the record/overdub function?
3. **Development machine:** macOS, Linux or Windows? It decides Docker vs native for the toolchain and which rescue tools apply (the no-hardware rescue script and FM-1 Transporter are Mac-hosted).
4. **Recovery:** will you build the XIAO RP2040 dongle, or accept the risk on one unit?
5. **Do you have, or will you buy, ST-50 reference audio?** The tuning and voice work depends on it.
6. **Do you intend to release it publicly?** That brings GPL-3.0 source publication and a name that is not "ST50".

## Caveats on this research

- The Hainbach and Zecchou videos and Reverb's article could not be fetched; they were covered through secondary write-ups. Watching them yourself is task one in M0 and will settle several inferences.
- Most ST-50 panel detail comes from one 2009 seller description and one Ask MetaFilter answer.
- The formula line in `04-firmware-design.md` under "Tuning" is garbled; use `tuning-reference.md`.
- The remark in `05-build-plan.md` that no Felucca-family firmware uses the second CPU core was checked by searching the Felucca, SLOOP and FoMni sources only.
- FM-1 firmware versions change weekly. Version numbers here are as of 9 Oct 2026.
- This Dropbox folder is `projects_2026/ST50` (your existing folder is named `projects_2026`, not `Projects-2026`).
