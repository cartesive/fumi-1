/* SPDX-License-Identifier: GPL-3.0-only */
/* FuMi-1 tuning (tuning.h). Pure functions over a tuning_t; no state, no libm. */
#include "tuning.h"
#include "fastmath.h"

const uint8_t TN_SCALE[SC_N][5] = {
    {0, 1, 5, 7, 8},                /* IN 陰: mi fa la ti do (the ST-50's lower row) */
    {0, 2, 5, 7, 9},                /* YO 陽: fa and do raised a semitone */
    {0, 3, 5, 7, 10},               /* MIN'YO 民謡: fa and do raised a whole tone */
};
const uint8_t TN_UPPER[5] = {2, 3, 6, 9, 10};   /* fa# sol ti-flat do# re (docs/research/10) */

/* offsets from equal temperament by pitch class above mi: 0 mi, 1 fa, 2 fa#, 3 sol, 4 (unused), 5 la,
 * 6 ti-flat, 7 ti, 8 do, 9 do#, 10 re, 11 (unused) */
const float TN_TABLE[TN_N][TN_NPC] = {
    /* 平均律 */
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    /* SUIKO = the ST-50's 純正律 as measured (docs/research/13, 14): fa -20.8, do -16.0 from two recordings
     * in two keys, within a cent of each other; ti-flat -25 from 31 frames of one recording (weak).
     * UNMEASURED, left at 0 until a clip has them held: fa# (2), sol (3), do# (9), re (10). */
    {0, -20.8f, 0, 0, 0, 0, -25.0f, 0, -16.0f, 0, 0, 0},
    /* KOTO = pure fifths and fourths (hira-joshi by ear, docs/research/07): 256/243, 9/8, 32/27, 81/64,
     * 4/3, 1024/729 (ti-flat as a lowered ti; derived, not sourced), 3/2, 128/81, 27/16, 16/9, 243/128 */
    {0, -9.78f, 3.91f, -5.87f, 7.82f, -1.96f, -11.73f, 1.96f, -7.82f, 5.87f, -3.91f, 9.78f},
    /* USER: the trims live in tuning_t.user */
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
};
const char *const TN_HON_NAME[TN_NHON] = {"\xe6\xb0\xb4" "4", "\xe6\xb0\xb4" "3", "\xe6\xb0\xb4" "2", "\xe6\xb0\xb4" "1",
                                          "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12"};
const char *const TN_HON_ASCII[TN_NHON] = {"S4", "S3", "S2", "S1", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12"};
const char *const TN_NAME[TN_N] = {"EQUAL", "SUIKO", "KOTO", "USER"};
const char *const SC_NAME[SC_N] = {"IN", "YO", "MIN'YO"};

void tuning_defaults(tuning_t *t)
{
    int i;
    t->scale = SC_IN;
    t->tuning = TN_SUIKO;
    t->hon = 4;                     /* 1本: 三 = A3, white key 1 = A2 */
    t->octave = 0;
    t->depth = 1.0f;
    t->fine = 0.0f;
    t->a4 = 440.0f;
    for (i = 0; i < TN_NPC; i++)
        t->user[i] = 0.0f;
}

int tuning_san_midi(const tuning_t *t)
{
    int h = t->hon < 0 ? 0 : t->hon >= TN_NHON ? TN_NHON - 1 : t->hon;
    return 53 + h + 12 * t->octave;
}

float tuning_offset(const tuning_t *t, int pc)
{
    pc = ((pc % TN_NPC) + TN_NPC) % TN_NPC;
    if (t->tuning == TN_USER)
        return t->user[pc];
    return TN_TABLE[t->tuning < TN_N ? t->tuning : TN_EQUAL][pc] * t->depth;
}

/* the pitch of a degree pc semitones above the mi of octave oct (0 = the octave below 三, 1 = 三's) */
static float tuning_cents(const tuning_t *t, int pc, int oct)
{
    int midi = tuning_san_midi(t) + 12 * (oct - 1) + pc;
    return 100.0f * (float)(midi - 69) + tuning_offset(t, pc) + t->fine;
}

float tuning_white_cents(const tuning_t *t, int w)
{
    w = w < 0 ? 0 : w > 15 ? 15 : w;
    return tuning_cents(t, TN_SCALE[t->scale < SC_N ? t->scale : SC_IN][w % 5], w / 5);
}

float tuning_black_cents(const tuning_t *t, int b)
{
    b = b < 0 ? 0 : b > 10 ? 10 : b;
    return tuning_cents(t, TN_UPPER[b % 5], b / 5);
}

float tuning_hz(float cents_from_a4, float a4) { return a4 * fm_exp2f(cents_from_a4 * (1.0f / 1200.0f)); }
