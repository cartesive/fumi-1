# 11 — The 音色 strip, read from a close crop

Written 9 Oct 2026. Completes the voice list left open in file 10. Names are printed vertically, one per slider position, left to right.

## Voices

| # | Printed | Reading | Instrument | Confidence |
|---|---|---|---|---|
| 1 | 琴 | koto | 13-string koto | High |
| 2 | 十七絃 | jūshichigen | 17-string bass koto | High |
| 3 | ?琴 | (two characters ending in 琴) | Unknown. First character looks like 唐 or 和; 和琴 (*wagon*, the six-string court zither) is a candidate | **Low** |
| 4 | スイコー | Suikō | **The "Suiko" original voice** | High |
| 5 | チャイム | chaimu | Chime | High |
| 6 | 鉄琴 | tekkin | Glockenspiel / metal bars | High |
| 7 | 尺八 | shakuhachi | End-blown bamboo flute | High |
| 8 | 篠笛 | shinobue | Transverse bamboo flute | High |
| 9 | 能管 | nōkan | Noh flute | High |
| 10 | 笙 | shō | Mouth organ | Medium (single small character) |
| 11 | 胡弓 | kokyū | Bowed lute | High |
| 12 | オーボエ | ōboe | Oboe | High |
| 13 | ストリングス | sutoringusu | Strings | High |
| 14 | ホルン | horun | Horn | High |
| 15 | コーラス | kōrasu | Chorus | High |
| 16 | 打楽器 | dagakki | Percussion mode | High |

Fifteen voices plus percussion, which matches the listing that claimed "15 selectable sounds" and a percussion set. The four-sound description (koto, shakuhachi, Suiko, strings) is presumably the earlier, smaller version.

Notes:

- **Koto is position 1.** It is the instrument's home voice, as it should be FuMi's.
- **スイコー is a named voice of its own**, between the kotos and the bells. Its character is unknown from the panel; it needs a listen.
- **Chime and glockenspiel** are bell tones, which is what 6-operator FM does best. Another sign the palette suits an FM engine.
- The plucked group (1–3), the bell group (5–6), the flutes (7–9), and the sustained group (10–15) fall neatly onto the PLUCK, BLOW and ENSEMBLE models in file 06, with bells as FM patches in the PLUCK family.

## Percussion map (right half of the strip)

Under 鍵盤 there is a small diagram of the two key rows with a drum name per key. Read with reasonable confidence: 大太鼓 (ō-daiko, big drum), 締太鼓 1 and 2 (shime-daiko), 拍子木 (hyōshigi, wooden clappers), 大鼓 (ōtsuzumi), 小鼓 (kotsuzumi), 鈴 (suzu, bells), 鉦 (shō, small gong), 鐘 (kane, bell), ティンパニー 1, 2, 3 (timpani), ゴング (gong). Two or three more names are too small to read (one begins 胴, a drum-body hit). Which name belongs to which key cannot be traced at this size. Parked with drum mode.

## For the first release

Suggested FuMi voice order, in the panel's own order, with the first four as the launch set:

1. **琴 Koto** (the audition step decides its patch)
2. **尺八 Shakuhachi**
3. **ストリングス Strings**
4. **スイコー Suiko** (once heard)

Then 十七絃, the flutes, chime and glockenspiel, kokyū, oboe, horn, chorus, as later patches on the same three models. ALGORITHM steps through them in panel order so the layout matches the original.
