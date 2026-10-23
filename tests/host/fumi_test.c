/* SPDX-License-Identifier: GPL-3.0-only */
/* dsp/fumi.c: the instrument on the host. Pitch of the keys through the whole engine, the tunings, 余韻,
 * polyphony and stealing, 単音, トリラー, ビブラート, the bend buttons, patches, filters, reverb, levels. */
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "tu.h"
#include "tu_yin.h"
#include "../../firmware/src/dsp/fumi.c"

static uint32_t midi_vel[64], midi_note[64], midi_nvel, midi_noff;   /* the note-ons sent: velocity, note and position
                                                                      (the strokes, 1.0.6); the note-offs counted */
static int midi_t[64], render_pos;
void fm_midi_out(uint32_t st, uint32_t d1, uint32_t d2)
{
    if (st == 0x90u && midi_nvel < 64u) {
        midi_t[midi_nvel] = render_pos;
        midi_note[midi_nvel] = d1;
        midi_vel[midi_nvel++] = d2;
    }
    if (st == 0x80u)
        midi_noff++;
}

static float buf[3 * 44100];                      /* mono (left) */
static int32_t blk[2 * 256];

/* render n samples (a multiple of 256) into buf from pos; returns the peak */
static float run(int pos, int n)
{
    int i, k;
    float pk = 0;
    for (i = 0; i < n; i += 256) {
        render_pos = pos + i;
        fm_render(blk, 256, 4096);
        for (k = 0; k < 256; k++) {
            float l = (float)blk[2 * k] / 8388608.0f;
            buf[pos + i + k] = l;
            if (fabsf(l) > pk)
                pk = fabsf(l);
        }
    }
    return pk;
}
static float peak_of(int from, int to)
{
    float pk = 0;
    int i;
    for (i = from; i < to; i++)
        if (fabsf(buf[i]) > pk)
            pk = fabsf(buf[i]);
    return pk;
}
static double db(float x) { return 20.0 * log10(x > 1e-9f ? (double)x : 1e-9); }
static float rms_of(int from, int to)
{
    double a = 0;
    int i;
    for (i = from; i < to; i++)
        a += (double)buf[i] * buf[i];
    return to > from ? (float)sqrt(a / (to - from)) : 0.0f;
}
static int slot_of(const char *name)              /* a built-in instrument's slot by name; -1 if none */
{
    int i;
    for (i = 0; i < fm_patch_count(); i++)
        if (!strcmp(fm_patch_name(i), name))
            return i;
    return -1;
}
static int koto_a(void)                           /* the slot of FuMi's own KOTO A, "Koto Pluck" (the owner's picks come first) */
{
    int i = slot_of("Koto Pluck");
    return i < 0 ? 0 : i;
}
static float ref[3 * 44100];                      /* a pass of the loop, kept */
static float pass_diff(uint32_t n)                /* the largest difference between buf and ref over n samples */
{
    float d = 0;
    uint32_t k;
    for (k = 0; k < n; k++)
        if (fabsf(buf[k] - ref[k]) > d)
            d = fabsf(buf[k] - ref[k]);
    return d;
}
static int ms_to_db_below(int from, float pk, double d)   /* the first ms after `from` at which the 5 ms peak stays d dB under pk */
{
    int i;
    for (i = from; i + 220 < 3 * 44100; i += 44)
        if (db(peak_of(i, i + 220)) < db(pk) - d && db(peak_of(i, 3 * 44100 > i + 8820 ? i + 8820 : 3 * 44100)) < db(pk) - d)
            return (i - from) / 44;
    return -1;
}
static int onsets(int from, int to)               /* rises of 4 dB within 10 ms of the 5 ms RMS envelope: a koto
                                                   * decaying at the ST-50's -28 dB/s drops only 5-6 dB between
                                                   * re-plucks at the middle trill rate */
{
    int i, n = 0, w = 220, last = -100000;
    for (i = from + 2 * w; i < to; i += 44) {
        double a = 0, b = 0;
        int k;
        for (k = 0; k < w; k++) {
            a += (double)buf[i - 2 * w + k] * buf[i - 2 * w + k];
            b += (double)buf[i - w + k] * buf[i - w + k];
        }
        if (b > 2.5 * a && b > 1e-6 && i - last > 2000) {
            n++;
            last = i;
        }
    }
    return n;
}

static int onset_times(int from, int to, int *t, int max)   /* as onsets(), the positions kept */
{
    int i, n = 0, w = 220, last = -100000;
    for (i = from + 2 * w; i < to; i += 44) {
        double a = 0, b = 0;
        int k;
        for (k = 0; k < w; k++) {
            a += (double)buf[i - 2 * w + k] * buf[i - 2 * w + k];
            b += (double)buf[i - w + k] * buf[i - w + k];
        }
        if (b > 2.5 * a && b > 1e-7 && i - last > 2000) {
            if (n < max)
                t[n] = i - w;
            n++;
            last = i;
        }
    }
    return n < max ? n : max;
}
static double centroid(int a, int b)              /* the spectral centroid of buf[a..b), Hz: Goertzel at 64 log-spaced frequencies */
{
    int k, n;
    double num = 0, den = 0;
    for (k = 0; k < 64; k++) {
        double f = 100.0 * pow(2.0, k / 8.0), w = 2.0 * M_PI * f / 44100.0, re = 0, im = 0, m;
        if (f > 20000.0)
            break;
        for (n = a; n < b; n++) {
            re += buf[n] * cos(w * (n - a));
            im += buf[n] * sin(w * (n - a));
        }
        m = re * re + im * im;
        num += f * m;
        den += m;
    }
    return den > 0 ? num / den : 0;
}

/* a sustaining test voice: OP1 carrier, level 99, held at L3 = 99, release R4 = 99 (fast) */
static void sustain_patch(uint8_t *pk, int r4)
{
    uint8_t v[FP_SIZE + 1];
    int k, i;
    memset(v, 0, sizeof v);
    for (k = 0; k < 6; k++) {
        uint8_t *op = v + k * FP_OP;
        for (i = 0; i < 4; i++) {
            op[FP_R1 + i] = 99;
            op[FP_L1 + i] = (uint8_t)(i < 3 ? 99 : 0);
        }
        op[FP_R1 + 3] = (uint8_t)r4;
        op[FP_BP] = 39;
        op[FP_FC] = 1;
        op[FP_DET] = 7;
        op[FP_OL] = (uint8_t)(k == 5 ? 99 : 0);
    }
    for (i = 0; i < 4; i++) {
        v[FP_PR1 + i] = 99;
        v[FP_PL1 + i] = 50;
    }
    v[FP_OKS] = 1;
    v[FP_LFS] = 35;
    v[FP_LPMS] = 3;
    v[FP_TRNSP] = 24;
    memcpy(v + FP_NAME, "SUSTAIN   ", 10);
    fm6_pack(v, pk);
}

static void reset(void)
{
    uint8_t pk[128];
    fm_init();
    sustain_patch(pk, 99);
    fm_patch_set(pk);
    fm_set(P_REVERB, 0);
    fm_set(P_HIGHCUT, 100);
    fm_set(P_LOWCUT, 0);
    fm_set(P_YOIN, 0);
    fm_set(P_LEVEL, 60);
    run(0, 256);
}

int main(void)
{
    int i;
    double hz;
    reset();
    CHECK("silence at rest", run(0, 2048) == 0.0f);

    /* pitch through the whole engine: 1本, 三 = A3 = 220 Hz; SUIKO moves fa by -20.8 and leaves la alone */
    fm_key(5, 1);
    run(0, 22016);
    hz = tu_yin(buf + 11000, 8000, 44100.0);
    CHECK_NEAR("white key 6 (三) at 1本 = 220 Hz", hz, 220.0, 0.3);
    fm_key(5, 0);
    run(0, 4096);
    fm_key(1, 1);
    run(0, 22016);
    CHECK_NEAR("white key 2 (fa) in SUIKO: -2300 - 20.8 cents from A4", tu_cents(tu_yin(buf + 11000, 12000, 44100.0), 440.0), -2320.8, 1.0);
    fm_set(P_TUNING, TN_EQUAL);
    run(0, 22016);
    CHECK_NEAR("switching to 平均律 while held: the note arrives at -2300", tu_cents(tu_yin(buf + 11000, 12000, 44100.0), 440.0), -2300.0, 1.0);
    fm_set(P_TUNING, TN_SUIKO);
    fm_key(1, 0);
    run(0, 4096);
    fm_key(2, 1);                                  /* la: no offset in SUIKO */
    run(0, 22016);
    CHECK_NEAR("white key 3 (la) in SUIKO: -1900 cents", tu_cents(tu_yin(buf + 11000, 12000, 44100.0), 440.0), -1900.0, 1.0);
    fm_key(2, 0);
    fm_set(P_HON, 0);                              /* 水4 */
    run(0, 4096);
    fm_key(0, 1);
    run(0, 44032);
    CHECK_NEAR("水4 white key 1 = F2 = 87.31 Hz", tu_yin(buf + 22000, 20000, 44100.0), 87.307, 0.3);
    fm_key(0, 0);
    fm_set(P_HON, 4);
    run(0, 4096);
    fm_key(16, 1);                                 /* black key 1 = fa# over the low octave */
    run(0, 22016);
    CHECK_NEAR("black key 1 (upper row) = fa# = -2200 cents", tu_cents(tu_yin(buf + 11000, 12000, 44100.0), 440.0), -2200.0, 1.0);
    fm_key(16, 0);
    fm_note(FM_NOTE_ID, 1, 702.0f);
    run(0, 22016);
    CHECK_NEAR("fm_note at +702 cents from A4", tu_cents(tu_yin(buf + 11000, 6000, 44100.0), 440.0), 702.0, 1.0);
    fm_note(FM_NOTE_ID, 0, 0);
    run(0, 4096);

    /* 余韻: short = damped within 100 ms of key-up; long = still ringing a second later */
    fm_set(P_YOIN, 0);
    fm_key(5, 1);
    run(0, 11008);
    fm_key(5, 0);
    run(11008, 44032);
    CHECK("余韻 short: -60 dB within 100 ms of key-up", db(peak_of(11008 + 4410, 11008 + 8820)) < -60.0);
    fm_set(P_YOIN, 100);
    fm_key(5, 1);
    run(0, 11008);
    fm_key(5, 0);
    run(11008, 44032);
    CHECK("余韻 long: one second after key-up still within 24 dB of the level at key-up",
          db(peak_of(11008 + 44032 - 4410, 11008 + 44032)) > db(peak_of(11008 - 4410, 11008)) - 24.0);
    CHECK("and it decays", peak_of(11008 + 44032 - 4410, 11008 + 44032) < peak_of(11008, 11008 + 4410));
    fm_set(P_YOIN, 0);

    /* polyphony: eight notes at once stay inside full scale; a ninth steals without a crash */
    for (i = 0; i < 9; i++)
        fm_key(i, 1);
    {
        float pk = run(0, 22016);
        CHECK("nine keys: output inside full scale, and there", pk < 1.0f && pk > 0.003f);
        CHECK("eight voices sound", fm_nvoices == FM_NVOICE);
    }
    for (i = 0; i < 9; i++)
        fm_key(i, 0);
    run(0, 8192);
    CHECK("all released: no voices", fm_nvoices == 0);

    /* 単音: last-note priority, and back to the held note on release */
    fm_set(P_MONO, 1);
    fm_key(5, 1);
    run(0, 8192);
    fm_key(10, 1);                                 /* 八: an octave up */
    run(0, 22016);
    CHECK("mono: one voice", fm_nvoices == 1);
    CHECK_NEAR("mono: the last key sounds (A4)", tu_yin(buf + 11000, 8000, 44100.0), 440.0, 0.5);
    fm_key(10, 0);
    run(0, 22016);
    CHECK_NEAR("mono: back to the held key (A3)", tu_yin(buf + 11000, 8000, 44100.0), 220.0, 0.5);
    fm_key(5, 0);
    fm_set(P_MONO, 0);
    run(0, 8192);

    /* トリラー: the held note re-plucked at the rate (on a decaying voice: the built-in koto) */
    fm_set(P_VOICE, koto_a());
    fm_set(P_TRILL_ON, 1);
    fm_set(P_TRILL_RATE, 50);
    fm_set(P_TRILL_VAR, 0);
    fm_key(5, 1);
    run(0, 44032);
    CHECK("trill: at least four re-plucks a second at the middle rate", onsets(2000, 44032) >= 4);
    fm_key(5, 0);
    fm_set(P_TRILL_ON, 0);
    run(0, 8192);
    /* the trill that sounds played (1.0.6): strokes in pairs, ka-ta, the ta lighter; the first stroke the hardest;
     * a key press restarts the pair; Trill feel 0 is the even machine of before; oroshi starts slow and speeds
     * up on the ornament key; the loop trills its own notes. Koto Pluck now responds to velocity. */
    {
        int t[64], n, k, gap[64];
        double r, avg, lv[64];
        fm_init();
        fm_set(P_VOICE, koto_a());
        fm_set(P_REVERB, 0);
        fm_set(P_YOIN, 0);
        fm_set(P_LEVEL, 60);
        fm_set(P_TRILL_RATE, 50);                 /* a period of 155 ms */
        fm_set(P_TRILL_VAR, 100);                 /* Trill feel: full */
        fm_set(P_TRILL_PAIR, 42);
        fm_set(P_TRILL_JIT, 0);
        fm_set(P_TRILL_OROSHI, 0);
        run(0, 256);
        midi_nvel = 0;
        fm_key(5, 1);                             /* trill off: an ordinary press is normal */
        run(0, 4096);
        fm_key(5, 0);
        run(0, 8192);
        CHECK("trill off: a key press is a normal stroke (velocity 100)", midi_nvel == 1u && midi_vel[0] == 100u);
        midi_nvel = 0;
        fm_ornament(ORN_STRONG, 1);
        fm_key(5, 1);
        run(0, 4096);
        fm_key(5, 0);
        fm_ornament(ORN_STRONG, 0);
        run(0, 8192);
        CHECK("the strong-pluck ornament: a strong stroke (velocity 127)", midi_nvel == 1u && midi_vel[0] == 127u);
        /* ARP trill, a key held two seconds: KA ta ka ta ... */
        fm_set(P_TRILL_ON, 1);
        midi_nvel = 0;
        fm_key(5, 1);
        run(0, 88064);
        fm_key(5, 0);
        n = (int)midi_nvel;                       /* the strokes, from the note-ons (to the 256-frame render) */
        for (k = 0; k < n; k++)
            t[k] = midi_t[k];
        for (k = 0; k + 1 < n; k++)
            gap[k] = t[k + 1] - t[k];
        printf("  trill pairs: %d strokes;", n);
        for (k = 0; k < 8 && k + 1 < n; k++)
            printf(" %d", gap[k] / 44);
        printf(" ms; velocities");
        for (k = 0; k < 6 && (uint32_t)k < midi_nvel; k++)
            printf(" %u", midi_vel[k]);
        printf("\n");
        CHECK("trill: twelve strokes in two seconds at the middle rate", n >= 12 && n <= 16);
        CHECK("the first stroke is the strong KA, then ta (light), ka (normal), ta ...",
              midi_nvel >= 5u && midi_vel[0] == 127u && midi_vel[1] < 72u && midi_vel[2] >= 90u && midi_vel[2] <= 104u && midi_vel[3] < 72u);
        r = 0;
        avg = 0;
        for (k = 2; k + 1 < 10 && k + 1 < n - 1; k += 2) {   /* pairs after the first: ka->ta short, ta->ka long */
            r += (double)gap[k] / (gap[k] + gap[k + 1]);
            avg += (gap[k] + gap[k + 1]) * 0.5;
        }
        r /= 4;
        avg /= 4;
        printf("  short gap share %.2f (want 0.42), stroke period %.0f ms (want 155)\n", r, avg / 44.1);
        CHECK("the short gap is 42 %% of the pair", r > 0.37 && r < 0.47);
        CHECK("the average stroke period is the trill rate", avg / 44.1 > 140.0 && avg / 44.1 < 170.0);
        CHECK("the gap after the first KA is a little longer than the later short gaps (the hand recovers)",
              gap[0] > gap[2] && gap[0] < gap[1]);
        for (k = 0; k < n && k < 64; k++)
            lv[k] = db(rms_of(t[k] + 256, t[k] + 256 + 2205));   /* the 50 ms after each stroke */
        printf("  stroke levels: KA %.1f, ta %.1f, ka %.1f, ta %.1f, ka %.1f dB\n", lv[0], lv[1], lv[2], lv[3], lv[4]);
        CHECK("the ta is at least 3 dB under the ka, the KA above the ka", lv[2] - lv[1] > 3.0 && lv[4] - lv[3] > 3.0 && lv[0] > lv[2] + 0.7);
        run(0, 44032);
        /* a key press restarts the pair: the ta follows the press after one short gap, whatever the clock said */
        midi_nvel = 0;
        fm_key(5, 1);
        run(0, 3072);                             /* 70 ms in */
        fm_key(8, 1);
        run(0, 22016);
        fm_key(5, 0);
        fm_key(8, 0);
        n = (int)midi_nvel;
        printf("  after a second key 70 ms in: next stroke %d ms after it\n", n >= 3 ? (midi_t[2] - midi_t[1]) / 44 : -1);
        CHECK("a key press restarts the pair: its ta comes one short gap later (not on the old clock)",
              n >= 3 && midi_vel[1] == 127u && (midi_t[2] - midi_t[1]) / 44 > 118 && (midi_t[2] - midi_t[1]) / 44 < 180);
        run(0, 44032);
        /* Trill feel 0: even gaps, identical strokes, no first-stroke accent (the ST-50's machine) */
        fm_set(P_TRILL_VAR, 0);
        midi_nvel = 0;
        fm_key(5, 1);
        run(0, 88064);
        fm_key(5, 0);
        n = (int)midi_nvel;
        for (k = 0; k + 1 < n; k++)
            gap[k] = midi_t[k + 1] - midi_t[k];
        r = 0;
        for (k = 1; k + 1 < 9 && k + 1 < n - 1; k += 2)
            r += (double)gap[k] / (gap[k] + gap[k + 1]);
        r /= 4;
        printf("  feel 0: short gap share %.2f, velocities %u %u %u\n", r, midi_vel[0], midi_vel[1], midi_vel[2]);
        CHECK("feel 0: even spacing", r > 0.46 && r < 0.54);
        CHECK("feel 0: every stroke normal", midi_nvel >= 3u && midi_vel[0] == 100u && midi_vel[1] == 100u && midi_vel[2] == 100u);
        /* the strong-pluck key held with the trill on: one stroke on the press, the pair restarting on it (review) */
        fm_set(P_TRILL_VAR, 100);
        fm_ornament(ORN_STRONG, 1);
        midi_nvel = 0;
        fm_key(5, 1);
        run(0, 22016);
        fm_key(5, 0);
        fm_ornament(ORN_STRONG, 0);
        printf("  strong key + trill: %u strokes in 500 ms, the second %d ms after the press\n", midi_nvel, midi_nvel >= 2u ? (midi_t[1] - midi_t[0]) / 44 : -1);
        CHECK("the strong-pluck key with the trill on: one stroke on the press, the ta one short gap later",
              midi_nvel >= 2u && midi_vel[0] == 127u && (midi_t[1] - midi_t[0]) / 44 > 118 && (midi_t[1] - midi_t[0]) / 44 < 180);
        run(0, 44032);
        /* mono back to the held key on a key-up is not a press: no strong accent (review) */
        fm_set(P_MONO, 1);
        fm_key(5, 1);
        run(0, 4096);
        fm_key(8, 1);
        run(0, 4096);
        midi_nvel = 0;
        fm_key(8, 0);
        run(0, 2048);
        CHECK("back to the held key on a key-up: a normal stroke, not a hard KA", midi_nvel >= 1u && midi_vel[0] < 115u);
        fm_key(5, 0);
        fm_set(P_MONO, 0);
        fm_set(P_TRILL_ON, 0);
        run(0, 44032);
        /* poly to mono with two keys held, the top one let go: nothing drones (a 1.0.5 defect found in review) */
        fm_key(5, 1);
        fm_key(8, 1);
        run(0, 4096);
        fm_set(P_MONO, 1);
        run(0, 4096);
        fm_key(8, 0);
        run(0, 4096);
        fm_key(5, 0);
        run(0, 44032);
        CHECK("poly to mono with two keys down, both released: silence", fm_nvoices == 0);
        fm_set(P_MONO, 0);
        /* oroshi on the ornament key: slow, then faster by a constant ratio, at the rate within the set time */
        fm_set(P_TRILL_OROSHI, 1);                /* Short: about 1.5 s from slow to the trill rate */
        fm_key(5, 1);
        run(0, 256);
        midi_nvel = 0;
        fm_ornament(ORN_TRILL, 1);
        run(0, 132096);                           /* 3 s */
        fm_ornament(ORN_TRILL, 0);
        fm_key(5, 0);
        n = (int)midi_nvel;                       /* [0] the KA at the ornament key */
        for (k = 0; k < n; k++)
            t[k] = midi_t[k];
        for (k = 0; k + 1 < n; k++)
            gap[k] = t[k + 1] - t[k];
        printf("  oroshi short:");
        for (k = 0; k < 12 && k + 1 < n; k++)
            printf(" %d", gap[k] / 44);
        printf(" ms; velocities");
        for (k = 0; k < 8 && k < n; k++)
            printf(" %u", midi_vel[k]);
        printf("\n");
        CHECK("oroshi: the first stroke is the KA, the first gap about four times the period (620 ms, the hand recovering)",
              n > 8 && midi_vel[0] == 127u && gap[0] / 44 > 560 && gap[0] / 44 < 780);
        CHECK("oroshi: the gaps shrink, one after the other", n > 8 && gap[1] < gap[0] && gap[2] < gap[1] && gap[3] < gap[2]);
        {
            uint32_t late = 127;
            for (k = n - 4; k < n; k++)
                if (midi_vel[k] < late)
                    late = midi_vel[k];
            CHECK("oroshi: the strokes after the KA start at normal strength and the ta lightens as it speeds up",
                  n > 8 && midi_vel[1] >= 88u && late < 72u);
        }
        {
            int reached = -1;
            for (k = 0; k + 1 < n; k++)
                if ((gap[k] + gap[k + 1]) / 88 < 155 * 1.25) {   /* a pair at the rate */
                    reached = (t[k] - t[0]) / 44;
                    break;
                }
            printf("  oroshi short: at the trill rate %d ms after the ornament key\n", reached);
            CHECK("oroshi short: at the trill rate within about 1.5 s", reached > 900 && reached < 2200);
        }
        run(0, 44032);
        fm_set(P_TRILL_ON, 1);                    /* the ARP trill stays plain whatever the setting */
        midi_nvel = 0;
        fm_key(5, 1);
        run(0, 44032);
        fm_key(5, 0);
        fm_set(P_TRILL_ON, 0);
        n = (int)midi_nvel;
        CHECK("the ARP trill is plain whatever the oroshi setting", n > 4 && (midi_t[1] - midi_t[0]) / 44 < 200);
        fm_set(P_TRILL_OROSHI, 0);
        run(0, 44032);
    }
    {   /* a trill on a sound that ends by itself (1.0.7): the hyoshigi's crack is over in under 200 ms, so between
         * strokes its voice is gone; the trill must strike the held key again all the same, at the rate and through an
         * oroshi's slow start (1.0.6 stopped after the KA: one clap, no roll) */
        int n_plain, n_oro;
        fm_init();
        fm_set(P_VOICE, slot_of("Hyoshigi"));
        fm_set(P_REVERB, 0);
        fm_set(P_LEVEL, 60);
        fm_set(P_TRILL_RATE, 50);
        fm_set(P_TRILL_OROSHI, ORO_NONE);         /* the plain trill (the default is Long since 1.1.0) */
        run(0, 256);
        fm_key(5, 1);
        run(0, 22016);                            /* the crack long over when the ornament key comes: it strikes again */
        midi_nvel = 0;
        fm_ornament(ORN_TRILL, 1);
        run(0, 132096);                           /* 3 s */
        fm_ornament(ORN_TRILL, 0);
        fm_key(5, 0);
        n_plain = (int)midi_nvel;
        run(0, 22016);
        fm_set(P_TRILL_OROSHI, ORO_LONG);
        fm_key(5, 1);
        run(0, 256);
        midi_nvel = 0;
        fm_ornament(ORN_TRILL, 1);
        run(0, 132096);
        fm_ornament(ORN_TRILL, 0);
        fm_key(5, 0);
        n_oro = (int)midi_nvel;
        fm_set(P_TRILL_OROSHI, 0);
        run(0, 22016);
        printf("  Hyoshigi trill: %d strokes in 3 s plain, %d with oroshi Long\n", n_plain, n_oro);
        CHECK("Hyoshigi: the trill goes on striking though each crack has ended (15 or more strokes in 3 s)", n_plain >= 15);
        CHECK("Hyoshigi: the oroshi rolls from slow to the rate (5 or more strokes in 3 s on Long)", n_oro >= 5);
        /* review (1.0.7): the key the trill strikes is the top of the held stack, whatever was plucked last */
        fm_key(5, 1);
        run(0, 256);
        fm_key(7, 1);
        run(0, 256);
        fm_ornament(ORN_TRILL, 1);
        run(0, 11008);
        fm_key(7, 0);
        run(0, 256);
        midi_nvel = 0;
        run(0, 44032);
        n_plain = (int)midi_nvel;
        fm_ornament(ORN_TRILL, 0);
        fm_key(5, 0);
        run(0, 22016);
        printf("  Hyoshigi trill back on the key still held: %d strokes in 1 s\n", n_plain);
        CHECK("Hyoshigi: two keys down, the top released: the trill goes on on the one still held", n_plain >= 5);
        /* and a retune while it rolls: the strokes follow (the octave up) */
        fm_key(5, 1);
        run(0, 256);
        midi_nvel = 0;
        fm_ornament(ORN_TRILL, 1);
        run(0, 22016);
        fm_set(P_OCTAVE, 1);
        run(0, 22016);
        n_oro = (int)midi_nvel;
        fm_ornament(ORN_TRILL, 0);
        fm_key(5, 0);
        fm_set(P_OCTAVE, 0);
        run(0, 22016);
        printf("  Hyoshigi trill across an octave switch: first note %u, last %u\n", midi_note[0], midi_note[n_oro - 1]);
        CHECK("Hyoshigi: the strokes follow a retune while the trill rolls", n_oro >= 6 && midi_note[n_oro - 1] == midi_note[0] + 12u);
        /* a sound that ends by itself with the key still down sends its MIDI note-off then, not never */
        midi_noff = 0;
        fm_key(5, 1);
        run(0, 22016);
        CHECK("Hyoshigi: the note-off goes out when the crack ends, the key still down", midi_noff == 1u);
        fm_key(5, 0);
        run(0, 22016);
        CHECK("and not again at the key-up", midi_noff == 1u);
    }
    {   /* the koto's left hand on the three empty ornament keys (1.0.7): 押し手 oshide presses the string behind the
         * bridge, the sounding note up a whole tone while the key is held; 引き色 hikiiro lets each new note dip after the
         * pluck and come back; 後押し atooshi pushes each new note up a semitone a moment after it sounds */
        uint8_t pk[128];
        double p0, p1, p2;
        fm_init();
        sustain_patch(pk, 99);
        fm_patch_set(pk);
        fm_set(P_REVERB, 0);
        fm_set(P_YOIN, 0);
        fm_set(P_LEVEL, 60);
        run(0, 256);
        fm_key(5, 1);
        run(0, 22016);
        p0 = tu_cents(tu_yin(buf + 11000, 8000, 44100.0), 220.0);
        fm_ornament(ORN_OSHIDE, 1);
        run(0, 22016);
        p1 = tu_cents(tu_yin(buf + 11000, 8000, 44100.0), 220.0);
        fm_ornament(ORN_OSHIDE, 0);
        run(0, 22016);
        p2 = tu_cents(tu_yin(buf + 11000, 8000, 44100.0), 220.0);
        printf("  oshide: %.1f cents, held %.1f, released %.1f\n", p0, p1, p2);
        CHECK_NEAR("oshide held: the sounding note a whole tone up", p1 - p0, 200.0, 6.0);
        CHECK_NEAR("oshide released: back on the string's pitch", p2 - p0, 0.0, 6.0);
        {   /* a note plucked under oshide is already pressed: it sounds a whole tone up from its first ms (review) */
            fm_key(5, 0);
            run(0, 22016);
            fm_ornament(ORN_OSHIDE, 1);
            run(0, 22016);
            fm_key(5, 1);
            run(0, 2205);
            p1 = tu_cents(tu_yin(buf + 220, 1764, 44100.0), 220.0);   /* 5-45 ms */
            printf("  plucked under oshide, 5-45 ms: %.1f cents up\n", p1 - p0);
            CHECK_NEAR("plucked under oshide: a whole tone up from the start, no swoop", p1 - p0, 200.0, 12.0);
            fm_ornament(ORN_OSHIDE, 0);
            run(0, 22016);
        }
        {   /* oshide comes in over about 80 ms, not at once */
            fm_ornament(ORN_OSHIDE, 1);
            run(0, 4410);
            p1 = tu_cents(tu_yin(buf + 441, 1764, 44100.0), 220.0);   /* 10-50 ms in: on the way */
            printf("  oshide 10-50 ms in: %.1f cents up\n", p1 - p0);
            CHECK("oshide rises over tens of ms (10-50 ms in: between 40 and 180 cents)", p1 - p0 > 40.0 && p1 - p0 < 180.0);
            fm_ornament(ORN_OSHIDE, 0);
            run(0, 22016);
        }
        fm_key(5, 0);
        run(0, 22016);
        fm_ornament(ORN_HIKIIRO, 1);
        fm_key(5, 1);
        run(0, 22016);
        p1 = tu_cents(tu_yin(buf + 441, 2205, 44100.0), 220.0);        /* 10-60 ms: the dip */
        p2 = tu_cents(tu_yin(buf + 13230, 8000, 44100.0), 220.0);      /* 300-480 ms: recovered */
        printf("  hikiiro: 10-60 ms %.1f cents, 300 ms on %.1f (string %.1f)\n", p1, p2, p0);
        CHECK("hikiiro: a new note dips 20 to 45 cents just after the pluck", p0 - p1 > 20.0 && p0 - p1 < 45.0);
        CHECK_NEAR("hikiiro: and is back on the string's pitch within 300 ms", p2 - p0, 0.0, 4.0);
        fm_key(5, 0);
        fm_ornament(ORN_HIKIIRO, 0);
        run(0, 22016);
        fm_ornament(ORN_ATOOSHI, 1);
        fm_key(5, 1);
        run(0, 44032);
        p1 = tu_cents(tu_yin(buf + 2205, 4410, 44100.0), 220.0);       /* 50-150 ms: as plucked */
        p2 = tu_cents(tu_yin(buf + 17640, 8000, 44100.0), 220.0);      /* 400-580 ms: pushed */
        printf("  atooshi: 50-150 ms %.1f cents, 400 ms on %.1f (string %.1f)\n", p1, p2, p0);
        CHECK_NEAR("atooshi: a new note sounds on the string's pitch first", p1 - p0, 0.0, 4.0);
        CHECK_NEAR("atooshi: and is pushed up a semitone a moment later", p2 - p0, 100.0, 6.0);
        fm_key(5, 0);
        fm_ornament(ORN_ATOOSHI, 0);
        run(0, 22016);
        CHECK("seven ornaments", ORN_N == 7);
    }
    {   /* the Trill page's shapes read as words (1.0.7): pair gap from even to tight, ta level from the same to faint */
        char t[16];
        int ok = 1;
        static const struct { int p, v; const char *w; } W[] = {
            {P_TRILL_PAIR, 50, "Even"}, {P_TRILL_PAIR, 47, "Loose"}, {P_TRILL_PAIR, 42, "Paired"}, {P_TRILL_PAIR, 30, "Tight"},
            {P_TRILL_TA, 100, "Same"}, {P_TRILL_TA, 90, "Softer"}, {P_TRILL_TA, 64, "Light"}, {P_TRILL_TA, 30, "Faint"},
            {P_TRILL_FIRST, 127, "127"}, {P_TRILL_JIT, 5, "5"}};
        for (i = 0; i < (int)(sizeof W / sizeof W[0]); i++) {
            fm_param_text(W[i].p, W[i].v, t);
            if (strcmp(t, W[i].w)) {
                printf("  param %d at %d reads \"%s\", want \"%s\"\n", W[i].p, W[i].v, t, W[i].w);
                ok = 0;
            }
        }
        CHECK("pair gap and ta level read as words; first stroke and jitter as numbers", ok);
    }
    {   /* Kokyu 胡弓 (1.0.6): the bowed voice, placed before Taiko as on the ST-50; the note swells in */
        double a, b, c;
        {   /* 1.0.8: twenty-one instruments in the ST-50 strip's families: kotos, bells, winds, the sustained
             * voices, percussion; Koto stays first (the boot voice) */
            static const char *const ORDER[21] = {"Koto", "Koto II", "Koto Pluck", "Koto Ring", "Koto Warm", "Harp",
                "Chimes", "Rin", "Bell", "Ice Bells", "Shakuhachi", "Bamboo Flt", "Dragon Flt", "Sho",
                "Kokyu", "Clarinet", "Strings", "Fairytale", "Taiko", "Hyoshigi", "Mokugyo"};
            int k, same = fm_patch_count() == 21;
            for (k = 0; same && k < 21; k++)
                same = !strcmp(fm_patch_name(k), ORDER[k]);
            CHECK("twenty-one instruments in the strip's families, Koto first", same);
            CHECK("the new voices carry their Japanese names", !strcmp(fm_patch_ja(slot_of("Kokyu")), "\xe8\x83\xa1\xe5\xbc\x93")
                  && !strcmp(fm_patch_ja(slot_of("Clarinet")), "\xe3\x82\xaf\xe3\x83\xa9\xe3\x83\xaa\xe3\x83\x8d\xe3\x83\x83\xe3\x83\x88")
                  && !strcmp(fm_patch_ja(slot_of("Rin")), "\xe9\x88\xb4"));
        }
        fm_init();
        fm_set(P_VOICE, slot_of("Kokyu"));
        fm_set(P_REVERB, 0);
        fm_set(P_YOIN, 50);
        fm_set(P_LEVEL, 60);
        run(0, 256);
        fm_key(5, 1);
        run(0, 88064);
        a = db(rms_of(0, 2646));
        b = db(rms_of(2646, 13230));
        c = db(rms_of(44100, 88064));
        printf("  Kokyu: first 60 ms %.1f dB, 60-300 ms %.1f dB, 1-2 s %.1f dB; pitch %.1f cents from A3\n", a, b, c,
               tu_cents(tu_yin(buf + 44100, 8820, 44100.0), 220.0));
        CHECK("Kokyu: bowed, the note swells in (the first 60 ms 20 dB under the sustain, 60-300 ms within 6 dB of it)",
              c - a > 20.0 && c - b < 6.0 && c > -50.0);
        CHECK_NEAR("Kokyu: on the key's pitch (三 = A3)", tu_cents(tu_yin(buf + 44100, 8820, 44100.0), 220.0), 0.0, 25.0);
        {   /* 1.0.7: the bow tremolo is gone (the owner's call); what swing is left is the ensemble's own beating */
            double mn = 1e9, mx = 0.0;
            int w = 2205, k;
            for (k = 44100; k + w <= 88200; k += w) {
                double r = rms_of(k, k + w);
                mn = r < mn ? r : mn;
                mx = r > mx ? r : mx;
            }
            printf("  Kokyu: level swing over 1-2 s (50 ms windows) %.1f dB\n", db((float)(mx / mn)));
            CHECK("Kokyu: no tremolo (the 50 ms level swings under 3.5 dB over 1-2 s)", mx / mn < 1.585);
        }
        fm_key(5, 0);
        run(0, 44032);
    }
    {   /* the loop trills its own notes: a key and the trill ornament recorded, played back with the hands off */
        uint32_t P;
        int t[64], n;
        fm_init();
        fm_set(P_VOICE, slot_of("Mokugyo"));
        fm_set(P_REVERB, 0);
        fm_set(P_YOIN, 0);
        fm_set(P_LEVEL, 60);
        fm_set(P_TRILL_RATE, 50);
        fm_set(P_LOOP_BPM, 120);
        fm_set(P_LOOP_BEATS, 8);
        fm_loop_rec();
        run(0, 256);
        fm_key(5, 1);
        run(0, 256);
        fm_ornament(ORN_TRILL, 1);
        run(0, 44032);
        fm_ornament(ORN_TRILL, 0);
        fm_key(5, 0);
        run(0, 8192);
        fm_loop_rec();
        run(0, 256);
        P = fm_loop_len * 32u;
        run(0, (int)P - 256);
        run(0, (int)P);                           /* a full pass, nothing live */
        n = onset_times(0, 44032, t, 64);
        printf("  loop trill: %d strokes in the first second of a pass\n", n);
        CHECK("the loop's trill ornament re-plucks the loop's own note", n >= 5);
        fm_loop_clear();
        run(0, 256);
    }
    {   /* Koto Pluck responds to how hard it is struck: a light stroke quieter and duller, a strong one brighter */
        double l64, l100, l127, c64, c100, c127;
        fm_init();
        fm_set(P_VOICE, koto_a());
        fm_set(P_REVERB, 0);
        fm_set(P_YOIN, 0);
        fm_set(P_LEVEL, 60);
        run(0, 256);
        pluck_vel(0, 5, -1500.0f, 100.0f, 1, 0, 100, 1.0f);
        run(0, 11008);
        l100 = db(rms_of(0, 11008));              /* over the first 250 ms, as the voicing harness measured */
        c100 = centroid(0, 2646);
        release(0);
        run(0, 44032);
        pluck_vel(0, 5, -1500.0f, 100.0f, 1, 0, 64, 1.0f);
        run(0, 11008);
        l64 = db(rms_of(0, 11008));
        c64 = centroid(0, 2646);
        release(0);
        run(0, 44032);
        pluck_vel(0, 5, -1500.0f, 100.0f, 1, 0, 127, 1.0f);
        run(0, 11008);
        l127 = db(rms_of(0, 11008));
        c127 = centroid(0, 2646);
        release(0);
        run(0, 44032);
        printf("  Koto Pluck by velocity: 64 %.1f dB %.0f Hz, 100 %.1f dB %.0f Hz, 127 %.1f dB %.0f Hz\n", l64, c64, l100, c100, l127, c127);
        CHECK("Koto Pluck at velocity 64: 3 to 7 dB quieter and darker than at 100", l100 - l64 > 3.0 && l100 - l64 < 7.0 && c64 < c100 * 0.95);
        CHECK("Koto Pluck at velocity 127: brighter than at 100 (a fifth more centroid), not much louder", c127 > c100 * 1.2 && l127 - l100 < 2.0);
    }
    {
        uint8_t pk[128];
        sustain_patch(pk, 99);
        fm_patch_set(pk);
        run(0, 256);
    }

    /* ビブラート: the pitch moves */
    fm_set(P_VIB_ON, 1);
    fm_set(P_VIB_RATE, 100);
    fm_set(P_VIB_DEPTH, 100);
    fm_key(5, 1);
    run(0, 44032);
    {
        double lo = 1e9, hi = 0;
        for (i = 20000; i < 40000; i += 1000) {
            double f = tu_yin(buf + i, 2000, 44100.0);
            if (f > 0) {
                lo = f < lo ? f : lo;
                hi = f > hi ? f : hi;
            }
        }
        CHECK("vibrato at full depth: at least 40 cents of movement", tu_cents(hi, lo) > 40.0);
    }
    fm_key(5, 0);
    fm_set(P_VIB_ON, 0);
    run(0, 8192);

    /* the bend buttons: OCT+ with the note held bends it up a semitone and back */
    fm_key(5, 1);
    run(0, 8192);
    fm_bend(BEND_UP, 1, 1);
    run(0, 22016);
    CHECK_NEAR("OCT+ held: +100 cents", tu_cents(tu_yin(buf + 11000, 8000, 44100.0), 220.0), 100.0, 2.0);
    fm_bend(BEND_UP, 0, 1);
    run(0, 22016);
    CHECK_NEAR("released: back to the note", tu_cents(tu_yin(buf + 11000, 8000, 44100.0), 220.0), 0.0, 2.0);
    fm_set(P_BEND_UP, BEND_T_WHOLE);
    fm_bend(BEND_UP, 1, 1);
    run(0, 22016);
    CHECK_NEAR("whole-tone target: +200", tu_cents(tu_yin(buf + 11000, 8000, 44100.0), 220.0), 200.0, 2.0);
    fm_bend(BEND_UP, 0, 1);
    fm_set(P_BEND_UP, BEND_T_SCALE);
    run(0, 22016);
    fm_bend(BEND_UP, 1, 1);
    run(0, 22016);
    CHECK_NEAR("next-scale-note target from mi: fa, +100 - 20.8", tu_cents(tu_yin(buf + 11000, 8000, 44100.0), 220.0), 79.2, 2.0);
    fm_bend(BEND_UP, 0, 1);
    fm_set(P_BEND_UP, BEND_T_SEMI);
    fm_key(5, 0);
    run(0, 22016);

    /* patches: set / get round trip, the built-in slots, and the label */
    {
        uint8_t a[128], b[128];
        sustain_patch(a, 50);
        fm_patch_set(a);
        run(0, 256);
        fm_patch_get(b);
        CHECK("patch set / get round trip", !memcmp(a, b, 128));
        CHECK("a sent patch is custom", fm_patch_custom == 1);
        CHECK("built-in patches exist", fm_patch_count() >= 2 && fm_patch_name(0)[0] != 0);
        fm_set(P_VOICE, koto_a());
        run(0, 256);
        CHECK("P_VOICE loads a built-in", fm_patch_custom == 0 && !strncmp((const char *)fm_patch_label, "Koto Pluck", 10));
        fm_key(5, 1);
        {
            float pk = run(0, 22016);
            CHECK("the built-in koto sounds", pk > 0.001f);
        }
        fm_key(5, 0);
        run(0, 22016);
        fm_patch_set(a);
        sustain_patch(a, 99);
        fm_patch_set(a);
        run(0, 256);
    }

    /* the high cut darkens: fewer zero crossings of a bright (modulated) patch */
    {
        uint8_t v[FP_SIZE + 1], pk[128];
        int zc0 = 0, zc1 = 0;
        fm_patch_get(pk);
        fm6_unpack(pk, v);
        v[4 * FP_OP + FP_OL] = 90;                 /* OP2 modulates OP1 hard, ratio 7 */
        v[4 * FP_OP + FP_FC] = 7;
        fm6_pack(v, pk);
        fm_patch_set(pk);
        fm_set(P_HIGHCUT, 100);
        fm_key(5, 1);
        run(0, 22016);
        for (i = 11001; i < 22016; i++)
            zc0 += (buf[i] >= 0) != (buf[i - 1] >= 0);
        fm_set(P_HIGHCUT, 0);
        run(0, 22016);
        for (i = 11001; i < 22016; i++)
            zc1 += (buf[i] >= 0) != (buf[i - 1] >= 0);
        CHECK("high cut at minimum: far fewer zero crossings", zc1 < zc0 / 2);
        fm_set(P_HIGHCUT, 100);
        fm_key(5, 0);
        run(0, 8192);
        sustain_patch(pk, 99);
        fm_patch_set(pk);
    }

    /* reverb: a tail after the dry note has gone */
    fm_set(P_REVERB, 100);
    fm_set(P_REV_SIZE, 100);
    run(0, 256);
    fm_key(5, 1);
    run(0, 11008);
    fm_key(5, 0);
    run(11008, 44032);
    CHECK("reverb: still above -50 dB 300 ms after the dry note stopped", db(peak_of(11008 + 13230, 11008 + 17640)) > -50.0);
    fm_set(P_REVERB, 0);
    fm_set(P_REV_SIZE, 0);                         /* let the plate's tail die before the next checks */
    run(0, 44032);

    /* the level: 0 is silent; the ornaments: DAMP silences held notes */
    fm_set(P_LEVEL, 0);
    fm_key(5, 1);
    run(0, 8192);
    CHECK("level 0: silent", peak_of(6144, 8192) == 0.0f);
    fm_set(P_LEVEL, 60);
    fm_set(P_YOIN, 100);
    run(0, 8192);
    fm_ornament(ORN_DAMP, 1);
    run(0, 8192);
    CHECK("DAMP held: the note is gone within 100 ms", db(peak_of(4410, 8192)) < -50.0);
    fm_ornament(ORN_DAMP, 0);
    fm_key(5, 0);
    fm_set(P_YOIN, 0);
    run(0, 8192);

    /* keyboard scaling: the built-in koto (rate and level scaling on its carriers) decays faster at the top
     * of the range than at the bottom; a constant note number into the FM6 core would make them identical */
    fm_set(P_VOICE, koto_a());
    fm_set(P_YOIN, 100);
    run(0, 256);
    {
        double lo, hi;
        fm_key(0, 1);
        run(0, 44032 + 11008);
        lo = db(peak_of(44032, 44032 + 11008));
        fm_key(0, 0);
        fm_panic();
        run(0, 2048);
        fm_key(15, 1);
        run(0, 44032 + 11008);
        hi = db(peak_of(44032, 44032 + 11008));
        fm_key(15, 0);
        fm_panic();
        run(0, 2048);
        printf("  koto a second in: key 1 %.1f dB, key 16 %.1f dB\n", lo, hi);
        CHECK("the top key has decayed at least 6 dB more than the bottom key a second in", lo - hi > 6.0);
    }
    fm_set(P_YOIN, 0);
    {
        uint8_t pk[128];
        sustain_patch(pk, 99);
        fm_patch_set(pk);
        run(0, 256);
    }

    /* the A reference: 430 Hz puts 三 at 215 Hz */
    fm_set(P_A4, 430);
    fm_key(5, 1);
    run(0, 22016);
    CHECK_NEAR("A = 430: 三 at 1本 = 215 Hz", tu_yin(buf + 11000, 8000, 44100.0), 215.0, 0.3);
    fm_key(5, 0);
    fm_set(P_A4, 440);
    run(0, 8192);
    fm_note(FM_NOTE_ID, 1, 0.0f);
    fm_set(P_A4, 430);
    run(0, 22016);
    CHECK_NEAR("a note by absolute pitch ignores the A reference", tu_yin(buf + 11000, 8000, 44100.0), 440.0, 0.5);
    fm_note(FM_NOTE_ID, 0, 0.0f);
    fm_set(P_A4, 440);
    run(0, 8192);
    {   /* MIDI in with velocity (1.0.7): a note's velocity reaches the pluck, as the trill's accents do; fm_note is 100 */
        double l64, l100, l127;
        fm_set(P_VOICE, koto_a());
        fm_set(P_REVERB, 0);
        fm_set(P_YOIN, 0);
        run(0, 4096);
        fm_note(FM_NOTE_ID, 1, -1500.0f);
        run(0, 11008);
        l100 = db(rms_of(0, 11008));
        fm_note(FM_NOTE_ID, 0, 0.0f);
        run(0, 44032);
        fm_note_vel(FM_NOTE_ID, 1, -1500.0f, 64);
        run(0, 11008);
        l64 = db(rms_of(0, 11008));
        fm_note(FM_NOTE_ID, 0, 0.0f);
        run(0, 44032);
        fm_note_vel(FM_NOTE_ID, 1, -1500.0f, 127);
        run(0, 11008);
        l127 = db(rms_of(0, 11008));
        fm_note(FM_NOTE_ID, 0, 0.0f);
        run(0, 44032);
        printf("  MIDI in on Koto Pluck: velocity 64 %.1f dB, 100 %.1f dB, 127 %.1f dB\n", l64, l100, l127);
        CHECK("MIDI in: a note at velocity 64 is 3 to 9 dB under one at 100", l100 - l64 > 3.0 && l100 - l64 < 9.0);
        CHECK("MIDI in: a note at velocity 127 is louder than one at 100, by under 3 dB", l127 > l100 && l127 - l100 < 3.0);
        fm_set(P_VOICE, 0);
        run(0, 4096);
    }

    /* the wood pair (1.0.4): 拍子木 Hyoshigi, two sticks, a crack with no tail whatever 余韻 says; 木魚 Mokugyo, the
     * wooden fish, a pitched tok that follows the keys. Both the key held and 余韻 full: percussion ends by itself */
    {
        int hy = slot_of("Hyoshigi"), mo = slot_of("Mokugyo");
        float a, b;
        CHECK("Hyoshigi and Mokugyo are built in", hy >= 0 && mo >= 0);
        fm_set(P_REVERB, 0);
        fm_set(P_LEVEL, 60);
        fm_set(P_YOIN, 100);
        fm_set(P_HON, 4);
        fm_set(P_VOICE, hy < 0 ? 0 : hy);
        run(0, 256);
        fm_key(5, 1);
        run(0, 44032);
        a = peak_of(0, 1323);
        b = peak_of(8820, 44032);
        printf("  Hyoshigi: peak %.1f dB, in 10 ms %.1f dB, after 200 ms %.1f dB; -40 dB at %d ms\n", db(a), db(peak_of(0, 441)), db(b), ms_to_db_below(0, a, 40.0));
        CHECK("Hyoshigi: hits at once (the peak inside 10 ms)", peak_of(0, 441) > 0.7f * a);
        CHECK("Hyoshigi: the crack is over in 200 ms with the key held and 余韻 full (40 dB down)", db(b) < db(a) - 40.0);
        CHECK("Hyoshigi: and it is heard (above -40 dBFS at level 60)", db(a) > -40.0);
        fm_key(5, 0);
        run(0, 44032);
        fm_set(P_VOICE, mo < 0 ? 0 : mo);
        run(0, 256);
        fm_key(5, 1);
        run(0, 44032);
        a = peak_of(0, 1323);
        b = peak_of(17640, 44032);
        printf("  Mokugyo: peak %.1f dB, in 10 ms %.1f dB, after 400 ms %.1f dB; -30 dB at %d ms; pitch %.1f cents from A3 (50-150 ms)\n",
               db(a), db(peak_of(0, 441)), db(b), ms_to_db_below(0, a, 30.0), tu_cents(tu_yin(buf + 2205, 4410, 44100.0), 220.0));
        CHECK("Mokugyo: hits at once (the peak inside 10 ms)", peak_of(0, 441) > 0.7f * a);
        CHECK("Mokugyo: the tok is over in 400 ms with the key held and 余韻 full (30 dB down)", db(b) < db(a) - 30.0);
        CHECK_NEAR("Mokugyo: pitched, following the key (三 at 1本 = A3)", tu_cents(tu_yin(buf + 2205, 4410, 44100.0), 220.0), 0.0, 15.0);
        fm_key(5, 0);
        run(0, 44032);
        fm_key(10, 1);                            /* a fifth... no: the next mi, an octave up */
        run(0, 44032);
        CHECK_NEAR("Mokugyo: the octave above on the next mi", tu_cents(tu_yin(buf + 2205, 4410, 44100.0), 440.0), 0.0, 15.0);
        fm_key(10, 0);
        run(0, 44032);
    }
    /* the looper (1.0.4): REC arms, the first key starts the loop, REC closes it and it plays, REC overdubs,
     * PLAY stops and starts, undo takes the last layer, clear empties it. The Mokugyo (ends by itself, key-synced
     * oscillators, no LFO) with no 余韻 and no reverb: a pass of the loop is the same audio every time */
    {
        uint32_t L, P;
        float pre, post;
        fm_init();
        fm_set(P_VOICE, slot_of("Mokugyo"));
        fm_set(P_REVERB, 0);
        fm_set(P_YOIN, 0);
        fm_set(P_LEVEL, 60);
        fm_set(P_LOOP_BPM, 120);
        fm_set(P_LOOP_BEATS, 8);
        run(0, 256);
        CHECK("looper: idle at boot", fm_loop_state == LP_IDLE && fm_loop_len == 0);
        fm_loop_rec();
        run(0, 256);
        CHECK("REC tap: armed", fm_loop_state == LP_ARMED);
        run(0, 44032);                            /* a second of waiting: nothing starts */
        CHECK("armed: the loop has not started", fm_loop_state == LP_ARMED);
        fm_key(5, 1);
        run(0, 256);
        CHECK("the first key starts the recording", fm_loop_state == LP_REC && fm_loop_pos <= 8u);
        run(0, 256 * 20);
        fm_key(5, 0);
        run(0, 256 * (150 - 21));                 /* 150 renders = 1200 blocks from the key */
        fm_loop_rec();
        run(0, 256);
        CHECK("REC again closes the loop and plays it", fm_loop_state == LP_PLAY);
        L = fm_loop_len;
        CHECK_NEAR("the loop is as long as what was played (blocks of 32)", (double)L, 1200.0, 8.0);
        CHECK("one layer, two events", fm_loop_layers == 1 && fm_loop_events == 2);
        P = L * 32u;
        run(0, (int)P - 256);                     /* the rest of the first pass: at the loop's start again */
        run(0, (int)P);                           /* a full pass, kept as the reference */
        memcpy(ref, buf, P * sizeof buf[0]);
        run(0, (int)P);                           /* the next pass */
        CHECK("a pass is the same as the last, sample for sample", pass_diff(P) < 1e-5f && peak_of(0, (int)P) > 0.003f);
        CHECK_NEAR("the loop's note is the key's pitch (三 = A3)", tu_cents(tu_yin(buf + 2205, 4410, 44100.0), 220.0), 0.0, 15.0);
        fm_set(P_HON, 6);                         /* the loop stores keys: 本数 up two retunes it */
        run(0, (int)P);
        run(0, (int)P);
        CHECK_NEAR("本数 up two: the loop follows (+200 cents)", tu_cents(tu_yin(buf + 2205, 4410, 44100.0), 220.0), 200.0, 15.0);
        fm_set(P_HON, 4);
        run(0, (int)P);
        /* 1.0.5: a layer keeps the instrument it was recorded in; PRESETS turns only what the hands play */
        fm_set(P_VOICE, slot_of("Hyoshigi"));
        run(0, (int)P);
        run(0, (int)P);
        CHECK("PRESETS turned while the loop plays: the loop still sounds in the instrument it was recorded in", pass_diff(P) < 1e-5f);
        fm_set(P_VOICE, slot_of("Mokugyo"));
        run(0, (int)P);
        /* overdub: a second note on a new layer, half way round; undo takes it away again. The windows compared
         * start at the loop's start every time (the counts of 256-frame renders keep the phase) */
        fm_loop_rec();
        run(0, 256);
        CHECK("REC while playing: overdub", fm_loop_state == LP_DUB);
        run(0, (int)P / 2);
        fm_key(8, 1);
        run(0, 256 * 10);
        fm_key(8, 0);
        run(0, 256);
        fm_loop_rec();
        run(0, 256);
        CHECK("REC again: overdub off, two layers, four events", fm_loop_state == LP_PLAY && fm_loop_layers == 2 && fm_loop_events == 4);
        run(0, (int)P - 256 - (int)P / 2 - 256 * 12);   /* the rest of this pass */
        run(0, (int)P);
        CHECK("the pass has the second note in it", pass_diff(P) > 0.003f);
        /* 1.0.5: a third layer in another instrument; each layer keeps its own through later turns of PRESETS */
        fm_set(P_VOICE, slot_of("Hyoshigi"));
        fm_loop_rec();
        run(0, 256);
        run(0, (int)P / 4);
        fm_key(12, 1);
        run(0, 256 * 4);
        fm_key(12, 0);
        run(0, 256);
        fm_loop_rec();
        run(0, 256);
        CHECK("a third layer, in the sticks", fm_loop_state == LP_PLAY && fm_loop_layers == 3 && fm_loop_events == 6);
        run(0, (int)P - 256 - (int)P / 4 - 256 * 6);
        run(0, (int)P);
        memcpy(ref, buf, P * sizeof buf[0]);        /* the three layers, two instruments */
        fm_set(P_VOICE, slot_of("Koto"));
        run(0, (int)P);
        run(0, (int)P);
        CHECK("PRESETS to Koto: the fish and the sticks still play as themselves", pass_diff(P) < 1e-5f);
        {                                         /* and a key played now is the koto: it rings where the fish is long gone */
            float early, late;
            fm_set(P_LOOP_BPM, 120);
            fm_loop_play();                       /* stopped: only the hands sound */
            run(0, 44032);
            fm_key(14, 1);
            early = run(0, 2205);
            run(0, 2205 * 3);
            late = run(0, 4410);
            fm_key(14, 0);
            run(0, 44032);
            printf("  live key after the turn to Koto: %.1f dB at the pluck, %.1f dB at 200-300 ms\n", db(early), db(late));
            CHECK("the hands play the current instrument (the koto still rings 200 ms in; the fish would be 30 dB down)", db(late) > db(early) - 20.0);
            fm_loop_play();                       /* playing again, from the top */
            run(0, 256);
        }
        fm_set(P_VOICE, slot_of("Mokugyo"));
        fm_loop_undo();                           /* the third layer goes */
        run(0, (int)P - 256);
        run(0, (int)P);
        run(0, (int)P);
        memcpy(ref, buf, P * sizeof buf[0]);
        run(0, (int)P);
        CHECK("undo of the third layer: two layers play the same, pass after pass", fm_loop_layers == 2 && pass_diff(P) < 1e-5f);
        /* back to the two-layer reference for the undo check below: the first pass, kept at the top, is rebuilt */
        fm_loop_undo();
        run(0, (int)P);
        run(0, (int)P);
        memcpy(ref, buf, P * sizeof buf[0]);
        fm_loop_rec();                            /* the second note again, as above */
        run(0, 256);
        run(0, (int)P / 2);
        fm_key(8, 1);
        run(0, 256 * 10);
        fm_key(8, 0);
        run(0, 256);
        fm_loop_rec();
        run(0, 256);
        run(0, (int)P - 256 - (int)P / 2 - 256 * 12);
        run(0, (int)P);
        CHECK("the second layer again: the pass has the second note in it", fm_loop_layers == 2 && pass_diff(P) > 0.003f);
        fm_loop_undo();
        run(0, (int)P);                           /* the pass after the undo: the removed layer's tails go */
        run(0, (int)P);
        printf("  undo: layers %d, events %u, difference from the first pass %.2e\n", fm_loop_layers, fm_loop_events, (double)pass_diff(P));
        CHECK("undo: one layer, two events, the first pass again", fm_loop_layers == 1 && fm_loop_events == 2 && pass_diff(P) < 1e-5f);
        /* PLAY stops (the notes let go) and starts from the top */
        fm_loop_play();
        run(0, 256);
        CHECK("PLAY: stopped", fm_loop_state == LP_STOP);
        run(0, 44032);
        CHECK("stopped: nothing sounds", fm_nvoices == 0 && fm_loop_pos == 0u);
        fm_loop_play();
        run(0, 256);
        CHECK("PLAY again: playing from the top", fm_loop_state == LP_PLAY && fm_loop_pos <= 8u && fm_nvoices == 1);
        fm_loop_clear();
        run(0, 256);
        CHECK("clear: idle, empty", fm_loop_state == LP_IDLE && fm_loop_events == 0 && fm_loop_len == 0 && fm_loop_layers == 0);
        /* the seam: a note held across the loop point rings over the join (余韻 80) */
        fm_set(P_YOIN, 80);
        fm_set(P_VOICE, slot_of("Koto"));
        fm_loop_rec();
        run(0, 256);
        fm_key(5, 1);
        run(0, 44032);
        fm_loop_rec();                            /* closed with the key still down */
        run(0, 256);
        fm_key(5, 0);
        L = fm_loop_len;
        P = L * 32u;
        run(0, (int)P - 256 - 4410);              /* to 100 ms before the join */
        pre = run(0, 4410);                       /* the 100 ms before */
        post = run(0, 4410);                      /* the 100 ms after */
        printf("  seam: %.1f dB before the join, %.1f dB after, %d voices\n", db(pre), db(post), fm_nvoices);
        CHECK("the tail rings over the join (not cut, not choked)", db(post) > db(pre) - 6.0 && fm_nvoices >= 2);
        fm_loop_clear();
        run(0, 256);
        /* the length by the ruler: 8 beats at 120 BPM close the loop by themselves after 4 s */
        fm_set(P_YOIN, 0);
        fm_set(P_VOICE, slot_of("Mokugyo"));
        fm_loop_rec();
        run(0, 256);
        fm_key(5, 1);
        run(0, 256);
        fm_key(5, 0);
        run(0, 44100 * 2);
        CHECK("still recording at 2 s", fm_loop_state == LP_REC);
        run(0, 44100 * 2 + 1024);
        CHECK("8 beats at 120 BPM: closed by itself at 4 s and playing", fm_loop_state == LP_PLAY && fm_loop_len == 5512u);
        fm_loop_clear();
        run(0, 256);
    }
    /* the looper's corners (found in review): nothing may drone. A sustaining patch, so a loop note still down
     * is a voice still counted */
    {
        uint8_t pk[128];
        int i, k, down;
        fm_init();
        sustain_patch(pk, 99);
        fm_patch_set(pk);
        fm_set(P_REVERB, 0);
        fm_set(P_YOIN, 0);
        fm_set(P_LEVEL, 60);
        fm_set(P_LOOP_BPM, 120);
        fm_set(P_LOOP_BEATS, 8);
        run(0, 256);
        /* 1: a key pressed in the same drain as the REC that closes the loop: its release must follow its press */
        fm_loop_rec();
        run(0, 256);
        fm_key(5, 1);
        run(0, 256 * 100);
        fm_key(5, 0);
        run(0, 256 * 20);
        fm_key(6, 1);
        fm_loop_rec();                            /* the same drain as the press */
        run(0, 256);
        fm_key(6, 0);
        CHECK("closed with a press in the same drain", fm_loop_state == LP_PLAY);
        run(0, 256 * 130);                        /* a pass and a bit: past the press at the end */
        run(0, 256 * 60);                         /* half way through the next pass: the loop note is released */
        down = 0;
        for (i = 0; i < FM_NVOICE; i++)
            if (vc[i].live && vc[i].down && lp_key_of(vc[i].id) == 6)
                down = 1;
        CHECK("the loop's copy of that key is not left down", !down);
        fm_loop_clear();
        run(0, 256);
        /* 2: the event store full: a press that cannot take its release with it is not recorded */
        fm_set(P_LOOP_BPM, 30);
        fm_set(P_LOOP_BEATS, 64);
        fm_loop_rec();
        run(0, 256);
        fm_key(20, 1);                            /* one tap first: the store then fills between a pair's two
                                                   * presses and their releases */
        run(0, 256);
        fm_key(20, 0);
        run(0, 256);
        for (k = 0; k < 270; k++) {               /* two keys at a time: 1082 events offered, 1024 fit */
            fm_key(k % FM_NKEY, 1);
            fm_key((k + 1) % FM_NKEY, 1);
            run(0, 256);
            fm_key(k % FM_NKEY, 0);
            fm_key((k + 1) % FM_NKEY, 0);
            run(0, 256);
        }
        fm_loop_rec();
        run(0, 256);
        CHECK("the store holds what fits", fm_loop_state == LP_PLAY && fm_loop_events <= 1024u && fm_loop_events > 1000u);
        run(0, (int)fm_loop_len * 32 - 256 - 100 * 32);   /* near the end of the pass: the taps that found no room
                                                           * were the last ones; every press taken has had its release */
        down = 0;
        for (i = 0; i < FM_NVOICE; i++)
            if (vc[i].live && vc[i].down && lp_key_of(vc[i].id) >= 0)
                down = 1;
        CHECK("a full store leaves no loop note down", !down);
        fm_loop_clear();
        run(0, 256);
        fm_set(P_LOOP_BPM, 120);
        fm_set(P_LOOP_BEATS, 8);
        /* 3: an ornament the loop turned on is turned off by stop, undo and clear */
        fm_loop_rec();
        run(0, 256);
        fm_key(5, 1);
        run(0, 256 * 10);
        fm_ornament(ORN_VIB, 1);
        run(0, 256 * 40);
        fm_ornament(ORN_VIB, 0);
        fm_key(5, 0);
        run(0, 256 * 10);
        fm_loop_rec();
        run(0, 256);
        run(0, 256 * 20);                         /* into the next pass, inside the ornament */
        CHECK("the loop turned the ornament on", orn[1][ORN_VIB] == 1);
        fm_loop_play();
        run(0, 256);
        CHECK("stop turns the loop's ornament off", orn[1][ORN_VIB] == 0);
        fm_loop_play();
        run(0, 256 * 21);
        CHECK("playing again: on again", orn[1][ORN_VIB] == 1);
        fm_loop_clear();
        run(0, 256);
        CHECK("clear turns it off", orn[1][ORN_VIB] == 0 && fm_loop_state == LP_IDLE);
        /* 4: an overdubbed note held longer than the loop: the loop's copy lets go when the hand does */
        fm_loop_rec();
        run(0, 256);
        fm_key(5, 1);
        run(0, 256 * 10);
        fm_key(5, 0);
        run(0, 256 * 50);
        fm_loop_rec();
        run(0, 256);
        fm_loop_rec();                            /* overdub */
        run(0, 256);
        fm_key(9, 1);
        run(0, (int)fm_loop_len * 32 + 256 * 10);   /* held through the wrap: the copy has started */
        fm_key(9, 0);
        run(0, 256 * 3);
        down = 0;
        for (i = 0; i < FM_NVOICE; i++)
            if (vc[i].live && vc[i].down && lp_key_of(vc[i].id) == 9)
                down = 1;
        CHECK("the loop's copy of a note held past the wrap lets go with the hand", !down);
        fm_loop_rec();
        run(0, 256);
        /* 5: undo during an overdub that recorded nothing undoes the layer below */
        CHECK("two layers", fm_loop_layers == 2);
        fm_loop_rec();
        run(0, 256);
        fm_loop_undo();
        run(0, 256);
        CHECK("undo in an empty overdub takes the layer below", fm_loop_layers == 1 && fm_loop_state == LP_PLAY);
        fm_loop_clear();
        run(0, 256);
    }
    /* cost and headroom: the six-operator koto, eight voices held, reverb; the pre-clip peak (fm_peak) stays
     * inside what the soft clip handles gracefully. The device figure comes from plat_cpu_pct() at M4 */
    fm_set(P_VOICE, koto_a());
    fm_set(P_REVERB, 50);
    fm_set(P_LEVEL, 80);
    run(0, 256);
    for (i = 0; i < 8; i++)
        fm_key(i, 1);
    {
        clock_t t0 = clock();
        float pre = 0;
        int k;
        for (k = 0; k < 44032; k += 256) {
            run(k, 256);
            if (fm_peak > pre)
                pre = fm_peak;
        }
        {
            double s = (double)(clock() - t0) / CLOCKS_PER_SEC;
            printf("  cost: KOTO A x 8 voices + reverb: 1 s in %.4f s of host time (%.0f ns a frame); pre-clip peak %.2f\n", s, s * 1e9 / 44032.0, (double)pre);
            CHECK("renders faster than real time on the host", s < 1.0);
            CHECK("pre-clip peak of eight koto attacks at level 80 stays under 2 (the soft clip rounds the rest)", pre < 2.0f);
            CHECK("and they are loud enough to matter", pre > 0.02f);
        }
    }
    for (i = 0; i < 8; i++)
        fm_key(i, 0);
    fm_panic();
    run(0, 256);
    CHECK("panic: quiet", fm_nvoices == 0);

    /* 1.1.0: the loop's ruler reaches 20..240 BPM and 4..256 beats (a loop of up to twelve minutes, sparse: 1024 events);
     * the trill starts at 85 and the ornament key's oroshi at Long, as the owner plays */
    CHECK("Loop BPM 20..240, default 60", fm_param_info(P_LOOP_BPM)->lo == 20 && fm_param_info(P_LOOP_BPM)->hi == 240 && fm_param_info(P_LOOP_BPM)->def == 60);
    CHECK("Loop beats 4..256, default 32", fm_param_info(P_LOOP_BEATS)->lo == 4 && fm_param_info(P_LOOP_BEATS)->hi == 256 && fm_param_info(P_LOOP_BEATS)->def == 32);
    CHECK("Trill rate default 85", fm_param_info(P_TRILL_RATE)->def == 85);
    CHECK("Oroshi default Long", fm_param_info(P_TRILL_OROSHI)->def == ORO_LONG);
    {   /* 1.1.0: the ruler's length as the Loop page prints it */
        char t[16];
        fm_loop_ruler_text(60, 32, t);
        CHECK("ruler 32 beats at 60 BPM reads 0:32", !strcmp(t, "0:32"));
        fm_loop_ruler_text(20, 256, t);
        CHECK("ruler 256 beats at 20 BPM reads 12:48", !strcmp(t, "12:48"));
        fm_loop_ruler_text(240, 4, t);
        CHECK("ruler 4 beats at 240 BPM reads 0:01", !strcmp(t, "0:01"));
    }
    {   /* 1.1.0: the loop as an image for the app's flash object, and back. A loop recorded in Koto II and stopped
         * is imaged; the engine restarts on Taiko; restored, the loop is back stopped with its events and length,
         * and its layer still plays Koto II */
        static uint8_t img[16384];
        uint32_t n, len, ev, k, gen;
        int found = 0;
        double l_loop, c_loop, l_live, c_live;
        fm_init();
        fm_set(P_VOICE, 1);                       /* Koto II */
        fm_set(P_REVERB, 0);
        fm_set(P_YOIN, 0);
        fm_set(P_LEVEL, 60);
        fm_set(P_LOOP_BPM, 120);
        fm_set(P_LOOP_BEATS, 8);
        run(0, 256);
        CHECK("an empty loop's image is a header alone", fm_loop_image(img, sizeof img, 0) == 16);
        CHECK("and restores to an empty loop", fm_loop_restore(img, 16) == 0 && fm_loop_state == LP_IDLE);
        gen = fm_loop_gen;
        fm_loop_rec();
        run(0, 256);
        fm_key(5, 1);
        run(0, 11008);
        fm_key(5, 0);
        run(0, 11008);
        fm_loop_rec();                            /* closed: it plays */
        run(0, 256);
        CHECK("a playing loop gives no image", fm_loop_state == LP_PLAY && fm_loop_image(img, sizeof img, 0) == 0);
        CHECK("the close moved the loop's generation", fm_loop_gen != gen);
        fm_loop_play();                           /* stopped */
        run(0, 256);
        CHECK("stopped", fm_loop_state == LP_STOP);
        len = fm_loop_len;
        ev = fm_loop_events;
        n = fm_loop_image(img, sizeof img, 0);
        CHECK("the image: the header, the layer's slot, one packed instrument, the events", n == 16u + 1u + 128u + 8u * ev);
        for (k = 0; k + 128 <= n; k++)
            if (!memcmp(img + k, fm_patch_builtin(1), 128))
                found = 1;
        CHECK("the image carries the layer's instrument, Koto II, packed", found);
        CHECK("an image too big for the buffer is refused", fm_loop_image(img, 100, 0) == 0);
        fm_init();
        fm_set(P_VOICE, slot_of("Taiko"));
        fm_set(P_REVERB, 0);
        fm_set(P_YOIN, 0);
        fm_set(P_LEVEL, 60);
        run(0, 256);
        img[6] ^= 0x80;                           /* a damaged image: refused, the loop stays empty */
        CHECK("a damaged image is refused", fm_loop_restore(img, n) != 0 && fm_loop_state == LP_IDLE);
        img[6] ^= 0x80;
        CHECK("restored", fm_loop_restore(img, n) == 0);
        run(0, 256);
        CHECK("back stopped, with its length, events and layer", fm_loop_state == LP_STOP && fm_loop_len == len && fm_loop_events == ev && fm_loop_layers == 1);
        fm_loop_play();                           /* the loop's pluck starts with the window, as the live one will */
        run(0, 11008);
        CHECK("it plays", fm_loop_state == LP_PLAY);
        l_loop = db(rms_of(0, 11008));
        c_loop = centroid(0, 2646);
        fm_loop_clear();
        run(0, 44032);
        fm_set(P_VOICE, 1);                       /* the same pluck live, in Koto II */
        run(0, 256);
        fm_key(5, 1);
        run(0, 11008);
        l_live = db(rms_of(0, 11008));
        c_live = centroid(0, 2646);
        fm_key(5, 0);
        run(0, 44032);
        printf("  restored loop: %.1f dB, centroid %.0f Hz; the pluck live in Koto II: %.1f dB, %.0f Hz\n", l_loop, c_loop, l_live, c_live);
        CHECK("the restored layer plays its own instrument, Koto II, though PRESETS is on Taiko",
              fabs(l_loop - l_live) < 1.0 && fabs(c_loop - c_live) < 0.1 * c_live);
    }
    {   /* review (1.1.0): a loop whose eight layers were each recorded in a different instrument uses every slot, the
         * eighth included, and still comes back */
        static uint8_t img[16384];
        uint32_t n, v, gen;
        fm_init();
        fm_set(P_REVERB, 0);
        fm_set(P_YOIN, 0);
        fm_set(P_LOOP_BPM, 120);
        fm_set(P_LOOP_BEATS, 8);
        for (v = 0; v < 8; v++) {
            fm_set(P_VOICE, (int)v);
            run(0, 256);
            fm_loop_rec();                        /* arm, or overdub on */
            run(0, 256);
            fm_key(5, 1);
            run(0, 2048);
            fm_key(5, 0);
            run(0, 2048);
            fm_loop_rec();                        /* close, or overdub off */
            run(0, 256);
        }
        fm_loop_play();
        run(0, 256);
        CHECK("eight layers, stopped", fm_loop_state == LP_STOP && fm_loop_layers == 8);
        n = fm_loop_image(img, sizeof img, &gen);
        CHECK("the image holds eight packed instruments", n == 16u + 8u + 8u * 128u + 8u * fm_loop_events && gen == fm_loop_gen);
        fm_init();
        CHECK("and restores, eight layers", fm_loop_restore(img, n) == 0 && fm_loop_layers == 8 && fm_loop_state == LP_STOP);
        fm_loop_clear();
        run(0, 256);
        gen = fm_loop_gen;
        fm_loop_clear();
        run(0, 256);
        CHECK("clearing an empty loop does not move the generation (no needless flash write)", fm_loop_gen == gen);
    }
    return tu_done("fumi");
}
