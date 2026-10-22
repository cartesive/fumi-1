#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""A patches.fm DX7 voice (its JSON: https://patches.fm/patches/dx7/<first two>/<hash>.json) as an entry for
tools/fumi_patches.py, ready to paste: the op(...) lines and the voice(...) tail, with the credit line.
  python3 tools/patchfm_import.py FILE.json NAME 名 [--selftest]
The engine ignores a patch's transpose, so a voice written an octave up (transpose 36) or down (12) has its
ratio operators' frequencies doubled or halved here (fixed-frequency operators stay); the fine part is
rounded, which is exact for even coarse values and within about a third of a percent (6 cents on that partial) otherwise."""
import json, sys

CURVE = {"-LIN": 0, "-EXP": 1, "+EXP": 2, "+LIN": 3}
WAVE = {"triangle": 0, "saw down": 1, "saw up": 2, "square": 3, "sine": 4, "sample and hold": 5}

def ratio_of(coarse, fine):
    return (0.5 if coarse == 0 else coarse) * (1 + fine / 100)

def coarse_fine(ratio):
    """the nearest DX7 coarse/fine for a ratio >= 0.5"""
    best = None
    for c in range(32):
        base = 0.5 if c == 0 else c
        f = int(round((ratio / base - 1) * 100))
        if 0 <= f <= 99:
            got = ratio_of(c, f)
            if best is None or abs(got - ratio) < abs(best[2] - ratio):
                best = (c, f, got)
    assert best is not None, ratio
    return best[0], best[1]

def convert(d):
    """(ops: [dict for op()], voice kwargs) in OP1..OP6 order (patches.fm lists OP1 first)"""
    shift = {24: 1.0, 36: 2.0, 12: 0.5}.get(d["transpose"], None)
    assert shift is not None, f"transpose {d['transpose']}: not an octave from 24"
    ops = []
    for o in d["operators"]:
        eg, kb, osc = o["envelope_generator"], o["keyboard"], o["oscillator"]
        ls = kb["level_scaling"]
        coarse, fine = osc["frequency"]["coarse"], osc["frequency"]["fine"]
        mode = 0 if osc["mode"] == "ratio" else 1
        if mode == 0 and shift != 1.0:
            coarse, fine = coarse_fine(ratio_of(coarse, fine) * shift)
        ops.append(dict(r=tuple(eg["rates"]), l=tuple(eg["levels"]), ol=o["output_level"], fc=coarse, ff=fine,
                        det=osc["detune"] + 7, mode=mode, kvs=kb["velocity_sensitivity"], ams=o["amp_mod_sensitivity"],
                        rs=kb["rate_scaling"], bp=ls["break_point"], ld=ls["left_depth"], rd=ls["right_depth"],
                        lc=CURVE[ls["left_curve"]], rc=CURVE[ls["right_curve"]]))
    peg, lfo = d["pitch_envelope_generator"], d["lfo"]
    kw = dict(alg=d["algorithm"], fb=d["feedback"], oks=int(bool(d["oscillator_key_sync"])), pr=tuple(peg["rates"]),
              pl=tuple(peg["levels"]), lfs=lfo["speed"], lfd=lfo["delay"], lpmd=lfo["pitch_mod_depth"],
              lamd=lfo["amp_mod_depth"], lks=int(bool(lfo["key_sync"])), lfw=WAVE[lfo["waveform"]],
              lpms=d["pitch_mod_sensitivity"], trnsp=24)
    return ops, kw

DEFAULTS = dict(ol=0, fc=1, ff=0, det=7, mode=0, kvs=0, ams=0, rs=0, bp=39, ld=0, rd=0, lc=0, rc=0)

def entry(d, name, ja):
    ops, kw = convert(d)
    shift = d["transpose"]
    note = {36: ", written an octave up: ratios doubled", 12: ", written an octave down: ratios halved"}.get(shift, "")
    L = [f"    ({name!r}, {ja!r}, voice({name!r}, {kw['alg']}, [  # was {d['NAME'].strip()}   # patches.fm: {d['AUTHOR']}, {d['BANK']}{note}"]
    for o in ops:
        parts = [f"r={o['r']}", f"l={o['l']}"] + [f"{k}={o[k]}" for k in DEFAULTS if o[k] != DEFAULTS[k]]
        L.append("        op(" + ", ".join(parts) + "),")
    tail = ", ".join(f"{k}={v}" for k, v in kw.items() if k != "alg")
    L.append(f"    ], {tail})),")
    return "\n".join(L)

def selftest():
    d = {"transpose": 12, "algorithm": 5, "feedback": 3, "oscillator_key_sync": False,
         "pitch_envelope_generator": {"rates": [99, 99, 99, 99], "levels": [50, 50, 50, 50]},
         "lfo": {"speed": 35, "delay": 91, "pitch_mod_depth": 1, "amp_mod_depth": 2, "key_sync": False, "waveform": "triangle"},
         "pitch_mod_sensitivity": 4, "NAME": "Ice.Bells1", "AUTHOR": "Tim Garrett", "BANK": "TX7-33.SYX",
         "operators": [{"envelope_generator": {"rates": [95, 30, 20, 40], "levels": [99, 80, 0, 0]},
                        "keyboard": {"level_scaling": {"break_point": 39, "left_depth": 0, "left_curve": "-LIN", "right_depth": 20, "right_curve": "+EXP"},
                                     "rate_scaling": 2, "velocity_sensitivity": 2},
                        "oscillator": {"detune": -3, "frequency": {"fine": 29, "coarse": 9}, "mode": "ratio"},
                        "amp_mod_sensitivity": 1, "output_level": 60}] * 5 +
                      [{"envelope_generator": {"rates": [99, 99, 99, 99], "levels": [99, 99, 99, 0]},
                        "keyboard": {"level_scaling": {"break_point": 0, "left_depth": 0, "left_curve": "-LIN", "right_depth": 0, "right_curve": "-LIN"},
                                     "rate_scaling": 0, "velocity_sensitivity": 0},
                        "oscillator": {"detune": 0, "frequency": {"fine": 79, "coarse": 3}, "mode": "fixed"},
                        "amp_mod_sensitivity": 0, "output_level": 49}]}
    ops, kw = convert(d)
    o = ops[0]
    assert (o["fc"], o["ff"]) == (4, 45), (o["fc"], o["ff"])          # 9 x 1.29 = 11.61, halved 5.805 = 4 x 1.45
    assert o["det"] == 4 and o["rc"] == 2 and o["lc"] == 0 and o["kvs"] == 2 and o["rs"] == 2 and o["rd"] == 20
    assert ops[5]["mode"] == 1 and (ops[5]["fc"], ops[5]["ff"]) == (3, 79)   # fixed: not transposed
    assert kw["oks"] == 0 and kw["lks"] == 0 and kw["lfw"] == 0 and kw["lpms"] == 4 and kw["trnsp"] == 24 and kw["fb"] == 3
    assert coarse_fine(2.0 * 1.18) == (2, 18) and coarse_fine(0.5 * 2) == (1, 0) and coarse_fine(1.0) == (1, 0)
    d["transpose"] = 36
    assert (convert(d)[0][0]["fc"], convert(d)[0][0]["ff"]) == (18, 29)   # 9.29 doubled: coarse doubles, fine stays
    assert "was Ice.Bells1" in entry(d, "Ice Bells", "氷鈴") and "op(r=(95, 30, 20, 40)" in entry(d, "Ice Bells", "氷鈴")
    print("patchfm_import: selftest ok")

if __name__ == "__main__":
    if "--selftest" in sys.argv:
        selftest()
    else:
        print(entry(json.load(open(sys.argv[1])), sys.argv[2], sys.argv[3]))
