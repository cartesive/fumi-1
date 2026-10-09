#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Publish the browser emulator and the audition bench to GitHub Pages (the gh-pages branch of origin): a small
# landing page, build/emu and build/bench (without serve.py: on the site the bench takes .syx and recordings
# from the visitor's disk and keeps everything in the browser). The full site (installer, firmware download)
# is M7 work: tools/make_pages.py still carries FoMni's text and is not used here.
#   tools/publish_emu.sh            -> https://cartesive.github.io/fumi-1/
set -e
cd "$(dirname "$0")/.."
sh web/emu/build.sh
OUT=build/pages
rm -rf "$OUT"
mkdir -p "$OUT/emu" "$OUT/bench"
cp build/emu/index.html build/emu/worklet.js build/emu/fumi.wasm "$OUT/emu/"
cp build/bench/index.html build/bench/bench.js build/bench/syx.js build/bench/session.js build/bench/worklet.js build/bench/fumi.wasm "$OUT/bench/"
SHA="$(git rev-parse --short HEAD)"
DATE="$(date +%Y-%m-%d)"
cat > "$OUT/index.html" <<HTML
<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>FuMi-1</title>
<meta name="description" content="FuMi-1: custom firmware for the M-VAVE FM-1 inspired by the Suiko ST-50 shigin conductor. Play it in the browser.">
<style>
body { max-width: 44rem; margin: 2rem auto; padding: 0 1rem; font: 17px/1.55 -apple-system, "Helvetica Neue", Arial, sans-serif; color: #3b2f28; background: #faf4e8; }
h1 { font-size: 2rem; margin-bottom: .2rem; }
a { color: #c44830; }
.go { display: inline-block; margin: 1rem 0; padding: .7em 1.4em; border-radius: 999px; background: #c44830; color: #fff; text-decoration: none; font-weight: 600; }
.muted { color: #8a7764; font-size: .9rem; }
</style>
</head>
<body>
<h1>FuMi-1</h1>
<p>Custom firmware for the M-VAVE FM-1, inspired by the Suiko ST-50 (水光トレーナー), the Japanese
poetry-accompaniment instrument. The sixteen white keys are the ST-50's lower row, mi fa la ti do three times
and a top mi; the black keys its upper row or koto ornaments; the key is set in 本数; the tuning switches between
平均律 and the ST-50's 純正律 as measured from recordings; the koto is a 6-operator FM voice.</p>
<a class="go" href="emu/">Play it in your browser</a>
<p>It is the firmware itself running in the page, at the FM-1's 44.1 kHz. White keys <b>Q–I</b> and <b>A–K</b>,
black keys <b>1–0 −</b>, <b>Z</b>/<b>X</b> OCT−/OCT+ (a sprung bend while a note is held), <b>Shift</b> vibrato,
<b>/</b> trill, arrows turn SELECT (本数).</p>
<p><a href="bench/">The audition bench</a> is the voicing tool: the same engine, with a patch editor, DX7
<code>.syx</code> import, blind A/B, morphs and hybrids, notes and ratings, and reference clips matched to the
engine's loudness. It reads <code>.syx</code> banks and recordings from your own disk and keeps them in your
browser; nothing is uploaded. A session file exported from it is what the voicing rounds work from.</p>
<p><b>Status:</b> early. The pitch core, the engine and the audition bench are done on the host; the koto voice is
being auditioned; nothing has been flashed to an FM-1 yet, and there is no installer page. Source, research and
plan: <a href="https://github.com/cartesive/fumi-1">github.com/cartesive/fumi-1</a> (GPL-3.0).</p>
<p class="muted">Built on <a href="https://github.com/charlesvestal/fm1-omnichord">FoMni</a>'s platform, from
<a href="https://github.com/hugelton/Felucca">Felucca</a>. Suiko and ST-50 are names of Suikohsha; FuMi-1 is not
affiliated with Suikohsha, M-VAVE or Hügelton Instruments. Site from commit $SHA, $DATE.</p>
</body>
</html>
HTML
REMOTE="$(git remote get-url origin)"
cd "$OUT"
rm -rf .git
git init -q -b gh-pages
git add -A
git -c user.name=cartesive -c user.email=claude.code@beatgroover.com commit -q -m "Emulator and bench site (from $SHA)"
git push -q -f "$REMOTE" gh-pages
rm -rf .git
REPO="$(printf %s "$REMOTE" | sed -E 's#(git@github.com:|https://github.com/)##; s#\.git$##')"
gh api -X POST "repos/$REPO/pages" -f 'source[branch]=gh-pages' -f 'source[path]=/' >/dev/null 2>&1 || \
    gh api -X POST "repos/$REPO/pages/builds" >/dev/null 2>&1 || echo "(could not ask GitHub Pages to build: check the repository's Pages settings)"
echo "pushed the emulator and bench site from $SHA to gh-pages"
