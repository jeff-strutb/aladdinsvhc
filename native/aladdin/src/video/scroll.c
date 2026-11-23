/*
 * scroll.c: the layers' positions as the NMI sends them to the picture.
 */
#include <stdio.h>
#include <string.h>

#include "system/state.h"
#include "video/between.h"
#include "video/video_internal.h"

static uint16_t layer_x[3], layer_y[3];
static int16_t sub_x[3], sub_y[3];      /* while a picture draws between updates: the eased places' fractions, in 256ths */

/* a position's pixels: the middle two bytes of its 24.8 */
static uint16_t pixels(uint32_t position)
{
    return (uint16_t)(position >> 8);
}

/*
 * @routine 80:8793 ScrollRegistersWrite
 * WHAT IT DOES: Sets BG1's, BG2's and BG3's scroll for the picture from the
 * game's positions: BG1 and BG2 with their shifts added, BG2's x not while
 * it is held (the picture keeps the last one sent), BG3 as it is.
 * HOW: Each sum low byte then high with the carry between, as the original
 * adds them. For BG2's x the original adds the shift's low byte to the high
 * byte too, not its high byte: kept.
 */
void video_scroll_latch(void)
{
    uint16_t x, lo;

    layer_x[0] = (uint16_t)(pixels(g.bg_x[0]) + g.bg1_x_shift);
    layer_y[0] = (uint16_t)(pixels(g.bg_y[0]) + g.layer_offset_y);
    if (!g.bg2_x_held) {
        x = pixels(g.bg_x[1]);
        lo = (uint16_t)((x & 0xFF) + (g.bg2_x_shift & 0xFF));
        layer_x[1] = (uint16_t)((lo & 0xFF) | (((x >> 8) + (g.bg2_x_shift & 0xFF) + (lo >> 8)) & 0xFF) << 8);
    }
    layer_y[1] = (uint16_t)(pixels(g.bg_y[1]) + g.bg2_y_shift);
    layer_x[2] = pixels(g.bg_x[2]);
    layer_y[2] = pixels(g.bg_y[2]);
}

/* the positions sent are the picture's: kept as the update before's world with the rest (between.h) */
void video_scroll_keep(void)
{
    between_keep(layer_x, sizeof layer_x);
    between_keep(layer_y, sizeof layer_y);
}

/* while a picture draws between updates (between.h): the positions sent eased towards the newest, each as it is sent
   (its pixels, with the shift or offset added) with its 24.8 place's fraction, so a shift or an offset that moves the
   layer eases as the camera does; BG2's x not while it is held; a layer that jumped further than 64 is shown where it
   was */
static uint16_t ease_layer(uint16_t was, uint32_t was_place, uint16_t now, uint32_t now_place, int16_t *sub)
{
    int64_t w = (int64_t)was << 8 | (was_place & 0xFF), n = (int64_t)now << 8 | (now_place & 0xFF);
    int64_t at = between_ease_fx(w, n, 24, 64 << 8);

    *sub = between_fractions() ? (int16_t)(at & 0xFF) : 0;
    return (uint16_t)(was + between_ease(w, n, 8, 24, 64));
}

void video_scroll_ease(void)
{
    const GameState *n = between_now();
    const uint16_t *nx = between_now_of(layer_x), *ny = between_now_of(layer_y);

    for (int i = 0; i < 3; i++) {
        sub_x[i] = sub_y[i] = 0;
        if (i != 1 || (!g.bg2_x_held && !n->bg2_x_held))
            layer_x[i] = ease_layer(layer_x[i], g.bg_x[i], nx[i], n->bg_x[i], &sub_x[i]);
        layer_y[i] = ease_layer(layer_y[i], g.bg_y[i], ny[i], n->bg_y[i], &sub_y[i]);
    }
}

/* the eased place's fraction of layer `layer`, in 256ths (0 but while a picture draws between updates) */
int32_t video_layer_x_sub(int layer)
{
    return between_easing() ? sub_x[layer] : 0;
}

int32_t video_layer_y_sub(int layer)
{
    return between_easing() ? sub_y[layer] : 0;
}

/* the developer's check (between.c): how many of the eased positions differ from the newest update's (at between 1
   they must not, but for a layer that jumped) or lie outside the update before's and the newest's (at any between) */
void video_scroll_check(float b, unsigned *checked, unsigned *wrong)
{
    const uint16_t *nx = between_now_of(layer_x), *ny = between_now_of(layer_y);
    uint16_t wx[3], wy[3];

    memcpy(wx, layer_x, sizeof wx);
    memcpy(wy, layer_y, sizeof wy);
    video_scroll_ease();
    for (int i = 0; i < 3; i++) {
        const uint16_t was[2] = { wx[i], wy[i] }, now[2] = { nx[i], ny[i] }, got[2] = { layer_x[i], layer_y[i] };

        for (int k = 0; k < 2; k++) {
            int32_t d = (int16_t)(uint16_t)(now[k] - was[k]), e = (int16_t)(uint16_t)(got[k] - was[k]);

            if (d > 64 || d < -64 || (k == 0 && i == 1 && (g.bg2_x_held || between_now()->bg2_x_held)))
                continue;               /* a jump, or BG2 held: not eased */
            (*checked)++;
            if (b >= 1.0f ? e != d : (d >= 0 ? e < 0 || e > d : e > 0 || e < d)) {
                static int shown;

                (*wrong)++;
                if (shown++ < 12)
                    fprintf(stderr, "between check: layer %d %c at %.1f: was %u now %u eased %u (bg %06X -> %06X)\n", i, "xy"[k],
                            (double)b, was[k], now[k], got[k], (unsigned)(k ? g.bg_y[i] : g.bg_x[i]),
                            (unsigned)(k ? between_now()->bg_y[i] : between_now()->bg_x[i]));
            }
        }
    }
    memcpy(layer_x, wx, sizeof wx);
    memcpy(layer_y, wy, sizeof wy);
}

int32_t video_layer_x(int layer)
{
    return layer_x[layer];
}

int32_t video_layer_y(int layer)
{
    return layer_y[layer];
}
