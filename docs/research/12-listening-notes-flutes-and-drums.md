# 12 — Listening notes: flutes and drums

Written 9 Oct 2026 from your listening. These are your judgements of the original; the scope change below is **proposed, not yet agreed**.

## What you heard

| Voice group | Verdict |
|---|---|
| Koto | Beautiful. The reason for the project |
| Flutes (shakuhachi, shinobue, nōkan) | "A bit rubbish". Probably the technology of the day |
| Drums (the big taiko especially) | Amazing. Big, robust bass drums; impressive through monitors |

## What follows from it

**FuMi is not a museum copy.** The rule becomes: be faithful where the ST-50 is beautiful, and be better where it is weak. Faithfulness applies to the layout, the tuning, the koto and the drums. It does not oblige FuMi to reproduce thin flutes.

**Flutes: aim at the instrument, not at the ST-50.** A shakuhachi's character is in its breath, its scoop into pitch, its late vibrato and its slides, none of which a mid-1980s preset could do and all of which FuMi's pitch core and key-slide already provide. So the shakuhachi is worth doing, as FuMi's own voice, judged against a real shakuhachi. It no longer needs to be in the launch set, and shinobue and nōkan can wait.

**Drums: proposed promotion into the first release.** You parked drum mode when the answer was "just the instrument". If the drums are one of the two best things about the original, they are part of the instrument. Proposal: launch with **koto + taiko**, with strings and the Suiko voice next and shakuhachi after.

## Notes for building the drums

- **What makes a big taiko big:** a low fundamental that drops slightly in pitch after the hit, a few inharmonic membrane modes above it, a short noisy stick transient, and a long decay. The size is in the decay and the pitch drop as much as in the bass.
- **Starting points already in the parts bin:** Felucca's PHYS engine has struck-membrane models, and its DRUM engine has synthesised kicks with long decays. Both are worth auditioning before writing anything new.
- **The FM-1's speaker cannot reproduce that bass.** Through monitors or headphones the drums can be huge; on the built-in speaker the fundamental will mostly vanish. Two mitigations: make sure the upper modes and the stick attack carry the drum on their own, and consider the platform's speaker EQ (Felucca has a BASS+ setting that adds harmonics so a small speaker implies the low note). Audition on both, as with the koto.
- **Headroom:** a full-scale bass drum plus ringing koto notes needs a limiter on the output so nothing clips. FoMni already has one.
- **A clue about the original:** drums that good next to flutes that weak suggests the two were made differently inside the ST-50, perhaps sampled percussion beside simpler synthesised tones. That is a guess; it does not change the plan.
- **Layout:** the percussion map on the panel assigns a drum to each key. The names are readable (file 11) but not which key each is on, so FuMi's drum layout will need a clearer picture of that strip, or its own sensible arrangement: big drums low, small and metallic sounds high.
- **The audition step applies:** reference clips of the ST-50's drums, candidates rendered and loaded, ranked by ear on monitors and on the FM-1.

## Revised launch set (if you agree)

1. 琴 Koto
2. 打楽器 Percussion, led by the big taiko
3. ストリングス Strings
4. スイコー Suiko voice

Later: shakuhachi (as FuMi's own), 17-string koto, chime and glockenspiel, the remaining voices.
