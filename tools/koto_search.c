/* SPDX-License-Identifier: GPL-3.0-only */
/* koto_search: a search over FM6 settings (FuMi's three-stack algorithm 5 layout) against the ST-50 koto
 * measurements of docs/research/13, section 4, at A4. The engine renders one second of each candidate; the
 * cost sums the distance from the targets below. Random restarts and hill climbing with 1-3 mutations a step.
 * KOTO A and B in tools/fumi_patches.py came out of it (seeds 5 and 11, 15000 steps); the numbers it prints go
 * into fumi_patches.py by hand, and tests/host/fumi_test.c measures the result again through the whole engine.
 *
 *   cc -O2 -std=gnu99 -Wno-unused-function -Ifirmware/src/dsp -o build/host/koto_search tools/koto_search.c -lm
 *   build/host/koto_search [SEED] [STEPS]
 *
 * Targets (A4): partials at onset 0, 0, -2, -9, -6 dB and the 6th up below -14; at 300 ms 0, -6, -2, -2 and
 * the 5th up below -20; -28 dB/s over the first 200 ms then -16; click 7-9 dB above 5 kHz in the first 12 ms
 * against the next 100 ms; peak within 5 ms; peak level 0.8 .. 1.2 of the engine's scale. Not in the cost:
 * everything the ear hears that these numbers do not, so the bench has the last word. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include "fm6_core.c"
#include "fm6_patch.c"
#define N 48510
static fm6_note_t note; static fm6_lfo_t lfo; static float buf[N + 64];
static const int32_t DT0[6] = {0,0,0,0,0,0};
/* the parameters: OP1..OP6 (op index 0..5 = OP1..OP6), each: ol, r2, l2, r3, l3; plus OP2 fc, OP5 fc, fb */
typedef struct { int ol[6], r2[6], l2[6], r3[6], l3[6], fc2, fc4, fc5, fc6, fb; } P;
static void build(const P *p, uint8_t *v) {
    int k, i;
    memset(v, 0, FP_SIZE + 1);
    for (k = 0; k < 6; k++) {                 /* v holds OP6 first: op OPn at slot 6-n */
        uint8_t *op = v + (5 - k) * FP_OP;
        op[FP_R1] = 99; op[FP_R1 + 1] = (uint8_t)p->r2[k]; op[FP_R1 + 2] = (uint8_t)p->r3[k]; op[FP_R1 + 3] = 60;
        op[FP_L1] = 99; op[FP_L1 + 1] = (uint8_t)p->l2[k]; op[FP_L1 + 2] = (uint8_t)p->l3[k]; op[FP_L1 + 3] = 0;
        op[FP_BP] = 39; op[FP_FC] = 1; op[FP_DET] = 7; op[FP_OL] = (uint8_t)p->ol[k];
        op[FP_RS] = (uint8_t)(k == 1 ? 4 : k == 3 ? 3 : 2);
        if (k == 0 || k == 2) { op[FP_RD] = (uint8_t)(k == 0 ? 25 : 30); op[FP_RC] = 0; }
    }
    v[4 * FP_OP + FP_FC] = (uint8_t)p->fc2; v[2 * FP_OP + FP_FC] = (uint8_t)p->fc4; v[1 * FP_OP + FP_FC] = (uint8_t)p->fc5; v[0 * FP_OP + FP_FC] = (uint8_t)p->fc6;
    v[1 * FP_OP + FP_DET] = 9; v[0 * FP_OP + FP_DET] = 5;
    for (i = 0; i < 4; i++) { v[FP_PR1 + i] = 99; v[FP_PL1 + i] = 50; }
    v[FP_ALG] = 4; v[FP_FB] = (uint8_t)p->fb; v[FP_OKS] = 1; v[FP_LFS] = 35; v[FP_LPMS] = 3; v[FP_TRNSP] = 24;
    memcpy(v + FP_NAME, "KOTO      ", 10);
}
static float render(const uint8_t *v) {
    int i, k; int32_t bus[FM6_N]; float pk = 0;
    fm6_lfo_reset(&lfo, v); fm6_lfo_key(&lfo);
    fm6_note_init(&note, v, 69, 100, 1);
    for (i = 0; i + FM6_N <= N; i += FM6_N) {
        int32_t lv = fm6_lfo_sample(&lfo), ld = fm6_lfo_delay(&lfo);
        if (!fm6_note_compute(&note, v, bus, lv, ld, fm6_logfreq_cents(6900.0f), v[FP_ALG], v[FP_FB], DT0, 0)) memset(bus, 0, sizeof bus);
        for (k = 0; k < FM6_N; k++) { buf[i + k] = (float)bus[k] / 8300000.0f; if (fabsf(buf[i + k]) > pk) pk = fabsf(buf[i + k]); }
    }
    return pk;
}
static void partials(int t0, int w, double *db) {       /* dB of partials 1..8 relative to the strongest */
    int p, i; double ref = -1e9;
    for (p = 1; p <= 8; p++) { double re = 0, im = 0;
        for (i = 0; i < w; i++) { double ph = 2 * M_PI * 440.0 * p * (t0 + i) / 44100.0, win = 0.5 - 0.5 * cos(2 * M_PI * i / w);
            re += buf[t0 + i] * win * cos(ph); im -= buf[t0 + i] * win * sin(ph); }
        db[p] = 20 * log10(sqrt(re * re + im * im) + 1e-12); if (db[p] > ref) ref = db[p]; }
    for (p = 1; p <= 8; p++) db[p] -= ref;
}
static double rms_db(int a, int b) { double s = 0; int i; for (i = a; i < b; i++) s += buf[i] * buf[i]; return 10 * log10(s / (b - a) + 1e-20); }
static double hp_energy_db(int a, int b) {              /* energy above ~5 kHz: a 2-pole one-pole high pass */
    double y1 = 0, x1 = 0, y2 = 0, x2 = 0, s = 0, k = exp(-2 * M_PI * 5000.0 / 44100.0); int i;
    for (i = (a > 200 ? a - 200 : 0); i < b; i++) { double y = k * (y1 + buf[i] - x1); x1 = buf[i]; y1 = y; double z = k * (y2 + y - x2); x2 = y; y2 = z; if (i >= a) s += z * z; }
    return 10 * log10(s / (b - a) + 1e-20);
}
typedef struct { double p0[9], p3[9], d1, d2, click, attack_ms, pk; } M;
static void measure(const uint8_t *v, M *m) {
    int i, ipk = 0; float pk;
    m->pk = pk = render(v);
    for (i = 0; i < N; i++) if (fabsf(buf[i]) > 0.999f * pk) { ipk = i; break; }
    m->attack_ms = ipk * 1000.0 / 44100.0;
    partials(220, 2048, m->p0); partials(13230, 2048, m->p3);
    { double e0 = rms_db(0, 441), e200 = rms_db(8820 - 441, 8820), e1000 = rms_db(44100 - 441, 44100);
      m->d1 = (e200 - e0) / 0.2; m->d2 = (e1000 - e200) / 0.8; }
    m->click = hp_energy_db(0, 529) - hp_energy_db(529, 4939);
}
static double sq(double x) { return x * x; }
static double below(double x, double lim) { return x > lim ? sq(x - lim) : 0; }
static double cost(const M *m) {
    static const double T0[6] = {0, 0, 0, -2, -9, -6}, T3[5] = {0, 0, -6, -2, -2};
    double c = 0; int p;
    for (p = 1; p <= 5; p++) c += sq(m->p0[p] - T0[p]);
    for (p = 6; p <= 8; p++) c += below(m->p0[p], -14);
    for (p = 1; p <= 4; p++) c += sq(m->p3[p] - T3[p]);
    for (p = 5; p <= 8; p++) c += below(m->p3[p], -20);
    c += 0.5 * sq(m->d1 + 28) + 1.0 * sq(m->d2 + 16);
    c += 4 * sq(m->click - 8);
    c += 4 * below(m->attack_ms, 5);
    c += 50 * below(m->pk, 1.2) + 50 * below(0.8, m->pk);
    return c;
}
static int clampi(int x, int lo, int hi) { return x < lo ? lo : x > hi ? hi : x; }
static void mutate(P *p, unsigned *seed) {
    int k = rand_r(seed) % 6, what = rand_r(seed) % 6, d = (rand_r(seed) % 11) - 5;
    switch (what) {
    case 0: p->ol[k] = clampi(p->ol[k] + d * 2, 0, 99); break;
    case 1: p->r2[k] = clampi(p->r2[k] + d * 2, 20, 99); break;
    case 2: p->l2[k] = clampi(p->l2[k] + d * 2, 0, 99); break;
    case 3: p->r3[k] = clampi(p->r3[k] + d * 2, 10, 99); break;
    case 4: p->l3[k] = clampi(p->l3[k] + d * 2, 0, 99); break;
    case 5: { int r = rand_r(seed) % 3; if (r == 0) { static const int FC[4] = {2, 3, 5, 7}; p->fc2 = FC[rand_r(seed) % 4]; } else if (r == 1) p->fc4 = 1 + rand_r(seed) % 2; else p->fb = clampi(p->fb + (d > 0 ? 1 : -1), 0, 7); } break;
    case 6: break;
    }
}
static void print(const P *p, const M *m, double c) {
    int k;
    printf("cost %.1f  attack %.1f ms  click %.1f dB  decay %.0f / %.0f dB/s  peak %.2f\n", c, m->attack_ms, m->click, m->d1, m->d2, m->pk);
    printf("  p0:"); for (k = 1; k <= 8; k++) printf(" %5.1f", m->p0[k]); printf("\n  p3:"); for (k = 1; k <= 8; k++) printf(" %5.1f", m->p3[k]); printf("\n");
    for (k = 0; k < 6; k++) printf("  OP%d ol=%d r=(99,%d,%d,60) l=(99,%d,%d,0)%s\n", k + 1, p->ol[k], p->r2[k], p->r3[k], p->l2[k], p->l3[k],
                                   k == 1 ? "" : "");
    printf("  fc2=%d fc4=%d fc5=%d fc6=%d fb=%d\n", p->fc2, p->fc4, p->fc5, p->fc6, p->fb);
}
int main(int argc, char **argv) {
    unsigned seed = argc > 1 ? (unsigned)atoi(argv[1]) : 1;
    int iters = argc > 2 ? atoi(argv[2]) : 20000, it, restart;
    P best; double bestc = 1e18; M bm;
    uint8_t v[FP_SIZE + 1];
    for (restart = 0; restart < 6; restart++) {
        P p; M m; double c;
        /* a start: click stack, body stack with a strong modulator, a sine for the fundamental */
        int k;
        for (k = 0; k < 6; k++) { p.ol[k] = 80; p.r2[k] = 60; p.l2[k] = 80; p.r3[k] = 40; p.l3[k] = 0; }
        p.ol[1] = 85; p.r2[1] = 95; p.l2[1] = 0;                /* OP2: the click, gone in ms */
        p.ol[0] = 70; p.ol[2] = 99; p.ol[3] = 88; p.ol[4] = 90; p.ol[5] = 30;
        p.fc2 = 2; p.fc4 = 1; p.fc5 = 1; p.fc6 = 1; p.fb = 3;
        for (k = 0; k < 6; k++) { p.ol[k] = clampi(p.ol[k] + (rand_r(&seed) % 21) - 10, 0, 99); p.l2[k] = clampi(p.l2[k] + (rand_r(&seed) % 21) - 10, 0, 99); }
        build(&p, v); measure(v, &m); c = cost(&m);
        for (it = 0; it < iters; it++) {
            P q = p; M mq; double cq; int n = 1 + rand_r(&seed) % 3, j;
            for (j = 0; j < n; j++) mutate(&q, &seed);
            { int kk, bad = 0; for (kk = 0; kk < 6; kk++) if (q.l3[kk] > q.l2[kk]) bad = 1; if (bad) continue; }
            build(&q, v); measure(v, &mq); cq = cost(&mq);
            if (cq <= c) { p = q; c = cq; m = mq; }
        }
        if (c < bestc) { bestc = c; best = p; bm = m; }
        fprintf(stderr, "restart %d: cost %.1f\n", restart, c);
    }
    print(&best, &bm, bestc);
    return 0;
}
