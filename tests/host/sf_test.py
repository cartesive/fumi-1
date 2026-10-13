#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""The SoundFont tools (tools/sf_bank.py, tools/sf_check.py): synthetic renders of three instruments, one of
each kind, through the bank builder, read back by the checker's own parser.
  python3 tests/host/sf_test.py"""
import shutil, struct, sys, tempfile, wave
from pathlib import Path
import numpy as np

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))
import sf_bank, sf_check  # noqa: E402

SR = 44100
fails = 0

def check(name, ok):
    global fails
    print(("  ok   " if ok else "  FAIL ") + name)
    fails += not ok

def tone(f, secs, decay_s=None, lead_s=0.03, level=0.11):
    """a stereo 16-bit 'render' as the host writes it: a lead-in of silence, then a tone, quiet like FuMi"""
    t = np.arange(int(secs * SR)) / SR
    x = np.sin(2 * np.pi * f * t) + 0.3 * np.sin(4 * np.pi * f * t)
    if decay_s:
        x *= np.exp(-t / decay_s)
    x = np.concatenate([np.zeros(int(lead_s * SR)), x * level])
    s = np.round(x * 32767).astype("<i2")
    return np.repeat(s, 2)

def write_wav(path, s):
    with wave.open(str(path), "wb") as w:
        w.setnchannels(2); w.setsampwidth(2); w.setframerate(SR)
        w.writeframes(s.tobytes())

def main():
    names = [("Koto", "pluck"), ("Sho", "sustain"), ("Taiko", "perc")]
    tmp = Path(tempfile.mkdtemp(prefix="sf_test_"))
    try:
        for i, (name, kind) in enumerate(names):
            d = tmp / "render" / f"{i:02d}-{name}"
            d.mkdir(parents=True)
            for k in range(16):
                f = sf_bank.target_hz(k)
                s = tone(f, 8.0, decay_s=0.5 if kind == "pluck" else 0.2 if kind == "perc" else None)
                write_wav(d / f"k{k:02d}.wav", s)
        out = tmp / "test.sf2"
        sf_bank.build(tmp / "render", out, "9.9.9", names=names, date="2026-10-13")
        check("the bank is written", out.exists() and out.stat().st_size > 100000)

        sf = sf_check.parse(out)
        check("three presets, bank 0, programs 0 1 2, in order, named as the firmware",
              [(p.bank, p.program, p.name) for p in sf.presets] == [(0, i, n) for i, (n, _) in enumerate(names)])
        check("file info names the FuMi version", "9.9.9" in sf.info.get("INAM", "") and "2026-10-13" == sf.info.get("ICRD"))
        check("the comment carries the tuning, the layout and the licence",
              all(w in sf.info.get("ICMT", "") for w in ("-20.8", "-16.0", "white", "CC BY 4.0")))
        for p in sf.presets:
            zones = p.zones
            check(f"{p.name}: sixteen zones on the white keys F3..G5, each rooted on its own key",
                  [(z.lo, z.hi, z.root) for z in zones] == [(k, k, k) for k in sf_check.WHITE])
            check(f"{p.name}: every black key is silent", all(not any(z.lo <= k <= z.hi for z in zones) for k in sf_check.BLACK))
            peak = max(np.abs(z.sample).max() for z in zones)
            want = sf_bank.SETTINGS[dict(names)[p.name]]["peak_db"]
            check(f"{p.name}: peak {20 * np.log10(peak / 32767):.2f} dBFS (want {want})", abs(20 * np.log10(peak / 32767) - want) < 0.1)
            rel = sf_bank.SETTINGS[dict(names)[p.name]]["release"]
            check(f"{p.name}: release {rel} s", all(abs(2 ** (z.release_tc / 1200) - rel) < 0.01 * rel for z in zones))
            check(f"{p.name}: no loop", all(z.sample_mode == 0 for z in zones))
            check(f"{p.name}: every sample ends in silence (faded)", all(abs(int(z.sample[-1])) <= 1 for z in zones))
            cents = [sf_check.pitch_cents(z.sample, sf_check.target_hz(i)) for i, z in enumerate(zones)]
            check(f"{p.name}: pitch within 0.5 cents of the Suiko target on every key (worst {max(map(abs, cents)):.2f})", max(map(abs, cents)) < 0.5)
            check(f"{p.name}: the onset is kept (sound within 3 ms of the start)", all(np.abs(z.sample[:132]).max() > 0.1 * np.abs(z.sample).max() for z in zones))
        lens = {p.name: [len(z.sample) / SR for z in p.zones] for p in sf.presets}
        # the synthetic pluck is at -60 dB after 0.5 * ln(1000) = 3.45 s: cut there, not at the 6 s limit
        check(f"pluck: cut where it fell 60 dB, about 3.45 s ({lens['Koto'][0]:.2f} s)", all(3.3 < s < 3.7 for s in lens["Koto"]))
        check(f"sustain: exactly 4 s ({lens['Sho'][0]:.2f} s)", all(abs(s - 4.0) < 0.002 for s in lens["Sho"]))
        check(f"perc: cut at -60 dB, about 1.38 s ({lens['Taiko'][0]:.2f} s)", all(1.25 < s < 1.5 for s in lens["Taiko"]))
        report = sf_check.check(out, names=names)
        check("sf_check passes its own bank", report.ok)
        # the real instrument list: every firmware instrument has a kind
        import fumi_patches
        check("every firmware instrument has a kind in sf_bank.KIND", all(n in sf_bank.KIND for n, _, _ in fumi_patches.PATCHES))
        check("sf_bank.names() follows the firmware's order", [n for n, _ in sf_bank.names()] == [n for n, _, _ in fumi_patches.PATCHES])
    finally:
        shutil.rmtree(tmp)
    print("sf tools: all checks passed" if not fails else f"sf tools: {fails} FAILED")
    return 1 if fails else 0

if __name__ == "__main__":
    sys.exit(main())
