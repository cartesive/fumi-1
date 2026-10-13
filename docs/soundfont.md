# FuMi-1 as a SoundFont

Every FuMi-1 instrument, rendered key by key through FuMi's own engine, in one SoundFont 2 file:
`FuMi-X.Y.Z.sf2`, attached to the GitHub release of the firmware version it was rendered from, next to the
firmware package. It is not in the repository (70 MB); the tools that make it are.

## Loading it

Any SoundFont player takes it: a DAW's own sampler where it reads `.sf2` (Logic's Sampler / EXS24, Ableton
Live's Simpler and Sampler, Reaper's ReaSamplOmatic via a SoundFont plugin, FL Studio's Fruity SoundFont
Player, Bitwig's sampler), a free plugin (sfizz, Sforzando with its SoundFont import, FluidSynth, Polyphone to
look inside), or a phone app. Pick a **program** (preset): the fourteen are in the FM-1's PRESETS order, bank
0, programs 0–13 (some players count from 1):

| Program | Instrument | 名 | Kind | Sample |
|---|---|---|---|---|
| 000 | Koto | 琴 | plucked | up to 6 s, cut where it has rung out |
| 001 | Koto II | 箏 | plucked | |
| 002 | Sho | 笙 | sustained | 4 s, then it fades |
| 003 | Shakuhachi | 尺八 | sustained | |
| 004 | Dragon Flt | 龍笛 | sustained | |
| 005 | Bamboo Flt | 篠笛 | sustained | |
| 006 | Kokyu | 胡弓 | sustained | slow onset, as on the FM-1 |
| 007 | Taiko | 太鼓 | percussion | plays out |
| 008 | Hyoshigi | 拍子木 | percussion | |
| 009 | Mokugyo | 木魚 | percussion | |
| 010 | Koto Pluck | 爪音 | plucked | |
| 011 | Koto Ring | 響 | plucked | |
| 012 | Koto Warm | 名残 | plucked | |
| 013 | Harp | 箜篌 | plucked | |

## The layout: white keys, as on the FM-1

The sixteen scale notes sit on the **white keys F3 to G5** (MIDI 53–79): mi fa la ti do three times and a
top mi, at **1本** (mi = A, so 三 = A3 on the sixth white key). The black keys are silent, and so are the
keys below F3 and above G5. There is no chromatic layout: FuMi-1 plays the in scale, and so does the file.

## The tuning is in the sound

Each key plays its own recording at the recorded speed (every zone is rooted on its own key, so no player
resamples), and the Suiko tuning, the ST-50's 純正律 as measured, comes with it: **fa −20.8 cents, do −16.0
cents**, la and ti at equal temperament. Transposing in the player or the DAW shifts the whole scale and keeps
the intervals, so a different 本数 is a transpose. A player's pitch bend and modulation wheel work as usual.

Some patches sit a few cents off the kotos by their own detune, exactly as on the FM-1: the shō about 5 to 14
cents flat and an octave above its key, the shakuhachi 4 to 10 cents sharp (measured on the 1.0.7 render).

## What is there, and what is not

The instruments, the tuning and the layout are FuMi-1's. The samples are dry: add your own reverb. A held
sustained note lasts four seconds and then fades; there are no loops, so shape shorter notes with the
sampler's own envelope. FuMi's sprung bend, the trill, oroshi, the koto's left hand, live 余韻 and the looper
are not in the file: they are the firmware's. Velocity works through the player's default response (softer
is quieter); FuMi's own velocity curves are not sampled.

## Each file is frozen

Once a SoundFont is in a song, a changed sample changes the song. A released file is never replaced: if an
instrument is revoiced in FuMi, the new render comes with the next firmware version under its own name.

## Licence and credits

The samples are released under **CC BY 4.0**: use them anywhere, credit FuMi-1 and the patch authors. Eight
instruments are voiced from community DX7 patches on patches.fm: Koto = N-KOTO-A5 (Aminet 073.syx), Koto II =
KOTO-KOTO (Aminet 072.syx), Sho = Air---*--3, Dragon Flt = ALTO FLUTE and Bamboo Flt (FLUTE01.SYX, author
unknown), Shakuhachi = PAN FL T A (AAAHGOOD.SYX, author unknown), Kokyu after Viola.Ens by Tim Garrett
(TX7-80B, the attack slowed), Taiko = Timpani.2 by Tim Garrett (TX7-26B). Hyoshigi, Mokugyo, Koto Pluck, Koto
Ring, Koto Warm and Harp are FuMi's own. The FM core that rendered them is msfa (Dexed, Apache-2.0). No Yamaha
factory ROM voice and no ST-50 audio is involved. The same text is in the file's comment field.

## Making it

```
sh host/build_host.sh                                        # the host simulator
python3 tools/sf_render.py                                   # build/sf/render/NN-Name/kNN.wav, every instrument, every white key
python3 tools/sf_bank.py build/sf/render build/FuMi-1.0.7.sf2 1.0.7
python3 tools/sf_check.py build/FuMi-1.0.7.sf2               # parses it back and checks layout, levels and pitch
```

`tools/sf_render.py` writes one simulator scenario per instrument (reverb 0, PRESETS stepped to the
instrument and checked, each white key held and released into its own WAV; the recording gain `wavgain 8`
because FuMi's output is −21 dB by design, halved again if anything clips). `tools/sf_bank.py` trims each
recording to just before the onset, keeps plucked notes up to 6 s and cuts them where they have fallen 60 dB,
sustained voices exactly 4 s with a 0.4 s fade, percussion up to 3 s, applies one gain per instrument (peak
−1 dBFS, the sustained voices −7 dBFS so a held flute does not swamp a koto), and writes the bank: mono, 16
bit, 44.1 kHz, release 1.3 s plucked, 0.4 s sustained, 2 s percussion. The whole run takes a few seconds.
The design and the decisions behind it are in `docs/research/` (the owner's SoundFont notes).
