# 15 — What Hainbach says in the video

Written 9 Oct 2026. Transcribed by machine (a small English speech model run locally) from the MP3 of "The rarest Japanese synthesizer? | Suiko ST-50". Covers 0:00–13:00; the remainder is him recording a track. Machine transcripts mishear words, so these are paraphrases of what he says with rough timestamps, not quotations. He says himself that he had no manual and worked the panel out with Google Translate and help from a friend, so his descriptions are an informed owner's reading, not documentation.

## New information

| Time | What he says | What it means for FuMi |
|---|---|---|
| 5:34 | It is called a koto synthesizer but is **really a ROM player: sample-based**, with samples you can shape a little. Not physical modelling | The engine question from file 01 gets its first direct answer, on an owner's judgement. It explains why the pitch is rock-steady from the first instant (file 13) and why a harp patch with a click matches better than a "realistic" model |
| 5:58 | **No round robin**: the same sample plays every time a key is hit, which gets repetitive | FuMi's per-pluck variation (file 06) is a real improvement on the original, not just a nicety. Worth a setting for how much, with zero being "as the ST-50" |
| 6:23 | The switch chooses Western tuning or "just intonation, which is the Japanese tuning" | Matches the panel (純正律). The measured table in 13 and 14 is what that position actually does |
| 6:23 | The fine-tune slider moves pitch "in quarter and half steps"; the 本数 slider transposes "up to an octave" | Matches files 10 and 14 |
| 6:51 | One slider sets the **speed of a repetitive effect** common in Japanese traditional music. Set fast, it becomes robotic and you hear the **same sample repeated like a machine gun** | **The trill is note repetition, not alternation between two pitches.** It is a tremolo re-pluck, like rapid repeated strokes on one koto string. File 06 had it as alternating with the next scale note; that is wrong. The slider labelled ビブラート速さ evidently governs this repeat speed |
| 7:17 | The other slider controls a "beautifully weird" **chorus/vibrato**, switched on with a button; settings in between sit somewhere between chorus and vibrato | The ビブラート button and depth slider give something lusher than plain vibrato. FuMi's vibrato should be able to go from pitch wobble toward a chorus-like shimmer |
| 7:44 | 余韻: "on some sounds we can shape the release" | Confirms it is release/ring time, and that it does not affect every voice |
| 7:44–8:36 | Auto-accompaniment needs an IC card, which he does not have; one thing is stored in the unit and plays from a button | Parked, as before |
| 8:36 | The button keyboard is "probably like the frets on a koto"; he had not mapped the notes | Our mapping from the panel and the audio is more complete than his |
| 9:03 | You can **sweep sounds in with the volume rocker**; it is **heavy and not easy to move** | The 強弱 rocker: stiff, stays put. Matches your description, and MASTER is the right stand-in |
| 9:30 | The **pitch rocker has a decent push and pull and snaps right back** | Sprung, both directions. Matches the OCT−/OCT+ bend buttons with spring return |
| 9:30 | The **four buttons add chords to a note**, giving "traditional Japanese harmonic relationships" | Supports the one-chord-per-button guess in file 10. Still parked |
| 10:23 | 9 V supply, left/right outputs, headphones; plugging in headphones turns the speakers off | — |
| 11:13 | The built-in speakers are remarkably good for the size | — |

He also says the instrument was made "about 20 years or more ago" (said in 2020), that the company still exists but nobody there now worked on it, and that every voice he demonstrates is a traditional Japanese instrument "except maybe the synthesizer and the strings".

## Consequences

1. **Trill redesign.** トリラー = repeated re-triggering of the held note at a set speed. On the koto voice that is a tremolo; FuMi should vary each repeat slightly so it does not machine-gun, with the option to turn that off.
2. **If the ST-50 is sample playback, the koto target is "a good sampled koto of that era", lightly shaped.** That supports FM as the route (clean, stable, a click and a round body) over a wandering physical model, and it supports the character stage in file 06.
3. **"Shape them in some ways"** suggests little or no filtering per voice beyond release. The fast darkening after the attack measured in file 13 is then a property of the recorded samples.
4. **Vibrato as chorus-vibrato** is a detail to audition: rate and depth ranges, and whether it is pitch modulation alone or a doubled, detuned voice.

## Not covered

Nothing about polyphony, the octave button, the 単音/和音 button, the percussion layout, or the pitch-rocker range. The last five minutes (track-making) were still transcribing when this was written and are unlikely to add instrument facts.
