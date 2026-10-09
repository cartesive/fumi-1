/* SPDX-License-Identifier: GPL-3.0-only */
/* dsp/tuning.c: the scales, the measured ST-50 tuning, 本数 and the key maps (docs/research/00, 13, 14). */
#include <string.h>
#include "tu.h"
#include "../../firmware/src/dsp/tuning.c"

static const float ET_IN[5] = {0, 100, 500, 700, 800};

int main(void)
{
    tuning_t t;
    int w, h, k;
    tuning_defaults(&t);
    CHECK("defaults: IN scale, SUIKO, 1本, depth 1, A = 440", t.scale == SC_IN && t.tuning == TN_SUIKO && t.hon == 4 &&
          t.depth == 1.0f && t.a4 == 440.0f && t.octave == 0 && t.fine == 0.0f);

    /* 本数: 三 (white key 6, index 5) = MIDI 53 + position; 1本 = position 4 = A3 (file 14) */
    t.hon = 4;
    CHECK("1本: 三 = A3 (MIDI 57)", tuning_san_midi(&t) == 57);
    t.hon = 0;
    CHECK("水4: 三 = F3 (MIDI 53)", tuning_san_midi(&t) == 53);
    t.hon = 15;
    CHECK("12本: 三 = G#4 (MIDI 68)", tuning_san_midi(&t) == 68);
    t.hon = 4;
    t.octave = -1;
    CHECK("octave down: 三 = A2", tuning_san_midi(&t) == 45);
    t.octave = 0;

    /* equal temperament: white key w = 三 + 12 * (w / 5 - 1) semitones + scale[w % 5] */
    t.tuning = TN_EQUAL;
    for (h = 0; h < 16; h++)
        for (w = 0; w < 16; w++) {
            t.hon = (int8_t)h;
            {
                double want = 100.0 * (53 + h + 12 * (w / 5 - 1) - 69) + ET_IN[w % 5];
                if (fabs(tuning_white_cents(&t, w) - want) > 1e-4) {
                    printf("  FAIL white %d at hon %d: %.4f want %.4f\n", w, h, tuning_white_cents(&t, w), want);
                    tu_fail++;
                }
            }
        }
    CHECK("every white key x 16 本 in equal temperament", 1);
    t.hon = 4;
    CHECK_NEAR("1本 white key 1 = A2 = -2400 cents from A4", tuning_white_cents(&t, 0), -2400, 1e-4);
    CHECK_NEAR("1本 white key 6 (三) = A3 = 220 Hz", tuning_hz(tuning_white_cents(&t, 5), t.a4), 220.0, 1e-3);
    CHECK_NEAR("1本 white key 16 = A5", tuning_white_cents(&t, 15), 1200, 1e-4);
    t.hon = 7;                                           /* 4本: 三 = C4 (file 01: 4本 = C) */
    CHECK_NEAR("4本 white key 11 (八) = C5 = 523.25 Hz", tuning_hz(tuning_white_cents(&t, 10), t.a4), 523.2511, 1e-3);
    t.hon = 0;
    CHECK_NEAR("水4 white key 1 = F2 = 87.31 Hz", tuning_hz(tuning_white_cents(&t, 0), t.a4), 87.3071, 1e-3);
    t.hon = 4;

    /* the measured 純正律 (SUIKO): fa -20.8, do -16.0, ti-flat -25; la and ti unmoved; depth scales it */
    t.tuning = TN_SUIKO;
    CHECK_NEAR("SUIKO fa", tuning_offset(&t, 1), -20.8, 1e-4);
    CHECK_NEAR("SUIKO la", tuning_offset(&t, 5), 0, 1e-4);
    CHECK_NEAR("SUIKO ti", tuning_offset(&t, 7), 0, 1e-4);
    CHECK_NEAR("SUIKO do", tuning_offset(&t, 8), -16.0, 1e-4);
    CHECK_NEAR("SUIKO ti-flat", tuning_offset(&t, 6), -25.0, 1e-4);
    CHECK_NEAR("SUIKO fa#, sol, do#, re unmeasured: 0", tuning_offset(&t, 2) + tuning_offset(&t, 3) + tuning_offset(&t, 9) + tuning_offset(&t, 10), 0, 1e-6);
    CHECK_NEAR("SUIKO white key 2 (fa) at 1本 = -2300 - 20.8", tuning_white_cents(&t, 1), -2320.8, 1e-4);
    CHECK_NEAR("SUIKO white key 10 (七, do) at 1本", tuning_white_cents(&t, 9), -400 - 16.0, 1e-4);
    t.depth = 0.5f;
    CHECK_NEAR("depth 50 %: fa -10.4", tuning_offset(&t, 1), -10.4, 1e-4);
    t.depth = 0.0f;
    CHECK_NEAR("depth 0: equal", tuning_white_cents(&t, 1), -2300, 1e-4);
    t.depth = 1.0f;

    /* KOTO: pure fifths (file 07) */
    t.tuning = TN_KOTO;
    CHECK_NEAR("KOTO fa -9.8", tuning_offset(&t, 1), -9.8, 0.05);
    CHECK_NEAR("KOTO la -2.0", tuning_offset(&t, 5), -2.0, 0.05);
    CHECK_NEAR("KOTO ti +2.0", tuning_offset(&t, 7), 2.0, 0.05);
    CHECK_NEAR("KOTO do -7.8", tuning_offset(&t, 8), -7.8, 0.05);
    CHECK_NEAR("KOTO fa# +3.9", tuning_offset(&t, 2), 3.9, 0.05);

    /* USER: per-pitch-class trim */
    t.tuning = TN_USER;
    for (k = 0; k < 12; k++)
        t.user[k] = (float)k;
    CHECK_NEAR("USER trim of do = 8", tuning_offset(&t, 8), 8.0, 1e-5);
    CHECK_NEAR("USER trim is not scaled by depth", (t.depth = 0.5f, tuning_offset(&t, 8)), 8.0, 1e-5);
    t.depth = 1.0f;
    t.tuning = TN_EQUAL;

    /* scales (Ginken, file 07): YO raises fa and do a semitone, MIN'YO a whole tone */
    t.scale = SC_YO;
    CHECK_NEAR("YO white key 2 = fa# (200)", tuning_white_cents(&t, 1), -2400 + 200, 1e-4);
    CHECK_NEAR("YO white key 5 = do# (900)", tuning_white_cents(&t, 4), -2400 + 900, 1e-4);
    t.scale = SC_MINYO;
    CHECK_NEAR("MIN'YO white key 2 = sol (300)", tuning_white_cents(&t, 1), -2400 + 300, 1e-4);
    CHECK_NEAR("MIN'YO white key 5 = re (1000)", tuning_white_cents(&t, 4), -2400 + 1000, 1e-4);
    t.scale = SC_YO;
    t.tuning = TN_KOTO;
    CHECK_NEAR("YO in KOTO tuning: fa# takes the fa# offset", tuning_white_cents(&t, 1), -2400 + 200 + 3.9, 0.05);
    t.scale = SC_IN;
    t.tuning = TN_EQUAL;

    /* fine tune and reference */
    t.fine = -50.0f;
    CHECK_NEAR("fine -50 (-1/2 本)", tuning_white_cents(&t, 5), -1200 - 50, 1e-4);
    t.fine = 0;
    CHECK_NEAR("A = 442: 三 at 1本 = 221 Hz", tuning_hz(tuning_white_cents(&t, 5), 442.0f), 221.0, 1e-3);

    /* black keys, upper-row mode: key b = degree b % 5 of fa# sol ti-flat do# re, octave b / 5 as the whites */
    CHECK_NEAR("black key 1 = fa# over white octave 0", tuning_black_cents(&t, 0), -2400 + 200, 1e-4);
    CHECK_NEAR("black key 3 = ti-flat (600)", tuning_black_cents(&t, 2), -2400 + 600, 1e-4);
    CHECK_NEAR("black key 6 = fa# an octave up", tuning_black_cents(&t, 5), -1200 + 200, 1e-4);
    CHECK_NEAR("black key 11 = fa# two octaves up", tuning_black_cents(&t, 10), 200, 1e-4);
    t.tuning = TN_SUIKO;
    CHECK_NEAR("black key 3 in SUIKO = 600 - 25", tuning_black_cents(&t, 2), -2400 + 575, 1e-4);

    /* the names shown on the screen */
    CHECK("hon names", !strcmp(TN_HON_NAME[0], "\xe6\xb0\xb4" "4") && !strcmp(TN_HON_NAME[4], "1") && !strcmp(TN_HON_NAME[15], "12"));
    return tu_done("tuning");
}
