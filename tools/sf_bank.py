#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""FuMi-1 as a SoundFont 2 bank: every instrument, rendered key by key through FuMi's own engine
(tools/sf_render.py), one preset each, the Suiko tuning in the audio.
  python3 tools/sf_bank.py RENDER_DIR OUT.sf2 VERSION [--date YYYY-MM-DD]
RENDER_DIR holds one folder per instrument, NN-Name/k00.wav .. k15.wav (sf_render.py writes them).

Layout: the sixteen scale notes on the white keys F3..G5 (MIDI 53..79), as on the FM-1, at 1本 (mi = A);
black keys silent. Every zone covers one key and is rooted on that key, so no player resamples: each key
plays its own recording at the recorded rate, and the tuning (fa -20.8, do -16.0 cents) comes with it.
Per kind (docs/soundfont.md): plucked notes kept up to 6 s and cut where they have fallen 60 dB; sustained
voices exactly 4 s, no loop, a short release; percussion up to 3 s, cut at -60 dB, plays out. One gain per
instrument, so the balance between keys is preserved; the sustained voices sit 6 dB lower.
The design and the owner's decisions: docs/research/ (FuMi-soundfont*.md in the owner's folder)."""
import argparse, datetime, struct, sys, wave
from pathlib import Path
import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parent))
import fumi_patches  # noqa: E402

SR = 44100
NKEY = 16
WHITE = [53, 55, 57, 59, 60, 62, 64, 65, 67, 69, 71, 72, 74, 76, 77, 79]        # F3..G5, the FM-1's white keys
DEG = [0, 1, 5, 7, 8]                                                            # mi fa la ti do, semitones from mi
DEG_NAME = ["mi", "fa", "la", "ti", "do"]
SUIKO = {1: -20.8, 8: -16.0}                                                     # the ST-50's 純正律 as measured (tuning.c)
LICENCE = "CC BY 4.0"
URL = "https://github.com/cartesive/fumi-1"

# the kinds, by the firmware's instrument name (sf_test.py checks every instrument has one)
KIND = {"Koto": "pluck", "Koto II": "pluck", "Sho": "sustain", "Shakuhachi": "sustain", "Dragon Flt": "sustain",
        "Bamboo Flt": "sustain", "Kokyu": "sustain", "Taiko": "perc", "Hyoshigi": "perc", "Mokugyo": "perc",
        "Koto Pluck": "pluck", "Koto Ring": "pluck", "Koto Warm": "pluck", "Harp": "pluck",
        # 1.0.8: the bells ring out (Bell holds while the key is down: a sustained voice), the clarinet, the strings and
        # the fairytale pad hold
        "Chimes": "pluck", "Rin": "pluck", "Bell": "sustain", "Ice Bells": "pluck",
        "Clarinet": "sustain", "Strings": "sustain", "Fairytale": "sustain"}
SETTINGS = {
    #            kept at most, cut where it fell to, fade if it reached the limit, fade after a cut, release, peak
    "pluck":   dict(limit=6.0, cut_db=-60.0, fade=0.6, fade_cut=0.03, release=1.3, peak_db=-1.0),
    "sustain": dict(limit=4.0, cut_db=None,  fade=0.4, fade_cut=0.03, release=0.4, peak_db=-7.0),
    "perc":    dict(limit=3.0, cut_db=-60.0, fade=0.6, fade_cut=0.03, release=2.0, peak_db=-1.0),
}
HOLD_MS = {"pluck": 6000, "sustain": 4500, "perc": 3000}                         # how long sf_render.py holds the key

def names():
    """(name, kind) for every firmware instrument, in the firmware's order (= the program numbers)"""
    return [(n, KIND[n]) for n, _, _ in fumi_patches.PATCHES]

def target_cents(k):
    """white key k (0..15) at 1本: cents from A4, the Suiko tuning in"""
    return -2400 + 1200 * (k // 5) + 100 * DEG[k % 5] + SUIKO.get(DEG[k % 5], 0.0)

def target_hz(k, a4=440.0):
    return a4 * 2 ** (target_cents(k) / 1200)

def read_render(path):
    """the host's stereo 16-bit WAV as mono float (-1..1); left and right are the same with the reverb off"""
    with wave.open(str(path)) as w:
        assert w.getframerate() == SR and w.getsampwidth() == 2, path
        x = np.frombuffer(w.readframes(w.getnframes()), "<i2").astype(np.float64) / 32767.0
        if w.getnchannels() == 2:
            x = x.reshape(-1, 2).mean(1)
    return x

def trim(x, kind):
    """the sample kept from a render: from just before the onset, up to the kind's limit, cut where it has
    fallen cut_db below its peak (plucked, percussion), faded at the end so it never stops abruptly"""
    s = SETTINGS[kind]
    peak = np.abs(x).max()
    assert peak > 0, "silent render"
    on = max(0, int(np.argmax(np.abs(x) > 0.01 * peak)) - 64)                   # about 1.5 ms before the onset
    y = x[on:on + int(s["limit"] * SR)].copy()
    assert len(y) >= int(s["limit"] * SR) or kind != "sustain", "a sustained render shorter than its limit"
    fade = s["fade"]
    if s["cut_db"] is not None:
        win = SR // 100                                                            # 10 ms windows
        n = len(y) // win
        env = np.abs(y[:n * win]).reshape(n, win).max(1)
        above = np.nonzero(env > peak * 10 ** (s["cut_db"] / 20))[0]
        last = (int(above[-1]) + 1) * win if len(above) else win
        if last < len(y):
            y = y[:last]
            fade = s["fade_cut"]
    nf = min(len(y), int(fade * SR))
    y[-nf:] *= np.linspace(1.0, 0.0, nf) ** 2
    return y

def tc(seconds):
    """SoundFont timecents"""
    return int(round(1200 * np.log2(seconds)))

def zstr(s, n):
    b = s.encode("ascii")[:n - 1]
    return b + b"\0" * (n - len(b))

def chunk(tag, data):
    return tag + struct.pack("<I", len(data)) + data + (b"\0" if len(data) & 1 else b"")

def info(tag, s):
    b = s.encode("ascii") + b"\0"
    return chunk(tag, b + (b"\0" if len(b) & 1 else b""))

def gen(op, amount):
    return struct.pack("<Hh", op, amount)

def key_range(lo, hi):
    return struct.pack("<HBB", 43, lo, hi)

CREDITS = ("Rendered by FuMi-1's own FM engine (the firmware for the M-VAVE FM-1 after the Suiko ST-50, " + URL + "). "
           "Sixteen scale notes on the white keys F3..G5, mi fa la ti do x 3 and a top mi, at 1 hon (mi = A); black keys "
           "silent. The Suiko tuning (fa -20.8, do -16.0 cents) is in the samples, so transposing keeps it. Dry: add your "
           "own reverb. Programs follow the FM-1's instrument order. "
           "Eight instruments are voiced from community DX7 patches (patches.fm): Koto = N-KOTO-A5 (Aminet 073.syx), "
           "Koto II = KOTO-KOTO (Aminet 072.syx), Sho = Air---*--3 and Dragon Flt = ALTO FLUTE and Bamboo Flt (FLUTE01.SYX, "
           "author unknown), Shakuhachi = PAN FL T A (AAAHGOOD.SYX, author unknown), Kokyu = after Viola.Ens by Tim Garrett "
           "(TX7-80B, attack slowed), Taiko = Timpani.2 by Tim Garrett (TX7-26B). Hyoshigi, Mokugyo, Koto Pluck, Koto Ring, "
           "Koto Warm and Harp are FuMi's own. The FM core is msfa (Dexed, Apache-2.0). No Yamaha ROM voice and no ST-50 "
           "audio is involved. These samples: " + LICENCE + " (credit FuMi-1 and the patch authors above). "
           "Each released SoundFont is frozen: a revoiced instrument comes as a new file under a new version.")

def write_sf2(out, presets, version, date):
    """presets: [(name, [(key, int16 samples, release_s), ...] in key order)], bank 0, programs 0, 1, 2, ..."""
    smpl = b""; shdr = b""; pos = 0
    inst = b""; ibag = b""; igen = b""; ng = 0; nb = 0
    phdr = b""; pbag = b""; pgen = b""
    sid = 0
    for p, (name, zones) in enumerate(presets):
        inst += zstr(name, 20) + struct.pack("<H", nb)
        for i, (key, d, release) in enumerate(zones):
            smpl += d.astype("<i2").tobytes() + b"\0" * (46 * 2)                   # 46 zero samples after each, as the spec asks
            sname = f"{name[:11]} {i + 1:02d}{DEG_NAME[i % 5]}"
            shdr += zstr(sname, 20) + struct.pack("<IIIIIBbHH", pos, pos + len(d), pos + 8, pos + len(d) - 8, SR, key, 0, 0, 1)
            pos += len(d) + 46
            ibag += struct.pack("<HH", ng, 0); nb += 1
            z = key_range(key, key) + gen(58, key) + gen(54, 0) + gen(38, tc(release)) + gen(53, sid)   # root = own key: no retune
            igen += z; ng += len(z) // 4; sid += 1
        phdr += zstr(name, 20) + struct.pack("<HHHIII", p, 0, p, 0, 0, 0)
        pbag += struct.pack("<HH", p, 0); pgen += gen(41, p)
    shdr += zstr("EOS", 20) + struct.pack("<IIIIIBbHH", 0, 0, 0, 0, 0, 0, 0, 0, 0)
    inst += zstr("EOI", 20) + struct.pack("<H", nb)
    ibag += struct.pack("<HH", ng, 0); igen += gen(0, 0)
    phdr += zstr("EOP", 20) + struct.pack("<HHHIII", 0, 0, len(presets), 0, 0, 0)
    pbag += struct.pack("<HH", len(presets), 0); pgen += gen(0, 0)
    zmod = b"\0" * 10
    pdta = b"pdta" + chunk(b"phdr", phdr) + chunk(b"pbag", pbag) + chunk(b"pmod", zmod) + chunk(b"pgen", pgen) + \
        chunk(b"inst", inst) + chunk(b"ibag", ibag) + chunk(b"imod", zmod) + chunk(b"igen", igen) + chunk(b"shdr", shdr)
    INFO = b"INFO" + chunk(b"ifil", struct.pack("<HH", 2, 1)) + info(b"isng", "EMU8000") + info(b"INAM", f"FuMi-1 {version}") + \
        info(b"ICRD", date) + info(b"IENG", "FuMi-1") + info(b"IPRD", "M-VAVE FM-1") + info(b"ICOP", f"{LICENCE}; {URL}") + \
        info(b"ICMT", CREDITS) + info(b"ISFT", "tools/sf_bank.py")
    sdta = b"sdta" + chunk(b"smpl", smpl)
    body = b"sfbk" + chunk(b"LIST", INFO) + chunk(b"LIST", sdta) + chunk(b"LIST", pdta)
    Path(out).write_bytes(b"RIFF" + struct.pack("<I", len(body)) + body)
    return len(body) + 8

def build(render_dir, out, version, names=None, date=None, log=print):
    """the bank from RENDER_DIR/NN-Name/kNN.wav; names: [(name, kind)] in program order (default: the firmware's)"""
    names = names or globals()["names"]()
    date = date or datetime.date.today().isoformat()
    presets = []
    for p, (name, kind) in enumerate(names):
        d = Path(render_dir) / f"{p:02d}-{name}"
        s = SETTINGS[kind]
        ys = [trim(read_render(d / f"k{k:02d}.wav"), kind) for k in range(NKEY)]
        g = 10 ** (s["peak_db"] / 20) * 32767 / max(np.abs(y).max() for y in ys)       # one gain per instrument
        zones = [(WHITE[k], np.round(y * g).astype(np.int16), s["release"]) for k, y in enumerate(ys)]
        secs = sum(len(y) for y in ys) / SR
        log(f"  {p:03d} {name:<10} {kind:<7} {secs:5.1f} s  gain {20 * np.log10(g / 32767):+5.1f} dB  "
            f"shortest {min(len(y) for y in ys) / SR:.2f} s  longest {max(len(y) for y in ys) / SR:.2f} s")
        presets.append((name, zones))
    size = write_sf2(out, presets, version, date)
    log(f"{out}: {size} bytes, {len(presets)} presets, {len(presets) * NKEY} samples")
    return size

def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("render_dir"); ap.add_argument("out"); ap.add_argument("version")
    ap.add_argument("--date", help="ICRD date (default today)")
    a = ap.parse_args(argv)
    build(a.render_dir, a.out, a.version, date=a.date)

if __name__ == "__main__":
    main()
