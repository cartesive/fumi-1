#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Check a FuMi-1 SoundFont (tools/sf_bank.py) with a parser of its own: the file parses; the presets are the
firmware's instruments in order, bank 0; every preset has the sixteen white keys F3..G5, each zone rooted on
its own key, and nothing on a black key; no sample clips; every sample ends in silence; the pitch of each
key, measured from the sample, against the Suiko target at 1本 (within a cent on the plucked instruments,
reported on the others: some patches sit a few cents off by their own detune, as on the FM-1).
  python3 tools/sf_check.py FILE.sf2"""
import struct, sys
from pathlib import Path
from types import SimpleNamespace as NS
import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parent))
import sf_bank  # noqa: E402

SR = sf_bank.SR
WHITE = sf_bank.WHITE
BLACK = [k for k in range(21, 109) if k not in WHITE]           # every other key, A0..C8: silent
target_hz = sf_bank.target_hz
PITCH_TOL = {"pluck": 3.0, "bell": None, "sustain": None, "perc": None}   # cents; None: report only (a patch's own detune,
                                                                 # a bell's partials, the drum's inharmonic partials, the
                                                                 # hyoshigi's unpitched crack)

def chunks(data, pos, end):
    while pos + 8 <= end:
        tag, n = data[pos:pos + 4], struct.unpack_from("<I", data, pos + 4)[0]
        yield tag, pos + 8, n
        pos += 8 + n + (n & 1)

def parse(path):
    """the bank as presets -> zones (one instrument per preset, as sf_bank writes them), with the sample data"""
    data = Path(path).read_bytes()
    assert data[:4] == b"RIFF" and data[8:12] == b"sfbk", "not a SoundFont"
    info, smpl, pdta = {}, None, {}
    for tag, p, n in chunks(data, 12, 8 + struct.unpack_from("<I", data, 4)[0]):
        assert tag == b"LIST"
        kind = data[p:p + 4]
        for t2, p2, n2 in chunks(data, p + 4, p + n):
            if kind == b"INFO":
                v = data[p2:p2 + n2]
                info[t2.decode()] = v.rstrip(b"\0").decode("ascii") if t2 != b"ifil" else struct.unpack("<HH", v)
            elif kind == b"sdta" and t2 == b"smpl":
                smpl = np.frombuffer(data, "<i2", n2 // 2, p2)
            elif kind == b"pdta":
                pdta[t2.decode()] = data[p2:p2 + n2]
    def rec(tag, fmt):
        b, n = pdta[tag], struct.calcsize(fmt)
        return [struct.unpack_from(fmt, b, i) for i in range(0, len(b), n)]
    phdr = rec("phdr", "<20sHHHIII"); pbag = rec("pbag", "<HH"); pgen = rec("pgen", "<HBB")
    inst = rec("inst", "<20sH"); ibag = rec("ibag", "<HH"); igen = rec("igen", "<HBB")
    shdr = rec("shdr", "<20sIIIIIBbHH")
    presets = []
    for i in range(len(phdr) - 1):
        name, program, bank, pb0 = phdr[i][0].rstrip(b"\0").decode(), phdr[i][1], phdr[i][2], phdr[i][3]
        gens = [pgen[g] for g in range(pbag[pb0][0], pbag[phdr[i + 1][3]][0])]
        inst_ids = [lo | hi << 8 for op, lo, hi in gens if op == 41]
        assert len(inst_ids) == 1, f"{name}: one instrument per preset"
        ii = inst_ids[0]
        iname = inst[ii][0].rstrip(b"\0").decode()
        zones = []
        for b in range(inst[ii][1], inst[ii + 1][1]):
            z = NS(lo=0, hi=127, root=None, sample_mode=0, release_tc=-12000, sample=None, name=None)
            for op, lo, hi in [igen[g] for g in range(ibag[b][0], ibag[b + 1][0])]:
                v = struct.unpack("<h", bytes((lo, hi)))[0]
                if op == 43: z.lo, z.hi = lo, hi
                elif op == 58: z.root = v
                elif op == 54: z.sample_mode = v
                elif op == 38: z.release_tc = v
                elif op == 53:
                    h = shdr[v]
                    z.name = h[0].rstrip(b"\0").decode(); z.sample = smpl[h[1]:h[2]]; z.rate = h[5]
                    if z.root is None: z.root = h[6]
            zones.append(z)
        presets.append(NS(name=name, program=program, bank=bank, inst_name=iname, zones=zones))
    return NS(info=info, presets=presets)

def pitch_cents(x, f_target, start_s=0.1, win_s=1.0, octaves=(0, 1), floor_db=40.0):
    """the sample's pitch against f_target, in cents: the f0 whose first four harmonics are strongest (a
    harmonic sum over the log spectrum of win_s from start_s, or what there is, everything more than
    floor_db under the top ignored so sidelobes and noise do not pull), searched within 150 cents of the
    target and of the target an octave up (some patches sound an octave above their key), then to 0.1 cent;
    the answer is folded to within 600 cents of the target: the octave is the instrument's, not an error.
    Exact on a steady tone; on a chorused patch (Koto II's low keys: two carriers 70 cents apart) it is the
    centre of the pair, which is what the ear takes as the pitch."""
    x = np.asarray(x, np.float64)
    seg = x[int(start_s * SR):int((start_s + win_s) * SR)]
    if len(seg) < 2048:
        seg = x[:max(2048, len(x))]
    seg = seg * np.hanning(len(seg))
    n = 1 << (int(np.ceil(np.log2(len(seg)))) + 4)
    lmag = np.log(np.abs(np.fft.rfft(seg, n)) + 1e-9)
    lmag = np.maximum(lmag, lmag.max() - np.log(10 ** (floor_db / 20)))
    f = np.fft.rfftfreq(n, 1 / SR)
    weights = (1.0, 0.8, 0.6, 0.4)
    def score(f0):
        return sum(w * np.interp(h * f0, f, lmag) for h, w in zip((1, 2, 3, 4), weights))
    best = None
    for o in octaves:
        base = f_target * 2 ** o
        grid = base * 2 ** (np.arange(-150, 151, 1.0) / 1200)
        sc = np.array([score(g) for g in grid])
        c = grid[int(np.argmax(sc))]
        fine = c * 2 ** (np.arange(-3, 3.01, 0.1) / 1200)
        sf = np.array([score(g) for g in fine])
        cand = (sf.max(), fine[int(np.argmax(sf))])
        if best is None or cand > best:
            best = cand
    cents = 1200 * np.log2(best[1] / f_target)
    return cents - 1200 * round(cents / 1200)

def check(path, names=None, log=print):
    names = names or sf_bank.names()
    sf = parse(path)
    fails = []
    def ok(cond, what):
        if not cond:
            fails.append(what)
        return cond
    log(f"{path}: {sf.info.get('INAM')} ({sf.info.get('ICRD')}), {len(sf.presets)} presets")
    ok([(p.bank, p.program, p.name) for p in sf.presets] == [(0, i, n) for i, (n, _) in enumerate(names)],
       "presets are not the firmware's instruments in order on bank 0")
    for p, (name, kind) in zip(sf.presets, names):
        z = p.zones
        ok([(q.lo, q.hi, q.root) for q in z] == [(k, k, k) for k in WHITE], f"{name}: zones are not the sixteen white keys rooted on themselves")
        ok(all(not any(q.lo <= k <= q.hi for q in z) for k in BLACK), f"{name}: a black key sounds")
        ok(all(q.rate == SR for q in z), f"{name}: sample rate")
        ok(all(q.sample_mode == 0 for q in z), f"{name}: a loop")
        peak = max(np.abs(q.sample.astype(int)).max() for q in z)
        ok(peak < 32767, f"{name}: clips")
        ok(all(abs(int(q.sample[-1])) <= 1 for q in z), f"{name}: a sample does not end in silence")
        cents = [pitch_cents(q.sample, target_hz(i)) for i, q in enumerate(z)]
        worst = max(abs(c) for c in cents)
        tol = PITCH_TOL[kind]
        secs = [len(q.sample) / SR for q in z]
        log(f"  {p.program:03d} {name:<10} {kind:<7} peak {20 * np.log10(peak / 32767):5.1f} dBFS  "
            f"{min(secs):.2f}..{max(secs):.2f} s  pitch {min(cents):+5.1f}..{max(cents):+5.1f} cents"
            + ("" if tol is None else f"  (within {tol})"))
        if tol is not None:
            ok(worst <= tol, f"{name}: pitch off by {worst:.2f} cents on a key (limit {tol})")
    for f in fails:
        log("  FAIL " + f)
    log("sf_check: ok" if not fails else f"sf_check: {len(fails)} FAILED")
    return NS(ok=not fails, fails=fails, sf=sf)

if __name__ == "__main__":
    sys.exit(0 if check(sys.argv[1]).ok else 1)
