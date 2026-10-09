/* SPDX-License-Identifier: GPL-3.0-only */
/* dsp/fumi.c: the instrument on the host. Pitch of the keys through the whole engine, the tunings, 余韻,
 * polyphony and stealing, 単音, トリラー, ビブラート, the bend buttons, patches, filters, reverb, levels. */
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "tu.h"
#include "tu_yin.h"
#include "../../firmware/src/dsp/fumi.c"

void fm_midi_out(uint32_t st, uint32_t d1, uint32_t d2) { (void)st; (void)d1; (void)d2; }

static float buf[3 * 44100];                      /* mono (left) */
static int32_t blk[2 * 256];

/* render n samples (a multiple of 256) into buf from pos; returns the peak */
static float run(int pos, int n)
{
    int i, k;
    float pk = 0;
    for (i = 0; i < n; i += 256) {
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
    return tu_done("fumi");
}
