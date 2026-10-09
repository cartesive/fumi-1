/* SPDX-License-Identifier: GPL-3.0-only */
/* Tiny test helpers for the FuMi host tests: a check that prints and counts, and a close-enough compare. */
#pragma once
#include <stdio.h>
#include <math.h>
static int tu_fail, tu_pass;
#define CHECK(what, ok) do { int ok_ = (ok); printf("  %s %s\n", ok_ ? "ok  " : "FAIL", what); if (ok_) tu_pass++; else tu_fail++; } while (0)
#define CHECK_NEAR(what, got, want, tol) do { double g_ = (got), w_ = (want); int ok_ = fabs(g_ - w_) <= (tol); \
    printf("  %s %s: %.4f (want %.4f, tol %g)\n", ok_ ? "ok  " : "FAIL", what, g_, w_, (double)(tol)); if (ok_) tu_pass++; else tu_fail++; } while (0)
static int tu_done(const char *name)
{
    printf("%s: %d passed, %d failed\n", name, tu_pass, tu_fail);
    return tu_fail ? 1 : 0;
}
