/* SPDX-License-Identifier: GPL-3.0-only */
/* dsp/bend.c: OCT- / OCT+ as a sprung pitch rocker when a note is held (docs/research/06, 00). */
#include "tu.h"
#include "../../firmware/src/dsp/bend.c"

static float run_to(bend_t *b, uint32_t *now, uint32_t until)
{
    float v = bend_value(b, *now);
    while (*now < until) {
        (*now)++;
        v = bend_value(b, *now);
    }
    return v;
}

int main(void)
{
    bend_t b;
    uint32_t now = 1000;
    float v;
    bend_init(&b);
    bend_config(&b, 100.0f, -40.0f, 90.0f, 120.0f);
    CHECK("idle: 0 cents", bend_value(&b, now) == 0.0f);

    /* no note held: the button is an octave shift, not a bend */
    CHECK("OCT+ with no note: not consumed (octave)", bend_button(&b, BEND_UP, 1, 0, now) == 0);
    CHECK("its release is not consumed either", bend_button(&b, BEND_UP, 0, 1, now + 10) == 0);
    CHECK("still 0 cents", run_to(&b, &now, now + 200) == 0.0f);

    /* OCT+ held with a note: an S-shaped rise over 90 ms to +100, held, then 120 ms back */
    CHECK("OCT+ with a note: consumed as a bend", bend_button(&b, BEND_UP, 1, 1, now) == 1);
    v = run_to(&b, &now, now + 45);
    CHECK_NEAR("half way in time = half way in pitch (S-curve)", v, 50.0, 2.0);
    v = run_to(&b, &now, now + 45);
    CHECK_NEAR("at 90 ms: the target", v, 100.0, 0.5);
    v = run_to(&b, &now, now + 500);
    CHECK_NEAR("held: stays at the target", v, 100.0, 1e-4);
    CHECK("release consumed", bend_button(&b, BEND_UP, 0, 1, now) == 1);
    v = run_to(&b, &now, now + 60);
    CHECK_NEAR("half way back after 60 ms", v, 50.0, 2.0);
    v = run_to(&b, &now, now + 60);
    CHECK_NEAR("back at 0 after 120 ms", v, 0.0, 0.5);
    v = run_to(&b, &now, now + 100);
    CHECK("exactly 0 once home", v == 0.0f);

    /* a tap: up and straight back (tsuki-iro), the full flick even though the press was short */
    CHECK("tap down", bend_button(&b, BEND_UP, 1, 1, now) == 1);
    now += 20;
    CHECK("tap up at 20 ms", bend_button(&b, BEND_UP, 0, 1, now) == 1);
    v = run_to(&b, &now, now + 70);
    CHECK_NEAR("a tap still reaches the target", v, 100.0, 0.5);
    v = run_to(&b, &now, now + 120);
    CHECK_NEAR("then returns", v, 0.0, 0.5);

    /* OCT-: the small drop (hiki-iro) */
    bend_button(&b, BEND_DOWN, 1, 1, now);
    v = run_to(&b, &now, now + 90);
    CHECK_NEAR("OCT- held: -40", v, -40.0, 0.5);
    bend_button(&b, BEND_DOWN, 0, 1, now);
    run_to(&b, &now, now + 200);

    /* both held: reserved for update mode; the bend lets go and neither button is consumed */
    bend_button(&b, BEND_UP, 1, 1, now);
    run_to(&b, &now, now + 90);
    CHECK("second button while the first bends: not consumed", bend_button(&b, BEND_DOWN, 1, 1, now) == 0);
    v = run_to(&b, &now, now + 150);
    CHECK_NEAR("both held: the bend returns to 0", v, 0.0, 0.5);
    CHECK("releases of the pair are not consumed", bend_button(&b, BEND_DOWN, 0, 1, now) == 0 && bend_button(&b, BEND_UP, 0, 1, now) == 0);
    CHECK("still 0", run_to(&b, &now, now + 50) == 0.0f);

    /* the note let go before the button: the button stays a bender until released, release is not an octave */
    bend_button(&b, BEND_UP, 1, 1, now);
    run_to(&b, &now, now + 90);
    CHECK("release with no note held: still consumed (it was a bend)", bend_button(&b, BEND_UP, 0, 0, now) == 1);
    v = run_to(&b, &now, now + 150);
    CHECK_NEAR("returned", v, 0.0, 0.5);

    /* the role is fixed at the press: a note pressed after an octave press does not turn it into a bend */
    CHECK("octave press", bend_button(&b, BEND_UP, 1, 0, now) == 0);
    v = run_to(&b, &now, now + 100);
    CHECK("a note now held changes nothing", v == 0.0f);
    CHECK("release", bend_button(&b, BEND_UP, 0, 1, now) == 0);

    /* a new target while gliding starts from the current value, no jump */
    bend_config(&b, 200.0f, -40.0f, 90.0f, 120.0f);
    bend_button(&b, BEND_UP, 1, 1, now);
    v = run_to(&b, &now, now + 45);
    bend_button(&b, BEND_UP, 0, 1, now);
    {
        float a = bend_value(&b, now), c = bend_value(&b, now + 1);
        CHECK_NEAR("release mid-glide continues from where it was", a, v, 1e-3);
        CHECK("and the next millisecond is close by", fabs((double)c - (double)a) < 10.0);
    }
    return tu_done("bend");
}
