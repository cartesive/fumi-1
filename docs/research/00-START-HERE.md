# FuMi-1 — start here

Consolidated state of the project as of 9 Oct 2026, 19:35. **This file is the current truth.** Files 01–16 are the research trail; several early ones are partly superseded (list at the end). If this file and an older one disagree, this file wins.

## What we are building

**FuMi-1** (文): custom firmware for the M-VAVE FM-1, inspired by the Suiko ST-50 (水光トレーナー), a Japanese poetry-accompaniment instrument. GPL-3.0, public release intended. Developed on macOS (M4 mini).

**The success test is feel, not function**: a beautiful koto and the ST-50's character. Be faithful where the ST-50 is beautiful (layout, tuning, koto) and better where it is weak (no round-robin, thin flutes).

**First release scope: just the instrument.** Koto first, then strings and the "Suiko" voice. Shakuhachi later, as FuMi's own voice. Drum mode, auto-chord, recorder, IC-card features: parked.

## Hard rules (from file 03)

1. Fork FoMni (`charlesvestal/fm1-omnichord`). Never modify `firmware/hal/`, `firmware/loader/`, `firmware/src/{ota,usb,storage,lcd,gfx,libc,midi_uart}.c`, `crt0.S`, or the package/install tools. All FuMi code goes in `firmware/src/app/` and `firmware/src/dsp/`. A script diffs those paths against the upstream commit and fails the build if they differ.
2. Keep every escape hatch: watchdog first, boot-loop guard, safe mode, **OCT− + OCT+ held 5 s = update mode (never reassigned)**, USB upgrade handler serviced on every page.
3. Host simulator before hardware. Nothing is flashed that has not run a scripted scenario on the Mac.
4. The owner does all flashing by hand. Claude Code never installs firmware on a device.
5. Unit must report stock V15 (`FM-1_015`) before any first custom install; keep the official `FM-1.fwsc`. First flash is an **unmodified** FoMni built locally, with a round trip to V15, before any FuMi code.
6. No `double` in firmware code. Nothing read from flash is trusted without length and CRC checks.
7. No recovery dongle: risk accepted on one unit, so rules 1–5 are the only safety net. The Mac-hosted `tools/fm1_rescue.py` is the fallback if the unit reaches the chip's own USB mode.

## The instrument, as established

### Keys (from the panel photo, file 10)

- **White keys 1–15 = the ST-50 lower row:** mi fa la ti do, three times, starting on mi. Key 16 = a bonus top mi. Tonics on keys 1, 6, 11 (and 16). Printed labels under keys: 水 (1), 乙 (3), 一 (4), 二 (5), 三 (6), 三′ (7), 五 (8), 六 (9), 七 (10), 八 (11). 三 is the reference tonic.
- **Black keys, switchable (SEQ):**
  - Upper-row mode: fa♯, sol, ti♭, do♯, re (200, 300, 600, 900, 1000 cents above mi), five per piano octave.
  - Ornament mode: momentary techniques on the sounding note (vibrato while held, tremolo re-pluck, strong pluck, damp, one-key glissandi).

### Tuning (measured from two recordings, files 13 and 14)

Offsets from equal temperament, by scale degree, transposing with the key:

| | mi | fa | la | ti | do | ti♭ | fa♯, sol, do♯, re |
|---|---|---|---|---|---|---|---|
| 平均律 (equal) | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| **純正律 = SUIKO (default)** | 0 | **−20.8** | 0 | 0 | **−16.0** | −25 (weak) | 0 (unmeasured) |
| KOTO (pure fifths) | 0 | −9.8 | −2.0 | +2.0 | −7.8 | — | per file 07 |
| USER | per-degree trim | | | | | | |

Two recordings in two keys by two methods agree within one cent. A depth control scales the SUIKO table (0 % = equal, 100 % = measured).

**Key (本数):** sixteen consecutive semitones. 三 (white key 6) = MIDI 53 + position, position 0–15 = 水4, 水3, 水2, 水1, 1, 2, … 12. So 1本 puts 三 on A3 (220 Hz) and white key 1 on A2. Good confidence, one assumption (file 14).

**Fine tune (微調):** ±50 cents. Reference A = 440, adjustable.

**Scales (PRESETS):** IN 陰 (0, 100, 500, 700, 800) default; YŌ 陽 (0, 200, 500, 700, 900); MIN'YŌ 民謡 (0, 300, 500, 700, 1000).

All pitch maths in cents as floats: see `tuning-reference.md` for the formula (its 本 table and "YO" scale are superseded by this file).

### Controls

| FM-1 | Function | ST-50 original |
|---|---|---|
| SELECT | 本数 (key), 水4 … 12 | 本数 slider |
| ALGORITHM | 音色 (voice), in panel order | voice slider |
| PRESETS | Scale IN / YŌ / MIN'YŌ | — |
| KNOB 1 | **余韻** ring/release time | 余韻 slider |
| KNOB 2 | Repeat speed (trill rate). Inference: Hainbach's description suggests the ビブラート速さ slider governs this; whether it also sets vibrato rate is unknown | ビブラート速さ |
| KNOB 3 | Vibrato / chorus depth | ビブラート深さ |
| KNOB 4 | Reverb (FuMi addition; default low) | — |
| **MASTER** | Volume and 強弱. **Left stock.** Fade-ins are played on it by hand | 強弱 rocker (stiff, stays put), 音量 |
| **OCT− / OCT+ with a note held** | **Pitch bend down / up, spring return** | 音程 rocker (sprung) |
| OCT− / OCT+ with no note held | Octave down / up | オクターブ (down only on the original) |
| LFO | ビブラート on/off (a chorus-vibrato) | ビブラート |
| ARP | トリラー on/off: **re-triggers the held note repeatedly** | トリラー |
| GLO | 単音/和音 (mono/poly) | 単音/和音 |
| SEL | 調律: 平均律 / 純正律; hold for depth, 微調 | 調律 switch, 微調 |
| SEQ | Black-key mode: upper row / ornaments | — |
| FX, EDIT, HOME, SAVE | Pages | — |
| ENV, PLAY, REC | Free (reserved for parked features) | — |

**Bend:** role decided at the moment of pressing. With a note held, OCT+ glides the most recent note up to the target (semitone, whole tone or next scale note; about 90 ms up, 120 ms back, S-shaped) and holds until release; a tap is an up-and-back flick; OCT− is a small drop. Both held together stays reserved for update mode. **Key slide** is a per-voice option (on for shakuhachi, off for koto). A control smoother still sits under everything, and a click regression test guards it (file 04).

**Trill:** note repetition at the set speed, not alternation of two pitches. Each repeat varied slightly so it does not machine-gun; variation can be set to zero for the original's behaviour.

### Voices

The ST-50 is, by an owner's account, **sample playback** with the same sample on every hit. Its fifteen voices in panel order: 琴 koto, 十七絃 17-string, ?琴, スイコー Suiko, チャイム chime, 鉄琴 glockenspiel, 尺八, 篠笛, 能管, 笙, 胡弓, オーボエ, ストリングス, ホルン, コーラス; plus 打楽器 percussion.

**Koto, what the ears and the measurements say:**

- The DX7 "KOTO" patch is close but too thin. **"HARP 2" is closer**, with a clear click at the start. A hybrid is the target.
- Measured on a clean plucked section: peak within about 5 ms; a click (7–9 dB of extra energy above 5 kHz in the first 12 ms); two-stage decay (about −28 dB/s for 200 ms, then −15 to −18); partials 1–4 strong; everything from the 5th up collapses to −20 dB within 300 ms; pitch steady from the first instant.
- So: **6-operator FM is the starting engine** (Felucca's FM6, the Dexed/msfa code, Apache-2.0), with the string model as second candidate. Three stacks: click; body and decay; fullness (detuned double or octave below).
- Hunches to test in audition: a high cut taming the twang (the fast darkening above), subtle reverb, and the tuning itself giving resonance.

**Feel layer (FuMi's improvement on the original):** per-pluck variation, gesture-derived dynamics (fast adjacent keys = lighter glissando plucks), same-key re-pluck re-excites the same voice, release governed by 余韻. Amount adjustable down to zero.

**Character stage** between voice and reverb: band-limit, warmth, optional period grain; one amount control.

## How voices get made: the audition bench (file 16)

Voicing is where the owner's time goes: loading patches, playing, comparing, writing notes. So the main tool is a **playable browser instrument running FuMi's real sound engine**, borrowed from FoMni's browser emulator (`web/emu/`): the same C code that goes on the device, compiled to WebAssembly. What is approved in the bench is what the firmware plays.

The bench provides:

- **Play:** FuMi's key layout on the computer keyboard, on-screen keys, and Web MIDI in (any MIDI keyboard; the FM-1 itself when it runs a firmware that sends its keys over USB). 本数, 調律, 余韻, vibrato, trill and bend all live.
- **Compare:** instant patch switching mid-phrase; A/B and blind A/B; ST-50 reference clips on a button, level-matched; stock phrases the bench plays itself so every patch is heard on identical material.
- **Tweak:** live sliders for every patch parameter and for low cut, high cut, character and reverb; duplicate-and-vary; a **morph slider between two patches** (the quick way to find a KOTO–HARP 2 hybrid).
- **Notes:** a notes box and rating per patch with fixed prompts (attack, body, tail, gliss, registers), autosaved, exported as one file into the repo for the next round.
- **Import/export:** DX7 `.syx` in; finalists out as a DX7 `.syx` bank.

**The speaker check, every round:** load the exported bank on the FM-1's stock firmware and listen on its own speaker. Monitors and laptop speakers are not the FM-1's speaker. That check is equal temperament only and has none of FuMi's filters or feel; it judges raw timbre on the speaker.

The loop: dump and diff KOTO and HARP 2 → first set of hybrids in the bench → owner plays, ranks, notes → export → next round varies around the winners → two or three rounds → the winner is written as FuMi's own patch (never Yamaha ROM data) and locked with a golden-file test. Repeat for every voice. The bench later becomes the public browser emulator with the voicing controls hidden.

## Milestones

| | Milestone | Exit |
|---|---|---|
| M0 | Prep: confirm identity, keep V15 file, gather reference audio | Checklist in file 05 done |
| M1 | Fork FoMni; host tests green; firmware builds in Docker; **flash unmodified FoMni and round-trip to V15** | Build chain and flashing proven with no FuMi code |
| M2 | Pitch core on host: tuning tables, 本数 mapping, smoother, bend-button and slide logic, click regression test | Every key × key-setting × tuning within 0.1 cent in tests |
| M2.5 | **Audition bench**: patch dump/diff, FM6 engine building on host with tests, the browser bench, reference-audio analysis tools | The owner is playing KOTO, HARP 2 and first hybrids in the browser |
| M3 | **The koto**: winning patch, feel layer, character stage, 余韻, reverb, key map, home screen | Owner enjoys playing it in the bench |
| M4 | First flash of FuMi; CPU and xrun check; listen on speaker, headphones, line | Left the firmware and came back |
| M5 | Strings, Suiko voice; vibrato/chorus; trill; mono/poly; black-key modes; scales; USER tuning; save/load; MIDI | First release candidate |
| M7 | Web installer, public browser emulator, then a web editor | Public page |
| M8 | Release: identity number, manual, licensing, source | Published |

(M6, extras, is parked.)

## Reference material the session needs

- The two MP3s the owner already has (the ST-50 preset demo, mi = A; and Hainbach's video, mi = F with the clean opening run). Place them in the repo under `refs/` and keep them out of git.
- The panel photo and the voice-strip crop.
- The DX7 `.syx` bank containing KOTO and HARP 2.
- Wanted: preset start times for the first MP3; a higher-quality copy of either video; a clip with the tuning switch on 平均律; upper-row notes held; any clean solo koto recording.

## Open questions

1. Tuning of fa♯, sol, do♯, re in 純正律 (unmeasured).
2. Whether the 本数 mapping's one assumption holds (file 14).
3. Voice 3's name; which section of the first MP3 is the koto.
4. Vibrato and repeat-speed ranges; pitch-rocker range.
5. Whether the tan keys (mi, la, ti in the main register) are coloured caps or lights; FuMi can light them either way.
6. Production date: 1980s per sellers, mid-1990s per Hainbach.
7. Whether the stock V15 firmware sends its keys as MIDI over USB (for playing the bench from the FM-1).

## Which older files are superseded where

| File | Still good for | Superseded parts |
|---|---|---|
| 01 | History, family, 本 system, sample packs | Voice list, upper row (la♭), "sustain switch", engine guess, "1980s" |
| 02 | FM-1 hardware, flash layout, firmware scene, why FoMni | — |
| 03 | **All of it. Hard rules.** | — |
| 04 | Click causes and fixes, smoother, regression test, MIDI, web | Control map, tuning formula line, YO scale, knob-as-bender, sustain button, voice approach |
| 05 | M0/M1 checklists, environment notes, risks | Milestone list and the kickoff prompt (use the one below) |
| 06 | Feel layer, character stage, bend buttons, slide | "String model first" (now FM first), trill as alternation, level-lever options, the render-to-WAV bench (now the browser bench) |
| 07 | Why semitones are narrow, KOTO preset, chord lead, label notation | "Start on ti" layout, WA defaults (now SUIKO measured) |
| 08 | MASTER decision, existing koto candidates | — |
| 09 | Why audition, hybrid stacks, the speaker rig | "Rig 2" page of clips (now the bench, file 16) |
| 10 | Panel reading, control inventory | 純正律 ambiguity (now measured) |
| 11 | Voice list | — |
| 12 | Notes on drums for later | Scope proposal (withdrawn); its remark that FoMni has a limiter (it does not appear to) |
| 13, 14 | Tuning measurements, 本数 mapping, timbre figures | 13's table superseded by 14 by under a cent |
| 15, 15b | What Hainbach says | — |
| 16 | The audition bench in full | — |
| tuning-reference | Formula | 本 table, YO scale, WA table |

## Kickoff prompt for Claude Code

> I'm building FuMi-1, custom firmware for the M-VAVE FM-1 inspired by the Suiko ST-50 Japanese poetry instrument. The research and plan are in my Dropbox folder `projects_2026/ST50`. Read `00-START-HERE.md` first and treat it as the current truth; then read `03-flashing-safety.md` in full (hard rules), and `02`, `13`, `14` and `16`. Use the other files as background only where START-HERE points to them.
>
> The goal is feel, not function: a beautiful koto with the ST-50's character and its measured tuning. Scope for the first release is just the instrument.
>
> We are at milestone M1. Do this, in order, and stop for me at each "STOP":
> 1. Clone `https://github.com/charlesvestal/fm1-omnichord` as the base. Clone `https://github.com/hugelton/Felucca` and `https://github.com/isod89/sloop-fm1` as read-only references. Record the commit hashes in the repo.
> 2. Read FoMni's `BUILDING.md`, `firmware/src/app/plat.h`, `firmware/src/app/main_fm1.c`, `firmware/src/dsp/omni.h`, `host/omni_host.c` and `web/emu/`. Give me a short summary of how app, DSP, platform and the browser emulator fit together and where FuMi's code will go.
> 3. Build the host simulator and run `tests/run_tests.sh` on the unmodified tree. Build and run the unmodified browser emulator. Report results. Do not change anything to make a test pass.
> 4. Set up the firmware build for macOS on Apple silicon (Docker, linux/amd64). If the JieLi toolchain or the SDK cannot be downloaded from where you are running, tell me the exact commands to run myself. Produce an unmodified `omni.fwsc`. STOP: I will flash it by hand and do a round trip to stock V15.
> 5. Add the platform-guard script that diffs the protected paths against the upstream commit and fails if they differ.
> 6. M2: `dsp/tuning.c` with the tables and 本数 mapping from START-HERE, the control smoother, the bend-button role logic, and tests for all of it including the click regression test.
> 7. M2.5: build the audition bench before any koto voicing, as described in `16-audition-bench.md`. It is FuMi's real DSP (with Felucca's FM6 engine) compiled to WebAssembly and playable in the browser, following FoMni's `web/emu/`. It needs: DX7 `.syx` import; instant patch switching and blind A/B; a morph slider between two patches; live sliders for patch parameters, low cut, high cut, character and reverb; the measured tuning and 本数 controls; Web MIDI and computer-keyboard input in FuMi's key layout; reference clips from `refs/` on a button, level-matched; per-patch notes and ratings that export to one file in the repo; and export of chosen patches as a DX7 `.syx` bank for the stock firmware. Also build the patch dump/diff tool and the reference-audio analysis tools (pitch per note, attack, click, decay, partials over time). I will put the reference MP3s, the panel photo and my `.syx` bank in `refs/`. Start me with KOTO, HARP 2 and a first set of hybrids loaded. STOP when it is ready for me to play.
>
> Rules: never modify anything under `firmware/hal/`, `firmware/loader/`, or `ota.c`, `usb.c`, `storage.c`, `lcd.c`, `gfx.c`, `libc.c`, `midi_uart.c`, `crt0.S`, or the packaging and install tools. Never attempt to install firmware on a device. No `double` in firmware code. Never reassign OCT− + OCT+ held together. Do not ship Yamaha ROM patch data or any ST-50 audio; references are for listening and measurement only. Keep `refs/` out of git. When something in the research is marked unmeasured or an assumption, keep it as a table that is easy to change, and ask me rather than guessing on anything that affects how it sounds or plays.
