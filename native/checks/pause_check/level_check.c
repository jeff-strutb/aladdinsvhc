/*
 * level_check.c: the level select screen outside the game, built from the game's own level_screen.c, beaten.c, level_pictures.c,
 * slot_grid.c and menu_art.c (level_build.sh). Its engine calls are stubs; the progress file is the one the environment gives.
 *
 *   LEVEL_FILE=PATH (environment): the bytes of the `levels` file the engine would read (none: no file, nothing beaten).
 *   level_check BASE.ppm W H PAGE SEL OUT.ppm [PICS_PREFIX]
 *       the screen over the game's picture BASE (a P6 ppm) in a window W x H as the game shows it (scaled to the window's height,
 *       centred, nearest), the overlays laid as slot_grid.c lays them, page PAGE (0 on) with cell SEL (0-3) chosen. With
 *       PICS_PREFIX, every picture the screen shows is also written as PICS_PREFIX-LL.ppm (LL the level, hex), as the screen has it.
 *   DATA_TEST_DIR names the folder of cart.dat and level_pictures.dat.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "data/data.h"
#include "game/beaten.h"
#include "nshvc/nshvc.h"
#include "pause/level_screen.h"
#include "pause/menu_art.h"
#include "pause/slot_grid.h"
#include "system/state.h"
#include "video/level_picture.h"

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
void NewGameCounts(void) {}
void runahead_keep(void *p, size_t bytes) { (void)p; (void)bytes; }

int nshvc_save_write(const char *name, const void *data, uint32_t size) { (void)name; (void)data; (void)size; return 0; }
int nshvc_save_read(const char *name, void *data, uint32_t size)
{
    const char *path = getenv("LEVEL_FILE");
    FILE *f = path ? fopen(path, "rb") : NULL;
    size_t n;

    (void)name;
    if (!f)
        return -1;
    n = fread(data, 1, size, f);
    fclose(f);
    return (int)n;
}

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
        int r = fgetc(f), gr = fgetc(f), b = fgetc(f);

        px[i] = 0xFF000000u | (uint32_t)r << 16 | (uint32_t)gr << 8 | (uint32_t)b;
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

GameState g;

int main(int argc, char **argv)
{
    int bw, bh, ww, wh, page, sel, x, y, d, i;
    uint32_t *base, *win, *img;
    float sc, sx, s, ox, oy;

    if (argc < 7) {
        fprintf(stderr, "usage: level_check BASE.ppm W H PAGE SEL OUT.ppm [PICS_PREFIX]\n");
        return 2;
    }
    base = read_ppm(argv[1], &bw, &bh);
    if (!base)
        return 1;
    ww = atoi(argv[2]); wh = atoi(argv[3]); page = atoi(argv[4]); sel = atoi(argv[5]);
    data_load();
    level_screen_open();
    {
        extern void slot_grid_select_for_check(int index);
        slot_grid_select_for_check(page * 4 + sel);
    }
    if (argc > 7)
        for (i = 0; i < (int)BeatenCount(); i++) {
            char name[1024];
            const uint32_t *pic = LevelPictureOf(BeatenListLevel(BeatenFind((unsigned)i)));

            if (pic) {
                snprintf(name, sizeof name, "%s-%02X.ppm", argv[7], BeatenListLevel(BeatenFind((unsigned)i)));
                write_ppm(name, pic, LEVEL_PICTURE_W, LEVEL_PICTURE_H);
            }
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
    write_ppm(argv[6], win, ww, wh);
    return 0;
}
