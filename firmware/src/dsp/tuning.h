/* SPDX-License-Identifier: GPL-3.0-only */
/* FuMi-1 tuning: the shigin scales, the ST-50's measured 純正律, the 本数 (key) slider and the key maps.
 * Everything is in cents, as floats; a frequency is made once, at the end (tuning_hz).
 *
 *   white key w (0..15)  = mi fa la ti do, three times, then a top mi. 三 (the reference tonic) is key 5.
 *   black key b (0..10)  = the ST-50's upper row fa# sol ti-flat do# re (upper-row mode), five per octave.
 *   本数 (hon)            = 16 positions, 水4 水3 水2 水1 1 2 .. 12: 三 = MIDI 53 + position (docs/research/14).
 *
 * The tuning tables are indexed by pitch class above mi (0..11 semitones), so any scale and the upper row
 * read the same table. Sources: docs/research/13 and 14 (SUIKO, measured from two recordings), 07 (KOTO,
 * pure fifths). Unmeasured entries are marked in tuning.c and are 0 until measured. */
#pragma once
#include <stdint.h>

enum { TN_EQUAL, TN_SUIKO, TN_KOTO, TN_USER, TN_N };    /* 平均律, 純正律 as measured, pure fifths, per-note trim */
enum { SC_IN, SC_YO, SC_MINYO, SC_N };                  /* 陰, 陽, 民謡 (Ginken's names, docs/research/07) */
#define TN_NHON 16
#define TN_NPC 12

typedef struct {
    uint8_t scale, tuning;
    int8_t hon;                     /* 0..15: 水4 .. 12本 */
    int8_t octave;                  /* -1, 0, 1 (OCT- / OCT+ with no note held) */
    float depth;                    /* scales the SUIKO / KOTO offsets: 0 = equal, 1 = as measured */
    float fine;                     /* 微調, cents, -50 .. 50 */
    float a4;                       /* reference, Hz (440 default; 430 .. 445) */
    float user[TN_NPC];             /* USER: cents per pitch class above mi */
} tuning_t;

extern const uint8_t TN_SCALE[SC_N][5];          /* semitones above mi of the five lower-row degrees */
extern const uint8_t TN_UPPER[5];                /* semitones above mi of the upper row */
extern const float TN_TABLE[TN_N][TN_NPC];       /* offsets from equal temperament, cents (USER row unused) */
extern const char *const TN_HON_NAME[TN_NHON];   /* "水4" .. "12" (UTF-8) */
extern const char *const TN_HON_ASCII[TN_NHON];  /* "S4" .. "12": the device font has no kanji */
extern const char *const TN_NAME[TN_N];
extern const char *const SC_NAME[SC_N];

void tuning_defaults(tuning_t *t);
int tuning_san_midi(const tuning_t *t);                 /* MIDI note of 三 (white key 5): 53 + hon + 12 * octave */
float tuning_offset(const tuning_t *t, int pc);         /* the tuning's offset of pitch class pc, with depth */
float tuning_white_cents(const tuning_t *t, int w);     /* white key w -> cents from A4 (fine included) */
float tuning_black_cents(const tuning_t *t, int b);     /* black key b in upper-row mode -> cents from A4 */
float tuning_hz(float cents_from_a4, float a4);
