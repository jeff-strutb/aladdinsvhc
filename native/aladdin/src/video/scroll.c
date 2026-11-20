/*
 * scroll.c: the layers' positions as the NMI sends them to the picture.
 */
#include "system/state.h"
#include "video/video_internal.h"

static uint16_t layer_x[3], layer_y[3];

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

int32_t video_layer_x(int layer)
{
    return layer_x[layer];
}

int32_t video_layer_y(int layer)
{
    return layer_y[layer];
}
