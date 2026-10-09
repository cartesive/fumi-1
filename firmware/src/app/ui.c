/* SPDX-License-Identifier: GPL-3.0-only */
/* FuMi-1 UI: the panel and the screen. Part of the unity build (after gfx.c and project.c); the host
 * simulator includes it too. The control map (docs/research/00-START-HERE.md):
 *
 *   white keys     mi fa la ti do x 3 and a top mi (the ST-50's lower row)
 *   black keys     the upper row fa# sol ti-flat do# re, or ornaments (SEQ switches)
 *   SELECT 本数    PRESETS 音色 (voice)    ALGORITHM scale IN / YO / MIN'YO
 *   KNOB 1-4       the page's four values; HOME: 余韻, trill rate, vibrato depth, reverb
 *   LFO ビブラート   ARP トリラー   GLO 単音/和音   SEL 調律 (tap: 平均律 / 純正律; hold: the tuning page)
 *   OCT- / OCT+    a note held: pitch bend down / up (sprung); none held: octave down / up
 *   HOME FX EDIT   the pages: HOME, SOUND, BEND.  SAVE saves (and it autosaves when quiet)
 *   OCT- + OCT+ held 5 s: update mode (main_fm1.c; never reassigned) */

enum { V_HOME, V_SOUND, V_BEND, V_TUNING, NVIEWS };
static const char *const VIEW_NAME[NVIEWS] = {"Home", "Sound", "Bend", "Tuning"};
#define K_NONE 255
static const uint8_t VIEW_KNOB[NVIEWS][4] = {
    {P_YOIN, P_TRILL_RATE, P_VIB_DEPTH, P_REVERB},
    {P_LOWCUT, P_HIGHCUT, P_CHARACTER, P_REV_SIZE},
    {P_BEND_UP, P_BEND_DOWN, P_BEND_TIME, P_SLIDE_TIME},
    {P_TUNING, P_DEPTH, P_FINE, P_A4},
};

static const uint8_t WHITE_K[FM_NWHITE] = {0, 2, 4, 6, 7, 9, 11, 12, 14, 16, 18, 19, 21, 23, 24, 26};
static const uint8_t BLACK_K[FM_NBLACK] = {1, 3, 5, 8, 10, 13, 15, 17, 20, 22, 25};
/* ornament mode: what each black key does (ORN_N = nothing yet) */
static const uint8_t BLACK_ORN[FM_NBLACK] = {ORN_VIB, ORN_TRILL, ORN_DAMP, ORN_STRONG, ORN_N,
                                             ORN_VIB, ORN_TRILL, ORN_DAMP, ORN_STRONG, ORN_N, ORN_N};
static const char *const DEG_NAME[5] = {"mi", "fa", "la", "ti", "do"};
static const char *const NOTE_NAME[12] = {"C", "C#", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B"};

/* the screen's colours: paper, ink, and one warm colour for what sounds */
#define K_BG RGB(250, 246, 236)
#define K_PANEL RGB(236, 229, 214)
#define K_LINE RGB(214, 204, 186)
#define K_DIM RGB(150, 138, 120)
#define K_TEXT RGB(64, 52, 44)
#define K_WHITE RGB(255, 255, 255)
#define K_KOTO RGB(196, 72, 48)
#define K_KOTO_T RGB(240, 200, 184)
#define K_TONIC RGB(120, 100, 80)

static struct {
    uint8_t view;
    uint32_t btn, keys, btn_used;
    uint32_t enc_t[NE];
    uint32_t sel_t;                    /* SEL pressed at (its hold opens the tuning page) */
    uint8_t oct_role[2];               /* OCT- / OCT+ while held: 0 up, 1 bend, 2 octave */
    uint8_t tuning_prev;               /* the 純正律 to come back to after a tap to 平均律 */
    uint8_t dirty;
    uint32_t act_t, saved_t;
    char msg[2][24];
    uint32_t msg_until;
    int8_t touched;
    uint32_t touch_until;
    uint32_t frame;
    uint32_t sig[3];
} ui;

static void say(const char *a, const char *b)
{
    uint32_t i;
    for (i = 0; i < 23u && a && a[i]; i++)
        ui.msg[0][i] = a[i];
    ui.msg[0][i] = 0;
    for (i = 0; i < 23u && b && b[i]; i++)
        ui.msg[1][i] = b[i];
    ui.msg[1][i] = 0;
    ui.msg_until = plat_ms() + 1300u;
}
void ui_say(const char *a, const char *b) { say(a, b); }
int ui_dirty(void) { return ui.dirty != 0; }
static void mark_dirty(void) { ui.dirty = 1; }

static void itoa_u(uint32_t v, char *b)
{
    char t[12];
    int n = 0;
    do {
        t[n++] = (char)('0' + v % 10u);
        v /= 10u;
    } while (v);
    while (n)
        *b++ = t[--n];
    *b = 0;
}

/* ------------------------------------------------------------- values --- */
static void knob_set(int p, int v)
{
    const fm_param_t *pi = fm_param_info(p);
    v = v < pi->lo ? pi->lo : v > pi->hi ? pi->hi : v;
    if (v == proj.par[p])
        return;
    proj.par[p] = (int16_t)v;
    fm_set(p, v);
    mark_dirty();
}

/* knob acceleration by speed: a slow turn is one step a detent, a quick one up to 6 */
static uint8_t accel_off;              /* the host simulator's "spin": exact steps */
static int32_t accel(int role, int32_t s, int range)
{
    uint32_t now = plat_ms(), dt = now - ui.enc_t[role], a = (uint32_t)(s < 0 ? -s : s), m;
    ui.enc_t[role] = now;
    if (range <= 24 || !a || accel_off)
        return s;
    if (a > 1)
        dt /= a;
    m = dt < 15u ? 6u : dt < 30u ? 4u : dt < 60u ? 2u : 1u;
    return s * (int32_t)m;
}

static void turn(int role, int p, int32_t e)
{
    const fm_param_t *pi;
    if (!e || p == K_NONE)
        return;
    pi = fm_param_info(p);
    knob_set(p, proj.par[p] + accel(role, e, pi->hi - pi->lo));
}

static void toggle(int p, const char *what)
{
    knob_set(p, !proj.par[p]);
    say(what, proj.par[p] ? "ON" : "OFF");
}

/* ------------------------------------------------------------ buttons --- */
static void set_view(int v)
{
    ui.view = (uint8_t)v;
    ui.touched = -1;
}

static int any_key_held(void) { return (ui.keys & ((1u << NKEYS) - 1u)) != 0; }

static void oct_press(int which)                   /* which: 0 OCT-, 1 OCT+ (bend.h BEND_DOWN / BEND_UP) */
{
    if (ui.oct_role[!which]) {                     /* the other is held: reserved (update mode, main_fm1.c) */
        ui.oct_role[which] = 2;
        fm_bend(which, 1, 0);                      /* the engine lets its bend go */
        return;
    }
    if (any_key_held()) {
        ui.oct_role[which] = 1;
        fm_bend(which, 1, 1);
        return;
    }
    ui.oct_role[which] = 2;
    knob_set(P_OCTAVE, proj.par[P_OCTAVE] + (which ? 1 : -1));
    {
        char t[12];
        fm_param_text(P_OCTAVE, proj.par[P_OCTAVE], t);
        say("OCTAVE", t);
    }
}

static void oct_release(int which)
{
    if (ui.oct_role[which] == 1)
        fm_bend(which, 0, any_key_held());
    ui.oct_role[which] = 0;
}

/* The three selectors move one step at a time, at most one every SEL_STEP_MS: on the FM-1 a single click of
 * PRESETS sometimes arrived as two counts and skipped an instrument (1.0.2). A person clicks a detented
 * encoder at well under 16 a second, so nothing deliberate is lost; a count inside the window is dropped. */
#define SEL_STEP_MS 60u
static int32_t sel_step(int role)
{
    static uint32_t next[NE];                      /* when the next step may be taken (0: at once) */
    int32_t e = plat_enc(role);
    uint32_t now = plat_ms();
    if (e == 0)
        return 0;
    if ((int32_t)(now - next[role]) < 0)
        return 0;
    next[role] = now + SEL_STEP_MS;
    return e > 0 ? 1 : -1;
}

static void sel_tap(void)                          /* 調律: 平均律 <-> 純正律 (whichever was on) */
{
    if (proj.par[P_TUNING] == TN_EQUAL) {
        knob_set(P_TUNING, ui.tuning_prev ? ui.tuning_prev : TN_SUIKO);
    } else {
        ui.tuning_prev = (uint8_t)proj.par[P_TUNING];
        knob_set(P_TUNING, TN_EQUAL);
    }
    say("TUNING", TN_NAME[proj.par[P_TUNING]]);
}

static void button(int b)                          /* on release, unless the press was used */
{
    switch (b) {
    case B_HOME: set_view(V_HOME); break;
    case B_FX: set_view(V_SOUND); break;
    case B_EDIT: set_view(V_BEND); break;
    case B_SEL: sel_tap(); break;
    case B_LFO: toggle(P_VIB_ON, "VIBRATO"); break;
    case B_ARP: toggle(P_TRILL_ON, "TRILL"); break;
    case B_GLO:
        knob_set(P_MONO, !proj.par[P_MONO]);
        say(proj.par[P_MONO] ? "MONO" : "POLY", proj.par[P_MONO] ? "(TANON)" : "(WAON)");
        break;
    case B_SEQ: {
        int i;
        knob_set(P_BLACK, !proj.par[P_BLACK]);
        for (i = 0; i < ORN_N; i++)                /* nothing stays held across the switch */
            fm_ornament(i, 0);
        for (i = 0; i < FM_NBLACK; i++)
            fm_key(FM_NWHITE + i, 0);
        say("BLACK KEYS", proj.par[P_BLACK] ? "ORNAMENTS" : "UPPER ROW");
        break;
    }
    case B_SAVE:
        if (project_save() == 0) {
            ui.dirty = 0;
            say("SAVED", 0);
        } else {
            say("NOT SAVED", "FLASH ERROR");
        }
        break;
    default: break;
    }
}

/* MIDI in (USB and the TRS jack): notes play the voice, chromatic (M5 adds a scale mode) */
static void midi_in(void)
{
    uint32_t pkt;
    while (plat_midi_in(&pkt)) {
        uint32_t st = (pkt >> 8) & 0xF0u, n = (pkt >> 16) & 0x7Fu, v = pkt >> 24;
        if (st == 0x90u && v)
            fm_note(FM_NOTE_ID + (int)n, 1, 100.0f * ((float)n - 69.0f));
        else if (st == 0x80u || (st == 0x90u && !v))
            fm_note(FM_NOTE_ID + (int)n, 0, 0.0f);
        else if (st == 0xB0u && (n == 123u || n == 120u))
            fm_panic();
    }
}

static void black_key(int b, int on)
{
    if (proj.par[P_BLACK]) {
        if (BLACK_ORN[b] < ORN_N)
            fm_ornament(BLACK_ORN[b], on);
    } else {
        fm_key(FM_NWHITE + b, on);
    }
}

static void input(void)
{
    uint32_t btn = plat_buttons(), keys = plat_keys(), ch, i;
    int32_t e;
    ch = keys ^ ui.keys;
    ui.keys = keys;
    for (i = 0; i < FM_NWHITE; i++)                /* keys first: a glissando is the fastest thing here */
        if (ch >> WHITE_K[i] & 1u)
            fm_key((int)i, (int)(keys >> WHITE_K[i] & 1u));
    for (i = 0; i < FM_NBLACK; i++)
        if (ch >> BLACK_K[i] & 1u)
            black_key((int)i, (int)(keys >> BLACK_K[i] & 1u));
    ch = btn ^ ui.btn;
    ui.btn = btn;
    for (i = 0; i < NB; i++) {
        uint32_t m = 1u << i;
        if (!(ch & m))
            continue;
        if (btn & m) {
            ui.btn_used &= ~m;
            if (i == B_OCTDN || i == B_OCTUP)
                oct_press(i == B_OCTUP);
            else if (i == B_SEL)
                ui.sel_t = plat_ms();
        } else if (i == B_OCTDN || i == B_OCTUP) {
            oct_release(i == B_OCTUP);
        } else if (!(ui.btn_used & m)) {
            button((int)i);
        }
    }
    if ((btn >> B_SEL & 1u) && !(ui.btn_used >> B_SEL & 1u) && plat_ms() - ui.sel_t > 400u) {
        ui.btn_used |= 1u << B_SEL;                /* SEL held: the tuning page, no toggle on release */
        set_view(V_TUNING);
    }
    if (btn || keys || ch)
        ui.act_t = plat_ms();
    midi_in();
    if ((e = sel_step(EN_SELECT)) != 0)            /* a detent is a step: no acceleration on these three */
        knob_set(P_HON, proj.par[P_HON] + e);
    if ((e = sel_step(EN_PRESET)) != 0)          /* PRESETS picks the instrument, as players expect */
        knob_set(P_VOICE, proj.par[P_VOICE] + e);
    if ((e = sel_step(EN_ALGO)) != 0)
        knob_set(P_SCALE, proj.par[P_SCALE] + e);
    for (i = 0; i < 4; i++)
        if ((e = plat_enc(EN_K1 + (int)i)) != 0) {
            turn(EN_K1 + (int)i, VIEW_KNOB[ui.view][i], e);
            ui.touched = (int8_t)i;
            ui.touch_until = plat_ms() + 1200u;
            ui.act_t = plat_ms();
        }
}

/* ------------------------------------------------------------- screen --- */
#define HDR_H 28
#define MAIN_Y 28
#define MAIN_H 144
#define KNB_Y 172
#define KNB_H 68

static uint32_t hash(uint32_t h, uint32_t v) { return (h ^ v) * 16777619u; }

static void cv_round(int32_t x, int32_t y, int32_t w, int32_t h, int32_t r, uint16_t c)
{
    static const uint8_t IN[7][6] = {{0}, {1}, {1, 0}, {2, 1, 0}, {2, 1, 0, 0}, {3, 2, 1, 0, 0}, {4, 2, 1, 1, 0, 0}};
    int32_t j;
    if (r > 6)
        r = 6;
    if (r > h / 2)
        r = h / 2;
    cv_rect(x, y + r, w, h - 2 * r, c);
    for (j = 0; j < r; j++) {
        int32_t d = IN[r][j];
        cv_rect(x + d, y + j, w - 2 * d, 1, c);
        cv_rect(x + d, y + h - 1 - j, w - 2 * d, 1, c);
    }
}

static void text_c(int32_t cx, int32_t y, const felucca_font_t *f, const char *s, uint16_t c)
{
    cv_text(cx - text_w(f, s) / 2, y, f, s, c);
}

static uint16_t mix(uint16_t a, uint16_t b, int t)   /* t 0..16: a -> b */
{
    int r = ((a >> 11) * (16 - t) + (b >> 11) * t) / 16, g = (((a >> 5) & 63) * (16 - t) + ((b >> 5) & 63) * t) / 16,
        bl = ((a & 31) * (16 - t) + (b & 31) * t) / 16;
    return (uint16_t)(r << 11 | g << 5 | bl);
}

static void draw_header(void)
{
    int msg = plat_ms() < ui.msg_until;
    uint32_t h = hash(hash(2166136261u, ui.view), (uint32_t)proj.par[P_VIB_ON] | (uint32_t)proj.par[P_TRILL_ON] << 1 |
                                                   (uint32_t)proj.par[P_MONO] << 2 | (uint32_t)proj.par[P_BLACK] << 3 |
                                                   (uint32_t)ui.dirty << 4 | (uint32_t)msg << 5);
    if (msg)
        h = hash(hash(h, (uint32_t)ui.msg[0][0] << 8 | ui.msg[0][1]), ui.msg_until);
    if (h == ui.sig[0])
        return;
    ui.sig[0] = h;
    cv_begin(240, HDR_H, K_BG);
    if (msg) {
        int32_t x = cv_text(10, 6, &FONT_B, ui.msg[0], K_TEXT);
        cv_text(x + 8, 6, &FONT_B, ui.msg[1], K_KOTO);
    } else {
        static const char *const TAG[4] = {"VIB", "TRILL", "MONO", "ORN"};
        int on[4] = {proj.par[P_VIB_ON], proj.par[P_TRILL_ON], proj.par[P_MONO], proj.par[P_BLACK]}, i;
        int32_t x = 10;
        cv_text(x, 6, &FONT_B, ui.view == V_HOME ? "FuMi-1" : VIEW_NAME[ui.view], K_TEXT);
        x = 232;
        for (i = 3; i >= 0; i--) {
            int32_t w = text_w(&FONT_XS, TAG[i]) + 10;
            if (!on[i])
                continue;
            x -= w + 4;
            cv_round(x, 6, w, 16, 6, K_KOTO_T);
            cv_text(x + 5, 7, &FONT_XS, TAG[i], K_KOTO);
        }
        if (ui.dirty)
            cv_round(x - 10, 12, 5, 5, 2, K_DIM);
    }
    cv_blit(0, 0);
}

static void draw_keys(int32_t y0, int32_t h)
{
    int k;
    for (k = 0; k < FM_NWHITE; k++) {
        float lv = fm_key_level[k];
        int t = (int)(lv * 16.0f + 0.5f), x = 6 + k * 14, tonic = (k % 5) == 0;
        uint16_t c = mix(tonic ? K_LINE : K_PANEL, K_KOTO, t > 16 ? 16 : t);
        cv_round(x, y0, 12, h, 4, c);
        if (tonic)
            cv_round(x + 4, y0 + h - 7, 4, 4, 2, t > 2 ? K_WHITE : K_TONIC);
    }
}

static void draw_main(void)
{
    char t[16];
    uint32_t h = hash(2166136261u, ui.view), s;
    int i;
    for (s = 0; s < FM_NWHITE; s++)
        h = hash(h, (uint32_t)(fm_key_level[s] * 24.0f));
    h = hash(hash(h, (uint32_t)proj.par[P_HON] | (uint32_t)proj.par[P_VOICE] << 8 | (uint32_t)proj.par[P_TUNING] << 16 |
                          (uint32_t)proj.par[P_SCALE] << 20 | (uint32_t)(proj.par[P_OCTAVE] + 1) << 24),
             (uint32_t)(fm_bend_cents * 4.0f + 1000.0f) | (uint32_t)fm_patch_custom << 16 | (uint32_t)fm_patch_label[0] << 20);
    if (h == ui.sig[1])
        return;
    ui.sig[1] = h;
    cv_begin(240, MAIN_H, K_BG);
    {   /* 本数, big, with the note 三 sounds; the voice; the tuning and the scale */
        int san = 53 + proj.par[P_HON] + 12 * proj.par[P_OCTAVE];
        int32_t x = 12;
        x = cv_text(x, 2, &FONT_L, TN_HON_ASCII[proj.par[P_HON]], K_TEXT);
        cv_text(x + 4, 20, &FONT_XS, "HON", K_DIM);
        t[0] = 0;
        {
            const char *n = NOTE_NAME[((san % 12) + 12) % 12];
            int k = 0;
            while (*n)
                t[k++] = *n++;
            t[k++] = (char)('0' + (san / 12 - 1));
            t[k] = 0;
        }
        cv_text(x + 4, 4, &FONT_S, t, K_DIM);
        if (fm_bend_cents != 0.0f) {           /* the bend: a small dial beside the key number */
            int32_t cx = 110, bw = (int32_t)(fm_bend_cents * 0.16f);
            cv_round(cx - 20, 14, 40, 6, 3, K_LINE);
            cv_round(bw < 0 ? cx + bw : cx, 14, (bw < 0 ? -bw : bw) + 4, 6, 3, K_KOTO);
        }
        cv_text(240 - 12 - text_w(&FONT_B, (const char *)fm_patch_label), 4, &FONT_B, (const char *)fm_patch_label,
                fm_patch_custom ? K_KOTO : K_TEXT);
        if (!fm_patch_custom) {                 /* the instrument's Japanese name beneath, Suiko-style */
            const char *ja = FM_PATCH_JA_LCD[proj.par[P_VOICE]];
            cv_text(240 - 12 - text_w(&FONT_JP, ja), 21, &FONT_JP, ja, K_TEXT);
        }
        {
            char line[40];
            int k = 0;
            const char *a = TN_NAME[proj.par[P_TUNING]], *b = SC_NAME[proj.par[P_SCALE]];
            while (*a)
                line[k++] = *a++;
            line[k++] = ' ';
            line[k++] = '/';
            line[k++] = ' ';
            while (*b)
                line[k++] = *b++;
            line[k] = 0;
            cv_text(240 - 12 - text_w(&FONT_XS, line), 39, &FONT_XS, line, K_DIM);
        }
    }
    draw_keys(52, 56);
    for (i = 0; i < FM_NWHITE; i++)              /* the degree under each key, the tonics' row marked */
        text_c(12 + i * 14, 112, &FONT_XS, DEG_NAME[i % 5], (i % 5) ? K_DIM : K_TEXT);
    cv_blit(0, MAIN_Y);
}

static void draw_knobs(void)
{
    uint32_t h = hash(2166136261u, ui.view | (uint32_t)ui.touched << 8), i;
    int now_touch = plat_ms() < ui.touch_until;
    for (i = 0; i < 4; i++)
        h = hash(h, (uint32_t)proj.par[VIEW_KNOB[ui.view][i]]);
    h = hash(h, (uint32_t)now_touch);
    if (h == ui.sig[2])
        return;
    ui.sig[2] = h;
    cv_begin(240, KNB_H, K_BG);
    cv_round(4, 2, 232, KNB_H - 6, 6, K_PANEL);
    for (i = 0; i < 4; i++) {
        int p = VIEW_KNOB[ui.view][i], lo, hi, v;
        const fm_param_t *pi;
        int32_t x = (int32_t)i * 60, cx = x + 30, bw;
        int hot = now_touch && ui.touched == (int)i;
        char t[16];
        if (p == K_NONE)
            continue;
        pi = fm_param_info(p);
        lo = pi->lo;
        hi = pi->hi;
        v = proj.par[p];
        if (i)
            cv_rect(x, 14, 1, KNB_H - 30, K_LINE);
        text_c(cx, 6, &FONT_XS, pi->name, hot ? K_TEXT : K_DIM);
        fm_param_text(p, v, t);
        {
            const char *q = t;
            int lower = 0;
            for (; *q; q++)
                lower |= *q >= 'a' && *q <= 'z';
            if (lower || text_w(&FONT_M, t) > 56)
                text_c(cx, 26, &FONT_B, t, hot ? K_KOTO : K_TEXT);
            else
                text_c(cx, 22, &FONT_M, t, hot ? K_KOTO : K_TEXT);
        }
        bw = hi > lo ? (int32_t)(44 * (v - lo) / (hi - lo)) : 0;
        cv_round(cx - 22, 50, 44, 6, 3, K_LINE);
        if (lo < 0) {
            int32_t m = cx - 22 + 44 * (0 - lo) / (hi - lo), e = cx - 22 + bw;
            cv_round(e < m ? e : m, 50, (e < m ? m - e : e - m) + 6, 6, 3, hot ? K_KOTO : K_TONIC);
        } else if (bw) {
            cv_round(cx - 22, 50, bw < 6 ? 6 : bw, 6, 3, hot ? K_KOTO : K_TONIC);
        }
    }
    cv_blit(0, KNB_Y);
}

static void leds(void)
{
    uint32_t b = 0, k = 0, i;
    static const uint8_t VIEW_BTN[NVIEWS] = {B_HOME, B_FX, B_EDIT, B_SEL};
    b |= 1u << VIEW_BTN[ui.view];
    if (proj.par[P_VIB_ON])
        b |= 1u << B_LFO;
    if (proj.par[P_TRILL_ON])
        b |= 1u << B_ARP;
    if (proj.par[P_MONO])
        b |= 1u << B_GLO;
    if (proj.par[P_BLACK])
        b |= 1u << B_SEQ;
    if (proj.leds) {
        for (i = 0; i < FM_NWHITE; i++)
            if (fm_key_level[i] > 0.15f || (i % 5) == 0)   /* the tonics: landmarks, always lit */
                k |= 1u << WHITE_K[i];
        for (i = 0; i < FM_NBLACK; i++)
            if (fm_key_level[FM_NWHITE + i] > 0.15f)
                k |= 1u << BLACK_K[i];
    }
    plat_glow(proj.leds == 1);
    plat_leds(b, k);
}

/* AUTOSAVE: a few seconds after a change, when nothing sounds and nothing is touched (a flash erase
 * silences the audio for a moment) */
#define AUTOSAVE_QUIET 4000u
static void autosave(void)
{
    uint32_t now = plat_ms();
    if (!ui.dirty || ui.btn || ui.keys || now - ui.act_t < AUTOSAVE_QUIET || fm_nvoices)
        return;
    if (project_save() == 0) {
        ui.dirty = 0;
        ui.saved_t = now;
    }
}

void ui_init(void)
{
    memset(&ui, 0, sizeof ui);
    ui.touched = -1;
    ui.tuning_prev = TN_SUIKO;
    lcd_fill(0, 0, 240, 240, K_BG);
}

void ui_frame(void)
{
    input();
    autosave();
    draw_header();
    draw_main();
    draw_knobs();
    leds();
    ui.frame++;
}

void ui_input_only(void) { input(); }
