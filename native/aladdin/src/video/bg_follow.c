/*
 * bg_follow.c: the BG positions taken from the scroll layers each frame.
 */
#include "system/state.h"
#include "video/video.h"

/* a BG position's pixels (its middle two bytes) set to a layer's */
static void follow(uint32_t *bg, uint32_t layer_position)
{
    *bg = (*bg & 0xFF0000FFu) | (layer_position >> 8 & 0xFFFFu) << 8;
}

/*
 * @routine 81:8568 BgFollowLayers
 * WHAT IT DOES: Takes this frame's BG1 and BG2 positions from the scroll
 * layers, as the level sets them to follow: BG1 the play field's x and y
 * unless held; BG2 by g.bg2_follows: $80 the play field's x, $40 the
 * backdrop's y, 0 the backdrop's x and y, anything else neither.
 * HOW: BgFollowLayersLong (81:8564), its call for other banks, is this.
 * Only the pixels are taken; the positions' fraction bytes stay.
 */
void BgFollowLayers(void)
{
    if (!g.bg1_held) {
        follow(&g.bg_x[0], g.layers[0].scroll_x);
        follow(&g.bg_y[0], g.layers[0].scroll_y);
    }
    switch (g.bg2_follows) {
    case 0x80:
        follow(&g.bg_x[1], g.layers[0].scroll_x);
        break;
    case 0x00:
        follow(&g.bg_x[1], g.layers[1].scroll_x);
        follow(&g.bg_y[1], g.layers[1].scroll_y);
        break;
    case 0x40:
        follow(&g.bg_y[1], g.layers[1].scroll_y);
        break;
    default:
        break;
    }
}
