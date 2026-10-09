/* SPDX-License-Identifier: GPL-3.0-only */
/* dsp/smooth.c: the control smoother that turns stepped targets (encoders, tuning switches) into glides. */
#include "tu.h"
#include "../../firmware/src/dsp/smooth.c"

int main(void)
{
    smooth_t s;
    int i, n_tc;
    float v = 0, prev;
    /* a 30 ms time constant, no slew limit, 0.7256 ms blocks (32 samples at 44.1 kHz) */
    smooth_init(&s, 30.0f, 0.0f, 32.0f / 44.1f);
    CHECK("starts at 0", smooth_step(&s) == 0.0f);
    smooth_set(&s, 100.0f);
    n_tc = (int)(30.0f / (32.0f / 44.1f) + 0.5f);
    for (i = 0; i < n_tc; i++)
        v = smooth_step(&s);
    CHECK_NEAR("63 % of a 100 cent step after one time constant", v, 63.2, 6.0);
    for (i = 0; i < 20 * n_tc; i++)
        v = smooth_step(&s);
    CHECK_NEAR("converges", v, 100.0, 1e-3);
    /* monotonic: never overshoots */
    smooth_set(&s, 0.0f);
    prev = smooth_step(&s);
    for (i = 0; i < 5000; i++) {
        float x = smooth_step(&s);
        if (x > prev || x < 0.0f) {
            CHECK("monotonic without overshoot", 0);
            return tu_done("smooth");
        }
        prev = x;
    }
    CHECK("monotonic without overshoot", 1);
    /* slew limit: 10 cents per ms at most */
    smooth_init(&s, 1.0f, 10.0f, 1.0f);
    smooth_set(&s, 1000.0f);
    v = smooth_step(&s);
    CHECK_NEAR("a block of 1 ms moves at most 10 cents", v, 10.0, 1e-4);
    /* jump: immediate, no glide */
    smooth_jump(&s, -300.0f);
    CHECK_NEAR("jump lands at once", smooth_step(&s), -300.0, 1e-6);
    CHECK_NEAR("and stays", smooth_step(&s), -300.0, 1e-6);
    return tu_done("smooth");
}
