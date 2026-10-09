/* SPDX-License-Identifier: GPL-3.0-only */
/* FuMi-1: the instrument. A shigin conductor after the Suiko ST-50, on 6-operator FM (fm6_core.c).
 *
 *   Keys     white keys 0..15 = mi fa la ti do three times and a top mi (tuning.h); black keys 0..10 = the
 *            ST-50's upper row, or ornaments (the app decides: fm_key / fm_ornament).
 *   Pitch    tuning.c (本数, scale, 調律, 微調) -> a target in cents per voice -> smooth.c (no steps reach an
 *            oscillator) -> + vibrato -> fm6_logfreq_cents. The bend buttons (bend.c) move the most recent note.
 *   Voices   FM_NVOICE notes of the current patch (a 155-byte voice, fm6_patch.c); re-plucking a key
 *            re-excites its own voice; stealing retriggers without a phase reset (no click).
 *   余韻      a release gain per voice after key-up, 20 ms .. 8 s. トリラー re-plucks the held note at a rate with
 *            a little variation. ビブラート a sine on the pitch. 単音 last-note priority.
 *   Output   low cut, high cut, character (tilt and warmth), the plate reverb (reverb.c), soft clip.
 *
 * Threads: fm_render() runs in the audio interrupt; everything else posts commands that the render drains at
 * the start of a block. The state marked (UI) is written by the render and only read elsewhere. Float only,
 * no libm (fastmath.h), no double. */
#pragma once
#include <stdint.h>
#include "tuning.h"

#define FM_SR 44100.0f
#define FM_NVOICE 8
#define FM_NWHITE 16
#define FM_NBLACK 11
#define FM_NKEY (FM_NWHITE + FM_NBLACK)          /* fm_key ids: 0..15 white, 16..26 black */
#define FM_NOTE_ID 32                            /* fm_note ids start here (MIDI, the bench) */

/* parameters (the project saves them; ranges and defaults in fm_param_info) */
enum {
    P_HON, P_SCALE, P_TUNING, P_DEPTH, P_FINE, P_A4, P_OCTAVE,           /* 本数, scale, 調律, depth %, 微調, A, octave */
    P_VOICE, P_YOIN, P_LEVEL,                                            /* patch slot, 余韻, output level */
    P_VIB_ON, P_VIB_RATE, P_VIB_DEPTH,                                   /* ビブラート */
    P_TRILL_ON, P_TRILL_RATE, P_TRILL_VAR,                               /* トリラー: re-plucks at a rate, with variation */
    P_MONO, P_BLACK, P_BEND_UP, P_BEND_DOWN, P_BEND_TIME, P_SLIDE, P_SLIDE_TIME,
    P_LOWCUT, P_HIGHCUT, P_CHARACTER, P_REVERB, P_REV_SIZE, P_MIDI_OUT,
    P_USER0, P_USER1, P_USER2, P_USER3, P_USER4, P_USER5, P_USER6, P_USER7, P_USER8, P_USER9, P_USER10, P_USER11,
    P_LOOP_BPM, P_LOOP_BEATS, P_LOOP_CLICK,                              /* the looper's ruler and its click */
    P_NPARAMS
};
typedef struct {
    const char *name;
    int16_t lo, hi, def;
} fm_param_t;
const fm_param_t *fm_param_info(int p);
void fm_param_text(int p, int v, char *buf);     /* as the screen shows it (12 bytes) */

enum { BEND_T_SEMI, BEND_T_WHOLE, BEND_T_SCALE };          /* P_BEND_UP */
enum { ORN_VIB, ORN_TRILL, ORN_DAMP, ORN_STRONG, ORN_N };   /* fm_ornament */

/* commands (main loop -> render) */
void fm_init(void);
void fm_set(int param, int value);
void fm_key(int key, int on);                    /* a panel key, FM_NKEY ids */
void fm_note(int id, int on, float cents);       /* any pitch (cents from A4): MIDI, the bench; id >= FM_NOTE_ID */
void fm_ornament(int orn, int on);               /* momentary techniques on the sounding notes */
void fm_bend(int which, int down, int note_held);   /* OCT- / OCT+ (bend.h BEND_DOWN / BEND_UP) */
void fm_panic(void);
/* the looper: REC arms, the first key starts the loop; REC again closes it (or the ruler does, at BPM x beats)
 * and it plays; REC while playing overdubs on a new layer; PLAY stops and starts from the top; undo takes the
 * top layer; clear empties it. Keys, ornaments and the bend buttons are recorded with their timing from the audio
 * clock; the loop stores keys, not pitches, so 本数 and 調律 retune it. One instrument; the loop lives in RAM. */
enum { LP_IDLE, LP_ARMED, LP_REC, LP_PLAY, LP_DUB, LP_STOP, LP_NSTATE };
void fm_loop_rec(void);                          /* the REC tap */
void fm_loop_play(void);                         /* the PLAY tap */
void fm_loop_undo(void);                         /* PLAY held: the top layer goes */
void fm_loop_clear(void);                        /* REC held */
extern volatile uint8_t fm_loop_state, fm_loop_layers;                /* LP_*, layers recorded */
extern volatile uint32_t fm_loop_pos, fm_loop_len, fm_loop_events;   /* position and length in blocks of 32; events */

void fm_patch_set(const uint8_t *packed128);     /* the current patch (a DX7-format packed voice) */
void fm_patch_get(uint8_t *packed128);           /* main loop: the patch the render uses */
int fm_patch_count(void);                        /* FuMi's own patches (P_VOICE) */
const char *fm_patch_name(int i);
const char *fm_patch_ja(int i);                   /* its Japanese name, UTF-8 (the screen uses fumi_kanji.h) */
const uint8_t *fm_patch_builtin(int i);          /* its packed record */
float fm_key_cents(int key);                     /* main loop: the pitch a panel key plays now, cents from A4 */

/* the render (audio ISR): n stereo frames (a multiple of 32), 24-bit in int32; gain Q12 (MASTER) */
void fm_render(int32_t *out_lr, uint32_t n, uint32_t gain_q12);

/* MIDI out, from the render: the platform sends it (status, data 1, data 2) */
void fm_midi_out(uint32_t st, uint32_t d1, uint32_t d2);

/* state for the screen (UI) */
extern volatile float fm_key_level[FM_NKEY];     /* each key's voice level, 0..1 (lights) */
extern volatile float fm_bend_cents;             /* the bend now */
extern volatile float fm_peak;                   /* output peak of the last block before the soft clip (1 = full scale) */
extern volatile uint8_t fm_nvoices;              /* voices sounding */
extern volatile char fm_patch_label[11];         /* the current patch's name */
extern volatile uint8_t fm_patch_custom;         /* 1: a patch sent by fm_patch_set, not a P_VOICE slot */
