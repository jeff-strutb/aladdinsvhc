/*
 * saves.c: the save slots' files (saves.h).
 *
 * File (little endian): "ASVG", format 1, total size, layout mark, snapshot length (u32 each), sequence, stage, the play
 * time's six digits, the update count (u64), the crop (96 x 72 u32 0xAARRGGBB), the snapshot, a FNV-1a 64 hash of every
 * byte before it.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "nshvc/nshvc.h"
#include "system/saves.h"
#include "system/snapshot.h"
#include "system/state.h"
#include "system/system.h"

#define FORMAT 1u
#define HEAD 36u                        /* 4 + 4*4 + 2 + 6 + 8 */
#define PIC_BYTES (SAVE_PIC_W * SAVE_PIC_H * 4u)
#define READ_CAP (6u << 20)             /* more than a snapshot's most (about 2.7 MB; it changes with the level): a longer file is refused by its size */

static SaveInfo infos[SAVE_SLOTS];
static int scanned;

static void put32(uint8_t *p, uint32_t v) { p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); p[2] = (uint8_t)(v >> 16); p[3] = (uint8_t)(v >> 24); }
static uint32_t get32(const uint8_t *p) { return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24; }

static uint64_t hash(const uint8_t *p, size_t n)
{
    uint64_t h = 0xcbf29ce484222325ull;

    while (n--)
        h = (h ^ *p++) * 0x100000001b3ull;
    return h;
}

static void name_of(int slot, char *out, size_t n) { snprintf(out, n, "save_%02d", slot); }

/* ---- the capture ---- */

/* The engine's picture is the window's, the game's picture in it scaled by the engine: for each game pixel the window pixel
   at its middle (nearest), found through nshvc_window_to_picture. Exact where the scale is a whole number and with no window. */
static const uint32_t *game_picture(int *gw, int *gh, uint32_t **own)
{
    int ww, wh, x, y, n;
    const uint32_t *px = nshvc_picture_read(&ww, &wh);
    int32_t gx, gy, maxx = -1, maxy = -1;
    int *colx, *rowy;                   /* the first and last window pixel of each game column and row, packed */
    uint32_t *out;
    int *lox, *hix, *loy, *hiy;

    *own = NULL;
    if (!px)
        return NULL;
    lox = malloc(sizeof(int) * (size_t)(ww + wh) * 4);
    if (!lox)
        return NULL;
    hix = lox + ww; loy = hix + ww; hiy = loy + wh;
    colx = lox; rowy = loy;
    (void)colx; (void)rowy;
    for (x = 0; x < ww; x++)
        lox[x] = hix[x] = -1;
    for (y = 0; y < wh; y++)
        loy[y] = hiy[y] = -1;
    /* window column x -> game column along the middle row; window row y -> game row along the middle column */
    {
        int *gcx = malloc(sizeof(int) * (size_t)(ww + wh));
        int *gry;

        if (!gcx) { free(lox); return NULL; }
        gry = gcx + ww;
        for (x = 0; x < ww; x++)
            gcx[x] = nshvc_window_to_picture((float)x + 0.5f, (float)wh / 2, &gx, &gy) ? (int)gx : -1;
        for (y = 0; y < wh; y++)
            gry[y] = nshvc_window_to_picture((float)ww / 2, (float)y + 0.5f, &gx, &gy) ? (int)gy : -1;
        for (x = 0; x < ww; x++)
            if (gcx[x] > maxx)
                maxx = gcx[x];
        for (y = 0; y < wh; y++)
            if (gry[y] > maxy)
                maxy = gry[y];
        if (maxx < 0 || maxy < 0) {     /* no mapping (no window): the picture is the game's own */
            free(gcx);
            free(lox);
            *gw = ww;
            *gh = wh;
            return px;
        }
        out = malloc((size_t)(maxx + 1) * (size_t)(maxy + 1) * 4);
        if (!out) { free(gcx); free(lox); return NULL; }
        {
            int gxn = maxx + 1, gyn = maxy + 1, c, r;

            for (c = 0; c < gxn; c++) lox[c] = -1, hix[c] = -1;
            for (r = 0; r < gyn; r++) loy[r] = -1, hiy[r] = -1;
            for (x = 0; x < ww; x++)
                if (gcx[x] >= 0) {
                    if (lox[gcx[x]] < 0) lox[gcx[x]] = x;
                    hix[gcx[x]] = x;
                }
            for (y = 0; y < wh; y++)
                if (gry[y] >= 0) {
                    if (loy[gry[y]] < 0) loy[gry[y]] = y;
                    hiy[gry[y]] = y;
                }
            for (r = 0; r < gyn; r++)
                for (c = 0; c < gxn; c++) {
                    n = 0;
                    if (lox[c] < 0 || loy[r] < 0) { out[r * gxn + c] = 0xFF000000u; continue; }
                    out[r * gxn + c] = px[((loy[r] + hiy[r]) / 2) * ww + (lox[c] + hix[c]) / 2];
                    (void)n;
                }
            *gw = gxn;
            *gh = gyn;
        }
        free(gcx);
    }
    free(lox);
    *own = out;
    return out;
}

void saves_capture(SaveCapture *c)
{
    uint32_t *own;
    int w, h, i, px, py;
    const uint32_t *pic;

    memset(c, 0, sizeof *c);
    c->updates = nshvc_updates();
    c->sequence = g.progress_sequence;
    c->stage = g.progress_stage;
    for (i = 0; i < 6; i++)
        c->digits[i] = g.play_digits[i];
    pic = game_picture(&w, &h, &own);
    if (!pic) {
        for (i = 0; i < SAVE_PIC_W * SAVE_PIC_H; i++)
            c->pic[i] = 0xFF000000u;
        return;
    }
    px = (int)(uint16_t)(g.actors[0].x >> 8) - (int)(uint16_t)(g.layers[0].scroll_x >> 8);
    py = (int)(uint16_t)(g.actors[0].y >> 8) - (int)(uint16_t)(g.layers[0].scroll_y >> 8) - SAVE_CROP_LIFT;
    if (px < 0 || px >= w || py < 0 || py >= h)
        px = w / 2, py = h / 2;
    saves_crop(pic, w, h, px, py, c->pic);
    c->have = 1;
    free(own);
}

/* ---- the files ---- */

static void caption(SaveInfo *s, int sequence, int stage, const uint8_t *d)
{
    if (sequence == 6)
        snprintf(s->line1, sizeof s->line1, "FINAL LEVEL");
    else if (sequence == 5 && stage == 3)
        snprintf(s->line1, sizeof s->line1, "LEVEL 6.4");
    else
        snprintf(s->line1, sizeof s->line1, "LEVEL %d.%d", (sequence & 15) + 1, (stage & 15) + 1);
    snprintf(s->line2, sizeof s->line2, "TIME %d:%d%d:%d%d", d[5] * 10 + d[4], d[3], d[2], d[1], d[0]);
}

/* the file's bytes checked whole: returns the snapshot's place and length, or 0 */
static int check(const uint8_t *f, size_t n, size_t *snap_at, size_t *snap_len)
{
    size_t len;

    if (n < HEAD + PIC_BYTES + 8 || memcmp(f, "ASVG", 4) || get32(f + 4) != FORMAT || get32(f + 8) != n)
        return 0;
    if (get32(f + 12) != snapshot_layout_mark())
        return 0;
    len = get32(f + 16);
    if (HEAD + PIC_BYTES + len + 8 != n)
        return 0;
    {
        uint64_t h = hash(f, n - 8), want = 0;
        int i;

        for (i = 0; i < 8; i++)
            want |= (uint64_t)f[n - 8 + i] << (8 * i);
        if (h != want)
            return 0;
    }
    *snap_at = HEAD + PIC_BYTES;
    *snap_len = len;
    return 1;
}

static uint8_t *read_file(int slot, size_t *n)
{
    char name[32];
    size_t cap = READ_CAP;
    uint8_t *f = malloc(cap);
    int got;

    *n = 0;
    if (!f)
        return NULL;
    name_of(slot, name, sizeof name);
    got = nshvc_save_read(name, f, (uint32_t)cap);
    if (got < 0) {
        free(f);
        return NULL;
    }
    *n = (size_t)got;
    return f;
}

void saves_scan(void)
{
    int s;

    scanned = 1;
    for (s = 0; s < SAVE_SLOTS; s++) {
        size_t n, at, len;
        uint8_t *f = read_file(s, &n);
        SaveInfo *si = &infos[s];

        memset(si, 0, sizeof *si);
        if (!f)
            continue;                   /* no file: empty */
        if (!check(f, n, &at, &len) || !snapshot_available()) {
            si->state = SAVE_UNREADABLE;
            snprintf(si->line1, sizeof si->line1, "UNREADABLE");
        } else {
            int i;

            si->state = SAVE_USED;
            for (i = 0; i < SAVE_PIC_W * SAVE_PIC_H; i++)
                si->pic[i] = get32(f + HEAD + 4u * (unsigned)i);
            caption(si, f[20], f[21], f + 22);
        }
        free(f);
    }
}

const SaveInfo *saves_info(int slot)
{
    if (!scanned)
        saves_scan();
    return &infos[slot < 0 || slot >= SAVE_SLOTS ? 0 : slot];
}

int saves_write(int slot, const SaveCapture *c)
{
    size_t max = snapshot_max(), n, len;
    uint8_t *f;
    char name[32];
    uint64_t h;
    int i;

    if (!snapshot_available() || slot < 0 || slot >= SAVE_SLOTS)
        return 0;
    f = malloc(HEAD + PIC_BYTES + max + 8);
    if (!f)
        return 0;
    len = snapshot_save(f + HEAD + PIC_BYTES, max);
    if (!len) {
        free(f);
        return 0;
    }
    n = HEAD + PIC_BYTES + len + 8;
    memcpy(f, "ASVG", 4);
    put32(f + 4, FORMAT);
    put32(f + 8, (uint32_t)n);
    put32(f + 12, snapshot_layout_mark());
    put32(f + 16, (uint32_t)len);
    f[20] = c->sequence;
    f[21] = c->stage;
    for (i = 0; i < 6; i++)
        f[22 + i] = c->digits[i];
    for (i = 0; i < 8; i++)
        f[28 + i] = (uint8_t)(c->updates >> (8 * i));
    for (i = 0; i < SAVE_PIC_W * SAVE_PIC_H; i++)
        put32(f + HEAD + 4u * (unsigned)i, c->pic[i]);
    h = hash(f, n - 8);
    for (i = 0; i < 8; i++)
        f[n - 8 + (size_t)i] = (uint8_t)(h >> (8 * i));
    name_of(slot, name, sizeof name);
    i = nshvc_save_write(name, f, (uint32_t)n) == 0;
    free(f);
    saves_scan();
    return i;
}

int saves_load(int slot)
{
    size_t n, at, len;
    uint8_t *f;
    int ok;

    if (!snapshot_available() || slot < 0 || slot >= SAVE_SLOTS || !(f = read_file(slot, &n)))
        return 0;
    ok = check(f, n, &at, &len) && snapshot_load(f + at, len);
    free(f);
    return ok;
}
