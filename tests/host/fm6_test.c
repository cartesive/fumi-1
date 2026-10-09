/* SPDX-License-Identifier: GPL-3.0-only */
/* dsp/fm6_core.c (Felucca's msfa port) and dsp/fm6_patch.c in FuMi: the cents entry point, pitch accuracy,
 * patch formats, envelopes, levels. */
#include <string.h>
#include <stdlib.h>
#include "tu.h"
#include "tu_yin.h"
#include "../../firmware/src/dsp/fm6_core.c"
#include "../../firmware/src/dsp/fm6_patch.c"

/* an init voice: OP1 a carrier at level ol, ratio 1; the others silent. 155 bytes, OP6 first */
static void make_init(uint8_t *v, int ol)
{
    int k, i;
    memset(v, 0, FP_SIZE + 1);
    for (k = 0; k < 6; k++) {
        uint8_t *op = v + k * FP_OP;
        for (i = 0; i < 4; i++) {
            op[FP_R1 + i] = 99;
            op[FP_L1 + i] = (uint8_t)(i < 3 ? 99 : 0);
        }
        op[FP_BP] = 39;
        op[FP_FC] = 1;
        op[FP_DET] = 7;
        op[FP_OL] = (uint8_t)(k == 5 ? ol : 0);
    }
    for (i = 0; i < 4; i++) {
        v[FP_PR1 + i] = 99;
        v[FP_PL1 + i] = 50;
    }
    v[FP_ALG] = 0;
    v[FP_OKS] = 1;
    v[FP_LFS] = 35;
    v[FP_LPMS] = 3;
    v[FP_TRNSP] = 24;
    memcpy(v + FP_NAME, "INIT      ", 10);
}

static fm6_note_t note;
static fm6_lfo_t lfo;
static float buf[44100];
static const int32_t DT0[6] = {0, 0, 0, 0, 0, 0};

/* render n samples of patch v at cents from MIDI note 0, key down for `down` samples; returns the peak */
static float render(const uint8_t *v, float cents, int n, int down, int fresh)
{
    int i, k;
    float pk = 0;
    int32_t bus[FM6_N];
    memset(buf, 0, sizeof buf);                        /* the tail past the last whole block too */
    fm6_lfo_reset(&lfo, v);
    fm6_lfo_key(&lfo);
    fm6_note_init(&note, v, 60, 100, fresh);
    for (i = 0; i + FM6_N <= n; i += FM6_N) {
        int32_t lv = fm6_lfo_sample(&lfo), ld = fm6_lfo_delay(&lfo);
        if (i >= down && note.down)
            fm6_note_key(&note, v, 0);
        if (!fm6_note_compute(&note, v, bus, lv, ld, fm6_logfreq_cents(cents), v[FP_ALG], v[FP_FB], DT0, 0))
            memset(bus, 0, sizeof bus);
        for (k = 0; k < FM6_N; k++) {
            buf[i + k] = (float)bus[k] / (float)(1 << 24);
            if (fabsf(buf[i + k]) > pk)
                pk = fabsf(buf[i + k]);
        }
    }
    return pk;
}

int main(void)
{
    uint8_t v[FP_SIZE + 1], w[FP_SIZE + 1], pk[128], pk2[128];
    float peak, single;
    int i;
    double hz;
    make_init(v, 99);

    /* pitch: the cents entry point against YIN, from the lowest key FuMi can reach to the top */
    single = render(v, 6900.0f, 22050, 22050, 1);
    hz = tu_yin(buf + 8000, 8000, 44100.0);
    CHECK_NEAR("a carrier at 6900 cents from MIDI 0 = 440 Hz", hz, 440.0, 0.5);
    CHECK("a single full carrier has a sensible level", single > 0.1f && single < 2.0f);
    render(v, 4500.0f, 22050, 22050, 1);
    CHECK_NEAR("4500 cents = A2 = 110 Hz", tu_yin(buf + 8000, 12000, 44100.0), 110.0, 0.2);
    render(v, 4100.0f, 22050, 22050, 1);
    CHECK_NEAR("4100 cents = F2 = 87.31 Hz (水4, white key 1)", tu_yin(buf + 8000, 12000, 44100.0), 87.307, 0.2);
    render(v, 2900.0f, 44100, 44100, 1);
    CHECK_NEAR("2900 cents = F1 = 43.65 Hz (水4, octave down)", tu_yin(buf + 8000, 30000, 44100.0), 43.654, 0.2);
    render(v, 9300.0f, 22050, 22050, 1);
    CHECK_NEAR("9300 cents = A6 = 1760 Hz", tu_yin(buf + 8000, 8000, 44100.0), 1760.0, 2.0);
    render(v, 6900.0f - 20.8f, 22050, 22050, 1);
    CHECK_NEAR("-20.8 cents (SUIKO fa) lands within a cent", tu_cents(tu_yin(buf + 8000, 8000, 44100.0), 440.0), -20.8, 1.0);
    render(v, 6900.0f + 0.5f, 22050, 22050, 1);
    CHECK_NEAR("half a cent resolves", tu_cents(tu_yin(buf + 8000, 12000, 44100.0), 440.0), 0.5, 0.4);

    /* the envelope: key up with the release at 99 goes silent fast, and the note reports done */
    render(v, 6900.0f, 22050, 11025, 1);
    peak = 0;
    for (i = 11025 + 2205; i < 22050; i++)
        if (fabsf(buf[i]) > peak)
            peak = fabsf(buf[i]);
    CHECK("50 ms after key-up (R4 = 99): silent", peak < 0.001f);
    CHECK("the note reports done", fm6_note_done(&note, v, v[FP_ALG]));
    v[5 * FP_OP + FP_R1 + 3] = 20;                      /* a slow release */
    render(v, 6900.0f, 22050, 11025, 1);
    peak = 0;
    for (i = 11025 + 2205; i < 11025 + 4410; i++)
        if (fabsf(buf[i]) > peak)
            peak = fabsf(buf[i]);
    CHECK("a slow release still sounds 50 .. 100 ms after key-up", peak > 0.05f);
    v[5 * FP_OP + FP_R1 + 3] = 99;

    /* every operator a carrier (algorithm 32) at full level: no clipping past six carriers, no DC */
    make_init(v, 99);
    for (i = 0; i < 6; i++)
        v[i * FP_OP + FP_OL] = 99;
    v[FP_ALG] = 31;
    peak = render(v, 6900.0f, 22050, 22050, 1);
    CHECK("six carriers: at most six times one", peak <= 6.05f * single);
    {
        double mean = 0;
        for (i = 4410; i < 22050; i++)
            mean += buf[i];
        mean /= 22050 - 4410;
        CHECK("no DC", fabs(mean) < 0.01 * peak);
    }

    /* a modulated patch (algorithm 1, OP2 modulating OP1) is brighter than the sine: more zero crossings */
    make_init(v, 99);
    v[4 * FP_OP + FP_OL] = 80;                          /* OP2 */
    v[4 * FP_OP + FP_FC] = 3;
    render(v, 6900.0f, 22050, 22050, 1);
    CHECK_NEAR("modulation keeps the fundamental", tu_yin(buf + 8000, 8000, 44100.0), 440.0, 1.0);

    /* formats: 128-byte packed <-> 155-byte voice; any packed record unpacks into range */
    srand(1);
    for (i = 0; i < 200; i++) {
        int k, same = 1;
        for (k = 0; k < 128; k++)
            pk[k] = (uint8_t)(rand() & 0x7F);
        fm6_unpack(pk, v);
        for (k = 0; k < FP_SIZE; k++)
            if (v[k] > fm6_max((uint32_t)k)) {
                same = 0;
                break;
            }
        if (!same) {
            CHECK("unpack stays in range", 0);
            break;
        }
        fm6_pack(v, pk2);
        fm6_unpack(pk2, w);
        if (memcmp(v, w, FP_SIZE)) {
            CHECK("pack / unpack round trip", 0);
            break;
        }
    }
    CHECK("200 random records: unpack in range, pack(unpack) round trips", i == 200);
    /* amplitude modulation sensitivity only ever lowers an operator: with the LFO's AMD at 0 an AMS 3 carrier
     * plays at the same level as an AMS 0 one (msfa's constant 1.2% dip aside). A wrong scale in the AMS
     * curve once drove flutes with AMS to eight times a full carrier */
    {
        float plain, ams;
        make_init(v, 99);
        plain = render(v, 6900.0f, 22050, 22050, 1);
        v[5 * FP_OP + FP_AMS] = 3;
        ams = render(v, 6900.0f, 22050, 22050, 1);
        printf("  AMS 3 at AMD 0: %.3f of the AMS 0 level (msfa's constant dip is about 0.89)\n", (double)(ams / plain));
        CHECK("AMS 3 with no LFO amplitude depth: within 1.5 dB of AMS 0", ams < plain * 1.02f && ams > plain * 0.84f);
        v[FP_LAMD] = 99;
        ams = render(v, 6900.0f, 22050, 22050, 1);
        CHECK("AMS 3 with full LFO amplitude depth: never louder than AMS 0", ams <= plain * 1.02f);
    }
    return tu_done("fm6");
}
