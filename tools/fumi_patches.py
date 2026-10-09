#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# The voice() / op() / pack() helpers follow Felucca's tools/gen_fm6_patches.py, Copyright (C) 2026 Leo
# Kuroshita (@kurogedelic), Hügelton Instruments. The patches are FuMi-1's own.
"""FuMi-1's own FM6 patches -> firmware/src/dsp/fumi_patches.h (packed 128-byte voices, the DX7 bank record).

  tools/fumi_patches.py firmware/src/dsp/fumi_patches.h

No factory ROM data of any instrument is in here (patches 6 on are community patches, see their notes): these are starting points written as readable operator
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


# Stack A: OP1 carrier <- OP2 at ratio 2 or 3: the odd partials and the twang (its index falls over the
# first 200 ms, bright to dark). Stack B: OP3 carrier <- OP4 at ratio 1: the body, partials 1 to 4.
# Stack C: OP5 carrier, detuned a little, <- OP6 with feedback: fullness and grain.
# KOTO A and B were found by a search over the engine against the ST-50 measurements of docs/research/13
# (section 4, A4: partials at onset 0, 0, -2, -9, -6 dB, at 300 ms 0, -6, -2, -2 with the 5th up below -20;
# click 7-9 dB above 5 kHz in the first 12 ms; -28 dB/s for 200 ms then -16; peak within 5 ms). WARM is A
# with the modulators turned down and the third stack up. Measured again through the host in tests.
PATCHES = [
    ("KOTO A", voice("KOTO A", 5, [
        op(r=(99, 45, 71, 60), l=(99, 72, 0, 0), ol=75, rs=2, rd=25, rc=0),
        op(r=(99, 45, 95, 60), l=(99, 90, 0, 0), ol=82, fc=2, rs=4),
        op(r=(99, 58, 34, 60), l=(99, 95, 0, 0), ol=89, rs=2, rd=30, rc=0),
        op(r=(99, 60, 32, 60), l=(99, 91, 90, 0), ol=91, fc=1, rs=3),
        op(r=(99, 69, 18, 60), l=(99, 87, 0, 0), ol=70, fc=1, det=9, rs=2),
        op(r=(99, 26, 10, 60), l=(99, 99, 97, 0), ol=84, fc=1, det=5, rs=2),
    ], fb=2)),
    ("KOTO B", voice("KOTO B", 5, [                 # brighter: the twang stack at ratio 3, harder
        op(r=(99, 52, 95, 60), l=(99, 87, 0, 0), ol=80, rs=2, rd=25, rc=0),
        op(r=(99, 93, 32, 60), l=(99, 90, 34, 0), ol=99, fc=3, rs=4),
        op(r=(99, 99, 32, 60), l=(99, 91, 0, 0), ol=89, rs=2, rd=30, rc=0),
        op(r=(99, 40, 26, 60), l=(99, 99, 86, 0), ol=84, fc=1, rs=3),
        op(r=(99, 95, 54, 60), l=(99, 83, 0, 0), ol=87, fc=1, det=9, rs=2),
        op(r=(99, 28, 77, 60), l=(99, 67, 8, 0), ol=75, fc=1, det=5, rs=2),
    ], fb=5)),
    ("KOTO WARM", voice("KOTO WARM", 5, [           # A, rounder: less twang and body index, more of stack C
        op(r=(99, 45, 71, 60), l=(99, 72, 0, 0), ol=72, rs=2, rd=25, rc=0),
        op(r=(99, 45, 95, 60), l=(99, 90, 0, 0), ol=72, fc=2, rs=4),
        op(r=(99, 58, 34, 60), l=(99, 95, 0, 0), ol=89, rs=2, rd=30, rc=0),
        op(r=(99, 60, 32, 60), l=(99, 91, 90, 0), ol=85, fc=1, rs=3),
        op(r=(99, 69, 18, 60), l=(99, 87, 0, 0), ol=78, fc=1, det=9, rs=2),
        op(r=(99, 26, 10, 60), l=(99, 99, 97, 0), ol=70, fc=1, det=5, rs=2),
    ], fb=2)),
    ("HARP SOFT", voice("HARP SOFT", 5, [           # a harp-like pluck: softer attack, long even decay
        op(r=(95, 70, 50, 65), l=(99, 70, 0, 0), ol=80, rs=2, rd=20, rc=0),
        op(r=(99, 85, 50, 80), l=(99, 0, 0, 0), ol=66, fc=3, rs=3),
        op(r=(96, 40, 30, 55), l=(99, 85, 0, 0), ol=99, rs=2, rd=25, rc=0),
        op(r=(99, 55, 45, 60), l=(99, 55, 0, 0), ol=60, fc=1, rs=2),
        op(r=(96, 38, 28, 55), l=(99, 70, 0, 0), ol=62, fc=1, det=10, rs=2),
        op(r=(99, 60, 40, 60), l=(99, 40, 0, 0), ol=30, fc=1, det=4, rs=2),
    ], fb=1)),
    ("N-KOTO-A5", voice("N-KOTO-A5", 2, [           # the owner's pick, 9 Oct 2026: the Aminet dx-syx collection, bank 073.syx,
                                                    # author unknown (patches.fm). Not a Yamaha ROM voice. Algorithm 2.
        op(r=(94, 43, 16, 34), l=(99, 92, 0, 0), ol=99, kvs=1, rs=7),
        op(r=(99, 47, 32, 48), l=(99, 86, 0, 0), ol=87, fc=4, rs=7, rd=11, rc=0),
        op(r=(94, 64, 20, 31), l=(99, 92, 0, 0), ol=99, kvs=1, rs=5),
        op(r=(90, 33, 18, 39), l=(99, 72, 0, 0), ol=89, kvs=1, rs=6, bp=10, rd=22, rc=1),
        op(r=(91, 54, 29, 29), l=(99, 90, 0, 0), ol=81, fc=4, kvs=1, rs=4, rd=5, rc=0),
        op(r=(82, 82, 37, 48), l=(99, 81, 0, 0), ol=0, fc=3, kvs=1, rs=5, rd=10, rc=0),
    ], fb=5, pr=(90, 11, 75, 53), pl=(49, 50, 50, 50), lfs=30, lfd=0, lpmd=0, lamd=16, lpms=2, trnsp=24)),
    # the owner's further picks (patches.fm), 9 Oct 2026: four flutes and a drum, community banks, authors as given
    ('Air---*--3', voice('Air---*--3', 1, [            # patches.fm: _Unknown, FLUTE01.SYX
        op(r=(58, 99, 0, 44), l=(99, 99, 99, 0), ol=99, fc=0, det=0, mode=1, kvs=1),
        op(r=(96, 94, 30, 39), l=(99, 99, 0, 0), ol=82, fc=2, det=0, rd=5, rc=0),
        op(r=(67, 99, 0, 44), l=(99, 99, 99, 0), ol=80, fc=3, det=11, kvs=1),
        op(r=(83, 46, 53, 30), l=(99, 99, 75, 29), ol=79, fc=3, ff=17, det=11, kvs=5),
        op(r=(93, 99, 99, 0), l=(99, 99, 99, 0), ol=99, fc=3, ff=15),
        op(r=(99, 99, 99, 0), l=(99, 99, 99, 0), ol=99, fc=9, ff=6),
    ], fb=7, oks=1, pr=(99, 99, 99, 99), pl=(50, 50, 50, 50), lfs=38, lfd=0, lpmd=10, lamd=0, lks=0, lfw=0, lpms=2, trnsp=12)),
    ('AirFltMal1', voice('AirFltMal1', 1, [            # patches.fm: _Unknown, FLUTE01.SYX
        op(r=(58, 99, 0, 44), l=(99, 99, 99, 0), ol=99, fc=12, det=1, mode=1),
        op(r=(96, 94, 30, 39), l=(99, 99, 0, 0), ol=82, det=0, rd=5, rc=0),
        op(r=(99, 99, 0, 44), l=(99, 99, 99, 0), ol=99, det=11, kvs=7),
        op(r=(99, 46, 53, 30), l=(99, 99, 75, 29), ol=98, fc=2, det=0, kvs=2, rd=7, rc=0),
        op(r=(93, 99, 99, 0), l=(99, 99, 99, 0), ol=39, fc=0, mode=1),
        op(r=(99, 99, 99, 0), l=(99, 99, 99, 0), ol=99, fc=11, ff=14, mode=1),
    ], fb=7, oks=1, pr=(99, 99, 99, 99), pl=(50, 50, 50, 50), lfs=38, lfd=0, lpmd=10, lamd=0, lks=0, lfw=0, lpms=2, trnsp=24)),
    ('ALTO FLUTE', voice('ALTO FLUTE', 14, [            # patches.fm: _Unknown, FLUTE01.SYX
        op(r=(53, 24, 37, 90), l=(99, 36, 0, 0), ol=94, det=5, kvs=7, ams=3, bp=99, ld=99, lc=1),
        op(r=(99, 41, 21, 99), l=(99, 99, 99, 0), ol=99, ams=1, rd=99, rc=0, ld=99, lc=0),
        op(r=(44, 62, 52, 63), l=(99, 98, 97, 0), ol=99, det=9, kvs=2, rs=2),
        op(r=(39, 60, 57, 33), l=(63, 99, 99, 0), ol=60, det=5, ams=2),
        op(r=(76, 99, 99, 35), l=(99, 97, 94, 0), ol=61, det=10),
        op(r=(49, 53, 58, 99), l=(75, 97, 91, 0), ol=41, fc=2, det=4, ams=1),
    ], fb=0, oks=1, pr=(98, 98, 98, 98), pl=(50, 50, 50, 50), lfs=33, lfd=42, lpmd=0, lamd=59, lks=0, lfw=4, lpms=1, trnsp=24)),
    ('Bamboo Flt', voice('Bamboo Flt', 5, [            # patches.fm: _Unknown, FLUTE01.SYX
        op(r=(46, 42, 99, 55), l=(99, 90, 90, 0), ol=99, kvs=2, ams=3, rs=1),
        op(r=(99, 99, 99, 43), l=(99, 99, 99, 0), ol=73, fc=2, det=10, kvs=2, bp=39, rd=99, rc=1),
        op(r=(46, 99, 99, 64), l=(99, 99, 99, 0), ol=64, fc=2, det=13, kvs=2, ams=2, rs=1),
        op(r=(99, 99, 99, 42), l=(99, 99, 99, 0), ol=56, fc=3, kvs=2, bp=39, rd=99, rc=1),
        op(r=(54, 99, 52, 99), l=(99, 99, 66, 0), ol=64, fc=0, det=11, kvs=4, ams=2, bp=27, rd=15, rc=1),
        op(r=(99, 99, 49, 99), l=(99, 99, 93, 0), ol=99, fc=10),
    ], fb=7, oks=1, pr=(99, 99, 99, 99), pl=(50, 50, 50, 50), lfs=26, lfd=39, lpmd=6, lamd=0, lks=0, lfw=4, lpms=2, trnsp=24)),
    ('T. Drum', voice('T. Drum', 15, [               # patches.fm: Tim Garrett, TX7-32B.SYX, renamed from 'Bongos.2'
        op(r=(99, 40, 74, 38), l=(99, 0, 0, 0), ol=62, ff=46, kvs=4, rs=4, bp=2),
        op(r=(99, 22, 42, 25), l=(99, 0, 0, 0), ol=41, fc=7, ff=75, mode=1, rs=5, bp=2),
        op(r=(99, 34, 37, 45), l=(99, 0, 0, 0), ol=99, fc=0, ff=4, kvs=1, rs=6, bp=2),
        op(r=(99, 99, 99, 98), l=(99, 99, 0, 0), ol=64, fc=4, ff=2, det=10, rs=7, bp=2),
        op(r=(99, 94, 99, 43), l=(99, 27, 0, 0), ol=99, fc=3, ff=4, det=6, rs=7, bp=14),
        op(r=(99, 75, 99, 23), l=(99, 0, 0, 0), ol=99, ff=50, det=8, mode=1, rs=7, bp=14),
    ], fb=0, oks=1, pr=(98, 98, 98, 98), pl=(50, 50, 50, 50), lfs=4, lfd=0, lpmd=0, lamd=80, lks=1, lfw=2, lpms=7, trnsp=24)),
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
