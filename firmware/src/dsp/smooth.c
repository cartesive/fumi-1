/* SPDX-License-Identifier: GPL-3.0-only */
/* FuMi-1 control smoother (smooth.h). */
#include "smooth.h"
#include "fastmath.h"

void smooth_init(smooth_t *s, float tc_ms, float slew_per_ms, float block_ms)
{
    s->y = s->target = 0.0f;
    s->k = tc_ms > 0.0f ? 1.0f - fm_expf(-block_ms / tc_ms) : 1.0f;
    s->slew = slew_per_ms > 0.0f ? slew_per_ms * block_ms : 0.0f;
}

void smooth_set(smooth_t *s, float target) { s->target = target; }

void smooth_jump(smooth_t *s, float v) { s->y = s->target = v; }

float smooth_step(smooth_t *s)
{
    float d = (s->target - s->y) * s->k;
    if (s->slew > 0.0f)
        d = d > s->slew ? s->slew : d < -s->slew ? -s->slew : d;
    s->y += d;
    if (fm_fabsf(s->target - s->y) < 1e-4f)
        s->y = s->target;
    return s->y;
}
