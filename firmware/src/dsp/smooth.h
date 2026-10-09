/* SPDX-License-Identifier: GPL-3.0-only */
/* FuMi-1 control smoother: a stepped target (an encoder detent, a tuning switch, a bend step) becomes a
 * glide. One-pole in the cents domain, then a slew limit, stepped once per audio block; the engine
 * interpolates between blocks. Nothing that reaches an oscillator's pitch or a gain should skip it. */
#pragma once
typedef struct {
    float y, target, k, slew;       /* value, target, one-pole coefficient per block, max change per block */
} smooth_t;
void smooth_init(smooth_t *s, float tc_ms, float slew_per_ms, float block_ms);   /* slew 0 = no limit */
void smooth_set(smooth_t *s, float target);
void smooth_jump(smooth_t *s, float v);         /* land at v at once (a new note, not a glide) */
float smooth_step(smooth_t *s);                 /* one block on: the value now */
