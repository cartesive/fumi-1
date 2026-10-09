# 05 — Build plan and Claude Code kickoff

Plan as of 9 Oct 2026. Milestones are ordered so that nothing of yours touches the hardware until it has run in the simulator, and so that the first thing flashed is somebody else's known-good code built by your toolchain.

## Ground rules

- **Fork FoMni** (`charlesvestal/fm1-omnichord`), not SLOOP. Reasons in `02-FM1-platform-and-firmware-scene.md`. Use SLOOP and Felucca as references and as a parts bin (reverb, string model, formant voice, editor protocol).
- **Platform code is read-only**: `firmware/hal/`, `firmware/loader/`, `firmware/src/{ota,usb,storage,lcd,gfx,libc,midi_uart}.c`, `crt0.S`, `tools/fm1pkg_make.py`, `tools/fm1_install.py`, `web/fm1ota.js`, `web/fm1pkg.js`. All ST50 work goes in `firmware/src/app/` and `firmware/src/dsp/`.
- **Host first.** Every feature lands with a host test or a simulator scenario before it is flashed.
- **You flash, by hand.** Flashing needs Web MIDI in Chrome/Edge or a local Python script on the computer the FM-1 is plugged into. A Claude Code session should build and test; it should never be the thing that sends an install.
- GPL-3.0-only. Keep upstream headers. Do not commit `FM-1.fwsc`, the JieLi toolchain or SDK files.

## Environment notes for the Claude Code session

- The JieLi toolchain is **Linux x86-64 only**. Native on Linux; Docker (linux/amd64) on macOS; WSL on Windows.
- `tools/get_toolchain.sh` downloads from `pkgman.jieliapp.com`; the SDK is cloned from `gitee.com`. A cloud sandbox with a restricted network may block both. If so, run those two downloads on your own machine, or do the firmware build locally and use the cloud session for host-side work only. **The host simulator and all host tests need only a normal C compiler**, so most development does not need the JieLi toolchain at all.
- Archive the toolchain tarball and the SDK checkout privately once you have them. If either download disappears you cannot build.

## Milestones

### M0 — Preparation (no code)

- [ ] Confirm what the unit reports: `python3 tools/fm1_install.py --info`.
- [ ] Download official V15 `FM-1.fwsc` from m-vave.com/download; verify SHA-256 `db1642b2…7edb8a` (full hash in `03-flashing-safety.md`); store it somewhere safe.
- [ ] Order a Seeed XIAO RP2040 for FM-1 Transporter, or decide to accept the risk.
- [ ] Get reference audio: the Aaron Horn ST-50 pack (£9.99), and watch Hainbach's and Zecchou's videos with a notebook. Note: panel layout and label order of the front row, what the tuning switch does to which notes, vibrato speed, trill behaviour, what the chord button plays, how the bend lever behaves.
- [ ] Answer the open questions in `README.md`.

### M1 — Toolchain and an unmodified build

- [ ] Fork and clone FoMni. Record the upstream commit hash.
- [ ] `host/build_host.sh` and `tests/run_tests.sh`: all green on the unmodified tree.
- [ ] Install the JieLi toolchain and SDK files; `./build.sh` produces `build/omni.fwsc`.
- [ ] Build the web emulator (`web/emu/build.sh`) and run it.
- [ ] **Flash your own build of unmodified FoMni.** Confirm it plays. Return to V15. Flash it again. The build chain and the round trip are now proven on your unit.

Exit: you can build, simulate, flash and un-flash, with none of your own code involved yet.

### M2 — Pitch core on the host

- [ ] `dsp/tuning.c`: the tables and formula from `tuning-reference.md`. Unit tests for every key × 本 × tuning.
- [ ] `dsp/smooth.c`: the control smoother (one-pole + slew, sub-block interpolation).
- [ ] A minimal sine voice through both. Scenario script: play the scale at 1本, 4本, 9本; bend in steps; switch tuning while holding.
- [ ] **Click regression test** (see `04-firmware-design.md`). It should fail if the smoother is bypassed; prove that once.
- [ ] Pitch-track the ST-50 reference recordings; write measured offsets into a `WA` table; note what was measured and from which file.

Exit: correct, click-free pitch in the simulator, with tests guarding it.

### M3 — Koto voice and the home screen

- [ ] Koto prototype A (plucked-string model) and B (FM pluck). Render both, compare to reference by ear, keep one.
- [ ] SUSTAIN on/off behaviour.
- [ ] Reverb (port Felucca's HALL; check its RAM against the 336 KB pool).
- [ ] Key map: white keys = front row; tonic landmarks lit; keys light as they sound.
- [ ] Home screen: 本, note name, voice, tuning.
- [ ] Encoders: SELECT = 本, ALGORITHM = voice, KNOB 1 = bend with spring return, MASTER = volume.
- [ ] CPU budget test on host (FoMni's build already reports image, RAM and pool sizes).

Exit: a playable one-voice instrument in the browser emulator.

### M4 — First flash of ST50

- [ ] Go through the checklist in `03-flashing-safety.md`.
- [ ] Flash. Check `plat_cpu_pct()` and `plat_xruns()` on the real hardware with 8 voices and reverb.
- [ ] Listen through the speaker, headphones and line out. Listen specifically for bend zipper, key-off clicks, voice-steal clicks.
- [ ] Confirm update mode (OCT− + OCT+ 5 s) and a return to V15 both work *from your firmware*.

Exit: ST50 runs on the device and you have left it and come back.

### M5 — The rest of the instrument

- [ ] Shakuhachi (scoop, breath, delayed vibrato, mono legato glide).
- [ ] Oboe, strings, chorus; ensemble effect.
- [ ] VIBRATO, TRILL, MONO/POLY, KNOB 2 level lever.
- [ ] Black keys: upper row (A), then ornament keys (B), then the switch (C).
- [ ] YO scale; USER tuning page.
- [ ] Save/load setup through platform storage, with validation and safe defaults.
- [ ] MIDI in and out.

### M6 — Extras

- [ ] CHORD.
- [ ] DRUM mode.
- [ ] Recorder: record / play / overdub.
- [ ] Optional built-in preludes.

### M7 — Web

- [ ] Rebranded web installer with "return to official V15", published with `tools/publish_pages.sh` to GitHub Pages.
- [ ] Browser emulator on the same site.
- [ ] Web editor: port the Felucca/SLOOP SysEx editor protocol with your own sub-ID; pages for tuning tables, voice parameters, key layout, backup/restore.

### M8 — Release

- [ ] Choose and reserve an identity number; check it against awesome-fm-1 and the Firmware Hub.
- [ ] Manual (controls page with a panel diagram; Felucca has `tools/gen_panel_diagram.py`).
- [ ] Power-loss and disconnect tests at each install stage, on a unit you can recover.
- [ ] LICENSING.md, source published, "inspired by" wording, no M-VAVE or Suiko files in the tree.
- [ ] Optional: submit to awesome-fm-1 and the FM-1 Firmware Hub.

## Risks

| Risk | Likelihood | Mitigation |
|---|---|---|
| Brick during development | Low if the rules are kept | Platform code untouched; safe mode; update-path tests; XIAO dongle on the desk |
| Koto voice does not convince | Medium | Two prototypes; reference recordings; reverb does a lot of the work |
| "Japanese tuning" stays a guess | Medium | Measure recordings; USER tuning as the escape hatch |
| CPU: string models + hall reverb + 8 voices | Medium | Budget tests from M3; FoMni runs on one core, a second core exists but nobody in the Felucca family uses it yet |
| Flash: 581 KB | Low for a synthesised instrument | No samples; fonts are the biggest asset (a kanji font subset must be small) |
| Toolchain or SDK download disappears | Low–medium | Archive both privately in M1 |
| Upstream moves fast | Certain | Pin the commit; platform code untouched makes rebasing cheap |

## Kickoff prompt for the Claude Code session

Paste this, with the Dropbox ST50 folder available to the session:

> I'm building custom firmware for the M-VAVE FM-1 that recreates the essence of the Suiko ST-50 Japanese poetry synth. Read all the files in my Dropbox `projects_2026/ST50` folder first: `README.md`, then 01 to 05 and `tuning-reference.md`. They are the research and the plan; treat `03-flashing-safety.md` as hard rules.
>
> We are at milestone M1 in `05-build-plan.md`. Do this:
> 1. Clone `https://github.com/charlesvestal/fm1-omnichord` as the base, plus `https://github.com/hugelton/Felucca` and `https://github.com/isod89/sloop-fm1` as read-only references. Record the commit hashes.
> 2. Read FoMni's `BUILDING.md`, `firmware/src/app/plat.h`, `firmware/src/app/main_fm1.c`, `firmware/src/dsp/omni.h` and `host/omni_host.c`, and tell me in a short summary how the app, the DSP and the platform fit together and where the ST50 code will go.
> 3. Build the host simulator and run `tests/run_tests.sh` on the unmodified tree. Report what passes. Do not change anything to make a test pass.
> 4. Try the firmware build. If the JieLi toolchain or SDK cannot be downloaded from this environment, stop and tell me exactly which commands to run on my own machine.
> 5. Then start M2: `dsp/tuning.c` and its tests from `tuning-reference.md`, and the control smoother with the click regression test.
>
> Rules: never modify anything under `firmware/hal/`, `firmware/loader/`, or `ota.c`, `usb.c`, `storage.c`, or the packaging and install tools. Add a script that diffs those paths against the upstream commit and fails if they differ. Never attempt to install firmware on a device; I do all flashing by hand. No `double` anywhere in firmware code. Ask me before making any of the decisions marked DECIDE in `04-firmware-design.md`.
