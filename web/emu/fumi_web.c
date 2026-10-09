/* SPDX-License-Identifier: GPL-3.0-only */
/* FuMi-1 in the browser: the host simulator (host/fumi_host.c: the whole app, UI and instrument,
 * against a simulated FM-1) compiled to WebAssembly and run in an AudioWorklet (FoMni's, from X0X's).
 * The audition bench (web/bench) uses the same build through the bench exports at the end. The worklet asks
 * for audio; producing it advances the device's clock one millisecond at a time, which runs the
 * engine every 256 samples (the I2S half buffer) and the UI every 16 ms, as on the FM-1. The page
 * sends input, and reads the screen, the lights and the saved objects back through these exports.
 *
 *   web/emu/build.sh   ->  build/emu/fumi.wasm (+ the page) */
#include <stdint.h>

static void web_audio(const int32_t *blk, uint32_t n);
#include "../../host/fumi_host.c"

#undef __attribute__

/* output ring, stereo float, filled by run_ms, drained by web_render */
#define RING 4096u
static float ring[RING * 2];
static uint32_t ring_w, ring_r;
static void web_audio(const int32_t *blk, uint32_t n)
{
    uint32_t k;
    for (k = 0; k < n; k++) {
        uint32_t i = (ring_w++ % RING) * 2u;
        ring[i] = (float)blk[2 * k] / 8388608.0f;
        ring[i + 1] = (float)blk[2 * k + 1] / 8388608.0f;
    }
}

static float out_l[1024], out_r[1024];
__attribute__((used, visibility("default"))) float *web_out_l(void) { return out_l; }
__attribute__((used, visibility("default"))) float *web_out_r(void) { return out_r; }

/* n frames (<= 1024) into out_l / out_r: the device runs until they exist */
__attribute__((used, visibility("default"))) void web_render(uint32_t n)
{
    uint32_t k;
    if (n > 1024u)
        n = 1024u;
    while (ring_w - ring_r < n)
        run_ms(1);
    for (k = 0; k < n; k++) {
        uint32_t i = (ring_r++ % RING) * 2u;
        out_l[k] = ring[i];
        out_r[k] = ring[i + 1];
    }
}

__attribute__((used, visibility("default"))) void web_boot(void) { boot(); }
__attribute__((used, visibility("default"))) void web_buttons(uint32_t m) { held_btn = m; }
__attribute__((used, visibility("default"))) void web_keys(uint32_t m) { held_keys = m; }
__attribute__((used, visibility("default"))) void web_enc(int role, int32_t n)
{
    if (role >= 0 && role < NE)
        enc_acc[role] += n;
}
__attribute__((used, visibility("default"))) void web_master(uint32_t v) { master = v > 4096u ? 4096u : v; }
__attribute__((used, visibility("default"))) uint32_t web_lit_buttons(void) { return lit_btn; }
__attribute__((used, visibility("default"))) uint32_t web_lit_keys(void) { return lit_keys; }
__attribute__((used, visibility("default"))) uint16_t *web_fb(void) { return fb; }
__attribute__((used, visibility("default"))) uint32_t web_blits(void) { return blits; }
__attribute__((used, visibility("default"))) uint32_t web_now(void) { return now_ms; }
__attribute__((used, visibility("default"))) uint32_t web_cpu(void) { return cpu_pct; }

/* saved objects: the page keeps them (localStorage) and gives them back before boot */
__attribute__((used, visibility("default"))) uint8_t *web_store(uint32_t obj) { return obj < OBJ_NOBJ ? store[obj] : 0; }
__attribute__((used, visibility("default"))) int web_store_len(uint32_t obj) { return obj < OBJ_NOBJ ? store_len[obj] : -1; }
__attribute__((used, visibility("default"))) void web_store_set_len(uint32_t obj, int n)
{
    if (obj < OBJ_NOBJ)
        store_len[obj] = n < 0 ? -1 : n > (int)PLAT_STORE_MAX ? (int)PLAT_STORE_MAX : n;
}
__attribute__((used, visibility("default"))) uint32_t web_store_writes(void) { return store_writes; }
__attribute__((used, visibility("default"))) uint32_t web_nobj(void) { return OBJ_NOBJ; }
__attribute__((used, visibility("default"))) uint32_t web_store_max(void) { return PLAT_STORE_MAX; }

/* ---- the audition bench (web/bench): patches, parameters and notes straight into the engine */
static uint8_t web_patch_buf[128];
__attribute__((used, visibility("default"))) uint8_t *web_patch(void) { return web_patch_buf; }
__attribute__((used, visibility("default"))) void web_patch_set(void) { fm_patch_set(web_patch_buf); }
__attribute__((used, visibility("default"))) void web_patch_get(void) { fm_patch_get(web_patch_buf); }
__attribute__((used, visibility("default"))) void web_param(int p, int v) { if (p >= 0 && p < P_NPARAMS) knob_set(p, v); }
__attribute__((used, visibility("default"))) int web_param_get(int p) { return p >= 0 && p < P_NPARAMS ? proj.par[p] : 0; }
__attribute__((used, visibility("default"))) int web_param_lo(int p) { return fm_param_info(p)->lo; }
__attribute__((used, visibility("default"))) int web_param_hi(int p) { return fm_param_info(p)->hi; }
__attribute__((used, visibility("default"))) const char *web_param_name(int p) { return fm_param_info(p)->name; }
__attribute__((used, visibility("default"))) void web_note(int id, int on, float cents) { fm_note(FM_NOTE_ID + id, on, cents); }
__attribute__((used, visibility("default"))) void web_key(int key, int on) { fm_key(key, on); }
__attribute__((used, visibility("default"))) void web_ornament(int o, int on) { fm_ornament(o, on); }
__attribute__((used, visibility("default"))) void web_bend(int which, int down, int held) { fm_bend(which, down, held); }
__attribute__((used, visibility("default"))) void web_panic(void) { fm_panic(); }
__attribute__((used, visibility("default"))) float web_peak(void) { return fm_peak; }
__attribute__((used, visibility("default"))) int web_nvoices(void) { return fm_nvoices; }
__attribute__((used, visibility("default"))) const volatile char *web_label(void) { return fm_patch_label; }
__attribute__((used, visibility("default"))) int web_custom(void) { return fm_patch_custom; }
__attribute__((used, visibility("default"))) float web_key_cents(int k) { return fm_key_cents(k); }
__attribute__((used, visibility("default"))) int web_npatch(void) { return fm_patch_count(); }
__attribute__((used, visibility("default"))) const uint8_t *web_builtin(int i) { return fm_patch_builtin(i); }
__attribute__((used, visibility("default"))) float web_key_level(int k) { return k >= 0 && k < FM_NKEY ? fm_key_level[k] : 0.0f; }
__attribute__((used, visibility("default"))) float web_bend_cents(void) { return fm_bend_cents; }
