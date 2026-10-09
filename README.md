# FuMi-1 for the M-VAVE FM-1

FuMi-1 (文) turns the M-VAVE FM-1 into a shigin conductor inspired by the Suiko ST-50, the Japanese
poetry-accompaniment instrument. The sixteen white keys are the ST-50's lower row: mi fa la ti do, three
times, and a top mi. The black keys are its upper row (fa♯ sol ti♭ do♯ re) or, at the flick of a button,
koto ornaments. The key is set in 本数 (水4 to 12本), the tuning switches between 平均律 and the ST-50's
純正律 as measured from recordings (fa −20.8 cents, do −16.0), and the voice is a koto on a 6-operator FM
engine, with 余韻 (ring time), vibrato, trill, a sprung pitch bend on the octave buttons, and a subtle
plate reverb.

It is built on [FoMni](https://github.com/charlesvestal/fm1-omnichord)'s platform (Felucca's HAL, update
loader, USB, storage, screen driver, installer and browser emulator), which is kept byte-for-byte: all of
FuMi lives in `firmware/src/app/` and `firmware/src/dsp/`.

- **Play it in your browser** (no FM-1 needed): <https://cartesive.github.io/fumi-1/emu/>
- **Install it** (Chrome or Edge, FM-1 on USB): <https://cartesive.github.io/fumi-1/install/>
- **The audition bench**: <https://cartesive.github.io/fumi-1/bench/>

**Status (10 Oct 2026):** version 1.0.3 (identity `FM-1_7010003`) runs on the owner's FM-1. It has the pitch
core, the FM engine with eleven instruments, the app, the host simulator, the browser emulator, the audition
bench and the web installer; the first flash (0.1) showed only that it was too loud and that PRESETS could
skip a step, both fixed. Releases are listed in `docs/CHANGELOG.md`; the research and plan are in
`docs/research/` (`00-START-HERE.md` is the current truth).

## Playing

| Control | Does |
|---|---|
| White keys 1–16 | mi fa la ti do × 3 and a top mi. The tonics (1, 6, 11, 16) are lit. 三 is key 6 |
| Black keys | The upper row fa♯ sol ti♭ do♯ re, five per octave. **SEQ** switches them to ornaments: vibrato, trill, damp, strong pluck (keys 1–4 and 6–9) |
| SELECT | 本数: 水4, 水3, 水2, 水1, 1 … 12. At 1本, 三 = A3 and key 1 = A2 |
| PRESETS | 音色, the instrument |
| ALGORITHM | The scale: IN 陰 (mi fa la ti do), YŌ 陽 (fa and do raised), MIN'YŌ 民謡 (raised a whole tone) |
| KNOB 1–4 | The four values of the page. HOME: 余韻, trill rate, vibrato depth, reverb. FX: low cut, high cut, character, reverb size. EDIT: bend target, bend down, bend time, slide time. Tuning page: tuning, depth, 微調, A |
| LFO | ビブラート on / off |
| ARP | トリラー on / off: the held note re-plucked at the rate, each repeat a little different |
| GLO | 単音 / 和音 (mono / poly) |
| SEL | Tap: 平均律 ↔ 純正律. Hold: the tuning page (EQUAL, SUIKO, KOTO pure fifths, USER; depth; 微調 ±50 cents; A = 430–445) |
| OCT− / OCT+ | With a note held: the 音程 rocker, a sprung bend down (−40 cents) / up (a semitone, a whole tone or the next scale note). With no note held: octave down / up |
| OCT− + OCT+ held 5 s | Update mode (the platform's; never reassigned) |
| MASTER | Volume and 強弱: play fade-ins on it by hand, as on the ST-50 |
| SAVE | Saves the setup (it also autosaves a few seconds after a change, when quiet) |

MIDI in (USB and the TRS jack) plays the voice chromatically; key presses go out as notes on channel 1.

## Building and testing

See [BUILDING.md](BUILDING.md) and `docs/dev-environment.md`.

```
tests/run_tests.sh      # the guard, the pitch core, the FM6 core, the instrument, the click test, the
                        # simulator scenarios, the browser build, the tools' self-tests
./build.sh              # build/fumi.fwsc (Docker, JieLi toolchain)
web/emu/build.sh        # the browser emulator and the audition bench (Emscripten)
web/bench/serve.py      # then open http://localhost:8765/build/bench/
```

`build/host/fumi_host SCRIPT OUTDIR` runs the whole app on a computer from a script
(`tests/scenarios/*.fumi`), with audio and screenshots coming out.

## The instruments

PRESETS steps through the built-in instruments; the screen shows each one's name with its Japanese name
beneath, Suiko-style. The first seven are the owner's picks from community DX7 banks (patches.fm; each
entry in `tools/fumi_patches.py` names its bank and author); the last four are FuMi's own.

| # | Name | 名 | Where from |
|---|---|---|---|
| 1 | Koto | 琴 | N-KOTO-A5, Aminet 073.syx. The boot instrument |
| 2 | Koto II | 箏 | KOTO-KOTO, Aminet 072.syx |
| 3 | Sho | 笙 | Air---*--3, FLUTE01.SYX |
| 4 | Shakuhachi | 尺八 | AirFltMal1, FLUTE01.SYX |
| 5 | Dragon Flt | 龍笛 | ALTO FLUTE, FLUTE01.SYX |
| 6 | Bamboo Flt | 篠笛 | Bamboo Flt, FLUTE01.SYX |
| 7 | Taiko | 太鼓 | Timpani.2 by Tim Garrett, TX7-26B.SYX |
| 8 | Koto Pluck | 爪音 | FuMi's own, found by `tools/koto_search.c` against the ST-50 measurements |
| 9 | Koto Ring | 響 | FuMi's own, brighter |
| 10 | Koto Warm | 名残 | FuMi's own, rounder |
| 11 | Harp | 箜篌 | FuMi's own |

No Yamaha factory ROM voice is among them (a ROM2B pick was declined). The kanji come from Noto Sans CJK
(SIL OFL 1.1) through `tools/gen_kanji.py`, which keeps only the seventeen characters these names use.

## The audition bench

Voicing happens in the browser, on FuMi's real engine (`web/bench/`, see `docs/research/16-audition-bench.md`):
patches from any DX7 `.syx` and FuMi's own, instant switching, A/B and blind A/B, a morph between two
patches, every patch byte on a slider, the voicing stages (low cut, high cut, character, reverb), 本数 and
調律, FuMi's key layout on the computer keyboard, Web MIDI, stock phrases, reference clips from `refs/`
level-matched, notes and ratings per patch, a session file saved into `bench/sessions/`, and a `.syx` bank
of the finalists to load on the FM-1's stock firmware for the speaker check.

It is also online at **https://cartesive.github.io/fumi-1/bench/**: there the `.syx` banks and the recordings
come from your own disk (Load .syx…, Load audio…) and stay in the browser; an exported session is a download
to put into `bench/sessions/`.

## Tools

- `tools/analyze_ref.py`: reference-audio analysis (pitch per note, spectral peaks, attack, click, decay, partials).
- `tools/syx_dump.py`: dump and diff DX7 voices.
- `tools/fumi_patches.py`: FuMi's own patches, as readable operator settings, into `fumi_patches.h`.
- `tools/koto_search.c`: a search over the engine's settings against the ST-50 koto measurements (how KOTO A and B were found).
- `tools/gen_fm6_tables.py`: the FM6 core's tables.
- `tools/guard_platform.sh`: fails if any protected platform file differs from upstream FoMni.

## Credits

The platform (hardware layer, USB, update loader, storage, installer, host simulator, browser emulator) is
[Felucca](https://github.com/hugelton/Felucca)'s by Leo Kuroshita (Hügelton Instruments), by way of
[X0X](https://github.com/charlesvestal/fm1-x0x) and [FoMni](https://github.com/charlesvestal/fm1-omnichord)
by Charles Vestal. The 6-operator FM core is Felucca's C port of msfa (Dexed, Google and Pascal Gauthier,
Apache-2.0). The plate reverb (Dattorro) is FoMni's. GPL-3.0-only; see [LICENSING.md](LICENSING.md).

Suiko and ST-50 are names of 水光社 (Suikohsha). FuMi-1 is inspired by the ST-50 and is not affiliated with
or endorsed by Suikohsha, M-VAVE or Hügelton Instruments. Yamaha DX7 patch data is read by the bench for
listening only and is never part of FuMi.
