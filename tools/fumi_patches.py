#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# The voice() / op() / pack() helpers follow Felucca's tools/gen_fm6_patches.py, Copyright (C) 2026 Leo
# Kuroshita (@kurogedelic), Hügelton Instruments. The patches are FuMi-1's own.
"""FuMi-1's own FM6 patches -> firmware/src/dsp/fumi_patches.h (packed 128-byte voices, the DX7 bank record).

  tools/fumi_patches.py firmware/src/dsp/fumi_patches.h

No factory ROM data of any instrument is in here (N-KOTO-A5 and patches 2 to 6 are community patches, see their notes): these are starting points written as readable operator
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
# (name on the record, up to 10 ASCII characters; the Japanese name the screen shows beneath it; the voice)
# Order (1.0.8): the ST-50 strip's families, kotos / bells / winds / sustained / percussion; Koto first, the boot voice.
# A saved setup names its instrument by this index: reordering needs a project migration (project.c).
PATCHES = [
    ("Koto", "琴", voice("Koto", 2, [  # was N-KOTO-A5           # the owner's pick, 9 Oct 2026: the Aminet dx-syx collection, bank 073.syx,
                                                    # author unknown (patches.fm). Not a Yamaha ROM voice. Algorithm 2.
        op(r=(94, 43, 16, 34), l=(99, 92, 0, 0), ol=99, kvs=1, rs=7),
        op(r=(99, 47, 32, 48), l=(99, 86, 0, 0), ol=87, fc=4, rs=7, rd=11, rc=0),
        op(r=(94, 64, 20, 31), l=(99, 92, 0, 0), ol=99, kvs=1, rs=5),
        op(r=(90, 33, 18, 39), l=(99, 72, 0, 0), ol=89, kvs=1, rs=6, bp=10, rd=22, rc=1),
        op(r=(91, 54, 29, 29), l=(99, 90, 0, 0), ol=81, fc=4, kvs=1, rs=4, rd=5, rc=0),
        op(r=(82, 82, 37, 48), l=(99, 81, 0, 0), ol=0, fc=3, kvs=1, rs=5, rd=10, rc=0),
    ], fb=5, pr=(90, 11, 75, 53), pl=(49, 50, 50, 50), lfs=30, lfd=0, lpmd=0, lamd=16, lpms=2, trnsp=24)),
    # the owner's further picks (patches.fm), 9 Oct 2026: four flutes and a drum, community banks, authors as given;
# they follow the boot patch, FuMi's own kotos come after
    ('Koto II', '箏', voice('Koto II', 2, [  # was Kuto S                # patches.fm: Aminet, 072.syx, renamed from 'KOTO-KOTO'
        op(r=(94, 62, 28, 34), l=(99, 95, 0, 0), ol=99, kvs=3, rs=6),
        op(r=(99, 64, 29, 24), l=(99, 91, 0, 0), ol=98, rs=6, bp=34, rd=6, rc=0),
        op(r=(94, 64, 28, 33), l=(99, 95, 0, 0), ol=99, kvs=3, rs=5),
        op(r=(90, 19, 17, 20), l=(99, 90, 0, 0), ol=80, kvs=1, rs=7, bp=10, rd=11, rc=1),
        op(r=(91, 40, 32, 29), l=(99, 93, 0, 0), ol=84, fc=3, kvs=1, rs=6, rd=5, rc=0),
        op(r=(82, 41, 32, 36), l=(99, 90, 0, 0), ol=82, fc=3, kvs=1, rs=7, rd=5, rc=0),
    ], fb=5, oks=1, pr=(90, 34, 75, 53), pl=(49, 50, 50, 50), lfs=30, lfd=0, lpmd=0, lamd=16, lks=1, lfw=1, lpms=6, trnsp=24)),
    ("Koto Pluck", "爪音", voice("Koto Pluck", 5, [  # was KOTO A
        op(r=(99, 45, 71, 60), l=(99, 72, 0, 0), ol=87, rs=2, rd=12, rc=0, kvs=3),
        op(r=(99, 45, 95, 60), l=(99, 90, 0, 0), ol=82, fc=2, rs=4, kvs=6),
        op(r=(99, 58, 34, 60), l=(99, 95, 0, 0), ol=99, rs=2, rd=15, rc=0, kvs=3),
        op(r=(99, 60, 32, 60), l=(99, 91, 90, 0), ol=91, fc=1, rs=3, kvs=5),
        op(r=(99, 69, 18, 60), l=(99, 87, 0, 0), ol=82, fc=1, det=9, rs=2, kvs=3),
        op(r=(99, 26, 10, 60), l=(99, 99, 97, 0), ol=84, fc=1, det=5, rs=2, kvs=5),
    ], fb=2)),
    ("Koto Ring", "響", voice("Koto Ring", 5, [  # was KOTO B                 # brighter: the twang stack at ratio 3, harder
        op(r=(99, 52, 95, 60), l=(99, 87, 0, 0), ol=92, rs=2, rd=12, rc=0, kvs=3),
        op(r=(99, 93, 32, 60), l=(99, 90, 34, 0), ol=99, fc=3, rs=4, kvs=2),
        op(r=(99, 99, 32, 60), l=(99, 91, 0, 0), ol=99, rs=2, rd=15, rc=0, kvs=3),
        op(r=(99, 40, 26, 60), l=(99, 99, 86, 0), ol=84, fc=1, rs=3, kvs=3),
        op(r=(99, 95, 54, 60), l=(99, 83, 0, 0), ol=97, fc=1, det=9, rs=2, kvs=3),
        op(r=(99, 28, 77, 60), l=(99, 67, 8, 0), ol=75, fc=1, det=5, rs=2, kvs=3),
    ], fb=5)),
    ("Koto Warm", "名残", voice("Koto Warm", 5, [  # was KOTO WARM           # A, rounder: less twang and body index, more of stack C
        op(r=(99, 45, 71, 60), l=(99, 72, 0, 0), ol=84, rs=2, rd=12, rc=0, kvs=3),
        op(r=(99, 45, 95, 60), l=(99, 90, 0, 0), ol=72, fc=2, rs=4, kvs=3),
        op(r=(99, 58, 34, 60), l=(99, 95, 0, 0), ol=99, rs=2, rd=15, rc=0, kvs=3),
        op(r=(99, 60, 32, 60), l=(99, 91, 90, 0), ol=85, fc=1, rs=3, kvs=2),
        op(r=(99, 69, 18, 60), l=(99, 87, 0, 0), ol=90, fc=1, det=9, rs=2, kvs=3),
        op(r=(99, 26, 10, 60), l=(99, 99, 97, 0), ol=70, fc=1, det=5, rs=2, kvs=2),
    ], fb=2)),
    ("Harp", "箜篌", voice("Harp", 5, [  # was HARP SOFT           # a harp-like pluck: softer attack, long even decay
        op(r=(95, 70, 50, 65), l=(99, 70, 0, 0), ol=92, rs=2, rd=10, rc=0, kvs=3),
        op(r=(99, 85, 50, 80), l=(99, 0, 0, 0), ol=66, fc=3, rs=3, kvs=2),
        op(r=(96, 40, 30, 55), l=(99, 85, 0, 0), ol=99, rs=2, rd=12, rc=0, kvs=3),
        op(r=(99, 55, 45, 60), l=(99, 55, 0, 0), ol=60, fc=1, rs=2, kvs=1),
        op(r=(96, 38, 28, 55), l=(99, 70, 0, 0), ol=74, fc=1, det=10, rs=2, kvs=3),
        op(r=(99, 60, 40, 60), l=(99, 40, 0, 0), ol=30, fc=1, det=4, rs=2, kvs=1),
    ], fb=1)),
    # 1.0.8: the owner's seven more picks (patches.fm, 22 Oct 2026), by family as on the ST-50's strip: the bells
    # after the kotos, the clarinet, the strings and the fairytale pad with the sustained voices. The engine ignores a
    # patch's transpose, so voices written an octave up or down have their ratios doubled or halved (tools/patchfm_import.py)
    ('Chimes', 'チャイム', voice('Chimes', 22, [  # was CHIMES 1   # patches.fm: _Unknown, CHRPER10.SYX, written an octave down: ratios halved
        op(r=(97, 20, 20, 34), l=(99, 90, 80, 0), ol=87, fc=2, rs=2, bp=0),
        op(r=(97, 20, 20, 34), l=(99, 90, 80, 0), ol=85, fc=4, kvs=1, rs=2, bp=0),
        op(r=(97, 20, 20, 34), l=(99, 90, 80, 0), ol=85, det=4, kvs=1, rs=2, bp=0),
        op(r=(97, 20, 20, 34), l=(99, 90, 80, 0), ol=67, fc=2, rs=2, bp=0),
        op(r=(97, 20, 20, 34), l=(99, 90, 80, 0), ol=84, det=10, kvs=2, rs=2, bp=0),
        op(r=(97, 20, 20, 34), l=(99, 90, 80, 0), ol=66, kvs=2, rs=2, bp=0),
    ], fb=7, oks=1, pr=(94, 67, 95, 60), pl=(50, 50, 50, 50), lfs=60, lfd=69, lpmd=0, lamd=0, lks=1, lfw=5, lpms=3, trnsp=24)),
    ('Rin', '鈴', voice('Rin', 27, [  # was TBTAN BELL   # patches.fm: Tim Conrardy, FLUTES.SYX, written an octave up: ratios doubled; carriers -5 (level into the band, -4 dB)
        op(r=(99, 28, 99, 29), l=(99, 25, 0, 0), ol=94, det=14, kvs=2, bp=28, ld=12, rc=3),
        op(r=(99, 28, 99, 29), l=(99, 25, 0, 0), ol=93, fc=2, ff=18, bp=28, ld=12, rc=3),
        op(r=(94, 28, 99, 29), l=(99, 25, 0, 0), ol=80, fc=6, ff=5, bp=28, ld=12, rc=3),
        op(r=(72, 28, 15, 25), l=(99, 25, 0, 0), ol=61, fc=2, ff=18, det=0, ams=3, bp=28, ld=12, rc=3),
        op(r=(22, 28, 99, 21), l=(99, 25, 0, 0), ol=72, det=14, rs=2, bp=28, ld=12, rd=12, rc=3),
        op(r=(78, 28, 99, 23), l=(99, 25, 0, 0), ol=77, fc=6, ff=5, det=8, bp=28, ld=12, rc=3),
    ], fb=5, oks=0, pr=(97, 94, 75, 60), pl=(50, 50, 50, 50), lfs=22, lfd=0, lpmd=0, lamd=99, lks=0, lfw=0, lpms=1, trnsp=24)),
    ('Bell', 'ベル', voice('Bell', 32, [  # was MICHEL  2   # patches.fm: Tim Conrardy, BELGIE.SYX; all -2 (level into the band, -1.5 dB)
        op(r=(78, 64, 87, 38), l=(99, 91, 90, 0), ol=97, fc=0, ams=1, rs=1, bp=36, lc=3),
        op(r=(96, 64, 87, 21), l=(82, 91, 78, 0), ol=87, fc=0, det=0, ams=2, rs=1, bp=41, ld=1, rd=13, lc=3),
        op(r=(96, 26, 22, 30), l=(72, 82, 67, 0), ol=97, fc=2, det=3, ams=3, rs=1, bp=43, rd=9, lc=3),
        op(r=(78, 64, 87, 38), l=(99, 91, 90, 0), ol=97, fc=0, ff=1, det=0, ams=1, rs=1, bp=34, lc=3),
        op(r=(96, 64, 87, 21), l=(75, 91, 78, 0), ol=87, fc=0, det=0, ams=2, rs=1, bp=36, ld=1, rd=13, lc=3),
        op(r=(96, 26, 22, 30), l=(74, 82, 67, 0), ol=97, fc=2, det=14, ams=3, rs=1, rd=9, lc=3),
    ], fb=0, oks=1, pr=(20, 67, 95, 60), pl=(50, 50, 50, 50), lfs=21, lfd=42, lpmd=0, lamd=8, lks=1, lfw=4, lpms=2, trnsp=24)),
    ('Ice Bells', '氷鈴', voice('Ice Bells', 5, [  # was Ice.Bells1   # patches.fm: Tim Garrett, TX7-33.SYX, written an octave down: ratios halved; carriers -11 (level into the band, -8 dB)
        op(r=(99, 40, 99, 39), l=(99, 41, 0, 0), ol=88, fc=4, ff=3, det=4, bp=22),
        op(r=(99, 40, 71, 28), l=(99, 41, 0, 0), ol=60, fc=4, ff=45, det=12, rs=4, bp=0, rd=96, lc=2, rc=3),
        op(r=(99, 39, 99, 39), l=(99, 41, 0, 0), ol=88, fc=4, ff=3, det=5, kvs=2, bp=22),
        op(r=(99, 40, 71, 28), l=(99, 41, 0, 0), ol=44, fc=4, ff=45, det=11, kvs=1, rs=4, bp=0, rd=96, lc=2, rc=3),
        op(r=(99, 40, 99, 39), l=(99, 41, 0, 0), ol=88, fc=3, ff=37, kvs=1, bp=22),
        op(r=(99, 40, 71, 28), l=(99, 41, 0, 0), ol=70, fc=3, ff=94, kvs=2, rs=4, bp=0, rd=96, lc=2, rc=3),
    ], fb=3, oks=1, pr=(85, 99, 99, 99), pl=(52, 50, 50, 50), lfs=35, lfd=91, lpmd=1, lamd=2, lks=0, lfw=0, lpms=4, trnsp=24)),
    # the winds
    ('Shakuhachi', '尺八', voice('Shakuhachi', 16, [  # was PAN FL T A          # patches.fm: _Unknown, AAAHGOOD.SYX (the owner's pick, 10 Oct 2026,
                                                                            # for the AirFltMal1 whose onset was a bright blast)
        op(r=(60, 71, 66, 65), l=(82, 75, 95, 0), ol=99, det=13, kvs=2, bp=41),
        op(r=(59, 53, 62, 54), l=(99, 60, 0, 0), ol=65, fc=4, ff=15, kvs=2, rs=4, bp=0),
        op(r=(53, 38, 75, 61), l=(88, 44, 24, 0), ol=50, fc=2, ff=1, det=14, kvs=3, bp=46, lc=3),
        op(r=(61, 25, 25, 60), l=(99, 99, 97, 0), ol=75, fc=2, det=0, rs=3, bp=60, ld=10, rd=10),
        op(r=(65, 38, 0, 61), l=(99, 0, 0, 0), ol=51, fc=2, kvs=3, bp=53, rd=43),
        op(r=(71, 64, 98, 61), l=(99, 67, 52, 0), ol=83, fc=4, ff=76, det=8, kvs=2, bp=46, rc=3),
    ], fb=4, oks=1, pr=(94, 67, 95, 60), pl=(50, 50, 50, 50), lfs=30, lfd=23, lpmd=0, lamd=0, lks=0, lfw=0, lpms=1, trnsp=24)),
    ('Bamboo Flt', '篠笛', voice('Bamboo Flt', 5, [            # patches.fm: _Unknown, FLUTE01.SYX
        op(r=(46, 42, 99, 55), l=(99, 90, 90, 0), ol=99, kvs=2, ams=3, rs=1),
        op(r=(99, 99, 99, 43), l=(99, 99, 99, 0), ol=73, fc=2, det=10, kvs=2, bp=39, rd=99, rc=1),
        op(r=(46, 99, 99, 64), l=(99, 99, 99, 0), ol=64, fc=2, det=13, kvs=2, ams=2, rs=1),
        op(r=(99, 99, 99, 42), l=(99, 99, 99, 0), ol=56, fc=3, kvs=2, bp=39, rd=99, rc=1),
        op(r=(54, 99, 52, 99), l=(99, 99, 66, 0), ol=64, fc=0, det=11, kvs=4, ams=2, bp=27, rd=15, rc=1),
        op(r=(99, 99, 49, 99), l=(99, 99, 93, 0), ol=99, fc=10),
    ], fb=7, oks=1, pr=(99, 99, 99, 99), pl=(50, 50, 50, 50), lfs=26, lfd=39, lpmd=6, lamd=0, lks=0, lfw=4, lpms=2, trnsp=24)),
    # 胡弓, the bowed string, the ST-50's 11th voice (1.0.6): Tim Garrett's Viola.Ens (patches.fm, TX7-80B.SYX), its
    # attack slowed (carrier R1 38/49 -> 34/42, modulator R1 99 -> 60/70 so the tone brightens as the bow bites) and
    # a bow tremolo added (LFO amplitude depth 30 on the two carriers, AMS 3, ~5.5 Hz, a short delay); the patch's
    # own pitch modulation off, FuMi's vibrato being the player's. The original is written an octave down (transpose
    # 12, which FuMi's engine does not read), so every ratio is halved to put it on the key's pitch. Measured in the
    # session: the first 60 ms about 30 dB under the sustain, full by 300 ms. Winds, then the bowed voice, then
    # percussion, as on the ST-50.
    ('Dragon Flt', '龍笛', voice('Dragon Flt', 14, [  # was ALTO FLUTE            # patches.fm: _Unknown, FLUTE01.SYX
        op(r=(53, 24, 37, 90), l=(99, 36, 0, 0), ol=94, det=5, kvs=7, ams=3, bp=99, ld=99, lc=1),
        op(r=(99, 41, 21, 99), l=(99, 99, 99, 0), ol=99, ams=1, rd=99, rc=0, ld=99, lc=0),
        op(r=(44, 62, 52, 63), l=(99, 98, 97, 0), ol=99, det=9, kvs=2, rs=2),
        op(r=(39, 60, 57, 33), l=(63, 99, 99, 0), ol=60, det=5, ams=2),
        op(r=(76, 99, 99, 35), l=(99, 97, 94, 0), ol=61, det=10),
        op(r=(49, 53, 58, 99), l=(75, 97, 91, 0), ol=41, fc=2, det=4, ams=1),
    ], fb=0, oks=1, pr=(98, 98, 98, 98), pl=(50, 50, 50, 50), lfs=33, lfd=42, lpmd=0, lamd=59, lks=0, lfw=4, lpms=1, trnsp=24)),
    ('Sho', '笙', voice('Sho', 1, [  # was Air---*--3            # patches.fm: _Unknown, FLUTE01.SYX
        op(r=(58, 99, 0, 44), l=(99, 99, 99, 0), ol=99, fc=0, det=0, mode=1, kvs=1),
        op(r=(96, 94, 30, 39), l=(99, 99, 0, 0), ol=82, fc=2, det=0, rd=5, rc=0),
        op(r=(67, 99, 0, 44), l=(99, 99, 99, 0), ol=80, fc=3, det=11, kvs=1),
        op(r=(83, 46, 53, 30), l=(99, 99, 75, 29), ol=79, fc=3, ff=17, det=11, kvs=5),
        op(r=(93, 99, 99, 0), l=(99, 99, 99, 0), ol=99, fc=3, ff=15),
        op(r=(99, 99, 99, 0), l=(99, 99, 99, 0), ol=99, fc=9, ff=6),
    ], fb=7, oks=1, pr=(99, 99, 99, 99), pl=(50, 50, 50, 50), lfs=38, lfd=0, lpmd=10, lamd=0, lks=0, lfw=0, lpms=2, trnsp=12)),
    # the sustained voices
    ('Kokyu', '胡弓', voice('Kokyu', 2, [  # was Viola.Ens            # patches.fm: Tim Garrett, TX7-80B.SYX; attack slowed (the tremolo of 1.0.6 removed in 1.0.7)
        op(r=(34, 22, 18, 45), l=(97, 97, 86, 0), ol=89, fc=2, rs=1, bp=0),
        op(r=(60, 17, 13, 37), l=(99, 98, 98, 0), ol=76, fc=1, det=9, rs=1, ld=2, rd=2, lc=3),
        op(r=(42, 18, 17, 56), l=(99, 90, 85, 0), ol=99, fc=1, kvs=4, rs=3, bp=0),
        op(r=(55, 26, 0, 45), l=(99, 98, 90, 0), ol=86, fc=1, kvs=3, rs=3, bp=46, ld=4, rd=13, lc=3),
        op(r=(70, 46, 55, 36), l=(99, 90, 83, 0), ol=74, fc=4, det=8, kvs=3, rs=2, bp=42, ld=1, rd=15, lc=3),
        op(r=(31, 42, 50, 51), l=(99, 99, 99, 0), ol=36, fc=7, ff=17, det=12, mode=1, kvs=1, bp=46, ld=13),
    ], fb=7, oks=0, pr=(82, 94, 0, 0), pl=(48, 51, 50, 50), lfs=40, lfd=25, lpmd=0, lamd=0, lks=0, lfw=4, lpms=1, trnsp=12)),
    ('Clarinet', 'クラリネット', voice('Clarinet', 3, [  # was Clarinet12   # patches.fm: Tim Garrett, TX7-62.SYX
        op(r=(52, 50, 50, 55), l=(99, 95, 95, 0), ol=99, bp=0),
        op(r=(52, 50, 50, 55), l=(99, 95, 95, 0), ol=61, fc=2, bp=0),
        op(r=(99, 99, 99, 99), l=(99, 99, 99, 0), bp=0),
        op(r=(99, 99, 99, 99), l=(99, 99, 99, 99), bp=0),
        op(r=(99, 99, 99, 99), l=(99, 99, 99, 99), bp=0),
        op(r=(99, 99, 99, 99), l=(99, 99, 99, 99), bp=0),
    ], fb=0, oks=1, pr=(99, 99, 99, 99), pl=(51, 50, 50, 50), lfs=28, lfd=36, lpmd=0, lamd=0, lks=1, lfw=0, lpms=7, trnsp=24)),
    ('Strings', '弦楽', voice('Strings', 2, [  # was STRINGS 2   # patches.fm: Tim Conrardy, BEST03.SYX, written an octave down: ratios halved
        op(r=(55, 56, 10, 47), l=(98, 98, 36, 0), ol=86, det=0, bp=98),
        op(r=(81, 13, 7, 25), l=(99, 92, 28, 0), ol=74, det=1, bp=0, ld=78, rd=2, lc=1, rc=2),
        op(r=(51, 15, 10, 47), l=(99, 92, 0, 0), ol=92, det=11, bp=0),
        op(r=(49, 74, 10, 32), l=(98, 98, 36, 0), ol=70, bp=53, ld=35, lc=1, rc=2),
        op(r=(76, 73, 10, 28), l=(99, 92, 0, 0), ol=72, bp=0, ld=99),
        op(r=(72, 76, 10, 32), l=(99, 92, 0, 0), ol=66, fc=4, ff=25, kvs=2, bp=0, ld=42, lc=1, rc=2),
    ], fb=7, oks=1, pr=(84, 95, 95, 60), pl=(50, 50, 50, 50), lfs=42, lfd=0, lpmd=9, lamd=0, lks=1, lfw=4, lpms=3, trnsp=24)),
    ('Fairytale', '御伽', voice('Fairytale', 7, [  # was Fairlyte1a   # patches.fm: _Unknown, CHOIR04.SYX; carriers -4 (level into the band, -3 dB)
        op(r=(53, 23, 26, 39), l=(97, 92, 0, 0), ol=95, fc=2, det=11, kvs=2, rs=1, bp=34, rd=9),
        op(r=(82, 39, 14, 21), l=(99, 83, 0, 0), ol=48, fc=9, det=12, kvs=1, ams=2, bp=5, rd=3),
        op(r=(53, 23, 26, 39), l=(97, 92, 0, 0), ol=95, det=2, kvs=2, rs=1, bp=34, rd=9),
        op(r=(61, 34, 14, 33), l=(65, 99, 85, 0), ol=52, fc=6, ff=51, det=13, kvs=1, ams=1, bp=5, rd=8),
        op(r=(64, 57, 11, 31), l=(69, 97, 87, 0), ol=67, det=12, kvs=1, bp=48, rd=6),
        op(r=(50, 40, 11, 38), l=(99, 89, 42, 0), ol=49, fc=3, ff=79, det=1, mode=1, bp=60, ld=25, rc=1),
    ], fb=7, oks=0, pr=(96, 99, 21, 87), pl=(51, 47, 50, 50), lfs=22, lfd=3, lpmd=31, lamd=7, lks=0, lfw=4, lpms=1, trnsp=24)),
    # percussion
    ('Taiko', '太鼓', voice('Taiko', 17, [                 # patches.fm: Tim Garrett, TX7-26B.SYX, renamed from 'Timpani.2'
        op(r=(87, 37, 98, 33), l=(99, 0, 0, 0), ol=99, fc=0, kvs=2, rs=3),
        op(r=(87, 76, 27, 23), l=(99, 72, 0, 0), ol=57, fc=0, kvs=1, rs=2, bp=41),
        op(r=(86, 77, 26, 23), l=(99, 72, 0, 0), ol=93, fc=0, ff=38, kvs=2, rs=7),
        op(r=(66, 31, 17, 30), l=(99, 75, 0, 0), ol=57, fc=0, ff=74, kvs=3, rs=3, bp=41, rd=15, rc=0),
        op(r=(99, 51, 26, 19), l=(99, 0, 0, 0), ol=78, fc=0, kvs=1, rs=1, bp=80),
        op(r=(98, 3, 26, 27), l=(98, 0, 0, 0), ol=73, fc=0, ff=56, kvs=1, rs=3, bp=41, rd=24, rc=0),
    ], fb=7, oks=1, pr=(98, 98, 71, 98), pl=(50, 51, 50, 50), lfs=11, lfd=0, lpmd=0, lamd=0, lks=0, lfw=0, lpms=0, trnsp=12)),
    # the wood pair, FuMi's own (1.0.4): percussion ends by itself, so 余韻 does not reach it. Algorithm 5, three
    # carrier-modulator pairs, every envelope to zero. Measured through the host in tests/host/fumi_test.c.
    # 拍子木: two hardwood sticks struck together (kabuki, sumo, the night watch): a dry crack with no pitch to
    # speak of. Fixed-frequency carriers (so every key is the same crack) at 1.8 and 2.6 kHz, the second stack a
    # few ms behind for the two sticks, their modulators inharmonic and collapsing in a few ms; the third pair a
    # feedback noise burst for the first instant.
    ("Hyoshigi", "拍子木", voice("Hyoshigi", 5, [
        op(r=(99, 69, 99, 99), l=(99, 0, 0, 0), ol=99, fc=3, ff=26, mode=1, kvs=3),
        op(r=(99, 92, 99, 99), l=(99, 0, 0, 0), ol=92, fc=3, ff=49, mode=1),
        op(r=(90, 71, 99, 99), l=(99, 0, 0, 0), ol=96, fc=3, ff=41, mode=1, kvs=3),
        op(r=(99, 93, 99, 99), l=(99, 0, 0, 0), ol=88, fc=3, ff=65, mode=1),
        op(r=(99, 86, 99, 99), l=(99, 0, 0, 0), ol=86, fc=3, ff=10, mode=1, kvs=3),
        op(r=(99, 95, 99, 99), l=(99, 0, 0, 0), ol=96, fc=3, ff=33, mode=1),
    ], fb=7, lfs=0)),
    # 木魚: the wooden fish, a hollow block struck with a padded beater to pace the chanting: a round, pitched tok.
    # A sine carrier on the key's pitch with a quick downward scoop (the pitch envelope starts above and falls in
    # ~20 ms), a modulator at 1.41 for the hollow wood, an octave stack for the knock, 150-250 ms decay.
    ("Mokugyo", "木魚", voice("Mokugyo", 5, [
        op(r=(99, 58, 99, 99), l=(99, 0, 0, 0), ol=99, rs=2, kvs=3),
        op(r=(99, 79, 99, 99), l=(99, 0, 0, 0), ol=68, ff=41, rs=2),
        op(r=(99, 70, 99, 99), l=(99, 0, 0, 0), ol=90, fc=2, rs=2, kvs=3),
        op(r=(99, 84, 99, 99), l=(99, 0, 0, 0), ol=62, fc=2, ff=40, rs=2),
        op(r=(99, 86, 99, 99), l=(99, 0, 0, 0), ol=80, fc=0, ff=70, rs=2, kvs=3),
        op(r=(99, 92, 99, 99), l=(99, 0, 0, 0), ol=80, fc=5, ff=30, rs=2),
    ], fb=4, pr=(96, 99, 99, 99), pl=(50, 50, 50, 58), lfs=0)),
    # velocity (1.0.6, for the trill's accents; measured with a harness in the session, RMS over 250 ms and the
    # centroid of the first 60 ms against velocity 100): carriers set the loudness, modulators the brightness, the
    # click modulator (OP2) most of all, so a light ta (64) is 5 dB quieter and a third duller and a strong KA (127)
    # is a third brighter and only 1 dB louder. The community patches are left as their authors wrote them.
]


# the ST-50 strip's families, for the screen's voice slider (a wider gap where the family changes): the kotos, the
# bells, the winds, the sustained voices, percussion
FAMILY = {"Koto": 0, "Koto II": 0, "Koto Pluck": 0, "Koto Ring": 0, "Koto Warm": 0, "Harp": 0,
          "Chimes": 1, "Rin": 1, "Bell": 1, "Ice Bells": 1,
          "Shakuhachi": 2, "Bamboo Flt": 2, "Dragon Flt": 2, "Sho": 2,
          "Kokyu": 3, "Clarinet": 3, "Strings": 3, "Fairytale": 3,
          "Taiko": 4, "Hyoshigi": 4, "Mokugyo": 4}


def main(path):
    L = ["/* generated by tools/fumi_patches.py: FuMi-1's own patches, packed 128-byte voices (no ROM data) */",
         "#pragma once", "#include <stdint.h>", f"#define FM_NPATCH {len(PATCHES)}",
         "static const char *const FM_PATCH_NAME[FM_NPATCH] = {" + ", ".join(f'"{n}"' for n, _, _ in PATCHES) + "};",
         "static const char *const FM_PATCH_JA[FM_NPATCH] = {" + ", ".join(f'"{j}"' for _, j, _ in PATCHES) + "};  /* UTF-8 */",
         "static const uint8_t FM_PATCH_FAMILY[FM_NPATCH] = {" + ", ".join(str(FAMILY[n]) for n, _, _ in PATCHES) + "};  /* kotos, bells, winds, sustained, percussion */",
         "static const uint8_t FM_PATCH[FM_NPATCH][128] = {"]
    for name, _, v in PATCHES:
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
