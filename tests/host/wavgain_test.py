#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""The host simulator's `wavgain G` script command (for the SoundFont renders, tools/sf_render.py): the output
is multiplied by G before the 16-bit WAV, and clipped (not wrapped) at full scale. FuMi's output is quiet by
design (OUT_TRIM, -21 dB), so a plain 16-bit recording wastes bits.
  python3 tests/host/wavgain_test.py        (after host/build_host.sh)"""
import subprocess, sys, wave
from pathlib import Path
import numpy as np

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "build" / "host" / "wavgain"
SCRIPT = """wait 300
turn K4 -70 40
expect par26 0
master 4096
wav g1.wav
tapkey w5
wait 1500
wavstop
wavgain 8
reboot
wait 300
turn K4 -70 40
expect par26 0
master 4096
wav g8.wav
tapkey w5
wait 1500
wavstop
wavgain 1000
reboot
wait 300
turn K4 -70 40
expect par26 0
master 4096
wav gclip.wav
tapkey w5
wait 1500
wavstop
"""

def read(name):
    with wave.open(str(OUT / name)) as w:
        return np.frombuffer(w.readframes(w.getnframes()), "<i2").astype(np.int64)

def onset(x):
    return int(np.argmax(np.abs(x) > 0.01 * np.abs(x).max()))

def main():
    OUT.mkdir(parents=True, exist_ok=True)
    (OUT / "wavgain.fumi").write_text(SCRIPT)
    r = subprocess.run([str(ROOT / "build/host/fumi_host"), str(OUT / "wavgain.fumi"), str(OUT)], capture_output=True, text=True)
    print(r.stdout.strip())
    fails = 0
    def check(name, ok):
        nonlocal fails
        print(("  ok   " if ok else "  FAIL ") + name)
        fails += not ok
    check("host ran the script", r.returncode == 0)
    if r.returncode != 0:
        return 1
    g1, g8, gc = read("g1.wav"), read("g8.wav"), read("gclip.wav")
    p1, p8 = np.abs(g1).max(), np.abs(g8).max()
    check(f"gain 1 peak {p1} is FuMi's quiet output (under -12 dBFS)", 0 < p1 < 8192)
    check(f"gain 8 peak {p8} is 8 x the gain-1 peak (within 1 %)", abs(p8 / p1 - 8) < 0.08)
    a, b = onset(g1), onset(g8)                       # the key lands on a different audio block each time
    n = 44100 * 2
    err = np.abs(g8[b:b + n] - 8 * g1[a:a + n]).max()
    check(f"gain 8 is the same recording from the onset, sample for sample (max error {err} LSB)", err <= 8)
    note = gc[np.argmax(np.abs(gc) > 0):][:int(0.5 * 44100 * 2)]
    full = np.mean(np.abs(note) == 32767)
    check(f"gain 1000 clips at full scale rather than wrapping ({full:.0%} of the first 500 ms at 32767)", full > 0.5)
    check("gain 1000 never exceeds full scale", np.abs(gc).max() <= 32767)
    print("wavgain: all checks passed" if not fails else f"wavgain: {fails} FAILED")
    return 1 if fails else 0

if __name__ == "__main__":
    sys.exit(main())
