# 10 — Reading the ST-50 panel

Written 9 Oct 2026 from a clear top-down photo you supplied (it looks like a frame from a video; the unit has someone's marker annotations on the keys). This is the first primary evidence in the project. **Where it disagrees with files 01–09, this file wins.** Anything I could not read with confidence is marked.

## The panel, section by section

**Top:** 水光トレーナー (Suiko Trainer). Speaker grilles left and right. Jacks: ACアダプター (AC adapter), ペダル (pedal), L–ライン–R (line out), ヘッドホン (headphones).

**Black strip, left: 音色 (voices).** About fifteen names printed vertically, selected by the unlabelled slider with tick marks directly below. Read with confidence: 十七絃 (17-string bass koto), 琴 (koto), 尺八 (shakuhachi), 篠笛 (shinobue), 能管 (nohkan), 胡弓 (kokyū), オーボエ (oboe), ストリングス (strings), ホルン (horn), コーラス (chorus), 打楽器 (percussion). Three or four more I cannot read at this resolution (one looks like 笙, one like 鉄琴). So this is the fifteen-sound version, and "various flutes" in the listings means shinobue and nohkan.

**Black strip, right: 鍵盤 (keyboard) percussion map.** A diagram of the keys with a drum per key: 大太鼓, 締太鼓 1 and 2, 拍子木, 大鼓, 小鼓, 鈴, 鉦, ティンパニー 1–3, ゴング and others. Out of scope for the first release; the photo is the reference when drum mode is built.

**Left block:**

| Control | Label | Meaning |
|---|---|---|
| Switch | 電源 入/切 | Power |
| **Switch** | **調律: (洋楽) 平均律 / 純正律 (邦楽)** | **Tuning: equal temperament (Western music) / pure tuning (Japanese music)** |
| Slider | 音量 小–大 | Volume |
| Slider | 微調: −1/2, −1/4, 標準, 1/4, 1/2 (本) | Fine tune in fractions of a 本: ±50 cents, with quarter marks at ±25 |
| Slider | (unlabelled, ticks) | Voice select |
| Slider | 本数: 水4 3 2 1, then 1–12 (本) | Key. Sixteen positions, 水4本 up to 12本 |

**Display:** red LED, 分/秒 (minutes/seconds). Modes: タイマー (timer), 記憶残量 (memory remaining), 曲番 (song number). Card indicators: ROMカード0, RAMカード1, RAMカード2.

**Right block, four sliders:**

| Label | Meaning |
|---|---|
| ビブラート速さ 遅–速 | Vibrato speed |
| ビブラート深さ 浅–深 | Vibrato depth |
| **余韻 短–長** | ***Yoin*: lingering resonance, short to long.** The decay/ring time |
| 自動伴奏音量 小–大 | Auto-accompaniment volume |

**Two rockers, far left:** 強弱 (dynamics, literally strong/weak) and 音程 (pitch).

**自動和音 (auto chord): four buttons, 1 2 3 4.**

**鍵盤 (keyboard) buttons:** ビブラート (vibrato, LED), トリラー (trill, LED), オクターブ (octave; its indicator is marked 下, "down"), 単音/和音 (single note / chord, i.e. mono/poly; indicator 和).

**自動伴奏 (auto accompaniment) buttons:** 記憶 (record; also カード書込, write to card), 再生 (play), 停止 (stop), 終了 (end), 音程ガイド (pitch guide, LED).

**ICカード / 表示部:** −, 選曲 (song select), +, 表示切替 (display mode). タイマー: スタート/ストップ, リセット.

## The keys

Two rows of fifteen round keys. The upper row is offset half a key to the right, so each upper key sits between two lower keys. Both rows are five to the octave and stay aligned.

| | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 | 13 | 14 | 15 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| **Upper** | ファ♯ | ソ | シ♭ | ド♯ | レ | ファ♯ | ソ | シ♭ | ド♯ | レ | ファ♯ | ソ | シ♭ | ド♯ | レ |
| **Lower** | ミ | ファ | ラ | シ | ド | ミ | ファ | ラ | シ | ド | ミ | ファ | ラ | シ | ド |
| **Printed below** | 水 | | 乙 | 一 | 二 | 三 | 三′ | 五 | 六 | 七 | 八 | | | | |

Lower keys 3, 4, 6, 8 and 9 (ラ乙, シ一, ミ三, ラ五, シ六) look tan rather than white. Those are mi, la and ti, the stable frame notes of the scale, in the main register. They may be coloured caps or lit by the pitch guide; I cannot tell from one frame.

## Corrections this forces

1. **The front row starts on mi.** Fifteen keys, mi fa la ti do three times, lowest key 水 (low mi). File 07's suggestion to start on ti is **wrong**; file 04's original layout was right. On the FM-1: white keys 1–15 are the row exactly, tonics on 1, 6 and 11, and the sixteenth white key is a bonus top mi. The reference tonic 三 is white key 6.

2. **The upper row is fa♯, sol, ti♭, do♯, re.** The third note is **シ♭ (ti-flat, 600 cents above mi)**, not la♭ as the forum post had it. Cents above mi: **200, 300, 600, 900, 1000**. Together the rows give every semitone except 400 and 1100. Fa♯/do♯ and sol/re are still the raised mood notes from file 07; ti♭ is a lowered ti.

3. **The label mapping in 07 is confirmed** by the printing: 水, 乙, 一, 二, 三, 三′, 五, 六, 七, 八 sit under exactly the notes Ginken's notation says.

4. **The tuning switch says 純正律.** That word means "pure" or "just" tuning, and it is ambiguous between two tables that pull the semitones in opposite directions:

| Reading | fa | la | ti | do | Semitone |
|---|---|---|---|---|---|
| Pure fifths (how a koto is actually tuned; file 07) | 90 | 498 | 702 | 792 | **Narrow** |
| Textbook just intonation (5-limit ratios 16/15, 8/5) | 112 | 498 | 702 | 814 | **Wide** |

   Koto practice and the 邦楽 label argue for narrow. But an engineer in the 1980s implementing "純正律" from a table could easily have used the textbook ratios. **This cannot be settled by reading; it has to be measured** from a recording with the switch in the 純正律 position. A 22-cent difference between the two on fa is easy for a pitch tracker. Until then, FuMi's default stays pure-fifths, and the audition page gets textbook-just as a third option so you can hear which one sounds like the Suiko.

5. **本 range is 水4 to 12**, not 1 to 12 with optional 水. Fine tune is **±1/2 本** with quarter-本 detents; that explains the "quarter-tone switch" in listings and removes it as a hint about the tuning depth.

6. **余韻 is a slider, not a sustain switch.** Continuous ring time from short to long. This is the control that makes notes overlap and bloom, so it matters for the resonance you described. It gets a dedicated knob.

7. **Vibrato has speed and depth sliders** as well as the on/off button.

8. **There are four auto-chord buttons**, and Ginken's harmony column lists exactly four basic chords (la–do–mi, re–fa–la, re–fa–ti, mi–ti). One chord per button is now a specific, testable guess. Still parked for the first release.

9. **Octave is a single button that drops an octave** (indicator 下). No octave-up is shown.

10. **The level rocker is labelled 強弱, dynamics, and there is a separate 音量 volume slider.** So on the ST-50 they are two controls: a set-and-forget volume and a performance rocker. On the FM-1 there is one output level, so MASTER can still serve as the 強弱 rocker as decided in 08. Worth knowing that the original treats it as expression, played with the left hand beside the pitch rocker, with the right hand on the keys.

11. **No reverb or effect control is on the panel.** Whatever space is in the sound comes from the voices, 余韻 and the stereo speakers. FuMi's reverb is an addition, so keep it subtle and default it low.

## Updated FuMi control map

| FM-1 control | Function | ST-50 original |
|---|---|---|
| White keys 1–15 (+16) | mi fa la ti do × 3 (+ top mi) | Lower row |
| Black keys | Upper row fa♯ sol ti♭ do♯ re, or ornament keys (SEQ switches) | Upper row |
| SELECT | 本数, 水4 … 12 | 本数 slider |
| ALGORITHM | 音色 (voice) | Voice slider |
| PRESETS | Scale: IN / YŌ / MIN'YŌ | — |
| KNOB 1 | **余韻** (ring time) | 余韻 slider |
| KNOB 2 | Vibrato speed | ビブラート速さ |
| KNOB 3 | Vibrato depth | ビブラート深さ |
| KNOB 4 | Reverb (FuMi addition) | — |
| MASTER | 強弱 / volume | 強弱 rocker, 音量 slider |
| OCT− / OCT+ with a note held | Pitch bend down / up | 音程 rocker |
| OCT− with no note held | Octave down toggle | オクターブ |
| OCT+ with no note held | Octave up (FuMi addition) | — |
| LFO | ビブラート on/off | ビブラート |
| ARP | トリラー on/off | トリラー |
| GLO | 単音/和音 (mono/poly) | 単音/和音 |
| SEL | 調律: 平均律 / 純正律; hold for depth and 微調 | 調律 switch, 微調 slider |
| ENV | Free (was sustain; 余韻 on KNOB 1 replaces it) | — |

Bend range and time move to the page shown while a bend button is held. The home screen can use the panel's own words: 本数, 音色, 調律, 余韻.

## Still open after the photo

1. Pure-fifths or textbook-just behind 純正律. Needs a measurement.
2. The three or four voice names I could not read. A closer crop of the 音色 strip would do it.
3. Whether the tan keys are coloured caps or lights.
4. What the 音程 rocker's range is, and how the 強弱 rocker is sprung.
5. What each of the four chord buttons plays.
