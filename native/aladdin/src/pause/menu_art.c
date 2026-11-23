/*
 * menu_art.c: the menu's pictures (menu_art.h).
 *
 * The text is the game's 8 x 8 text tiles (unpack set 1, the options screen's; a tile's number is the character's ASCII)
 * and the hand is the options screen's cursor (tiles $7E, $02-$04, $12-$14), coloured with the palettes that screen loads
 * (set $08: the gold of its items, the cream, the pink of its title, the glove's blue). The frame is drawn from the gold's
 * colours. The controller is pad_art.c's.
 */
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "data/data.h"
#include "input/controls.h"
#include "pause/menu_art.h"
#include "pause/pause_art.h"
#include "system/system.h"

#define FONT_BYTES 4096                 /* the text tiles: 256 of 16 bytes, 2 bits a pixel */
#define FONT_SET 1                      /* the unpack set the options screen puts at $5000 */
#define PALETTE_SET 0x08                /* the options screen's text palettes: 0 glove, 1 cream, 2 gold, 3 pink */

enum { PAL_GLOVE, PAL_CREAM, PAL_GOLD, PAL_PINK };

static uint8_t font[FONT_BYTES];
static uint32_t colour[4][4];           /* [palette][pixel value]; 0 is see-through */
static int ready;

const MenuTag menu_tags[MENU_TAGS] = {
    { 0, 0, 10, "L" }, { 0, 1, 4, "UP" }, { 0, 2, 6, "LEFT" }, { 0, 3, 7, "RIGHT" }, { 0, 4, 5, "DOWN" },
    { 0, 5, 2, "SELECT" },
    { 1, 0, 11, "R" }, { 1, 1, 9, "X" }, { 1, 2, 1, "Y" }, { 1, 3, 8, "A" }, { 1, 4, 0, "B" }, { 1, 5, 3, "START" },
};

/* each button's spot on the controller: Stunt Race FX's (its panel's points, its drawing at 0.3645 of them about
   (320, 190)) put back in the drawing's own pixels, and its reach there */
#define SRFX_K 0.3645f
static const struct { float x, y, r; } spots[MENU_TAGS] = {
    { 228.9f, 124.8f, 10 }, { 227.9f, 173.1f, 7 }, { 211.1f, 189.8f, 7 }, { 244.4f, 189.8f, 7 }, { 227.9f, 206.4f, 7 },
    { 297.6f, 201.0f, 8 }, { 411.3f, 124.8f, 10 }, { 407.7f, 163.6f, 10.5f }, { 383.3f, 188.2f, 10.5f },
    { 437.2f, 191.1f, 10.5f }, { 412.6f, 215.9f, 10.5f }, { 332.3f, 201.0f, 8 },
};

#define PAD_K 0.2f                      /* game pixels a drawing pixel: the controller is 200 x 120 */
#define PAD_CX 176.0f
#define PAD_CY 117.0f
#define TAG_W 76
#define TAG_H 26
#define TAG_X(right) ((right) ? MENU_W - 6 - TAG_W : 6)
#define TAG_Y(row) (34 + 28 * (row))
#define HINT_Y 208
#define ITEM_Y(row) (226 + 17 * (row))
#define ITEM_HAND_X(col) ((col) ? 190 : 34)
#define ITEM_TEXT_X(col) ((col) ? 220 : 64)
#define ITEM_W 138
#define ITEM_H 16

static uint32_t rgb(uint16_t c)
{
    unsigned r = c & 31u, g = c >> 5 & 31u, b = c >> 10 & 31u;

    return 0xFF000000u | ((r * 255u + 15u) / 31u) << 16 | ((g * 255u + 15u) / 31u) << 8 | ((b * 255u + 15u) / 31u);
}

#define FILL 0.94f                      /* of the safe area the panel may take, each way */
#define MAX_D 6                         /* image pixels to a game pixel, at most (a bigger window scales these) */

void menu_art_layout(float l, float t, float w, float h, float *scale, int *d, float *ox, float *oy)
{
    float s = fminf(w * FILL / MENU_W, h * FILL / MENU_H);

    if (s >= 1)
        s = floorf(s);
    *scale = s;
    *d = s < 1 ? 1 : (int)s > MAX_D ? MAX_D : (int)s;
    *ox = floorf(l + (w - MENU_W * s) / 2);
    *oy = floorf(t + (h - MENU_H * s) / 2);
}

void menu_art_start(void)
{
    UnpackSetEntry u;
    int p, v;

    if (ready)
        return;
    ready = 1;
    u = unpack_set(FONT_SET);
    memset(font, 0, sizeof font);
    if (u.packed.groups * 8u <= FONT_BYTES)
        unpack_graphics(u.packed, font);
    for (p = 0; p < 4; p++)
        for (v = 0; v < 4; v++)
            colour[p][v] = v ? rgb(palette_load_colour(PALETTE_SET, (uint16_t)(p * 4 + v))) : 0;
}

/* ---- drawing, in game pixels ---- */

typedef struct { uint32_t *px; int d; } Art;

static void rect(Art *a, int x, int y, int w, int h, uint32_t c)
{
    int i, j, W = MENU_W * a->d, H = MENU_H * a->d;

    for (j = y * a->d; j < (y + h) * a->d && j < H; j++)
        for (i = x * a->d; i < (x + w) * a->d && i < W; i++)
            if (i >= 0 && j >= 0)
                a->px[j * W + i] = c;
}

static void dot(Art *a, int x, int y, uint32_t c) { rect(a, x, y, 1, 1, c); }

/* a text tile at game pixel (x, y), each of its pixels s game pixels square */
static void tile(Art *a, int t, int pal, int x, int y, int s)
{
    int r, c;

    for (r = 0; r < 8; r++) {
        uint8_t lo = font[t * 16 + 2 * r], hi = font[t * 16 + 2 * r + 1];

        for (c = 0; c < 8; c++) {
            int v = (lo >> (7 - c) & 1) | (hi >> (7 - c) & 1) << 1;

            if (v)
                rect(a, x + c * s, y + r * s, s, s, colour[pal][v]);
        }
    }
}

static void text(Art *a, const char *s, int pal, int x, int y, int scale)
{
    for (; *s; s++, x += 8 * scale) {
        int ch = (unsigned char)*s;

        if (ch >= 'a' && ch <= 'z')
            ch -= 32;
        if (ch != ' ')
            tile(a, ch, pal, x, y, scale);
    }
}

static int text_w(const char *s) { return (int)strlen(s) * 8; }

/* the options screen's cursor: a hand pointing right, 24 x 24, its top left at (x, y) */
static void hand(Art *a, int x, int y)
{
    tile(a, 0x7E, PAL_GLOVE, x + 8, y, 1);
    tile(a, 0x02, PAL_GOLD, x, y + 8, 1);
    tile(a, 0x03, PAL_GLOVE, x + 8, y + 8, 1);
    tile(a, 0x04, PAL_GLOVE, x + 16, y + 8, 1);
    tile(a, 0x12, PAL_GOLD, x, y + 16, 1);
    tile(a, 0x13, PAL_GLOVE, x + 8, y + 16, 1);
    tile(a, 0x14, PAL_GLOVE, x + 16, y + 16, 1);
}

static void spot_place(int i, float *x, float *y, float *r)
{
    *x = PAD_CX + ((spots[i].x - 320) / SRFX_K) * PAD_K;
    *y = PAD_CY + ((spots[i].y - 190) / SRFX_K) * PAD_K;
    *r = spots[i].r / SRFX_K * PAD_K;
}

/* ---- the frame ---- */

static void frame(Art *a, int x, int y, int w, int h, uint32_t outer, uint32_t mid, uint32_t light)
{
    rect(a, x, y, w, 1, outer); rect(a, x, y + h - 1, w, 1, outer);
    rect(a, x, y, 1, h, outer); rect(a, x + w - 1, y, 1, h, outer);
    rect(a, x + 1, y + 1, w - 2, 1, mid); rect(a, x + 1, y + h - 2, w - 2, 1, mid);
    rect(a, x + 1, y + 1, 1, h - 2, mid); rect(a, x + w - 2, y + 1, 1, h - 2, mid);
    rect(a, x + 2, y + 2, w - 4, 1, light); rect(a, x + 2, y + h - 3, w - 4, 1, light);
    rect(a, x + 2, y + 2, 1, h - 4, light); rect(a, x + w - 3, y + 2, 1, h - 4, light);
    rect(a, x + 3, y + 3, w - 6, 1, outer); rect(a, x + 3, y + h - 4, w - 6, 1, outer);
    rect(a, x + 3, y + 3, 1, h - 6, outer); rect(a, x + w - 4, y + 3, 1, h - 6, outer);
}

void menu_art_panel(uint32_t *px, int d, const char *title)
{
    Art a = { px, d };
    uint32_t dark, mid, light;
    int i;

    menu_art_start();
    dark = colour[PAL_GOLD][3], mid = colour[PAL_GOLD][2], light = colour[PAL_GOLD][1];
    rect(&a, 0, 0, MENU_W, MENU_H, 0xF2080402u);
    frame(&a, 0, 0, MENU_W, MENU_H, dark, mid, light);
    for (i = 0; i < 4; i++) {           /* a stud in each corner, as the screens' flourishes close the corners */
        int x = i & 1 ? MENU_W - 10 : 5, y = i & 2 ? MENU_H - 10 : 5;

        rect(&a, x, y, 5, 5, dark);
        rect(&a, x + 1, y + 1, 3, 3, mid);
        dot(&a, x + 2, y + 2, light);
    }
    text(&a, title, PAL_PINK, (MENU_W - text_w(title) * 2) / 2, 10, 2);
    rect(&a, 20, 30, MENU_W - 40, 1, dark);
    rect(&a, 20, 31, MENU_W - 40, 1, mid);
}

void menu_art_text(uint32_t *px, int d, const char *s, int pal, int x, int y, int scale)
{
    Art a = { px, d };

    text(&a, s, pal, x, y, scale);
}

int menu_art_text_w(const char *s) { return text_w(s); }

void menu_art_hand(uint32_t *px, int d, int x, int y)
{
    Art a = { px, d };

    hand(&a, x, y);
}

void menu_art_box(uint32_t *px, int d, int x, int y, int w, int h, int bright)
{
    Art a = { px, d };

    menu_art_start();
    frame(&a, x, y, w, h, colour[PAL_GOLD][3], bright ? colour[PAL_CREAM][2] : colour[PAL_GOLD][2], bright ? colour[PAL_CREAM][1] : colour[PAL_GOLD][1]);
}

void menu_art_blit(uint32_t *px, int d, int x, int y, const uint32_t *src, int w, int h)
{
    Art a = { px, d };
    int i, j;

    for (j = 0; j < h; j++)
        for (i = 0; i < w; i++)
            rect(&a, x + i, y + j, 1, 1, src[j * w + i]);
}

void menu_art_static(uint32_t *px, int d)
{
    Art a = { px, d };
    uint32_t dark, mid;
    int i;

    menu_art_panel(px, d, "PAUSED");
    dark = colour[PAL_GOLD][3], mid = colour[PAL_GOLD][2];
    rect(&a, 20, 203, MENU_W - 40, 1, dark);
    rect(&a, 20, 204, MENU_W - 40, 1, mid);
    pad_draw(px, MENU_W * d, MENU_H * d, (float)d, PAD_K, PAD_CX, PAD_CY);
    for (i = 0; i < MENU_TAGS; i++) {   /* a line from each tag to its button: to its edge for a face button, to a dot on the rest */
        float sx, sy, sr, ex = menu_tags[i].right ? (float)TAG_X(1) : (float)(TAG_X(0) + TAG_W);
        float ey = (float)(TAG_Y(menu_tags[i].row) + TAG_H / 2), dx, dy, l, end;
        int face = (controls_buttons[menu_tags[i].button] & (PAD_A | PAD_B | PAD_X | PAD_Y)) != 0, n, k;

        spot_place(i, &sx, &sy, &sr);
        dx = sx - ex;
        dy = sy - ey;
        l = hypotf(dx, dy);
        end = face ? sr : 0;
        n = (int)(l - end);
        for (k = 0; k <= n; k++)
            dot(&a, (int)(ex + dx / l * (float)k), (int)(ey + dy / l * (float)k), mid);
        if (!face)
            rect(&a, (int)sx - 1, (int)sy - 1, 3, 3, mid);
    }
}

/* ---- the dynamic picture ---- */

static void line_dots(Art *a, float x0, float y0, float x1, float y1, uint32_t c)
{
    float l = hypotf(x1 - x0, y1 - y0);
    int n = (int)l, i;

    for (i = 0; i < n; i += 2)
        dot(a, (int)(x0 + (x1 - x0) * (float)i / l), (int)(y0 + (y1 - y0) * (float)i / l), c);
}

static void ring(Art *a, float cx, float cy, float r, uint32_t c)
{
    int x, y;

    for (y = (int)(cy - r - 2); y <= (int)(cy + r + 2); y++)
        for (x = (int)(cx - r - 2); x <= (int)(cx + r + 2); x++) {
            float dd = hypotf((float)x + 0.5f - cx, (float)y + 0.5f - cy);

            if (dd >= r && dd < r + 1.6f)
                dot(a, x, y, c);
        }
}

static void item_text(int item, const MenuView *v, char *out, size_t n)
{
    switch (item) {
    case MENU_RESUME: strncpy(out, "RESUME", n); break;
    case MENU_DEFAULTS: strncpy(out, "DEFAULTS", n); break;
    case MENU_SAVE: strncpy(out, "SAVE MENU", n); break;
    case MENU_FULL: strncpy(out, v->full_on ? "FULL SCREEN ON" : "FULL SCREEN OFF", n); break;
    default: strncpy(out, v->quit_asked ? "REALLY QUIT" : "QUIT", n); break;
    }
    out[n - 1] = 0;
}

static void item_place(int item, int *col, int *row)
{
    /* RESUME and DEFAULTS; SAVE MENU and FULL SCREEN; QUIT */
    *col = (item == MENU_DEFAULTS || item == MENU_FULL);
    *row = item >= MENU_QUIT ? 2 : item >= MENU_SAVE;
}

void menu_art_dynamic(uint32_t *px, int d, const MenuView *v)
{
    Art a = { px, d };
    uint32_t dark, mid, light;
    static const char hint[] = "B OR A CHOOSE   START OR ESC RESUME";
    char buf[24];
    int i;

    menu_art_start();
    dark = colour[PAL_GOLD][3], mid = colour[PAL_GOLD][2], light = colour[PAL_GOLD][1];
    for (i = 0; i < MENU_TAGS; i++) {
        int x = TAG_X(menu_tags[i].right), y = TAG_Y(menu_tags[i].row), on = v->chosen == i;
        const char *lab = v->asking == i ? "PRESS" : v->label[i];

        rect(&a, x, y, TAG_W, TAG_H, on ? 0xFF3A2010u : 0xB0000000u);
        frame(&a, x, y, TAG_W, TAG_H, dark, on ? light : mid, on ? light : dark);
        rect(&a, x + 3, y + 3, TAG_W - 6, TAG_H - 6, on ? 0xFF3A2010u : 0xFF100804u);
        text(&a, v->action[i], PAL_GOLD, x + (TAG_W - text_w(v->action[i])) / 2, y + 5, 1);
        text(&a, lab, PAL_CREAM, x + (TAG_W - text_w(lab)) / 2, y + 14, 1);
    }
    if (v->chosen < MENU_TAGS) {
        float sx, sy, sr, ex, ey;
        int right = menu_tags[v->chosen].right, row = menu_tags[v->chosen].row;

        spot_place(v->chosen, &sx, &sy, &sr);
        ex = right ? (float)TAG_X(1) : (float)(TAG_X(0) + TAG_W);
        ey = (float)(TAG_Y(row) + TAG_H / 2);
        line_dots(&a, ex, ey, sx, sy, light);
        ring(&a, sx, sy, sr + 2.0f, light);
        ring(&a, sx, sy, sr + 3.6f, mid);
    }
    text(&a, hint, PAL_CREAM, (MENU_W - text_w(hint)) / 2, HINT_Y, 1);
    for (i = MENU_RESUME; i < MENU_ITEMS; i++) {
        int col, row;

        if (i == MENU_FULL && !v->full_offered)
            continue;
        item_place(i, &col, &row);
        item_text(i, v, buf, sizeof buf);
        text(&a, buf, v->chosen == i ? PAL_CREAM : PAL_GOLD, ITEM_TEXT_X(col), ITEM_Y(row), 1);
        if (v->chosen == i)
            hand(&a, ITEM_HAND_X(col), ITEM_Y(row) - 8);
    }
}

int menu_art_item_at(float x, float y, int full_offered)
{
    int i, best = -1;
    float bd = 0;

    for (i = MENU_RESUME; i < MENU_ITEMS; i++) {
        int col, row;

        if (i == MENU_FULL && !full_offered)
            continue;
        item_place(i, &col, &row);
        if (x >= ITEM_HAND_X(col) && x < ITEM_HAND_X(col) + ITEM_W && y >= ITEM_Y(row) - 8 && y < ITEM_Y(row) - 8 + ITEM_H + 2)
            return i;
    }
    for (i = 0; i < MENU_TAGS; i++) {
        int tx = TAG_X(menu_tags[i].right), ty = TAG_Y(menu_tags[i].row);

        if (x >= tx && x < tx + TAG_W && y >= ty && y < ty + TAG_H)
            return i;
    }
    for (i = 0; i < MENU_TAGS; i++) {   /* a button on the controller: the nearest within its ring */
        float sx, sy, sr, dd;

        spot_place(i, &sx, &sy, &sr);
        dd = hypotf(x - sx, y - sy);
        if (dd <= sr + 2.0f && (best < 0 || dd < bd)) {
            best = i;
            bd = dd;
        }
    }
    return best;
}
