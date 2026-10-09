/* SPDX-License-Identifier: GPL-3.0-only */
/* FuMi-1 bend buttons: OCT- / OCT+ as the ST-50's sprung 音程 rocker (docs/research/06, 00).
 *
 *   The role is decided at the press. A note held: the button is a bender until it is released; the pitch
 *   glides to the target over the up time (an S-curve) and stays; the release glides back over the down
 *   time. A tap (released before the target) still completes the flick: up and straight back. No note held:
 *   the button is an octave shift (not this module's business: bend_button returns 0).
 *   Both buttons held is update mode: the bend lets go and neither press or release is consumed. The caller
 *   must also ignore octave shifts while the other button is held.
 * The value is read once per block (bend_value, in cents) and goes through the smoother after that. */
#pragma once
#include <stdint.h>
enum { BEND_DOWN, BEND_UP };
typedef struct {
    float target[2];                /* cents: BEND_DOWN (negative), BEND_UP */
    float t_up, t_down;             /* ms */
    float cur, from, to, dur;       /* the glide: from -> to over dur ms from t0; dur 0 = at rest at cur */
    uint32_t t0;
    uint8_t role[2];                /* per button while held: 0 up, 1 bender, 2 octave or reserved */
    uint8_t active;                 /* the bender driving the glide + 1, 0 none */
    uint8_t flick;                  /* released before the target: return once it is reached */
} bend_t;
void bend_init(bend_t *b);
void bend_config(bend_t *b, float up_cents, float down_cents, float t_up_ms, float t_down_ms);
/* a press (down = 1) or release of BEND_DOWN / BEND_UP; note_held: any note key down now.
 * 1 = taken as a bend; 0 = not a bend (an octave shift on a press; nothing on a release) */
int bend_button(bend_t *b, int which, int down, int note_held, uint32_t now_ms);
float bend_value(bend_t *b, uint32_t now_ms);    /* cents now */
