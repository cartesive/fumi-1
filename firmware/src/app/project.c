/* SPDX-License-Identifier: GPL-3.0-only */
/* FuMi-1 project: one flash object (OBJ_PROJ). A project of another format, or none, gives the defaults;
 * every value is range-checked (a firmware must never crash on its own saved data). Part of the unity
 * build, after app.h. */
project_t proj;

void project_defaults(void)
{
    int i;
    memset(&proj, 0, sizeof proj);
    proj.magic = PROJ_MAGIC;
    proj.format = PROJ_FORMAT;
    for (i = 0; i < P_NPARAMS; i++)
        proj.par[i] = fm_param_info(i)->def;
    proj.leds = 1;
}

static int project_valid(void)
{
    int i;
    if (proj.magic != PROJ_MAGIC || proj.format != PROJ_FORMAT)
        return 0;
    for (i = 0; i < P_NPARAMS; i++) {
        const fm_param_t *p = fm_param_info(i);
        if (proj.par[i] < p->lo || proj.par[i] > p->hi)
            return 0;
    }
    if (proj.leds > 2u)
        return 0;
    return 1;
}

/* 1.0.8 put the instruments into the strip's families (Koto, Koto II, Koto Pluck, Koto Ring, Koto Warm, Harp, the
 * bells, Shakuhachi, Bamboo Flt, Dragon Flt, Sho, Kokyu, the sustained voices, Taiko, Hyoshigi, Mokugyo): a format-3
 * (1.0.6, 1.0.7) instrument number, by name, in the new order */
static const uint8_t VOICE_V3_TO_V4[14] = {0, 1, 13, 10, 12, 11, 14, 18, 19, 20, 2, 3, 4, 5};

/* a format-2 project (1.0.4, 1.0.5): the same parameters up to the looper's; Kokyu was put before Taiko, so an
 * instrument from Taiko on moves up one, and then the 1.0.8 renumbering; Trill var became Trill feel (a new meaning:
 * its new default); the trill's defaults are new */
static int project_migrate(const uint8_t *raw, int n)
{
    uint32_t i;
    if (n < (int)(8u + 2u * PROJ_NPAR_V2 + 1u))
        return 0;
    project_defaults();
    for (i = 0; i < PROJ_NPAR_V2; i++)
        proj.par[i] = (int16_t)((uint32_t)raw[8 + 2 * i] | (uint32_t)raw[9 + 2 * i] << 8);
    if (proj.par[P_VOICE] >= 6)
        proj.par[P_VOICE]++;
    if (proj.par[P_VOICE] < 0 || proj.par[P_VOICE] >= 14)
        return 0;
    proj.par[P_VOICE] = VOICE_V3_TO_V4[proj.par[P_VOICE]];
    proj.par[P_TRILL_VAR] = fm_param_info(P_TRILL_VAR)->def;
    proj.leds = raw[8 + 2 * PROJ_NPAR_V2];
    return project_valid();
}

/* a format-3 project (1.0.6, 1.0.7): the same parameters; the saved instrument is renumbered to keep its name */
static int project_migrate_v3(const uint8_t *raw, int n)
{
    if (n < (int)sizeof proj)
        return 0;
    memcpy(&proj, raw, sizeof proj);
    proj.format = PROJ_FORMAT;
    if (proj.par[P_VOICE] < 0 || proj.par[P_VOICE] >= 14)
        return 0;
    proj.par[P_VOICE] = VOICE_V3_TO_V4[proj.par[P_VOICE]];
    return project_valid();
}

int project_load(void)
{
    int n;
    uint8_t raw[sizeof proj];
    memset(&proj, 0, sizeof proj);
    memset(raw, 0, sizeof raw);
    n = plat_store_load(OBJ_PROJ, raw, sizeof raw);
    if (n >= 8) {
        uint32_t magic = (uint32_t)raw[0] | (uint32_t)raw[1] << 8 | (uint32_t)raw[2] << 16 | (uint32_t)raw[3] << 24;
        uint32_t format = (uint32_t)raw[4] | (uint32_t)raw[5] << 8 | (uint32_t)raw[6] << 16 | (uint32_t)raw[7] << 24;
        if (magic == PROJ_MAGIC && format == PROJ_FORMAT) {
            memcpy(&proj, raw, sizeof proj);  /* an older, shorter project of this format loads with zeros after it */
            if (project_valid())
                return 0;
        } else if (magic == PROJ_MAGIC && format == 2u && project_migrate(raw, n)) {
            return 0;
        } else if (magic == PROJ_MAGIC && format == 3u && project_migrate_v3(raw, n)) {
            return 0;
        }
    }
    project_defaults();
    return -1;
}

int project_save(void) { return plat_store_save(OBJ_PROJ, &proj, sizeof proj); }

void project_apply(void)
{
    int i;
    for (i = 0; i < P_NPARAMS; i++)
        fm_set(i, proj.par[i]);
}
