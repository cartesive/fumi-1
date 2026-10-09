# 08 — Steers: name, level lever, and koto voices that already exist

Written 9 Oct 2026. Overrides earlier files where they differ.

## Decided

- **Name: FuMi-1** (文, *fumi*). Public description: "inspired by the Suiko ST-50".
- **MASTER stays exactly as it is.** The ST-50's level rocker is not sprung: it sits where you leave it and goes down to silence. That is what the FM-1's MASTER pot already does, smoothly, and it is the one analogue control on the panel. So:
  - No "LEVER mode", no moving volume to a menu. Delete that idea from 06.
  - KNOB 2 is free again (it was the level swell in 04).
  - A fade-in is played the way it is on the ST-50: left hand on MASTER.
  - The optional "swell button" from 06 is not needed for faithfulness. Keep it only as a later nicety if it turns out to be wanted.
  - One thing to check on hardware: FoMni maps MASTER with a squared curve (`gain = k² / 256` on the 10-bit reading) after light smoothing. Listen for whether the bottom of the travel fades to silence gracefully or drops off a cliff, and whether a fast sweep zippers. Both are a few lines to adjust, in app code, not platform code.

## Koto voices that already exist on this hardware

Before writing a note of DSP, three candidate kotos can be heard on your FM-1's own speaker today.

| Candidate | Where | What it is |
|---|---|---|
| **SLOOP "KOTO"** | SLOOP, PHYS engine, in the pluck presets | A physical model: an extended Karplus–Strong string (Felucca's port of DaisySP / Mutable Instruments Rings code, MIT). Defined in `sloop-fm1/firmware/src/eng_phys.c` as one line of parameters |
| **DX7 "KOTO"** | The original DX7 ROM 1A factory bank (voice 23, from memory; confirm in any ROM1A listing) | A 6-operator FM pluck. Load the ROM1A `.syx` into the stock firmware, into FM-1+VA, or into Felucca's FM6 engine through its web editor (which imports `.syx`) |
| **FM-1 factory voices** | Stock V15; the 128 voices are archived at KingParamount/fm1-factory-presets as four DX7-format banks | Bank 2 is guitars and basses, bank 3 has woodwind, string and voice groups. I could not read the per-voice name list, so I do not know whether a koto is among them |

Also in SLOOP's PHYS presets and worth a listen for ideas: SITAR and TANPURA (strings with sympathetic strings), PHYS HARP, BANJO.

What this changes:

- **M0 gains a listening session.** Play the SLOOP KOTO and the DX7 KOTO back to back on the speaker and on headphones, across the range, single notes and fast runs. Write down what each gets right and wrong against a real koto and against the ST-50 recordings. That decides the PLUCK model's starting point with your ears instead of my reasoning.
- **Both engines are available to FuMi.** The PHYS string code and the FM6 engine (msfa from Dexed, Apache-2.0) are both in Felucca and GPL-compatible. FuMi can carry either or both. My expectation from 06 stands, but it is only an expectation: the string model will be closer to a real koto, the FM pluck may be closer to the ST-50's 1980s character.
- **FM is a legitimate route for the other voices too.** FM flutes, oboes and string pads were what the DX7 was known for, and a shakuhachi-like breathy flute is a classic FM patch. The Japanese-market DX7 ROM banks (3A/4A) are worth a search for traditional instrument voices as study material.
- **Do not ship Yamaha's ROM patches.** They are fine for private listening and for learning what the operators are doing. FuMi's patches should be its own, written as readable operator settings, the way Felucca's are (`tools/gen_fm6_patches.py` states that it contains no factory ROM data).

A caution on SLOOP's KOTO: it is one preset among 153 in a groovebox, tuned to sit in a mix. It lacks everything in 06's feel layer (gesture dynamics, per-pluck variation, same-string re-pluck, press-bends). It is a starting timbre, not the finished instrument.

## Pictures of the ST-50

An image search found plenty (Reverb listings, Synthtopia's Hainbach article, Yahoo Auctions and Mercari listings under 水光トレーナー ST-50), but the search tool shows the pictures in the conversation without passing them to me, so **I have not seen the panel**. The research notes are unchanged by it.

A clear, straight-on photo of the top panel attached to a session would let me read:

1. The physical start note and label order of both key rows (settles the white-key layout in 07).
2. The fifth upper-row note I could not place.
3. The Japanese labels on every switch, slider and rocker, and so the real list of functions.
4. The voice names, which settles the four-versus-fifteen-sounds question.
5. Where the rockers sit relative to the keys, for which hand does what.

The Japanese auction listings usually have the sharpest top-down shots.

## Sources

- SLOOP source: `firmware/src/eng_phys.c`, `firmware/src/ui.c`, README (PHYS engine and preset list) — https://github.com/isod89/sloop-fm1
- Felucca: `firmware/src/eng_phys.c`, `tools/gen_fm6_patches.py`, README (FM6 engine, `.syx` import) — https://github.com/hugelton/Felucca
- FM-1 factory voices: https://github.com/KingParamount/fm1-factory-presets
- FoMni MASTER handling: `firmware/src/app/main_fm1.c` — https://github.com/charlesvestal/fm1-omnichord
- The DX7 ROM1A slot number is from memory, not from a fetched source.
