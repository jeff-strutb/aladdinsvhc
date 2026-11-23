/*
 * grid_check.c: the save slots' screen outside the game, built from the game's own slot_grid.c, menu_art.c and save_crop.c
 * (grid_build.sh). Its engine calls are stubs.
 *
 *   grid_check BASE.ppm W H FROM PAGE SEL ASK USED OUT.ppm [CROP_OUT.ppm]
 *       the screen over the game's picture BASE (a P6 ppm) in a window W x H as the game shows it (scaled to the window's
 *       height, centred, nearest), the overlays laid as slot_grid.c lays them. FROM: game or title. PAGE 0-3, SEL 0-5 the
 *       cell on the page chosen, ASK: 0 none, 1 OVERWRITE/LOAD/CANCEL, 2 REALLY OVERWRITE, 3 REALLY LOAD. USED: 0 every slot
 *       empty, 1 slots with pictures cropped from BASE (as saves_crop crops the game's picture), the first of each page
 *       empty. CROP_OUT: the first used slot's crop, to compare with BASE.
 *   GRID_PICS="FILE.ppm:X:Y;..." (environment): the slots' pictures cropped as the game crops them, around the player: (X, Y)
 *       is where his feet are in that picture (the play's trace: his place less the camera); the slots take them in turn.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "data/data.h"
#include "nshvc/nshvc.h"
#include "pause/menu_art.h"
#include "pause/slot_grid.h"
#include "system/saves.h"

static FILE *asset_file;

nshvc_asset nshvc_asset_open(const char *path)
{
    nshvc_asset a = { 0 };
    const char *dir = getenv("DATA_TEST_DIR");
    char full[4096];

    snprintf(full, sizeof full, "%s/%s", dir ? dir : ".", path);
    asset_file = fopen(full, "rb");
    a.id = asset_file ? 1 : 0;
    return a;
}
uint32_t nshvc_asset_read(nshvc_asset a, void *data, uint32_t size) { (void)a; return (uint32_t)fread(data, 1, size, asset_file); }
void nshvc_asset_close(nshvc_asset a) { (void)a; fclose(asset_file); }
const char *nshvc_controller_label(uint32_t c, uint32_t ctl) { (void)c; (void)ctl; return ""; }
nshvc_display nshvc_display_info(void) { nshvc_display d; memset(&d, 0, sizeof d); return d; }
nshvc_image nshvc_image_create(uint32_t w, uint32_t h, nshvc_image_format f) { nshvc_image i = { 0 }; (void)w; (void)h; (void)f; return i; }
void nshvc_image_write(nshvc_image i, const void *t) { (void)i; (void)t; }
void nshvc_overlay_set(uint32_t id, nshvc_image i, const float c[6], float a) { (void)id; (void)i; (void)c; (void)a; }
void SoundQueueAdd(unsigned char c) { (void)c; }

static uint32_t *read_ppm(const char *path, int *w, int *h)
{
    FILE *f = fopen(path, "rb");
    int max;
    uint32_t *px;
    long i;

    if (!f || fscanf(f, "P6 %d %d %d", w, h, &max) != 3)
        return NULL;
    fgetc(f);
    px = malloc((size_t)*w * (size_t)*h * 4);
    for (i = 0; i < (long)*w * *h; i++) {
        int r = fgetc(f), g = fgetc(f), b = fgetc(f);

        px[i] = 0xFF000000u | (uint32_t)r << 16 | (uint32_t)g << 8 | (uint32_t)b;
    }
    fclose(f);
    return px;
}

static void write_ppm(const char *path, const uint32_t *px, int w, int h)
{
    FILE *f = fopen(path, "wb");
    int i;

    fprintf(f, "P6\n%d %d\n255\n", w, h);
    for (i = 0; i < w * h; i++) {
        fputc((int)(px[i] >> 16 & 0xFF), f);
        fputc((int)(px[i] >> 8 & 0xFF), f);
        fputc((int)(px[i] & 0xFF), f);
    }
    fclose(f);
}

static void blend(uint32_t *d, uint32_t s, float alpha)
{
    float a = (float)(s >> 24) / 255.0f * alpha;
    int k;
    uint32_t out = 0xFF000000u;

    for (k = 0; k < 3; k++) {
        float sv = (float)(s >> (16 - 8 * k) & 0xFF), dv = (float)(*d >> (16 - 8 * k) & 0xFF);

        out |= (uint32_t)(sv * a + dv * (1 - a) + 0.5f) << (16 - 8 * k);
    }
    *d = out;
}

static SlotCell cells[SAVE_SLOTS];
static uint32_t pics[SAVE_SLOTS][SAVE_PIC_W * SAVE_PIC_H];
static const SlotCell *cell_of(void *ctx, int i) { (void)ctx; return &cells[i]; }

int main(int argc, char **argv)
{
    int bw, bh, ww, wh, i, x, y, d, game, page, sel, ask, used;
    uint32_t *base, *win, *img;
    float sc, sx, s, ox, oy;
    SlotGridSpec spec;
    const char *env;
    uint32_t *pic[8];
    int pw[8], ph[8], pcx[8], pcy[8], npic = 0;

    if (argc < 10) {
        fprintf(stderr, "usage: grid_check BASE.ppm W H game|title PAGE SEL ASK USED OUT.ppm [CROP_OUT.ppm]\n");
        return 2;
    }
    base = read_ppm(argv[1], &bw, &bh);
    if (!base)
        return 1;
    ww = atoi(argv[2]); wh = atoi(argv[3]); game = !strcmp(argv[4], "game");
    page = atoi(argv[5]); sel = atoi(argv[6]); ask = atoi(argv[7]); used = atoi(argv[8]);
    data_load();
    if ((env = getenv("GRID_PICS")) != NULL) {
        char *list = strdup(env), *item, *save = NULL;

        for (item = strtok_r(list, ";", &save); item && npic < 8; item = strtok_r(NULL, ";", &save)) {
            char *c1 = strchr(item, ':'), *c2 = c1 ? strchr(c1 + 1, ':') : NULL;

            if (!c2)
                continue;
            *c1 = *c2 = 0;
            pic[npic] = read_ppm(item, &pw[npic], &ph[npic]);
            pcx[npic] = atoi(c1 + 1);
            pcy[npic] = atoi(c2 + 1);
            npic += pic[npic] != NULL;
        }
    }
    for (i = 0; i < SAVE_SLOTS; i++) {
        memset(&cells[i], 0, sizeof cells[i]);
        snprintf(cells[i].line1, sizeof cells[i].line1, "SLOT %d", i + 1);
        if (used && i % 6) {
            if (npic) {
                saves_crop(pic[i % npic], pw[i % npic], ph[i % npic], pcx[i % npic], pcy[i % npic] - SAVE_CROP_LIFT, pics[i]);
            } else
                saves_crop(base, bw, bh, bw / 2, bh / 2, pics[i]);
            cells[i].used = 1;
            cells[i].picture = pics[i];
            snprintf(cells[i].line1, sizeof cells[i].line1, "LEVEL %d.%d", i / 6 + 1, i % 3 + 1);
            snprintf(cells[i].line2, sizeof cells[i].line2, "TIME 0:%02d:%02d", i, 7 * i % 60);
        }
    }
    if (argc > 10 && used)
        write_ppm(argv[10], pics[1], SAVE_PIC_W, SAVE_PIC_H);   /* slot 2's crop */
    memset(&spec, 0, sizeof spec);
    spec.title = game ? "SAVE GAME" : "SAVED GAMES";
    spec.cols = 3; spec.rows = 2; spec.pages = SAVE_SLOTS / 6;
    spec.pic_w = SAVE_PIC_W; spec.pic_h = SAVE_PIC_H;
    spec.cell = cell_of;
    slot_grid_open(&spec);
    {
        /* the grid has no setter for the selection: keys would do it; the check moves it with the page and cell numbers by
           asking the grid to draw as if chosen */
        extern void slot_grid_select_for_check(int index);
        slot_grid_select_for_check(page * 6 + sel);
    }
    if (ask == 1) {
        static const char *const w[3] = { "OVERWRITE", "LOAD", "CANCEL" };
        slot_grid_ask("SLOT 2", w, 3);
    } else if (ask >= 2) {
        static const char *const yn[2] = { "YES", "NO" };
        slot_grid_ask(ask == 2 ? "REALLY OVERWRITE SLOT 2?" : "REALLY LOAD SLOT 2?", yn, 2);
    }
    win = calloc((size_t)ww * (size_t)wh, 4);
    sc = (float)wh / (float)bh;
    sx = (float)ww - (float)bw * sc;
    for (y = 0; y < wh; y++)
        for (x = 0; x < ww; x++) {
            int px = (int)(((float)x + 0.5f - sx / 2) / sc), py = (int)(((float)y + 0.5f) / sc);

            win[y * ww + x] = px >= 0 && px < bw && py < bh ? base[py * bw + px] : 0xFF000000u;
        }
    menu_art_layout(0, 0, (float)ww, (float)wh, &s, &d, &ox, &oy);
    for (i = 0; i < ww * wh; i++)
        blend(&win[i], 0xFF000000u, 0.62f);
    img = calloc((size_t)MENU_W * d * (size_t)MENU_H * d, 4);
    slot_grid_render(img, d);
    for (y = 0; y < wh; y++)
        for (x = 0; x < ww; x++) {
            float u = ((float)x + 0.5f - ox) / (MENU_W * s), v = ((float)y + 0.5f - oy) / (MENU_H * s);
            int ix = (int)(u * (float)(MENU_W * d)), iy = (int)(v * (float)(MENU_H * d));

            if (u < 0 || v < 0 || ix >= MENU_W * d || iy >= MENU_H * d)
                continue;
            blend(&win[y * ww + x], img[iy * MENU_W * d + ix], 1);
        }
    write_ppm(argv[9], win, ww, wh);
    return 0;
}
