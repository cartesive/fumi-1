#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# The voice() / op() / pack() helpers follow Felucca's tools/gen_fm6_patches.py, Copyright (C) 2026 Leo
# Kuroshita (@kurogedelic), Hügelton Instruments. The patches are FuMi-1's own.
"""FuMi-1's own FM6 patches -> firmware/src/dsp/fumi_patches.h (packed 128-byte voices, the DX7 bank record).

  tools/fumi_patches.py firmware/src/dsp/fumi_patches.h

No factory ROM data of any instrument is in here: these are starting points written as readable operator
settings from what the ST-50's koto measures like (docs/research/13: a click in the first 12 ms, a body of
partials 1 to 4, a two-stage decay, steady pitch). The audition bench (web/bench) is where they get better;
a finalist is written back here as numbers.

Operators are listed OP1..OP6; the record starts with OP6, as the format does. Algorithm 5 is three
carrier-modulator pairs (1<-2, 3<-4, 5<-6 with the feedback): FuMi's three stacks: A click, B body, C fullness.
"""
import sys
from pathlib import Path


def op(r=(99, 99, 99, 99), l=(99, 99, 99, 0), ol=0, fc=1, ff=0, det=7, mode=0, kvs=0, ams=0, rs=0,
       bp=39, ld=0, rd=0, lc=0, rc=0):
    return dict(r=r, l=l, ol=ol, fc=fc, ff=ff, det=det, mode=mode, kvs=kvs, ams=ams, rs=rs, bp=bp, ld=ld, rd=rd,
                lc=lc, rc=rc)


def voice(name, alg, ops, fb=0, oks=1, pr=(99, 99, 99, 99), pl=(50, 50, 50, 50), lfs=35, lfd=0, lpmd=0, lamd=0,
          lks=1, lfw=0, lpms=3, trnsp=24):
    """the 155-byte single-voice layout (OP6 first); alg 1..32"""
    assert len(ops) == 6 and 1 <= alg <= 32 and len(name) <= 10
    v = []
    for o in reversed(ops):                        # OP6 .. OP1
        v += list(o["r"]) + list(o["l"]) + [o["bp"], o["ld"], o["rd"], o["lc"], o["rc"], o["rs"], o["ams"],
                                            o["kvs"], o["ol"], o["mode"], o["fc"], o["ff"], o["det"]]
    v += list(pr) + list(pl) + [alg - 1, fb, oks, lfs, lfd, lpmd, lamd, lks, lfw, lpms, trnsp]
    v += [ord(c) for c in name.ljust(10)]
    assert len(v) == 155
    return v


def pack(v):
    """155 -> the 128-byte record (the C side: fm6_patch.c fm6_pack)"""
    b = []
    for k in range(6):
        o = v[k * 21:k * 21 + 21]
        b += o[0:11]
        b += [(o[11] & 3) | (o[12] & 3) << 2, (o[13] & 7) | (o[20] & 15) << 3, (o[14] & 3) | (o[15] & 7) << 2,
              o[16], (o[17] & 1) | (o[18] & 31) << 1, o[19]]
    b += v[126:135]
    b += [(v[135] & 7) | (v[136] & 1) << 3]
    b += v[137:141]
    b += [(v[141] & 1) | (v[142] & 7) << 1 | (v[143] & 7) << 4, v[144]]
    b += v[145:155]
    assert len(b) == 128 and all(0 <= x < 128 for x in b)
    return b


# Stack A: OP1 carrier (fast, the click's body) <- OP2 (a high ratio, gone in a few ms).
# Stack B: OP3 carrier (the body: loud 150 ms, then a long quieter tail) <- OP4 (rounds the first partials).
# Stack C: OP5 carrier an octave below, quiet <- OP6 (feedback, a little grain).
PATCHES = [
    ("KOTO A", voice("KOTO A", 5, [
        op(r=(99, 85, 60, 70), l=(99, 70, 0, 0), ol=78, rs=3, rd=25, rc=0),
        op(r=(99, 92, 50, 80), l=(99, 0, 0, 0), ol=72, fc=7, rs=4),
        op(r=(99, 44, 30, 55), l=(99, 78, 0, 0), ol=99, rs=2, rd=30, rc=0),
        op(r=(99, 70, 45, 60), l=(99, 55, 0, 0), ol=64, fc=2, rs=3),
        op(r=(99, 40, 28, 55), l=(99, 70, 0, 0), ol=68, fc=0, det=9, rs=2),
        op(r=(99, 60, 40, 60), l=(99, 40, 0, 0), ol=42, fc=1, det=5, rs=2),
    ], fb=3)),
    ("KOTO B", voice("KOTO B", 5, [                 # brighter: a harder click, more of the second partial
        op(r=(99, 88, 60, 70), l=(99, 75, 0, 0), ol=85, rs=3, rd=30, rc=0),
        op(r=(99, 95, 50, 80), l=(99, 0, 0, 0), ol=80, fc=11, rs=4),
        op(r=(99, 46, 32, 55), l=(99, 80, 0, 0), ol=99, rs=2, rd=30, rc=0),
        op(r=(99, 66, 45, 60), l=(99, 60, 0, 0), ol=72, fc=2, rs=3),
        op(r=(99, 42, 28, 55), l=(99, 65, 0, 0), ol=60, fc=0, det=9, rs=2),
        op(r=(99, 60, 40, 60), l=(99, 40, 0, 0), ol=48, fc=1, det=5, rs=2),
    ], fb=4)),
    ("KOTO WARM", voice("KOTO WARM", 5, [           # rounder: less click, more of the octave below
        op(r=(99, 80, 60, 70), l=(99, 60, 0, 0), ol=70, rs=3, rd=20, rc=0),
        op(r=(99, 90, 50, 80), l=(99, 0, 0, 0), ol=60, fc=5, rs=4),
        op(r=(99, 42, 28, 55), l=(99, 80, 0, 0), ol=99, rs=2, rd=35, rc=0),
        op(r=(99, 72, 45, 60), l=(99, 50, 0, 0), ol=56, fc=1, rs=3),
        op(r=(99, 38, 26, 55), l=(99, 72, 0, 0), ol=78, fc=0, det=8, rs=2),
        op(r=(99, 60, 40, 60), l=(99, 40, 0, 0), ol=36, fc=1, det=6, rs=2),
    ], fb=2)),
    ("HARP SOFT", voice("HARP SOFT", 5, [           # a harp-like pluck: softer attack, long even decay
        op(r=(95, 70, 50, 65), l=(99, 70, 0, 0), ol=80, rs=2, rd=20, rc=0),
        op(r=(99, 85, 50, 80), l=(99, 0, 0, 0), ol=66, fc=3, rs=3),
        op(r=(96, 40, 30, 55), l=(99, 85, 0, 0), ol=99, rs=2, rd=25, rc=0),
        op(r=(99, 55, 45, 60), l=(99, 55, 0, 0), ol=60, fc=1, rs=2),
        op(r=(96, 38, 28, 55), l=(99, 70, 0, 0), ol=62, fc=1, det=10, rs=2),
        op(r=(99, 60, 40, 60), l=(99, 40, 0, 0), ol=30, fc=1, det=4, rs=2),
    ], fb=1)),
    ("SINE", voice("SINE", 1, [op(ol=99, r=(99, 99, 99, 60), l=(99, 99, 99, 0))] + [op() for _ in range(5)])),
]


def main(path):
    L = ["/* generated by tools/fumi_patches.py: FuMi-1's own patches, packed 128-byte voices (no ROM data) */",
         "#pragma once", "#include <stdint.h>", f"#define FM_NPATCH {len(PATCHES)}",
         "static const char *const FM_PATCH_NAME[FM_NPATCH] = {" + ", ".join(f'"{n}"' for n, _ in PATCHES) + "};",
         "static const uint8_t FM_PATCH[FM_NPATCH][128] = {"]
    for name, v in PATCHES:
        b = pack(v)
        L.append(f"    {{ /* {name} */")
        for i in range(0, 128, 16):
            L.append("        " + ", ".join(f"{x:3d}" for x in b[i:i + 16]) + ",")
        L.append("    },")
    L += ["};", ""]
    Path(path).write_text("\n".join(L))
    print(f"patches: {path} ({len(PATCHES)})")


if __name__ == "__main__":
    main(sys.argv[1])
