/* SPDX-License-Identifier: GPL-3.0-only */
/* YIN pitch for the host tests (double is fine here: test code). */
#pragma once
#include <math.h>
#include <stdlib.h>
/* the pitch of x[0..n) at sr Hz: the first dip of the cumulative-mean-normalised difference under the threshold
 * between the periods of 2000 and 25 Hz, parabolic-interpolated; 0 if nothing periodic */
static double tu_yin(const float *x, int n, double sr)
{
    int W = n / 2, lo = (int)(sr / 2000.0), hi = (int)(sr / 25.0), tau, t, best = 0;
    double *d, *cm, run = 0, y0, y1, y2, den, tf;
    if (hi > W - 2)
        hi = W - 2;
    if (lo < 2)
        lo = 2;
    d = calloc((size_t)hi + 2, sizeof *d);
    cm = calloc((size_t)hi + 2, sizeof *cm);
    for (tau = 1; tau <= hi + 1; tau++) {
        double s = 0;
        for (t = 0; t < W; t++) {
            double v = (double)x[t] - (double)x[t + tau];
            s += v * v;
        }
        d[tau] = s;
    }
    cm[0] = 1;
    for (tau = 1; tau <= hi + 1; tau++) {
        run += d[tau];
        cm[tau] = run > 0 ? d[tau] * tau / run : 1.0;
    }
    for (tau = lo; tau < hi; tau++)
        if (cm[tau] < 0.15 && cm[tau] <= cm[tau + 1]) {
            best = tau;
            break;
        }
    if (!best) {
        double m = 1e9;
        for (tau = lo; tau < hi; tau++)
            if (cm[tau] < m) {
                m = cm[tau];
                best = tau;
            }
        if (m > 0.5) {
            free(d);
            free(cm);
            return 0;
        }
    }
    y0 = cm[best - 1];
    y1 = cm[best];
    y2 = cm[best + 1];
    den = y0 - 2 * y1 + y2;
    tf = best + (den != 0 ? 0.5 * (y0 - y2) / den : 0);
    free(d);
    free(cm);
    return sr / tf;
}
static double tu_cents(double hz, double ref) { return 1200.0 * log2(hz / ref); }
