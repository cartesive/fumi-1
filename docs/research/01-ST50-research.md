# 01 — Suiko ST-50 research

Researched 9 Oct 2026. Every claim is tagged **[sourced]**, **[single source]** or **[inference]**. Two things could not be read directly: the Hainbach and Zecchou YouTube videos (fetch was blocked), and Reverb's "Find of the Week" article. No manual scan, teardown or chip information was found anywhere.

## What it is

- Maker: 水光社 (Suikohsha), Minami-Ikebukuro, Tokyo, founded 1982, still trading (current products: HT-100/HT-200 Handy Trainers, KT-1000 "Kizuna", CF-30 pitch pipe). [sourced: suikohsha.com]
- Japanese listings call it 水光トレーナー ST-50, 詩吟コンダクター ("shigin conductor") or 邦楽トレーナー. [sourced: Yahoo Auctions, PayPay Flea Market]
- Purpose: accompany shigin (詩吟, chanted poetry). A "conductor" plays the prelude (前奏) and gives the reciter their pitch; teachers play the scale at a pitch that suits the student. "Conductor" was originally one company's product name that became generic. [sourced: shigin-fan.net, natural-shigin.com]
- Date: sellers say "1980s"; one 2009 listing says "around 1990". No exact year found.
- No MIDI. About 390 × 210 × 40 mm, 1.5 kg. 6 batteries or 9 V DC centre-negative. Stereo speakers; stereo out, phones, pedal and DC jacks. [single source: seller listings]

## Family (for context)

| Model | Notes |
|---|---|
| ST-10 | Mono; shakuhachi, koto, pipe organ; cassette data sequencer; suitcase with speaker |
| ST-20 | One analog oscillator, same three tones, top-note priority, octave switch, register dial, sustain, scale switch, **quarter-tone fine-tune switch** |
| ST-30 | Simpler interface than the ST-50 |
| ST-40 | Handy trainer, two sounds |
| **ST-50** | The subject of this project |
| ST-60 "Yume" | Built-in songs |
| ST-70 | Smallest |
| ST-100 | Many presets (koto, 17-string, shakuhachi, shinobue, nohkan, sho, kokyu, taiko), memory card |
| VS-1 | Eight voices |

Other makers' conductors show the same ideas: a transposing dial, a stored opening melody, and a switch shifting one note by roughly a quarter tone. [sourced: MATRIXSYNTH, Reverb listings]

## Front panel and functions

Mostly from one 2009 seller description, so treat as **[single source]** until checked against photos or video.

- **Two rows of button keys.** The front row is a Japanese scale, laid out so you can do koto-style glissando by sliding across it.
- **Voices:** shakuhachi, koto, oboe, strings, chorus. Strings and chorus are described as dark and Mellotron-like. Another listing names koto, shakuhachi, "Suiko" and strings.
- **Drum mode:** the keys become Japanese percussion. A paper overlay (打楽器シート) shipped with it.
- **Switches:** Japanese tuning, sustain, octave shift, vibrato, bend, mono/poly, trill (imitating a koto trill).
- **Two bend levers:** one for pitch, one for level.
- **Auto-chord button:** hold a key and press it for a non-Western "Japanese" chord.
- **Recorder:** auto play/record of a performance, with overdub using another voice.
- **IC card slot** for accompaniment cards (e.g. 吟詠集「水」 by Funakawa Toshio).

## Key layout and notes

From one Ask MetaFilter answer, read off a listing photo and a demo video. **[single source]**

- Lower (front) row: **mi fa la ti do**, repeated three times — three octaves of the in scale.
- Upper row: **fa♯ sol la♭ do♯ re**, repeated three times.
- Printed number labels: 1 2 3 3' 5 6 7 8.
- In the demo, mi = F. Lower row sounds F, G♭, B♭, C, D♭; upper row G, A♭, A, D, E♭. Button "6" is middle C.
- The asker says the unit switches between equal temperament and a traditional tuning.

Reading of that: the two rows together give ten of the twelve semitones above the tonic (0, 100, 200, 300, 400, 500, 700, 800, 900, 1000 cents); 600 and 1100 are absent. **[inference, arithmetic checked against the demo notes]**

The labels look like shigin degree numbers: 3 = mi (tonic), 3' = fa, 5 = la, 6 = ti, 7 = do, 8 = upper mi, with 1 and 2 being ti and do below the tonic. That fits "6 = middle C when mi = F" but nothing confirms it. **[inference]**

## Shigin scale and the 本 (hon) key system

- Standard shigin scale is 陰音階 (in scale, miyako-bushi): **mi fa la ti do** with mi as tonic. From the tonic in equal temperament: **0, 100, 500, 700, 800 cents**. [sourced: Ginken (日本吟剣詩舞振興会), shigin-fan.net]
- Haiku recitation uses 陽旋法 (yo): mi sol la ti re = **0, 300, 500, 700, 1000 cents**. [sourced: shigin-fan.net]
- 本 names the pitch of the tonic, one semitone per 本. Directly sourced points: **4本 = C, 6本 = D, 8本 = E**; 1本 measured at 440 Hz on a conductor with a frequency counter. [sourced: Ginken, junko-ishihara.com, daii.jp]
- 水 numbers count down below 1本: 水1本 = 12本 an octave lower (G♯), down to 水5本 = 8本 (E). 13–15本 equal 1–3本. [sourced: Ginken]
- Range used is one octave either side of the tonic. Men commonly use 1–3本, women 6–8本; male voices sound an octave lower. [sourced: shigin-fan.net, daii.jp]
- Koto accompaniment is transcribed at about 90 bpm in 8-beat lines. [sourced: Ginken]

Derived table (only 1, 4, 6, 8 are directly sourced; the rest is interpolation at one semitone per 本):

| 本 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| Tonic | A | A♯ | B | C | C♯ | D | D♯ | E | F | F♯ | G | G♯ |

The demo's mi = F would be 9本. No source describes the ST-50's own 本 selector or its range.

## Sound engine

- **Unknown.** No polyphony count, chip list, sample rate or service data found.
- Probably digital (PCM or similar): poly mode, stereo, "strings/chorus" voices, IC cards, and Reverb filing some listings under digital synths. Reverb's product page says "Analog, 1 voice", which looks copied from the ST-20. **[inference]**
- Reverb's news piece calls the sounds "samples" of Japanese instruments; a MATRIXSYNTH commenter says it resembles physical modelling.

## Character, as described by owners

- Close to koto and shakuhachi; Japanese scales only; "haunting", "timeless".
- Owners nearly always add reverb, delay or granular effects. An ST-20 owner finds it harsh when dry.
- Vibrato and the chords are praised; the trill is called repetitive (low-reliability source).

## Existing emulations and sample packs

| Item | Price | Licence | Use to us |
|---|---|---|---|
| [Aaron Horn "Sukio" ST-50 pack](https://aaronhorn.gumroad.com/l/SUKIOST50) | £9.99 | Not shown | Only direct ST-50 sample pack found: 160 one-shots, 104 loops, 24-bit WAV. Best listening reference. |
| [Soundgas ST-50 / Habit / 636P](https://soundgas.com/products/soundgas-samples-suiko-st50-habit-type-636p) | £12 | Royalty-free in music, no sound-library reuse | Heavily processed; less useful as a clean reference |
| [SampleScience Virtual Suiko](https://www.samplescience.info/2024/05/virtual-suiko.html) | Free per KVR | Not stated | VS-1, not ST-50. VST/AU, Decent Sampler, Kontakt |
| Alpha Chrome Yayo VS-1 pack | Free | Not stated | VS-1, via MATRIXSYNTH |

- **No ST-50 emulator exists** that I could find: no VST, soundfont, Pianobook library, Reaktor ensemble or GitHub project. Cherry Audio's forum has a request thread only.
- A Synthtopia commenter says Hainbach's Patreon has ST-50 samples (unverified).
- None of these packs grants redistribution, so **none can be embedded in firmware**. They are fine as private listening and pitch-measurement references.

## What is still unknown

1. Exact offsets of the "Japanese tuning" switch (which degrees move, by how many cents).
2. Vibrato rate and depth, bend range, trill interval and speed, envelope times.
3. The ST-50's 本 selector range and how it wraps octaves.
4. Which scales are selectable beyond the in scale.
5. What the auto-chord actually plays.
6. Polyphony and the engine type.

Items 1, 2 and 5 can be measured from recordings (the Aaron Horn one-shots, or audio from Hainbach's video) with a pitch tracker. That is a planned task in `05-build-plan.md`.

## Sources

- https://suikohsha.com/company.html and https://suikohsha.com/products.html
- https://www.matrixsynth.com/2009/02/suiko-st50-japanese-exotic-koto-synth.html
- https://www.matrixsynth.com/2020/10/the-rarest-japanese-synthesizer-suiko.html
- https://www.matrixsynth.com/search/label/SUIKO and https://www.matrixsynth.com/search/label/Shigin
- https://www.synthtopia.com/content/2020/10/05/suiko-st-50-koto-synthesizer-hands-on-demo-with-hainbach/
- https://reverb.com/news/find-of-the-week-suiko-st-50
- https://ask.metafilter.com/361261/What-notes-do-the-keys-of-the-Suiko-ST-50-play-so-I-can-DIY-one-myself
- https://soundgas.com/products/suiko-st-50-poetry-trainer
- https://sonicstate.com/news/2024/08/08/japanese-poetry-synthesizers/ (Zecchou documentary)
- https://www.youtube.com/watch?v=Rzo9ebsFd6Q (Hainbach; not fetched)
- https://www.ginken.or.jp/index.php/reading_content/reading_content-1663/
- https://www.ginken.or.jp/index.php/reading_content/reading_content-12582/
- https://www.ginken.or.jp/index.php/reading_content/reading_content-11637/
- https://www.shigin-fan.net/yougo/onkai/ , /yougo/honsuu/ , /yougo/conductor/
- https://daii.jp/gin/sigin.php
- https://natural-shigin.com/column/gintore
- https://equipboard.com/items/suiko-st-50-koto-synthesizer (reads as auto-generated; low reliability)
