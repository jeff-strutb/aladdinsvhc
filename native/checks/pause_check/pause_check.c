/*
 * pause_check.c: the menu's art outside the game, built from the game's own menu_art.c and pad_art.c (build.sh).
 *
 *   pause_check pad SCALE OUT        the controller alone, a pixel of its drawing at SCALE pixels: 1000 SCALE x 600 SCALE,
 *       0xAARRGGBB straight alpha, raw (check.py holds it against rsvg-convert's drawing of SNES_controller.svg)
 *   pause_check menu BASE.ppm WINDOW_W WINDOW_H CHOSEN OUT.ppm
 *       the menu over the game's picture BASE (a P6 ppm, any size) shown in a window WINDOW_W x WINDOW_H as the game
 *       shows it (the picture scaled to the window's height, centred, nearest), the overlays laid as pause/menu.c lays
 *       them (menu_art_layout), the tags with the game's default keys, CHOSEN the item chosen (0-11 a button, 12 RESUME,
 *       13 DEFAULTS, 14 FULL SCREEN, 15 QUIT, 16 QUIT asked)
 *
 * It reads cart.dat from the directory DATA_TEST_DIR (the cartridge's data the game reads: the text and the colours).
 * Each says on stderr how long its drawing took.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "data/data.h"
#include "nshvc/nshvc.h"
#include "pause/menu_art.h"
#include "pause/pause_art.h"

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

static double now(void)
{
    struct timespec t;

    clock_gettime(CLOCK_MONOTONIC, &t);
    return (double)t.tv_sec + (double)t.tv_nsec * 1e-9;
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
        int r = fgetc(f), g = fgetc(f), b = fgetc(f);

        px[i] = 0xFF000000u | (uint32_t)r << 16 | (uint32_t)g << 8 | (uint32_t)b;
    }
    fclose(f);
    return px;
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

/* an image of iw x ih pixels laid over pic (w x h) in the rectangle (ox, oy, rw, rh), nearest */
static void over(uint32_t *pic, int w, int h, const uint32_t *img, int iw, int ih, float ox, float oy, float rw, float rh, float alpha)
{
    int x, y;

    for (y = 0; y < h; y++)
        for (x = 0; x < w; x++) {
            float u = ((float)x + 0.5f - ox) / rw, v = ((float)y + 0.5f - oy) / rh;
            int ix = (int)(u * (float)iw), iy = (int)(v * (float)ih);

            if (u < 0 || v < 0 || ix >= iw || iy >= ih)
                continue;
            blend(&pic[y * w + x], img[iy * iw + ix], alpha);
        }
}

static const char *const k_default_names[MENU_TAGS] = { "Q", "UP", "LEFT", "RIGHT", "DOWN", "SHIFT",
                                                         "W", "S", "A", "X", "Z", "RETURN" };

/* what the options screen's first control type gives each button */
static const char *const k_default_actions[MENU_TAGS] = { "UNUSED", "LOOK UP", "GO LEFT", "GO RIGHT", "CROUCH", "UNUSED",
                                                          "HOVER", "UNUSED", "DASH", "THROW", "JUMP", "MENU" };

int main(int argc, char **argv)
{
    double t0;

    if (argc == 4 && !strcmp(argv[1], "pad")) {
        float s = (float)atof(argv[2]);
        int w = (int)(1000 * s + 0.5f), h = (int)(600 * s + 0.5f);
        uint32_t *px = calloc((size_t)w * (size_t)h, 4);
        FILE *f = fopen(argv[3], "wb");

        t0 = now();
        pad_draw(px, w, h, 1.0f, s, 500.0f * s, 300.0f * s);
        fprintf(stderr, "pad: %.1f ms\n", (now() - t0) * 1000);
        fwrite(px, 4, (size_t)w * (size_t)h, f);
        fclose(f);
        return 0;
    }
    if (argc >= 7 && !strcmp(argv[1], "menu")) {
        int bw, bh, ww = atoi(argv[3]), wh = atoi(argv[4]), chosen = atoi(argv[5]), d, i, x, y;
        uint32_t *base = read_ppm(argv[2], &bw, &bh), *win, *st, *dy;
        float s, ox, oy, sc, sx;
        MenuView v;
        FILE *f;

        if (!base)
            return 1;
        data_load();
        win = calloc((size_t)ww * (size_t)wh, 4);
        sc = (float)wh / (float)bh;                 /* the picture scaled to the window's height, centred */
        sx = (float)ww - (float)bw * sc;
        for (y = 0; y < wh; y++)
            for (x = 0; x < ww; x++) {
                int px = (int)(((float)x + 0.5f - sx / 2) / sc), py = (int)(((float)y + 0.5f) / sc);

                win[y * ww + x] = px >= 0 && px < bw && py < bh ? base[py * bw + px] : 0xFF000000u;
            }
        menu_art_layout(0, 0, (float)ww, (float)wh, &s, &d, &ox, &oy);
        memset(&v, 0, sizeof v);
        v.chosen = chosen > MENU_QUIT ? MENU_QUIT : chosen;
        v.quit_asked = chosen > MENU_QUIT;
        v.asking = -1;
        v.full_offered = 1;
        v.full_on = 1;
        for (i = 0; i < MENU_TAGS; i++)
        {
            strcpy(v.label[i], k_default_names[i]);
            strcpy(v.action[i], k_default_actions[i]);
        }
        st = calloc((size_t)MENU_W * d * (size_t)MENU_H * d, 4);
        dy = calloc((size_t)MENU_W * d * (size_t)MENU_H * d, 4);
        t0 = now();
        menu_art_static(st, d);
        fprintf(stderr, "static (d=%d, scale %.2f): %.1f ms\n", d, s, (now() - t0) * 1000);
        t0 = now();
        menu_art_dynamic(dy, d, &v);
        fprintf(stderr, "dynamic: %.1f ms\n", (now() - t0) * 1000);
        {
            uint32_t black = 0xFF000000u;

            for (i = 0; i < ww * wh; i++)
                blend(&win[i], black, 0.62f);
        }
        over(win, ww, wh, st, MENU_W * d, MENU_H * d, ox, oy, MENU_W * s, MENU_H * s, 1);
        over(win, ww, wh, dy, MENU_W * d, MENU_H * d, ox, oy, MENU_W * s, MENU_H * s, 1);
        f = fopen(argv[6], "wb");
        fprintf(f, "P6\n%d %d\n255\n", ww, wh);
        for (i = 0; i < ww * wh; i++) {
            fputc((int)(win[i] >> 16 & 0xFF), f);
            fputc((int)(win[i] >> 8 & 0xFF), f);
            fputc((int)(win[i] & 0xFF), f);
        }
        fclose(f);
        return 0;
    }
    fprintf(stderr, "usage: pause_check pad SCALE OUT | menu BASE.ppm W H CHOSEN OUT.ppm\n");
    return 1;
}
