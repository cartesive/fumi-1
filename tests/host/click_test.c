/* SPDX-License-Identifier: GPL-3.0-only */
/* The click regression test (docs/research/04): a held note while bend steps, a tuning switch, a 本数 change,
 * a level change, a voice steal and a key-up go by. A click is a sample-to-sample jump far above what the
 * waveform's own slope allows: |x[i] - x[i-1]| > 8 x the RMS of the differences over the previous 50 ms.
 * Built with -DFM_NO_SMOOTH the engine jumps instead of gliding and this test must fail (run_tests.sh
 * proves that once). */
#include <string.h>
#include <stdlib.h>
#include "tu.h"
#include "../../firmware/src/dsp/fumi.c"

void fm_midi_out(uint32_t st, uint32_t d1, uint32_t d2) { (void)st; (void)d1; (void)d2; }

static float buf[6 * 44100];
static int32_t blk[2 * 256];
static int pos;
static void run(int n)
{
    int i, k;
    for (i = 0; i < n; i += 256) {
        fm_render(blk, 256, 4096);
        for (k = 0; k < 256; k++)
            buf[pos++] = (float)blk[2 * k] / 8388608.0f;
    }
}

static int clicks(int from, int to, const char *what)
{
    int i, n = 0, w = 2205;
    double acc = 0;
    for (i = from + 1; i < from + w; i++)
        acc += ((double)buf[i] - buf[i - 1]) * ((double)buf[i] - buf[i - 1]);
    for (i = from + w; i < to; i++) {
        double d = (double)buf[i] - buf[i - 1], old = (double)buf[i - w + 1] - buf[i - w], rms = sqrt(acc / w);
        if (fabs(d) > 8.0 * rms && fabs(d) > 2e-3) {
            if (n < 3)
                printf("       click at %d (%s): jump %.4f, rms %.5f\n", i, what, d, rms);
            n++;
        }
        acc += d * d - old * old;
        if (acc < 0)
            acc = 0;
    }
    return n;
}

int main(void)
{
    int i, start, total = 0;
    uint8_t pk[128], v[FP_SIZE + 1];
    fm_init();
    fm_set(P_REVERB, 0);
    fm_set(P_YOIN, 100);
    fm_set(P_LEVEL, 70);
    /* a sustaining carrier, held at L3 */
    memset(v, 0, sizeof v);
    for (i = 0; i < 6; i++) {
        uint8_t *op = v + i * FP_OP;
        int k;
        for (k = 0; k < 4; k++) {
            op[FP_R1 + k] = 99;
            op[FP_L1 + k] = (uint8_t)(k < 3 ? 99 : 0);
        }
        op[FP_R1 + 3] = 60;
        op[FP_BP] = 39;
        op[FP_FC] = 1;
        op[FP_DET] = 7;
        op[FP_OL] = (uint8_t)(i == 5 ? 99 : 0);
    }
    v[FP_OKS] = 1;
    v[FP_LFS] = 35;
    v[FP_TRNSP] = 24;
    fm6_pack(v, pk);
    fm_patch_set(pk);
    run(2048);
    fm_key(5, 1);
    run(8192);

    start = pos;
    fm_bend(BEND_UP, 1, 1);
    run(8192);
    fm_bend(BEND_UP, 0, 1);
    run(8192);
    total += clicks(start, pos, "bend up and back");

    start = pos;
    fm_set(P_TUNING, TN_EQUAL);
    run(8192);
    fm_set(P_TUNING, TN_SUIKO);
    run(8192);
    total += clicks(start, pos, "tuning switch");

    start = pos;
    fm_set(P_HON, 9);
    run(8192);
    fm_set(P_HON, 4);
    run(8192);
    total += clicks(start, pos, "本数 change");

    start = pos;
    fm_set(P_LEVEL, 20);
    run(8192);
    fm_set(P_LEVEL, 70);
    run(8192);
    total += clicks(start, pos, "level change");

    start = pos;
    for (i = 0; i < 8; i++)                        /* fill the voices, then one more: a steal */
        if (i != 5)
            fm_key(i, 1);
    run(4096);
    fm_key(10, 1);
    run(8192);
    total += clicks(start, pos, "voice steal");
    for (i = 0; i < 11; i++)
        if (i != 5)
            fm_key(i, 0);
    run(8192);

    start = pos;
    fm_set(P_YOIN, 0);
    run(2048);
    fm_key(5, 0);
    run(8192);
    total += clicks(start, pos, "key-up with short 余韻");

    CHECK("no clicks through bend, tuning switch, 本数, level, steal and key-up", total == 0);
    return tu_done("click");
}
