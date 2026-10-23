/* SPDX-License-Identifier: GPL-3.0-only */
/* FuMi-1 platform on the FM-1 (FoMni's): plat.h over Felucca's HAL, panel map, USB rings and storage.
 * Part of the unity build (omni.c), after the HAL, usb.c, storage.c and panel.c. */

uint32_t plat_ms(void) { return fm1_ms; }

uint32_t plat_buttons(void)
{
    uint32_t raw = fm1_in.buttons, out = 0, i;
    for (i = 0; i < NB; i++)
        if (raw & (1u << panel.btn[i]))
            out |= 1u << i;
    return out;
}

uint32_t plat_keys(void) { return fm1_in.notes & ((1u << NKEYS) - 1u); }

int32_t plat_enc(int role)
{
    if (role < 0 || role >= NE)
        return 0;
    return fm1_enc_take(panel.enc[role]) * panel.dir[role];
}

static volatile uint32_t master_q12 = 2048;
uint32_t plat_master(void) { return master_q12; }

/* LEDs: the picture is built off-line and copied one byte per column (Felucca: clearing
 * and relighting would let the 10 kHz scan catch the dark gap and flicker) */
static uint8_t led_pos[FM1_NKEY];
static void led_pos_init(void)
{
    uint32_t id, p, r;
    for (id = 0; id < FM1_NKEY; id++) {
        led_pos[id] = 0xFF;
        for (p = 0; p < FM1_NCOL; p++)
            for (r = 1; r < 5u; r++)
                if (FM1_KEYMAP[r][p] == (int8_t)id)
                    led_pos[id] = (uint8_t)((p << 3) | r);
    }
}
/* Glow: every button not lit is lit dim, on one scan frame in GLOW_DIV (the HAL's fm1_led_dim) */
#define GLOW_DIV 4u
static uint8_t glow_on = 1;
void plat_glow(int on) { glow_on = (uint8_t)(on != 0); }
/* PLAY has two LEDs: its red at the button's own position, a green at the keyless matrix position
 * column 8, row PA5 (found on the device). Green, unless recording. */
static uint8_t play_red;
void plat_play_red(int red) { play_red = (uint8_t)(red != 0); }
void plat_leds(uint32_t buttons, uint32_t keys)
{
    uint8_t nl[FM1_NCOL] = {0}, dl[FM1_NCOL] = {0};
    uint32_t i, c;
    for (i = 0; i < NB + NKEYS; i++) {
        uint32_t id = i < NB ? panel.btn[i] : 14u + (i - NB);
        int on = i < NB ? (buttons >> i) & 1u : (keys >> (i - NB)) & 1u;
        uint8_t q = led_pos[id];
        if (i == B_PLAY && !play_red)            /* PLAY's green LED instead (keyless position col 8, row PA5) */
            q = (uint8_t)((8u << 3) | 1u);
        if (q == 0xFF)
            continue;
        if (on)
            nl[q >> 3] |= (uint8_t)(1u << (q & 7u));
        else if (i < NB && glow_on)
            dl[q >> 3] |= (uint8_t)(1u << (q & 7u));
    }
    for (c = 0; c < FM1_NCOL; c++) {
        fm1_led[c] = nl[c];
        fm1_led_dim[c] = dl[c];
    }
    fm1_led_dim_div = glow_on ? GLOW_DIV : 0u;
}

/* MIDI: Felucca's rings carry 4-byte USB-MIDI event packets, byte 0 = cable / CIN */
int plat_midi_in(uint32_t *pkt)
{
    if (mi_r == mi_w)
        return 0;
    *pkt = midi_in_q[mi_r % MQ];
    mi_r++;
    return 1;
}
void plat_midi_out(uint32_t pkt) { midi_out_event(pkt); }
/* the engine's MIDI out (from the render): a USB-MIDI packet, cable 0, CIN = the status's high nibble */
void fm_midi_out(uint32_t st, uint32_t d1, uint32_t d2) { plat_midi_out(st >> 4 | st << 8 | d1 << 16 | d2 << 24); }

int plat_store_load(uint32_t obj, void *dst, uint32_t max) { return flash_ok ? st_load(obj, dst, max) : -1; }
int plat_store_save(uint32_t obj, const void *src, uint32_t len) { return flash_ok ? st_save(obj, src, len) : -9; }

/* the loop's object (plat.h): storage.c's A/B scheme over three sectors a copy. A save erases the copy not in charge,
 * programs the payload from offset 256 in pages, then the header, and reads everything back; on load the valid copy with
 * the higher seq wins, the other if its payload fails its CRC. */
#define LOOP_SECTORS 3u
#define LOOP_TYPE 0x4C50u                               /* "LP": not an OBJ_* of storage.c */
static uint32_t loop_base(int copy) { return copy ? 0xD9000u : 0xD6000u; }
static int loop_head(int copy, st_hdr_t *h)
{
    if (st_read(loop_base(copy), h, sizeof *h))
        return -1;
    return h->magic == ST_MAGIC && h->type == LOOP_TYPE && h->len <= PLAT_LOOP_MAX && h->hcrc == st_crc32(h, sizeof *h - 4u) ? 0 : -1;
}
static int loop_heads(st_hdr_t *h, int *ok)              /* the copy in charge by its header: 0 / 1, or -1 for none */
{
    ok[0] = loop_head(0, &h[0]) == 0;
    ok[1] = loop_head(1, &h[1]) == 0;
    return ok[0] && ok[1] ? (h[1].seq > h[0].seq) : ok[1] ? 1 : ok[0] ? 0 : -1;
}
int plat_loop_load(void *dst, uint32_t max)
{
    st_hdr_t h[2];
    int ok[2], cur, k;
    if (!flash_ok || (cur = loop_heads(h, ok)) < 0)
        return -1;
    for (k = 0; k < 2; k++) {                            /* the copy in charge, else the other */
        int copy = cur ^ k;
        if (!ok[copy] || h[copy].len > max)
            continue;
        if (st_read(loop_base(copy) + ST_PAYLOAD_OFF, dst, h[copy].len) == 0 && st_crc32(dst, h[copy].len) == h[copy].crc)
            return (int)h[copy].len;
    }
    return -1;
}
int plat_loop_save(const void *src, uint32_t len)
{
    st_hdr_t h[2], w, chk;
    int ok[2], cur, copy, rc;
    uint32_t base, off;
    if (!flash_ok || len > PLAT_LOOP_MAX)
        return -1;
    cur = loop_heads(h, ok);
    copy = cur == 0 ? 1 : 0;
    base = loop_base(copy);
    for (off = 0; off < LOOP_SECTORS * 4096u; off += 4096u)
        if ((rc = st_erase(base + off)) != 0)
            return rc;
    for (off = 0; off < len; off += 256u) {
        uint32_t n = len - off > 256u ? 256u : len - off;
        if ((rc = st_prog(base + ST_PAYLOAD_OFF + off, (const uint8_t *)src + off, n)) != 0)
            return rc;
    }
    w.magic = ST_MAGIC;
    w.type = (uint16_t)LOOP_TYPE;
    w.slot = (uint16_t)copy;
    w.seq = (cur < 0 ? 0u : h[cur].seq) + 1u;
    w.len = len;
    w.crc = st_crc32(src, len);
    w.rsv[0] = w.rsv[1] = 0xFFFFFFFFu;
    w.hcrc = st_crc32(&w, sizeof w - 4u);
    if ((rc = st_prog(base, &w, sizeof w)) != 0)         /* the commit record, last */
        return rc;
    if (loop_head(copy, &chk) || chk.seq != w.seq || chk.crc != w.crc)
        return -7;
    for (off = 0; off < len; off += 256u) {              /* read back: a failing part must not report saved */
        uint8_t pg[256];
        uint32_t n = len - off > 256u ? 256u : len - off, i;
        if (st_read(base + ST_PAYLOAD_OFF + off, pg, n))
            return -7;
        for (i = 0; i < n; i++)
            if (pg[i] != ((const uint8_t *)src)[off + i])
                return -7;
    }
    return 0;
}

static volatile uint32_t audio_cpu_pct, audio_xruns;
uint32_t plat_cpu_pct(void) { return audio_cpu_pct; }
uint32_t plat_xruns(void) { return audio_xruns; }
