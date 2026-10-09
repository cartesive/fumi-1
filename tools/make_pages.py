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
between 平均律 and the ST-50's 純正律 as measured from recordings; and the sounds are a koto, a second koto,
sho, shakuhachi, two more flutes and a taiko, each with its name in kanji on the screen.</p>
<div class="shots">
  <img src="img/screen-home.png" width="240" height="240" alt="FuMi-1's screen: 1本, A3, the koto 琴, SUIKO tuning, the in scale">
  <img src="img/screen-play.png" width="240" height="240" alt="FuMi-1's screen with two keys sounding">
  <img src="img/screen-tuning.png" width="240" height="240" alt="FuMi-1's tuning page: SUIKO, depth, fine, A = 440">
</div>
<div class="status"><strong>Version __VERSION__, the first release.</strong> It installs and uninstalls the way
Felucca, X0X and FoMni do, and the installer can put M-VAVE's own firmware back. Installing is at your own
risk; read the notes on the install page first. Your FM-1 should be on M-VAVE's V15 before any custom
firmware.</div>
<nav class="ways" aria-label="Get FuMi-1">
  <a href="emu/"><strong>Try it in the browser</strong><span>The same code the FM-1 runs, with sound. White keys Q–I and A–K, black keys 1–0; no FM-1 needed.</span></a>
  <a href="install/"><strong>Install</strong><span>From Chrome or Edge, with the FM-1 connected by USB. Nothing to install on the computer.</span></a>
  <a href="firmware/__PKG__"><strong>Download __PKG__</strong><span>For the command-line installer: <code>python3 tools/fm1_install.py __PKG__</code></span></a>
  <a href="bench/"><strong>The audition bench</strong><span>The voicing tool: the engine with a patch editor, DX7 .syx import, blind A/B, morphs, notes and reference clips, all in the browser.</span></a>
  <a href="__REPO__"><strong>Source</strong><span>GitHub, GPL-3.0. Built on FoMni and Felucca; the FM engine is Dexed's msfa.</span></a>
</nav>

<h2>Playing</h2>
<table>
<tr><td>White keys</td><td>mi fa la ti do × 3 and a top mi. The tonics (keys 1, 6, 11, 16) are lit. 三 is key 6.</td></tr>
<tr><td>Black keys</td><td>The upper row fa♯ sol ti♭ do♯ re, five per octave. SEQ switches them to ornaments: vibrato, trill, damp, strong pluck.</td></tr>
<tr><td>SELECT</td><td>本数: 水4, 水3, 水2, 水1, 1 … 12. At 1本, 三 = A3.</td></tr>
<tr><td>PRESETS</td><td>The instrument: Koto 琴, Koto II 箏, Sho 笙, Shakuhachi 尺八, Dragon Flt 龍笛, Bamboo Flt 篠笛, Taiko 太鼓, and FuMi's own Koto Pluck 爪音, Koto Ring 響, Koto Warm 名残, Harp 箜篌.</td></tr>
<tr><td>ALGORITHM</td><td>The scale: IN 陰, YŌ 陽, MIN'YŌ 民謡.</td></tr>
<tr><td>OCT− · OCT+</td><td>With a note held: a sprung bend down or up (semitone, whole tone or the next scale note). With nothing held: the octave. Both held 5 s: update mode.</td></tr>
<tr><td>LFO · ARP · GLO</td><td>Vibrato, trill, mono (slide between notes).</td></tr>
<tr><td>SEL</td><td>Tap: 平均律 ↔ 純正律. Hold: the tuning page (EQUAL, SUIKO as measured, KOTO pure fifths, USER; depth; 微調; A = 430–445).</td></tr>
<tr><td>KNOB 1–4</td><td>The page's four values. HOME: 余韻, trill rate, vibrato depth, reverb. FX: low cut, high cut, character, reverb size. EDIT: bend target, bend down, bend time, slide time.</td></tr>
<tr><td>SAVE</td><td>Saves. FuMi-1 also saves by itself a few seconds after a change, once it's quiet.</td></tr>
</table>
<p>MIDI comes in over USB (chromatic) and goes out over USB. The patches are Yamaha DX7-format voices; the
built-ins are community patches and FuMi's own, with no factory ROM voice among them.</p>
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
                                    .replace("__REPO__", REPO).replace("__SHA__", sha), encoding="utf-8")
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
