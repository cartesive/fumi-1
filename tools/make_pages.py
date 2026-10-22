#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Make the FuMi-1 site for GitHub Pages (https://cartesive.github.io/fumi-1/):

  index.html                    what FuMi-1 is, how it plays, and the ways in: try, bench, install, download, source
  img/                          screens (docs/img, from the host simulator)
  emu/                          FuMi-1 in the browser (build/emu, from web/emu/build.sh)
  bench/                        the audition bench (build/bench; hosted mode: files from the visitor's disk)
  install/index.html            the web installer (web/fumi_installer.html, with fm1pkg.js, fm1ota.js and
                                the package's metadata inlined; Chrome or Edge, Web MIDI)
  firmware/fumi-VERSION.fwsc    the package the installer writes; also the download

  tools/make_pages.py build/fumi-0.1.fwsc 0.1 OUT_DIR

The package must be a release build (./build.sh --release X.Y): its identity, FM-1_7XXYYZZ, is what
the installer checks the download against and what the FM-1 reports after the install. The wasm,
worklet and bench scripts get the version as a query string so a plain reload sees a new build."""
import html
import json
import re
import shutil
import subprocess
import sys
from pathlib import Path

SRC = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(SRC / "web"))
from make_site import product_of, strip_module  # noqa: E402  (Felucca's: the package format)

REPO = "https://github.com/cartesive/fumi-1"
SF_VERSION = "1.0.8"                         # the latest release that carries a SoundFont (docs/soundfont.md): a released
                                             # file is frozen, and a version without a new render links the last one

LANDING = """<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>FuMi-1</title>
<meta name="description" content="FuMi-1: custom firmware for the M-VAVE FM-1 inspired by the Suiko ST-50 shigin conductor. Koto, flutes and a drum in the ST-50's tuning; play it in the browser or install it.">
<style>
:root { --ground: #faf4e8; --ink: #3b2f28; --muted: #8a7764; --rule: #e6dccb; --accent: #c44830; --card: #fffaf1; color-scheme: light; }
@media (prefers-color-scheme: dark) {
  :root { --ground: #1c1814; --ink: #f2e8d8; --muted: #b3a28e; --rule: #3a322a; --accent: #f08a6a; --card: #26201b; color-scheme: dark; }
}
body { background: var(--ground); color: var(--ink); margin: 0; padding: 48px 16px 64px; font: 400 17px/1.6 -apple-system, "Helvetica Neue", Arial, sans-serif; }
main { max-width: 42rem; margin: 0 auto; }
h1 { font-size: 3rem; line-height: 1; margin: 0 0 .4rem; color: var(--accent); }
h1 small { font-size: 1.2rem; color: var(--muted); margin-left: .5rem; }
h2 { font-size: 1.4rem; margin: 2.2rem 0 .6rem; }
.lede { font-size: 1.15rem; margin: 0 0 1.6rem; max-width: 36rem; }
.shots { display: grid; grid-template-columns: repeat(auto-fit, minmax(150px, 1fr)); gap: 12px; margin: 0 0 1.8rem; }
.shots img { width: 100%; height: auto; border-radius: 14px; image-rendering: pixelated; box-shadow: 0 1px 0 var(--rule), 0 6px 18px rgba(72, 57, 47, .12); }
.status { border: 1px solid var(--rule); background: var(--card); border-radius: 14px; padding: 12px 16px; margin: 0 0 1.6rem; }
.ways { display: grid; gap: 12px; margin: 0 0 1rem; }
.ways a { display: block; text-decoration: none; color: var(--ink); background: var(--card); border: 1px solid var(--rule); border-radius: 14px; padding: 14px 18px; }
.ways a:hover, .ways a:focus-visible { border-color: var(--accent); outline: none; }
.ways strong { font-size: 1.25rem; color: var(--accent); display: block; }
.ways span { color: var(--muted); font-size: .95rem; }
table { border-collapse: collapse; width: 100%; font-size: .95rem; }
td, th { border-top: 1px solid var(--rule); padding: 8px 10px 8px 0; vertical-align: top; text-align: left; }
td:first-child { font-weight: 600; white-space: nowrap; }
p.small { color: var(--muted); font-size: .9rem; }
a { color: var(--accent); }
code { font-size: .9em; }
ul.pages { padding-left: 1.2rem; margin: 0 0 1rem; }
ul.pages li { margin: 0 0 .4rem; }
</style>
</head>
<body>
<main>
<h1>FuMi-1 <small>文</small></h1>
<p class="lede">Custom firmware for the M-VAVE FM-1, inspired by the Suiko ST-50 (水光トレーナー), the Japanese
poetry-accompaniment instrument. The sixteen white keys are the ST-50's lower row, mi fa la ti do three times
and a top mi; the black keys its upper row or koto ornaments; the key is set in 本数; the tuning switches
between 平均律 and the ST-50's 純正律 as measured from recordings; the sounds are a koto, a second koto,
sho, shakuhachi, two more flutes, a bowed kokyū, a taiko, two wood blocks and four of FuMi's own, each with
its name in kanji on the screen; the trill plays in pairs like a hand; and REC is a looper, as the ST-50's
自動伴奏 was: play a phrase, loop it, play over it.</p>
<div class="shots">
  <img src="img/screen-home.png" width="240" height="240" alt="FuMi-1's screen: 1本, A3, SUIKO tuning, the in scale, and the koto 琴 large">
  <img src="img/screen-play.png" width="240" height="240" alt="FuMi-1's screen with two keys sounding">
  <img src="img/screen-shakuhachi.png" width="240" height="240" alt="FuMi-1's screen on the tuning page with Shakuhachi 尺八">
  <img src="img/screen-tuning.png" width="240" height="240" alt="FuMi-1's tuning page: SUIKO, depth, fine, A = 440">
  <img src="img/screen-loop.png" width="240" height="240" alt="FuMi-1's screen with a loop overdubbing: the DUB tag and the loop bar under the keys">
  <img src="img/screen-trill.png" width="240" height="240" alt="FuMi-1's Trill page: feel, pair gap, ta level, oroshi">
</div>
<div class="status"><strong>Version __VERSION__.</strong> It installs and uninstalls the way Felucca, X0X and
FoMni do, and the installer can put M-VAVE's own firmware back. Installing is at your own risk; read the
notes on the install page first. Your FM-1 should be on M-VAVE's V15 before any custom firmware. FuMi-1 is
quieter than the stock firmware at the same MASTER setting: turn MASTER up.</div>
<nav class="ways" aria-label="Get FuMi-1">
  <a href="emu/"><strong>Try it in the browser</strong><span>The same code the FM-1 runs, with sound. White keys Q–I and A–K, black keys 1–0 and −; no FM-1 needed.</span></a>
  <a href="install/"><strong>Install</strong><span>From Chrome or Edge, with the FM-1 connected by USB. Nothing to install on the computer.</span></a>
  <a href="firmware/__PKG__"><strong>Download __PKG__</strong><span>For the command-line installer: <code>python3 tools/fm1_install.py __PKG__</code></span></a>
  <a href="__REPO__/releases/download/fumi-__SF_VERSION__/FuMi-__SF_VERSION__.sf2"><strong>The SoundFont, FuMi-__SF_VERSION__.sf2</strong><span>The instruments of __SF_VERSION__ as samples for a DAW or any SoundFont player: white keys F3–G5, the Suiko tuning in the sound, dry, CC BY 4.0. 114 MB, from that release; the read-me is docs/soundfont.md in the source.</span></a>
  <a href="bench/"><strong>The audition bench</strong><span>The voicing tool: the engine with a patch editor, DX7 .syx import, blind A/B, morphs, notes and reference clips, all in the browser.</span></a>
  <a href="__REPO__"><strong>Source</strong><span>GitHub, GPL-3.0. Built on FoMni and Felucca; the FM engine is Dexed's msfa.</span></a>
</nav>

<h2>Playing</h2>
<table>
<tr><td>White keys</td><td>mi fa la ti do × 3 and a top mi. The tonics (keys 1, 6, 11, 16) are lit. 三 is key 6.</td></tr>
<tr><td>Black keys</td><td>The upper row fa♯ sol ti♭ do♯ re, five per octave. SEQ switches them to ornaments (below).</td></tr>
<tr><td>SELECT</td><td>本数: 水4, 水3, 水2, 水1, 1 … 12. At 1本, 三 = A3.</td></tr>
<tr><td>PRESETS</td><td>The instrument, twenty-one of them (below); the slider under the keys shows where in the list you are.</td></tr>
<tr><td>ALGORITHM</td><td>The scale: IN 陰, YŌ 陽, MIN'YŌ 民謡.</td></tr>
<tr><td>OCT− · OCT+</td><td>With a note held: a sprung bend down or up (semitone, whole tone or the next scale note). With nothing held: the octave. Both held 5 s: update mode.</td></tr>
<tr><td>LFO · ARP · GLO</td><td>Vibrato on / off; trill on / off (in pairs, ka-ta, the first stroke the hardest); mono / poly. GLO held: the Trill pages.</td></tr>
<tr><td>SEL</td><td>Tap: 平均律 ↔ 純正律. Hold: the tuning page (EQUAL, SUIKO as measured, KOTO pure fifths, USER; depth; 微調; A = 430–445).</td></tr>
<tr><td>KNOB 1–4</td><td>The page's four values. HOME: 余韻, trill rate, vibrato depth, reverb. FX: low cut, high cut, character, reverb size. EDIT: bend target, bend down, bend time, slide time. Trill (GLO held): feel, pair gap, ta level, oroshi; held again: first stroke, jitter.</td></tr>
<tr><td>REC</td><td>The looper. Tap: arm, and the first key starts the loop. Tap again: the loop closes where you are and plays (or it closes itself at 32 beats of 60 BPM). While it plays: overdub on / off. Each layer keeps the instrument it was recorded in, so turn PRESETS and play over it. Hold a second: clear.</td></tr>
<tr><td>PLAY</td><td>Stop / start the loop from the top. Hold a second: undo the last layer.</td></tr>
<tr><td>ENV</td><td>The Loop page: BPM, length in beats, click.</td></tr>
<tr><td>SAVE</td><td>Saves. FuMi-1 also saves by itself a few seconds after a change, once it's quiet.</td></tr>
</table>
<p>MIDI comes in over USB (chromatic, each note at its velocity) and goes out over USB with the strokes' velocities. The patches are
Yamaha DX7-format voices; the built-ins are community patches and FuMi's own, with no factory ROM voice
among them.</p>

<h2>The instruments</h2>
<p>PRESETS steps through them; the screen shows each name with its Japanese name beneath, Suiko-style, and
a voice slider with a notch per instrument. The order follows the ST-50's voice strip by family: the kotos, the
bells, the winds, the sustained voices, percussion. Fifteen are picks from community DX7 banks on patches.fm,
six are FuMi's own. FuMi's own six respond to how hard they are struck.</p>
<table>
<tr><td>1 Koto 琴</td><td>N-KOTO-A5, a community patch. The boot instrument.</td></tr>
<tr><td>2 Koto II 箏</td><td>KOTO-KOTO, a community patch.</td></tr>
<tr><td>3 Koto Pluck 爪音</td><td>FuMi's own, found by a search against measurements of the ST-50's koto.</td></tr>
<tr><td>4 Koto Ring 響</td><td>FuMi's own, brighter.</td></tr>
<tr><td>5 Koto Warm 名残</td><td>FuMi's own, rounder.</td></tr>
<tr><td>6 Harp 箜篌</td><td>FuMi's own.</td></tr>
<tr><td>7 Chimes チャイム</td><td>CHIMES 1, a community patch.</td></tr>
<tr><td>8 Rin 鈴</td><td>TBTAN BELL by Tim Conrardy: the altar bowl, struck and left to ring.</td></tr>
<tr><td>9 Bell ベル</td><td>MICHEL 2 by Tim Conrardy.</td></tr>
<tr><td>10 Ice Bells 氷鈴</td><td>Ice.Bells1 by Tim Garrett.</td></tr>
<tr><td>11 Shakuhachi 尺八</td><td>PAN FL T A, a community patch.</td></tr>
<tr><td>12 Bamboo Flt 篠笛</td><td>Bamboo Flt, a community patch: the shinobue.</td></tr>
<tr><td>13 Dragon Flt 龍笛</td><td>ALTO FLUTE, a community patch: the ryūteki of gagaku.</td></tr>
<tr><td>14 Sho 笙</td><td>Air---*--3, a community patch: the mouth organ.</td></tr>
<tr><td>15 Kokyu 胡弓</td><td>The kokyū, the bowed string, one of the ST-50's own voices: after Viola.Ens by Tim Garrett, the attack slowed so the note swells in.</td></tr>
<tr><td>16 Clarinet クラリネット</td><td>Clarinet12 by Tim Garrett.</td></tr>
<tr><td>17 Strings 弦楽</td><td>STRINGS 2 by Tim Conrardy.</td></tr>
<tr><td>18 Fairytale 御伽</td><td>Fairlyte1a, a community patch: a slow pad that fades as it is held.</td></tr>
<tr><td>19 Taiko 太鼓</td><td>Timpani.2 by Tim Garrett.</td></tr>
<tr><td>20 Hyoshigi 拍子木</td><td>FuMi's own: two hardwood sticks struck together, a dry crack the same on every key. The trill rolls them.</td></tr>
<tr><td>21 Mokugyo 木魚</td><td>FuMi's own: the wooden fish, a round pitched tok.</td></tr>
</table>

<h2>What comes from the ST-50</h2>
<p>The Suiko ST-50 is a 水光 trainer for shigin, the chanted recitation of Chinese and Japanese poems: a
keyboard that gives the singer the scale, the key and a koto or flute under the voice. FuMi-1 keeps its
logic on the FM-1's panel.</p>
<table>
<tr><td>Lower row</td><td>The sixteen white keys: mi fa la ti do three times and a top mi, the tonics lit. 三 is key 6.</td></tr>
<tr><td>Upper row</td><td>The black keys: fa♯ sol ti♭ do♯ re, five per octave.</td></tr>
<tr><td>本数</td><td>SELECT: the key, 水4 to 12本, sixteen semitones. At 1本, 三 = A3.</td></tr>
<tr><td>調律</td><td>SEL: 平均律 or 純正律. The ST-50's 純正律 was measured from recordings: fa 20.8 cents and do 16.0 cents below equal temperament, within a cent across two recordings in two keys. A depth control scales it; KOTO is pure fifths; USER is your own.</td></tr>
<tr><td>余韻</td><td>KNOB 1: the ring after the key is released, 20 ms to 8 s.</td></tr>
<tr><td>トリラー</td><td>ARP, and the trill ornament key: the held note re-plucked, now in pairs.</td></tr>
<tr><td>ビブラート</td><td>LFO, and the vibrato ornament key; KNOB 3 its depth.</td></tr>
<tr><td>単音 / 和音</td><td>GLO: mono (with slide) or poly.</td></tr>
<tr><td>音程 rocker</td><td>OCT− / OCT+ with a note held: a sprung bend down or up.</td></tr>
<tr><td>強弱</td><td>MASTER: play fade-ins on it by hand, as on the ST-50's rocker.</td></tr>
<tr><td>自動伴奏</td><td>REC and PLAY: 記憶, 再生, 停止 as a looper.</td></tr>
<tr><td>Voices</td><td>Of the ST-50's sixteen, FuMi has 琴, 箏, 笙, 尺八, 篠笛, 胡弓 and percussion; the rest are FuMi's own choices.</td></tr>
</table>

<h2>The black keys as ornaments</h2>
<p>Press SEQ and the black keys stop being the upper row and become techniques on the note that sounds.
They are momentary: held, they act; released, they stop. The first four come twice, one set per hand position;
the fifth of each set and the eleventh are the koto's left hand, the techniques a player presses into the
strings behind the bridge.</p>
<table>
<tr><td>Black 1 and 6</td><td>Vibrato while held.</td></tr>
<tr><td>Black 2 and 7</td><td>Trill while held: the held note re-plucked in pairs, the first stroke the hardest. With oroshi set on the Trill page, it starts slow and speeds up (颪, the drum roll of the theatre; the header shows 颪 while it is set); press the key again to start from slow again.</td></tr>
<tr><td>Black 3 and 8</td><td>Damp: every note let go at once, whatever 余韻 says.</td></tr>
<tr><td>Black 4 and 9</td><td>Strong pluck: the next notes struck hard.</td></tr>
<tr><td>Black 5</td><td>押し手 oshide: the sounding notes rise a whole tone over about 80 ms while the key is held, and fall back when it is released.</td></tr>
<tr><td>Black 10</td><td>引き色 hikiiro: while held, each new note dips 40 cents just after the pluck and comes back over 150 ms.</td></tr>
<tr><td>Black 11</td><td>後押し atooshi: while held, each new note is pushed up a semitone 200 ms after it sounds.</td></tr>
</table>
<p>Ornaments recorded in a loop act on the loop's notes; your own ornament keys act on what your hands play.</p>

<h2>The trill</h2>
<p>Strokes come as <em>ka-ta, ka-ta</em>: a short gap inside the pair (42 % of it), a longer one between pairs,
a little jitter mostly on the long gap. The first stroke is the hardest, each <em>ta</em> light, each later
<em>ka</em> normal, and no two strokes alike. Every key you press restarts the pair, so the strokes line up with
your hand. Trill rate on HOME sets the speed; Trill feel (hold GLO) goes from 0, the ST-50's even machine,
to 100, the full pairs and accents. The Trill pages keep the defaults behind the feel knob: pair gap, ta
level, first stroke, jitter, and oroshi for the ornament key: None, Short, Medium or Long, the time the trill
takes from slow to full speed. Pair gap and ta level read as words, Even to Tight and Same to Faint, the bar
beneath keeping the exact position. The ARP button's trill stays plain whatever the setting.</p>

<h2>The looper</h2>
<p>REC arms; the first key you play starts the loop; REC again closes it where you are and it plays (or it
closes itself at 32 beats of 60 BPM, set on the ENV page). REC while it plays overdubs a layer. PLAY stops
and starts from the top; held a second it undoes the last layer; REC held a second clears. Keys, the ornament
keys and the bend buttons are recorded with their timing; knobs and MASTER are not. The loop stores keys, not
pitches, so change 本数 or the tuning and it follows. Each layer keeps the instrument it was recorded in, so
turn PRESETS and play over it. The loop lives in memory until the FM-1 is switched off.</p>
<p>The whole story, the research and the measurements are in the <a href="__REPO__#readme">README</a> and in
<code>docs/research/</code> on GitHub.</p>
<p class="small">To go back to the official firmware, use the installer's "Back to the stock firmware", or
M-VAVE's updater, M-UPGRADE, from <a href="https://www.m-vave.com/download">m-vave.com/download</a>. Suiko
and ST-50 are names of Suikohsha; M-VAVE and FM-1 are trademarks of their owners. FuMi-1 is affiliated with
none of them. Site from commit __SHA__.</p>
</main>
</body>
</html>
"""


def bust(path, version):
    """the scripts and the wasm are fetched by name: give them the version as a query string"""
    v = re.sub(r"[^A-Za-z0-9.-]", "-", version)
    s = path.read_text(encoding="utf-8")
    for name in ("fumi.wasm", "worklet.js", "bench.js", "syx.js", "session.js"):
        s = re.sub(r'(["\'](?:\./)?' + re.escape(name) + r')(["\'])', r"\1?v=" + v + r"\2", s)
    path.write_text(s, encoding="utf-8")


def main(pkg, version, out):
    pkg, out = Path(pkg), Path(out)
    raw = pkg.read_bytes()
    product = product_of(raw)
    if not re.fullmatch(r"FM-1_7\d{6}", product):
        raise SystemExit(f"{pkg}: identity {product!r}: make a release build (./build.sh --release X.Y)")
    if b"FELUCCA-LOADER-1" not in raw:
        raise SystemExit(f"{pkg}: no update loader in it")
    name = f"fumi-{re.sub(r'[^A-Za-z0-9.-]', '-', version)}.fwsc"
    if out.exists():
        shutil.rmtree(out)
    for d in ("install", "firmware"):
        (out / d).mkdir(parents=True)
    shutil.copy(pkg, out / "firmware" / name)
    page = (SRC / "web" / "fumi_installer.html").read_text(encoding="utf-8")
    lib = strip_module((SRC / "web" / "fm1pkg.js").read_text(encoding="utf-8")) + "\n" + \
        strip_module((SRC / "web" / "fm1ota.js").read_text(encoding="utf-8"))
    meta = json.dumps({"version": version, "product": product, "pkg": "../firmware/" + name})
    for mark in ("/*LIB*/", "/*META*/"):
        if page.count(mark) != 1:
            raise SystemExit(f"fumi_installer.html must contain {mark} once")
    (out / "install" / "index.html").write_text(page.replace("/*LIB*/", lib).replace("/*META*/", meta), encoding="utf-8")
    shutil.copytree(SRC / "docs" / "img", out / "img")
    try:
        sha = subprocess.run(["git", "rev-parse", "--short", "HEAD"], cwd=SRC, capture_output=True, text=True).stdout.strip() or "?"
    except OSError:
        sha = "?"
    (out / "index.html").write_text(LANDING.replace("__VERSION__", html.escape(version)).replace("__PKG__", name)
                                    .replace("__REPO__", REPO).replace("__SF_VERSION__", SF_VERSION).replace("__SHA__", sha), encoding="utf-8")
    emu, bench = SRC / "build" / "emu", SRC / "build" / "bench"
    if not (emu / "fumi.wasm").exists() or not (bench / "fumi.wasm").exists():
        raise SystemExit("no build/emu/fumi.wasm or build/bench/fumi.wasm: run web/emu/build.sh (needs Emscripten)")
    (out / "emu").mkdir()
    for f in ("index.html", "worklet.js", "fumi.wasm"):
        shutil.copy(emu / f, out / "emu" / f)
    (out / "bench").mkdir()
    for f in ("index.html", "bench.js", "syx.js", "session.js", "worklet.js", "fumi.wasm"):   # not serve.py: hosted mode
        shutil.copy(bench / f, out / "bench" / f)
    for f in ("emu/index.html", "bench/index.html", "bench/bench.js"):
        bust(out / f, version)
    (out / ".nojekyll").write_text("")
    print(f"site: {out}: index.html, install/ ({product}), emu/, bench/, firmware/{name} ({len(raw)} B)")


if __name__ == "__main__":
    if len(sys.argv) != 4:
        sys.exit(__doc__)
    main(*sys.argv[1:4])
