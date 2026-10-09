# 16 — The audition bench: a playable browser instrument for voicing

Written 9 Oct 2026. Amends the audition step (file 09) and step 7 of the kickoff prompt in `00-START-HERE.md`. **Read this alongside START-HERE.**

## The idea

FoMni compiles its firmware C code to WebAssembly and runs it in a web page as a playable emulator (`web/emu/`). Borrow that for voicing: instead of a page of pre-rendered clips, the audition tool is **FuMi's real sound engine running live in the browser**, with everything needed for long listening sessions built around it. You will spend hours loading patches, playing, comparing and writing notes, so that loop should take seconds, not a flash cycle.

Because it is the same C code that later goes on the device, what you approve in the bench is what the firmware plays. Nothing gets re-implemented.

## What the bench does

**Play**
- FuMi's key layout on the computer keyboard (a row of keys = the sixteen white keys, the row above = the black keys), plus on-screen keys.
- **Web MIDI in**, so any MIDI keyboard works. The FM-1 itself can be the controller when it is running a firmware that sends its keys over USB (FoMni and Felucca do; whether stock V15 does needs checking). Then your fingers are on the real keys while the bench makes the sound.
- 本数, 調律 (equal / Suiko / koto / user, with depth), 余韻, vibrato, repeat (trill), bend buttons, all live.

**Compare**
- A list of candidate patches. Click or press a number key to switch instantly, mid-phrase, with held notes re-voiced.
- **A/B and blind A/B**: two patches on a toggle, optionally with names hidden and order shuffled, so the choice is made by ear.
- **Reference clips** from the ST-50 recordings on a button next to each patch, level-matched, so the comparison is always one keypress away.
- A small set of **stock phrases** the bench can play by itself (single notes in three registers, a slow overlapping phrase, a fast glissando, a bend), so every patch is heard on identical material.

**Tweak**
- Every patch parameter on a slider, live: the three operator stacks (click, body, fullness), their levels, ratios, envelopes and detune.
- The shared stages as separate controls: low cut, high cut, character amount, reverb amount and size.
- "Duplicate and vary": copy a patch and nudge it, so trying an idea never loses the one you had.
- **Morph** between two patches on one slider. This is the quickest way to audition a KOTO–HARP 2 hybrid: slide from one to the other and stop where it sounds right.

**Record what you think**
- A notes box and a star rating per patch, with a few fixed prompts (attack, body, tail, gliss, low/mid/high register) so judgements are comparable.
- Everything autosaves in the browser and **exports as one file** (patches + notes + ratings). That file goes in the repo; Claude Code reads it to generate the next round.
- Session history: which patches were kept, dropped or varied, so rounds can be traced.

**Check on the real thing**
- **Export finalists as a DX7 `.syx` bank** for the stock firmware, to hear them on the FM-1's own speaker. This is the only way the speaker gets judged, and it must happen every round: laptop speakers and monitors are not the FM-1's speaker.
- Limits of that check: stock firmware is equal temperament only and has none of FuMi's filters, feel layer or tuning. It judges raw timbre on the speaker, nothing else.
- Optional later: a "small speaker" preview switch in the bench (a band-limit curve approximating the FM-1's speaker), once the real speaker's response has been measured by recording a sweep from it.

## What it is built from

- **Engine:** FuMi's `dsp/` code, including the FM6 engine taken from Felucca (which is the Dexed code DX7 patches run on), compiled with Emscripten the way FoMni's `web/emu/build.sh` does, running in an AudioWorklet.
- **Patch import:** DX7 `.syx` in, so KOTO, HARP 2 and anything else you find can be loaded as starting points.
- **Reference clips:** cut from the files in `refs/`, loaded locally, never published.
- **No server:** one local page opened from a small local web server. Notes stay on your machine.

## What it cannot tell you

- How it sounds on the FM-1's speaker (use the `.syx` export).
- How the FM-1's keys feel under the hand, unless you play it from the FM-1 over MIDI.
- Whether the firmware has enough CPU for the patch. The bench shows an estimated cost per voice from the host build; the real answer comes at first flash.

## Where it fits

- It replaces "Rig 2, an audition page of clips" in file 09. Rig 1 (the `.syx` bank on the speaker) stays as the reality check.
- It needs the pitch core (M2) and the FM engine running on the host, so the order in M2.5 becomes: patch dump/diff tool → FM6 engine building on the host with tests → the bench → round 1.
- It is the tool for every later voice too, and it grows into the public browser emulator in M7 with the voicing controls hidden.

## Amended step 7 for the kickoff prompt

> 7. M2.5: build the audition bench before any koto voicing, as described in `16-audition-bench.md`. It is FuMi's real DSP (with Felucca's FM6 engine) compiled to WebAssembly and playable in the browser, following FoMni's `web/emu/`. It needs: DX7 `.syx` import; instant patch switching and blind A/B; a morph slider between two patches; live sliders for patch parameters, low cut, high cut, character and reverb; the measured tuning and 本数 controls; Web MIDI and computer-keyboard input in FuMi's key layout; reference clips from `refs/` on a button, level-matched; per-patch notes and ratings that export to one file in the repo; and export of chosen patches as a DX7 `.syx` bank for the stock firmware. Also build the patch dump/diff tool and the reference-audio analysis tools. Start me with KOTO, HARP 2 and a first set of hybrids loaded. STOP when it is ready for me to play.
