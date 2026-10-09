# 07 — Traditional koto tuning, and what it settles

Researched 9 Oct 2026. This replaces the "provisional" WA tuning in `tuning-reference.md` with a sourced one, corrects the YO scale, and explains the ST-50's number labels and upper row. Where this file disagrees with 01, 04 or `tuning-reference.md`, this file wins.

## Short version

- Traditional koto tuning (hira-jōshi) is done **by ear with pure fifths, fourths and octaves**. That makes the two semitones of the in scale **narrow**: about 90 cents instead of 100.
- A koto school's tuner guide gives the offsets from equal temperament string by string, and they are exactly the pure-fifths values I had guessed: **fa −9, la −2, ti +2, do −7** (tonic 0).
- Players often go further. A demonstration reported by a music-education magazine measured a professional's ear-tuned semitone at **about 20 cents narrower** than equal temperament, and notes it varies with piece, school, weather and the player.
- So the "Japanese tuning" is not one fixed table. It is one idea, narrow semitones, with a depth. The firmware should expose that depth as a single control.

## Sources and what each says

| Source | What it gives | Confidence |
|---|---|---|
| Ando, "Koto scales and tuning", J. Acoust. Soc. Jpn. (E) 10(5), 1989, pp. 279–287 (Tokyo University of the Arts) | The academic study. Abstract: describes the traditional tuning method using hira-jōshi, measures individual performers' pitch deviations **especially for minor seconds**, finds the minor seconds are **related to Pythagorean seconds**, and that tuning scatter across the range is similar to a normally tuned piano. Proposes a more reliable method for beginners | High, but **I could only read the abstract**: the PDF is free on J-STAGE but blocked to my fetch tool. It will have the measured tables. Worth ten minutes of your time |
| Sōgakusha koto school blog, "tuner" article | String-by-string tuner targets for hira-jōshi with 五 = D at A = 440: D 0, G −2, A +2, B♭ −7, E♭ −9, octaves likewise. Their working tolerance: semitone strings "about −10, never flatter than −10". Most strings are tuned by ear from perfect intervals; some teachers take only two strings from the tuner | Good. A practitioner's guide, and the numbers match pure-fifths arithmetic to the cent, which is a strong cross-check |
| Kyōiku Ongaku (Ongaku no Tomo) report on koto in school teaching | A lecturer tuned hira-jōshi to equal temperament with a tuner, played, then retuned by ear: the semitone came out **about 20 cents narrower**. Size varies with piece, school, weather, performer's condition. Schools often just use equal temperament | Good for the direction and rough size; one demonstration, not a study |
| Ginken (Japan Ginkenshibu Foundation) shigin theory columns, 2023–2024 | Scale definitions and conductor number notation (below) | High for shigin practice |

## Why the semitones come out narrow

Tune the tonic. Tune a pure fourth up (la, 498) and a pure fifth up (ti, 702). The two remaining notes sit a semitone above the tonic and a semitone above ti, and are reached by further pure intervals or simply by ear as "close". Either way they land low:

| Degree | mi | fa | la | ti | do |
|---|---|---|---|---|---|
| Equal temperament | 0 | 100 | 500 | 700 | 800 |
| Pure-fifths (Pythagorean) | 0 | 90.2 | 498.0 | 702.0 | 792.2 |
| Offset from ET | 0 | −9.8 | −2.0 | +2.0 | −7.8 |
| Sōgakusha tuner guide | 0 | −9 | −2 | +2 | −7 |

In hira-jōshi with string 五 on D, that is D (mi), E♭ (fa), G (la), A (ti), B♭ (do): the same five notes as the shigin in scale.

## The tuning model for the firmware

One parameter, **semitone depth `x`** in cents, plus fixed pure la and ti:

```
mi = 0
fa = 100 − x
la = 498.0
ti = 702.0
do = 802 − x          (a semitone of the same size above ti)
```

| Preset | x | fa | la | ti | do | Basis |
|---|---|---|---|---|---|---|
| ET 平均律 | — | 100 | 500 | 700 | 800 | Equal temperament (la and ti also equal-tempered) |
| WA 和 | 9.8 | 90.2 | 498.0 | 702.0 | 792.2 | Pure fifths; matches the school tuner guide and Ando's "Pythagorean seconds" |
| WA deep | 20 | 80 | 498.0 | 702.0 | 782 | The ear-tuned demonstration |
| USER | 0–35 | | | | | One knob for depth, plus per-degree trim ±20 for anyone who wants it |

On the SEL (TUNING) page, KNOB 1 is the depth. This is a nicer instrument than a two-position switch: you can hear the scale darken as the semitones close.

Raised notes follow the same logic (pure-fifths values): fa♯ 203.9, sol 294.1, do♯ 905.9, re 996.1.

**What the ST-50 itself does is still unmeasured.** A hardware "Japanese tuning" switch from that period most likely selects one fixed table of this kind. The smaller ST-20 is described as having a "quarter-tone fine-tune switch", which hints the Suiko offset might be larger than 10 cents. Pitch-tracking an ST-50 recording with the switch on would settle the default depth. Until then WA = 9.8 is the defensible default and "deep" is one click away.

## Correction: the scales

Ginken defines the shigin scales by what happens to the two "mood" notes, fa and do (the 2nd and 4th tones, 商 and 羽):

| Scale | Notes | Cents (ET) | How it is derived |
|---|---|---|---|
| **IN 陰音階** | mi fa la ti do | 0, 100, 500, 700, 800 | The standard shigin scale |
| **YŌ 陽音階** | mi fa♯ la ti do♯ | 0, 200, 500, 700, 900 | fa and do raised one semitone ("1本差") |
| **MIN'YŌ 民謡音階** (also "shakuhachi scale") | mi sol la ti re | 0, 300, 500, 700, 1000 | fa and do raised a whole tone ("2本差") |

File 01 and `tuning-reference.md` called 0-300-500-700-1000 "YO", following another site. Ginken is the federation, so use its names: **three scales, IN / YŌ / MIN'YŌ.** PRESETS selects between them.

## This explains the ST-50's upper row

The upper row is fa♯, sol, la♭, do♯, re. Four of those five are exactly the raised versions of the two mood notes:

- fa♯ and do♯ turn IN into YŌ.
- sol and re turn IN into MIN'YŌ.

So the upper row is not a random set of leftover chromatic notes. It is where a player reaches to shift the mode mid-phrase. The fifth note (la♭) I cannot place from these sources. This also means a koto player's weak and strong press-bends on the fa and do strings land on exactly these notes, which is a good argument for the bend buttons defaulting to "semitone" and "whole tone".

## This confirms the number labels

Ginken gives the conductor notation: **三 = mi, 三′ = fa, 五 = la, 六 = ti, 七 = do, 八 = mi (upper)**, with 一 = ti and 二 = do an octave below 六 and 七, 乙 = low la, 水 = low mi. 四 is the raised fa (fa♯), and 二′ = re.

That matches the ST-50's printed labels 1 2 3 3' 5 6 7 8 and the "4" at the start of the upper row, and it matches the demo where key "6" was middle C with mi = F. Two consequences:

1. **The front row reads ti, do, mi, fa, la, ti, do, mi in label order.** Shigin melodies dip to the two notes below the tonic constantly, so the natural home position has them under the hand. For the FM-1's sixteen white keys I now suggest starting on ti rather than on the tonic:

| White key | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 | 13 | 14 | 15 | 16 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| Label | 一 | 二 | 三 | 三′ | 五 | 六 | 七 | 八 | … | | | | | | | |
| Degree | ti | do | **mi** | fa | la | ti | do | **mi** | fa | la | ti | do | **mi** | fa | la | ti |

Tonics on keys 3, 8 and 13, lit as landmarks. Still a table; still to be checked against a photo of the real panel, since I do not know which note the ST-50's row physically begins on.

2. **The screen can show real conductor notation** (三, 三′, 五 …) on the keys, which is what a shigin player would expect to see.

## A lead on the auto-chord

Ginken's column on harmony for group chanting lists four basic chords in this idiom: **la–do–mi, re–fa–la, re–fa–ti, and mi–ti** (an open fifth), with rules for which accompanies which melody note (melody mi → la+do or ti; fa → re+la or ti; la → do+mi or re+fa; ti → re+fa or mi; do → la+mi) and the accompaniment kept below the melody. The ST-50's "Japanese chord" button may well do something like this: pick the chord from the held key. It is a far better starting table than my earlier tonic-fifth-octave guess. Chord is out of scope for the first release, so this is parked with a source attached.

## Reference pitch

- Shigin conductors: 1本 measured at 440 Hz (file 01).
- The koto school guide tunes at A = 440 and advises against 442.
- Gagaku koto uses A = 430 (Stanford gagaku project). Not relevant to shigin, but a reason to keep the reference adjustable.

Default A = 440, adjustable 430–445.

## Still to do

1. **Read Ando's paper** (J-STAGE, free PDF, link below) for the measured semitone sizes across players. If his mean is well below 90, move the WA default deeper.
2. **Measure an ST-50** with its tuning switch on.
3. Confirm the front-row start note and the fifth upper-row note from a clear panel photo.

## Sources

- Ando, M. (1989). Koto scales and tuning. https://www.jstage.jst.go.jp/article/ast1980/10/5/10_5_279/_article — PDF: https://www.jstage.jst.go.jp/article/ast1980/10/5/10_5_279/_pdf
- Sōgakusha koto school, on tuners: https://sohgakusha.livedoor.blog/archives/10588062.html
- Kyōiku Ongaku report on koto basics for schools: https://kyoikuongaku.ontomo-mag.com/report/4255/
- Ginken, shigin music basics, Sept 2023 (scales, 四): https://www.ginken.or.jp/index.php/reading_content/reading_content-12351/
- Ginken, shigin music basics, June 2024 (conductor notation, chords): https://www.ginken.or.jp/index.php/reading_content/reading_content-12741/
- Stanford gagaku project, koto: https://gagaku.stanford.edu/en/strings/koto
- The Japanese pages were read through a summarising fetch tool, not by me line by line. The cent values from the school guide agree with the arithmetic, which is reassuring; the label mapping and chord list are worth a second look by a Japanese reader before they go in a manual.
