/* SPDX-License-Identifier: GPL-3.0-only */
/* FuMi-1: the instrument (fumi.h). One compilation unit, -O2: the pitch core, the FM6 core, the reverb and
 * the output stage are included here. */
#include "fumi.h"
#include "fastmath.h"
#ifdef FM_HOST
#define FM_POOL
#else
#define FM_POOL __attribute__((section(".pool")))
#endif
#define BARRIER() __asm__ volatile("" ::: "memory")

#include "tuning.c"
#include "smooth.c"
#include "bend.c"
#include "fm6_core.c"
#include "fm6_patch.c"
#include "reverb.c"
#include "fumi_patches.h"

#define BLK FM6_N                                  /* 32 samples: the control block */
/* -DFM_NO_SMOOTH: the click regression test's proof that it can fail. Gains jump instead of ramping, a steal
 * resets the phases, a short 余韻 gates the note. Never defined in a real build. */
#define BLK_MS (1000.0f * (float)BLK / FM_SR)

/* ------------------------------------------------------------- params --- */
static const fm_param_t PARAMS[P_NPARAMS] = {
    {"Hon", 0, TN_NHON - 1, 4}, {"Scale", 0, SC_N - 1, SC_IN}, {"Tuning", 0, TN_N - 1, TN_SUIKO}, {"Depth", 0, 150, 100},
    {"Fine", -50, 50, 0}, {"A", 430, 445, 440}, {"Octave", -1, 1, 0},
    {"Voice", 0, FM_NPATCH - 1, 4}, {"Yoin", 0, 100, 80}, {"Level", 0, 100, 80},
    {"Vibrato", 0, 1, 0}, {"Vib rate", 0, 100, 45}, {"Vib depth", 0, 100, 80},
    {"Trill", 0, 1, 0}, {"Trill rate", 0, 100, 50}, {"Trill var", 0, 100, 30},
    {"Mono", 0, 1, 0}, {"Black keys", 0, 1, 0}, {"Bend up", 0, 2, BEND_T_SEMI}, {"Bend down", 0, 100, 40},
    {"Bend time", 20, 300, 90}, {"Slide", 0, 1, 0}, {"Slide time", 10, 500, 80},
    {"Low cut", 0, 100, 10}, {"High cut", 0, 100, 100}, {"Character", 0, 100, 0}, {"Reverb", 0, 100, 55},
    {"Rev size", 0, 100, 50}, {"MIDI out", 0, 1, 1},
    {"User mi", -50, 50, 0}, {"User fa", -50, 50, 0}, {"User fa#", -50, 50, 0}, {"User sol", -50, 50, 0},
    {"User 4", -50, 50, 0}, {"User la", -50, 50, 0}, {"User tib", -50, 50, 0}, {"User ti", -50, 50, 0},
    {"User do", -50, 50, 0}, {"User do#", -50, 50, 0}, {"User re", -50, 50, 0}, {"User 11", -50, 50, 0},
};
const fm_param_t *fm_param_info(int p) { return (p >= 0 && p < P_NPARAMS) ? &PARAMS[p] : &PARAMS[0]; }

static void itoa_s(int v, char *b)
{
    char t[8];
    int n = 0, neg = v < 0;
    if (neg)
        v = -v;
    do {
        t[n++] = (char)('0' + v % 10);
        v /= 10;
    } while (v && n < 6);
    if (neg)
        *b++ = '-';
    while (n)
        *b++ = t[--n];
    *b = 0;
}
static void copy_s(const char *s, char *b, int max)
{
    int k = 0;
    while (s[k] && k < max) {
        b[k] = s[k];
        k++;
    }
    b[k] = 0;
}

void fm_param_text(int p, int v, char *b)
{
    static const char *const ONOFF[2] = {"Off", "On"};
    static const char *const BENDT[3] = {"Semi", "Whole", "Scale"};
    static const char *const BLACKM[2] = {"Upper", "Ornam"};
    const fm_param_t *pi = fm_param_info(p);
    v = v < pi->lo ? pi->lo : v > pi->hi ? pi->hi : v;
    switch (p) {
    case P_HON: copy_s(TN_HON_ASCII[v], b, 11); return;
    case P_SCALE: copy_s(SC_NAME[v], b, 11); return;
    case P_TUNING: copy_s(TN_NAME[v], b, 11); return;
    case P_VOICE: copy_s(FM_PATCH_NAME[v], b, 11); return;
    case P_VIB_ON: case P_TRILL_ON: case P_MONO: case P_SLIDE: case P_MIDI_OUT: copy_s(ONOFF[v ? 1 : 0], b, 11); return;
    case P_BEND_UP: copy_s(BENDT[v], b, 11); return;
    case P_BLACK: copy_s(BLACKM[v], b, 11); return;
    default: break;
    }
    if ((p == P_FINE || p == P_OCTAVE || p >= P_USER0) && v > 0)
        *b++ = '+';
    itoa_s(v, b);
}

/* ----------------------------------------------------------- commands --- */
enum { C_SET, C_KEY, C_NOTE, C_ORN, C_BEND, C_PANIC, C_PATCH };
typedef struct {
    uint8_t c, a;
    int16_t b;
    float f;
} cmd_t;
#define QN 128u
static cmd_t q[QN];
static volatile uint32_t q_w, q_r;
static void post(uint32_t c, int a, int b, float f)
{
    uint32_t w = q_w;
    if (w - q_r >= QN)
        return;                                   /* full: dropped (never at the UI's rate) */
    q[w % QN].c = (uint8_t)c;
    q[w % QN].a = (uint8_t)a;
    q[w % QN].b = (int16_t)b;
    q[w % QN].f = f;
    BARRIER();
    q_w = w + 1;
}
void fm_set(int p, int v) { post(C_SET, p, v, 0.0f); }
void fm_key(int key, int on) { post(C_KEY, key, on, 0.0f); }
void fm_note(int id, int on, float cents) { post(C_NOTE, id, on, cents); }
void fm_ornament(int orn, int on) { post(C_ORN, orn, on, 0.0f); }
void fm_bend(int which, int down, int note_held) { post(C_BEND, which, down | note_held << 1, 0.0f); }
void fm_panic(void) { post(C_PANIC, 0, 0, 0.0f); }

/* the patch: the main loop writes the pending record and bumps the generation; the render adopts it */
static uint8_t patch_pending[FM6_PACKED];
static volatile uint32_t patch_gen, patch_seen;
void fm_patch_set(const uint8_t *pk)
{
    uint32_t i;
    for (i = 0; i < FM6_PACKED; i++)
        patch_pending[i] = pk[i];
    BARRIER();
    patch_gen++;
    post(C_PATCH, 0, 0, 0.0f);
}
static uint8_t patch_cur_packed[FM6_PACKED];       /* what the render plays, packed (fm_patch_get) */
void fm_patch_get(uint8_t *pk)
{
    uint32_t i;
    for (i = 0; i < FM6_PACKED; i++)
        pk[i] = patch_cur_packed[i];
}
int fm_patch_count(void) { return FM_NPATCH; }
const char *fm_patch_name(int i) { return FM_PATCH_NAME[(i < 0 || i >= FM_NPATCH) ? 0 : i]; }
const uint8_t *fm_patch_builtin(int i) { return FM_PATCH[(i < 0 || i >= FM_NPATCH) ? 0 : i]; }

/* --------------------------------------------------------------- state --- */
volatile float fm_key_level[FM_NKEY];
volatile float fm_bend_cents;
volatile float fm_peak;
volatile uint8_t fm_nvoices;
volatile char fm_patch_label[11];
volatile uint8_t fm_patch_custom;

static int16_t par[P_NPARAMS];
static tuning_t tun;
static bend_t bend;
static uint8_t patch[FP_SIZE + 1];                /* the current voice, unpacked */
static fm6_lfo_t plfo;
static int32_t lfo_v, lfo_d;
static const int32_t DT0[6] = {0, 0, 0, 0, 0, 0};

typedef struct {
    fm6_note_t n;
    int id;                                       /* the key (FM_NKEY ids) or note id; -1 free */
    float base;                                   /* cents from A4 of the key, through the tuning */
    float next_up;                                /* the next scale note above, cents (for BEND_T_SCALE) */
    smooth_t pitch;                               /* the glide to the target (bend, tuning switch, slide) */
    uint8_t sliding;                              /* the glide runs at the slide time until it arrives */
    float yo_g, yo_m;                             /* 余韻: the release gain and its multiplier per sample */
    float g, dc, lvl;                             /* last block's gain (ramps), DC blocker, level for the lights */
    uint8_t down, live, vel;
    uint32_t order;
} voice_t;
static voice_t vc[FM_NVOICE];
static uint32_t order_n;
static int bend_voice = -1;                       /* the voice the bend buttons move: the most recent note */

static int held[64], nheld;                       /* keys down, in press order (the mono / slide stack) */
static uint8_t orn[ORN_N];
static uint32_t ms_count;                         /* the engine's clock, ms (bend.c) */
static float ms_acc;
static float vib_ph, vib_inc, vib_depth, vib_cur;
static float trill_left, trill_period, trill_var;
static uint32_t rnd = 0x1234567u;
static smooth_t level_sm;
static float level_cur;
static float yo_mult;                             /* 余韻: the multiplier per sample after key-up */
static float k_fast, k_slide;                     /* the voice smoother's coefficients: 12 ms, the slide time */
static float a4_cents;                            /* the A reference as an offset: 1200 log2(A / 440) */
static float hp_a, hp_s, lp_a, lp_s1, lp_s2, warm_s, char_amt, rev_send;
static float mix[BLK];

static float frand(void)                          /* -1 .. 1 */
{
    rnd = rnd * 1664525u + 1013904223u;
    return (float)(int32_t)(rnd >> 8) * (1.0f / 8388608.0f) - 1.0f;
}

/* ------------------------------------------------------------- pitch --- */
static void tuning_from_params(void)
{
    int i;
    tun.hon = (int8_t)par[P_HON];
    tun.scale = (uint8_t)par[P_SCALE];
    tun.tuning = (uint8_t)par[P_TUNING];
    tun.depth = (float)par[P_DEPTH] * 0.01f;
    tun.fine = (float)par[P_FINE];
    tun.a4 = (float)par[P_A4];
    tun.octave = (int8_t)par[P_OCTAVE];
    for (i = 0; i < TN_NPC; i++)
        tun.user[i] = (float)par[P_USER0 + i];
    a4_cents = 1200.0f * fm_log2f(tun.a4 * (1.0f / 440.0f));
}

/* the pitch of a key id (white, black in upper-row mode) and the next scale note above it */
static float key_cents(int id, float *next_up)
{
    if (id < FM_NWHITE) {                          /* the keys follow the A reference; absolute notes (MIDI, the bench) do not */
        float c = tuning_white_cents(&tun, id);
        *next_up = (id < FM_NWHITE - 1 ? tuning_white_cents(&tun, id + 1) : c + 100.0f) - c;
        return c + a4_cents;
    }
    if (id < FM_NKEY) {
        float c = tuning_black_cents(&tun, id - FM_NWHITE);
        *next_up = 100.0f;
        return c + a4_cents;
    }
    *next_up = 100.0f;
    return 0.0f;
}

float fm_key_cents(int key)                        /* (reads the render's tuning: a glance, not a contract) */
{
    float nu;
    return key_cents(key, &nu);
}

static void retune(void)                           /* a tuning change: every key voice's target moves (and glides) */
{
    int i;
    for (i = 0; i < FM_NVOICE; i++)
        if (vc[i].live && vc[i].id >= 0 && vc[i].id < FM_NKEY)
            vc[i].base = key_cents(vc[i].id, &vc[i].next_up);
}

static void bend_targets(void)
{
    float up = par[P_BEND_UP] == BEND_T_WHOLE ? 200.0f : 100.0f, t = (float)par[P_BEND_TIME];
    if (par[P_BEND_UP] == BEND_T_SCALE && bend_voice >= 0)
        up = vc[bend_voice].next_up;
    bend_config(&bend, up, -(float)par[P_BEND_DOWN], t, t * 4.0f / 3.0f);
}

/* ------------------------------------------------------------- voices --- */
static void voice_level_for_lights(voice_t *v, float pk)
{
    v->lvl = pk > 1.0f ? 1.0f : pk;
    if (v->id >= 0 && v->id < FM_NKEY)
        fm_key_level[v->id] = v->lvl;
}

static int find_voice(int id)
{
    int i;
    for (i = 0; i < FM_NVOICE; i++)
        if (vc[i].live && vc[i].id == id)
            return i;
    return -1;
}

static int alloc_voice(void)
{
    int i, best = -1;
    uint32_t oldest = 0xFFFFFFFFu;
    for (i = 0; i < FM_NVOICE; i++)
        if (!vc[i].live)
            return i;
    for (i = 0; i < FM_NVOICE; i++) {              /* steal: a released voice first, else the oldest */
        uint32_t o = vc[i].order + (vc[i].down ? 0x80000000u : 0u);
        if (o < oldest) {
            oldest = o;
            best = i;
        }
    }
    return best;
}

/* (re)start voice i on pitch `cents` (from A4) for key id; fresh = from silence */
static void pluck(int i, int id, float cents, float next_up, int fresh)
{
    voice_t *v = &vc[i];
    int vel = orn[ORN_STRONG] ? 127 : 100;
    /* the nearest MIDI note of the pitch: the FM6 core's keyboard level and rate scaling and its detune curve
     * are set from it at note-on, as on the DX7 (the pitch itself comes from the cents, per block) */
    int mn = (int)fm_floorf((cents + 6900.0f) * 0.01f + 0.5f);
    mn = mn < 0 ? 0 : mn > 127 ? 127 : mn;
    if (fresh) {
        v->dc = 0.0f;
        v->g = 0.0f;
    }
    smooth_jump(&v->pitch, cents);                 /* a pluck is a new pitch at once (phase-continuous: no click) */
    v->pitch.k = k_fast;
    v->sliding = 0;
    v->id = id;
    v->base = cents;
    v->next_up = next_up;
    v->yo_g = 1.0f;
    v->yo_m = 1.0f;
    v->down = 1;
    v->live = 1;
    v->vel = (uint8_t)vel;
    v->order = ++order_n;
    fm6_note_init(&v->n, patch, mn, vel, fresh);
    fm6_lfo_key(&plfo);
    bend_voice = i;
    bend_targets();
    if (par[P_MIDI_OUT] && id < FM_NKEY) {
        int m = (int)fm_floorf((cents + 6900.0f) * 0.01f + 0.5f);
        fm_midi_out(0x90, (uint32_t)(m < 0 ? 0 : m > 127 ? 127 : m), (uint32_t)vel);
    }
}

/* voice i slides to key id without a new attack (SLIDE, or 単音 with SLIDE) */
static void slide_to(int i, int id, float cents, float next_up)
{
    voice_t *v = &vc[i];
    v->id = id;
    v->base = cents;
    v->next_up = next_up;
    v->pitch.k = k_slide;
    v->sliding = 1;
    smooth_set(&v->pitch, cents);
    v->order = ++order_n;
    bend_voice = i;
    bend_targets();
}

/* key-up: 余韻 is the release. The FM6 envelope is not told (its own R4 would cut a long 余韻 short): the
 * note keeps decaying as it would under a held key and the 余韻 gain takes it away, 20 ms .. 8 s */
static void release(int i)
{
    voice_t *v = &vc[i];
    if (!v->live || !v->down)
        return;
    v->down = 0;
    v->yo_m = yo_mult;
#ifdef FM_NO_SMOOTH
    if (par[P_YOIN] == 0)
        v->yo_g = 0.0f;
#endif
    if (par[P_MIDI_OUT] && v->id < FM_NKEY) {
        int m = (int)fm_floorf((v->base + 6900.0f) * 0.01f + 0.5f);
        fm_midi_out(0x80, (uint32_t)(m < 0 ? 0 : m > 127 ? 127 : m), 0);
    }
}

static void held_push(int id)
{
    int k;
    for (k = 0; k < nheld; k++)
        if (held[k] == id)
            return;
    if (nheld < 64)
        held[nheld++] = id;
}
static void held_remove(int id)
{
    int k, j = 0;
    for (k = 0; k < nheld; k++)
        if (held[k] != id)
            held[j++] = held[k];
    nheld = j;
}

/* the legato voice (単音, or SLIDE): the one voice that follows the held stack */
static int legato_voice(void)
{
    int i, best = -1;
    uint32_t newest = 0;
    for (i = 0; i < FM_NVOICE; i++)
        if (vc[i].live && vc[i].down && vc[i].order >= newest) {
            newest = vc[i].order;
            best = i;
        }
    return best;
}

static void note_on(int id, float cents, float next_up)
{
    int i = find_voice(id), lv;
    held_push(id);
    if (i >= 0) {                                  /* the same key again: re-excite its own voice */
        pluck(i, id, cents, next_up, 0);
        return;
    }
    lv = legato_voice();
    if (par[P_MONO] && lv >= 0) {                  /* 単音: the one voice moves to the new key */
        if (par[P_SLIDE])
            slide_to(lv, id, cents, next_up);
        else
            pluck(lv, id, cents, next_up, 0);
        return;
    }
    if (par[P_SLIDE] && lv >= 0) {                 /* SLIDE in 和音: the most recent voice slides, the rest ring */
        slide_to(lv, id, cents, next_up);
        return;
    }
    i = alloc_voice();
#ifdef FM_NO_SMOOTH
    pluck(i, id, cents, next_up, 1);
#else
    pluck(i, id, cents, next_up, !vc[i].live);
#endif
}

static void note_off(int id)
{
    int i = find_voice(id), lv;
    held_remove(id);
    if (i < 0)
        return;
    if ((par[P_MONO] || par[P_SLIDE]) && nheld > 0 && vc[i].down) {   /* back to the key still held */
        int back = held[nheld - 1];
        float nu, c = back < FM_NKEY ? key_cents(back, &nu) : vc[i].base;
        lv = i;
        if (par[P_SLIDE])
            slide_to(lv, back, c, nu);
        else
            pluck(lv, back, c, nu, 0);
        return;
    }
    release(i);
}

static void all_off(void)
{
    int i;
    for (i = 0; i < FM_NVOICE; i++)
        release(i);
}

static void quiet(void)
{
    int i;
    for (i = 0; i < FM_NVOICE; i++) {
        vc[i].live = vc[i].down = 0;
        vc[i].id = -1;
        vc[i].yo_g = 0.0f;
    }
    nheld = 0;
    bend_voice = -1;
    bend_init(&bend);
    bend_targets();
    if (par[P_MIDI_OUT])
        fm_midi_out(0xB0, 123, 0);
}

/* ------------------------------------------------------------ settings --- */
static void adopt_patch(void)
{
    uint32_t i;
    for (i = 0; i < FM6_PACKED; i++)
        patch_cur_packed[i] = patch_pending[i];
    fm6_unpack(patch_cur_packed, patch);
    fm6_lfo_reset(&plfo, patch);
    for (i = 0; i < 10u; i++)
        fm_patch_label[i] = (char)patch[FP_NAME + i];
    fm_patch_label[10] = 0;
}

static void load_builtin(int s)
{
    uint32_t i;
    s = s < 0 ? 0 : s >= FM_NPATCH ? FM_NPATCH - 1 : s;
    for (i = 0; i < FM6_PACKED; i++)
        patch_pending[i] = FM_PATCH[s][i];
    adopt_patch();
    fm_patch_custom = 0;
}

static void apply(int p, int v)
{
    const fm_param_t *pi = &PARAMS[p];
    v = v < pi->lo ? pi->lo : v > pi->hi ? pi->hi : v;
    par[p] = (int16_t)v;
    switch (p) {
    case P_HON: case P_SCALE: case P_TUNING: case P_DEPTH: case P_FINE: case P_A4: case P_OCTAVE:
        tuning_from_params();
        retune();
        break;
    case P_VOICE: load_builtin(v); break;
    case P_YOIN: {                                 /* the time to -60 dB after key-up: 20 ms .. 8 s */
        float t = 0.02f * fm_exp2f((float)v * 0.01f * 8.64f);   /* 2^8.64 = 400 */
        yo_mult = fm_expf(-6.9078f / (t * FM_SR));
        break;
    }
    case P_LEVEL: smooth_set(&level_sm, (float)v * 0.01f * (float)v * 0.01f); break;
    case P_VIB_RATE: vib_inc = 0.5f * fm_exp2f((float)v * 0.04f) * (float)BLK / FM_SR; break;   /* 0.5 .. 8 Hz */
    case P_VIB_DEPTH: vib_depth = (float)v * 0.5f; break;                                       /* 0 .. 50 cents */
    case P_TRILL_RATE: trill_period = 0.4f * fm_exp2f(-(float)v * 0.02737f) * FM_SR; break;     /* 400 .. 60 ms */
    case P_TRILL_VAR: trill_var = (float)v * 0.01f * 0.35f; break;
    case P_BEND_UP: case P_BEND_DOWN: case P_BEND_TIME: bend_targets(); break;
    case P_SLIDE_TIME: k_slide = 1.0f - fm_expf(-BLK_MS / (float)v); break;
    case P_LOWCUT: hp_a = 1.0f - fm_expf(-FM_TWO_PI * 20.0f * fm_exp2f((float)v * 0.0432f) / FM_SR); break;   /* 20 .. 400 Hz */
    case P_HIGHCUT:
        lp_a = v >= 100 ? 1.0f : 1.0f - fm_expf(-FM_TWO_PI * 1000.0f * fm_exp2f((float)v * 0.04f) / FM_SR);   /* 1 .. 16 kHz */
        break;
    case P_CHARACTER: char_amt = (float)v * 0.01f; break;
    case P_REVERB: rev_send = (float)v * 0.01f * (float)v * 0.01f; break;
    case P_REV_SIZE: plate_size((float)v * 0.01f); break;
    case P_MIDI_OUT:
        if (!v)
            fm_midi_out(0xB0, 123, 0);
        break;
    default:
        if (p >= P_USER0) {
            tuning_from_params();
            retune();
        }
        break;
    }
}

void fm_init(void)
{
    int i;
    q_w = q_r = 0;
    tuning_defaults(&tun);
    bend_init(&bend);
    plate_init();
    smooth_init(&level_sm, 8.0f, 0.0f, BLK_MS);
    level_cur = 0.0f;
    k_fast = 1.0f - fm_expf(-BLK_MS / 12.0f);
    for (i = 0; i < FM_NVOICE; i++) {
        vc[i].live = vc[i].down = 0;
        vc[i].id = -1;
        vc[i].yo_g = vc[i].lvl = vc[i].g = vc[i].dc = 0.0f;
        smooth_init(&vc[i].pitch, 12.0f, 0.0f, BLK_MS);
        vc[i].sliding = 0;
    }
    for (i = 0; i < FM_NKEY; i++)
        fm_key_level[i] = 0.0f;
    for (i = 0; i < ORN_N; i++)
        orn[i] = 0;
    nheld = 0;
    bend_voice = -1;
    ms_count = 0;
    ms_acc = 0.0f;
    vib_ph = vib_cur = 0.0f;
    trill_left = 0.0f;
    hp_s = lp_s1 = lp_s2 = warm_s = 0.0f;
    fm_peak = 0.0f;
    fm_nvoices = 0;
    for (i = 0; i < P_NPARAMS; i++)
        apply(i, PARAMS[i].def);
    load_builtin(0);
    patch_seen = patch_gen;
}

static void drain(void)
{
    while (q_r != q_w) {
        cmd_t c = q[q_r % QN];
        BARRIER();
        q_r++;
        switch (c.c) {
        case C_SET: apply(c.a, c.b); break;
        case C_KEY:
            if (c.a < FM_NKEY) {
                if (c.b) {
                    float nu, cents = key_cents(c.a, &nu);
                    note_on(c.a, cents, nu);
                } else {
                    note_off(c.a);
                }
            }
            break;
        case C_NOTE:
            if (c.b)
                note_on(c.a, c.f, 100.0f);
            else
                note_off(c.a);
            break;
        case C_ORN:
            if (c.a < ORN_N) {
                orn[c.a] = (uint8_t)(c.b != 0);
                if (c.a == ORN_DAMP && c.b) {      /* damp: every note let go fast, whatever 余韻 says */
                    int i;
                    for (i = 0; i < FM_NVOICE; i++)
                        if (vc[i].live) {
                            release(i);
                            vc[i].yo_m = fm_expf(-6.9078f / (0.03f * FM_SR));
                        }
                }
            }
            break;
        case C_BEND:
            bend_button(&bend, c.a, c.b & 1, (c.b >> 1) & 1, ms_count);
            break;
        case C_PANIC: quiet(); break;
        case C_PATCH:
            if (patch_seen != patch_gen) {
                patch_seen = patch_gen;
                adopt_patch();
                fm_patch_custom = 1;
            }
            break;
        default: break;
        }
    }
}

/* -------------------------------------------------------------- render --- */
static void controls_block(void)
{
    int i, vib_on = par[P_VIB_ON] || orn[ORN_VIB], trill_on = par[P_TRILL_ON] || orn[ORN_TRILL];
    float b;
    ms_acc += BLK_MS;
    while (ms_acc >= 1.0f) {
        ms_acc -= 1.0f;
        ms_count++;
    }
    b = bend_value(&bend, ms_count);
    fm_bend_cents = b;
    /* vibrato: a sine, its depth easing in and out over about 100 ms */
    vib_ph += vib_inc;
    if (vib_ph >= 1.0f)
        vib_ph -= 1.0f;
    vib_cur += ((vib_on ? vib_depth : 0.0f) - vib_cur) * 0.03f;
    /* trill: the most recent held key re-plucked at the rate, each repeat a little early or late */
    if (trill_on && nheld > 0) {
        trill_left -= (float)BLK;
        if (trill_left <= 0.0f) {
            int id = held[nheld - 1], v = find_voice(id);
            if (v >= 0)
                pluck(v, id, vc[v].base, vc[v].next_up, 0);
            trill_left = trill_period * (1.0f + trill_var * frand());
        }
    } else {
        trill_left = 0.0f;
    }
    lfo_v = fm6_lfo_sample(&plfo);
    lfo_d = fm6_lfo_delay(&plfo);
    for (i = 0; i < FM_NVOICE; i++) {
        voice_t *v = &vc[i];
        if (!v->live)
            continue;
        smooth_set(&v->pitch, v->base + (i == bend_voice ? b : 0.0f));
    }
}

static void render_voices(void)
{
    int i, k, nv = 0;
    float vib = vib_cur * fm_sin_turns(vib_ph);
    for (i = 0; i < FM_NVOICE; i++) {
        voice_t *v = &vc[i];
        int32_t bus[BLK];
        float cents, g0, g1, dg, g, pk = 0.0f;
        if (!v->live)
            continue;
        cents = smooth_step(&v->pitch);
        if (v->sliding && cents == v->pitch.target) {   /* arrived: bends and tuning changes glide fast again */
            v->sliding = 0;
            v->pitch.k = k_fast;
        }
        cents += vib;
        if (!fm6_note_compute(&v->n, patch, bus, lfo_v, lfo_d, fm6_logfreq_cents(cents + 6900.0f), patch[FP_ALG] & 31u,
                              patch[FP_FB] & 7u, DT0, 0) || (!v->down && v->yo_g < 1e-4f)) {
            if (fm6_note_done(&v->n, patch, patch[FP_ALG] & 31u) || (!v->down && v->yo_g < 1e-4f)) {
                v->live = 0;
                v->g = 0.0f;
                voice_level_for_lights(v, 0.0f);
                if (bend_voice == i)
                    bend_voice = -1;
                continue;
            }
            v->g = 0.0f;
            voice_level_for_lights(v, 0.0f);
            nv++;
            continue;
        }
        nv++;
        /* the gain over the block: 余韻 after key-up, ramped from the last block's value */
        g0 = v->g;
        if (!v->down) {
            float m = v->yo_m, mb = m;
            for (k = 1; k < 5; k++)
                mb *= mb;                         /* m^16 */
            mb *= mb;                             /* m^32 = the block's multiplier */
            v->yo_g *= mb;
            (void)m;
        }
        g1 = 0.18f * v->yo_g;                    /* a full carrier = 0.36; eight koto attacks at once reach the soft clip gently */
        dg = (g1 - g0) * (1.0f / (float)BLK);
#ifdef FM_NO_SMOOTH
        g0 = g1;
        dg = 0.0f;
#endif
        g = g0;
        for (k = 0; k < BLK; k++) {
            float x = (float)bus[k] * (1.0f / 16777216.0f), y = x - v->dc;
            v->dc += y * (1.0f / 1024.0f);         /* ~7 Hz: FM leaves sidebands at 0 Hz */
            g += dg;
            y *= g;
            mix[k] += y;
            if (fm_fabsf(y) > pk)
                pk = fm_fabsf(y);
        }
        v->g = g1;
        voice_level_for_lights(v, pk * 2.0f);
    }
    fm_nvoices = (uint8_t)nv;
}

static void render_block(int32_t *out, float gain)
{
    int k;
    float lv0 = level_cur, lv1 = smooth_step(&level_sm), dlv = (lv1 - lv0) * (1.0f / (float)BLK), lv = lv0, pk = 0.0f;
#ifdef FM_NO_SMOOTH
    lv = lv1;
    dlv = 0.0f;
#endif
    for (k = 0; k < BLK; k++)
        mix[k] = 0.0f;
    controls_block();
    render_voices();
    level_cur = lv1;
    for (k = 0; k < BLK; k++) {
        float m = mix[k], wl, wr, l, r;
        hp_s += hp_a * (m - hp_s);                 /* low cut */
        m -= hp_s;
        lp_s1 += lp_a * (m - lp_s1);               /* high cut, two poles */
        lp_s2 += lp_a * (lp_s1 - lp_s2);
        m = lp_s2;
        if (char_amt > 0.0f) {                     /* character: warmth (a tilt toward the low mids) and a soft hand */
            warm_s += 0.18f * (m - warm_s);
            m += char_amt * 0.6f * (warm_s - m);
            m = m + char_amt * (fm_tanhf(1.6f * m) * 0.625f - m);
        }
        reverb(m * rev_send * 0.5f, &wl, &wr);
        lv += dlv;
        l = (m + wl) * lv * gain;
        r = (m + wr) * lv * gain;
        if (fm_fabsf(l) > pk)
            pk = fm_fabsf(l);
        out[2 * k] = (int32_t)(fm_tanhf(l) * 8300000.0f);
        out[2 * k + 1] = (int32_t)(fm_tanhf(r) * 8300000.0f);
    }
    fm_peak = pk;                                  /* before the soft clip: above 1 means it is working */
}

void fm_render(int32_t *out, uint32_t n, uint32_t gain_q12)
{
    const float gain = (float)gain_q12 * (1.0f / 4096.0f) * 2.0f;
    drain();
    while (n >= BLK) {
        render_block(out, gain);
        out += 2 * BLK;
        n -= BLK;
    }
}
