/* SPDX-License-Identifier: GPL-3.0-only */
/* FuMi-1 bend buttons (bend.h). */
#include "bend.h"

void bend_init(bend_t *b)
{
    b->cur = b->from = b->to = b->dur = 0.0f;
    b->t0 = 0;
    b->role[0] = b->role[1] = 0;
    b->active = b->flick = 0;
    bend_config(b, 100.0f, -40.0f, 90.0f, 120.0f);
}

void bend_config(bend_t *b, float up_cents, float down_cents, float t_up_ms, float t_down_ms)
{
    b->target[BEND_UP] = up_cents;
    b->target[BEND_DOWN] = down_cents;
    b->t_up = t_up_ms < 1.0f ? 1.0f : t_up_ms;
    b->t_down = t_down_ms < 1.0f ? 1.0f : t_down_ms;
}

static void glide(bend_t *b, float to, float dur, uint32_t now)
{
    b->from = b->cur;
    b->to = to;
    b->dur = dur;
    b->t0 = now;
}

static void let_go(bend_t *b, uint32_t now)
{
    b->flick = 0;
    b->active = 0;
    if (b->cur != 0.0f || b->dur > 0.0f)
        glide(b, 0.0f, b->t_down, now);
}

int bend_button(bend_t *b, int which, int down, int note_held, uint32_t now_ms)
{
    int other = which ? 0 : 1;
    which &= 1;
    bend_value(b, now_ms);
    if (down) {
        if (b->role[other]) {                     /* both held: reserved (update mode); the bend lets go */
            b->role[other] = b->role[which] = 2;
            let_go(b, now_ms);
            return 0;
        }
        if (!note_held) {
            b->role[which] = 2;
            return 0;
        }
        b->role[which] = 1;
        b->active = (uint8_t)(which + 1);
        b->flick = 0;
        glide(b, b->target[which], b->t_up, now_ms);
        return 1;
    }
    {
        int was = b->role[which];
        b->role[which] = 0;
        if (was != 1)
            return 0;
        if (b->active == which + 1) {
            if (b->dur > 0.0f)
                b->flick = 1;                     /* a tap: finish the rise, then return */
            else
                let_go(b, now_ms);
        }
        return 1;
    }
}

float bend_value(bend_t *b, uint32_t now_ms)
{
    if (b->dur > 0.0f) {
        float u = (float)(now_ms - b->t0) / b->dur;
        if (u >= 1.0f) {
            b->cur = b->to;
            b->dur = 0.0f;
            if (b->flick)
                let_go(b, now_ms);
        } else {
            float s = u * u * (3.0f - 2.0f * u);  /* the S-curve: a finger pressing a string */
            b->cur = b->from + (b->to - b->from) * s;
        }
    }
    return b->cur;
}
