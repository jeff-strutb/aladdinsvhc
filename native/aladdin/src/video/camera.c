/*
 * camera.c: where the play field may scroll: empty blocks stop it. The
 * screen map (g.screen_map.places) marks the 256-pixel blocks the camera
 * may not show with a negative entry; after the scroll moves, the screen's
 * leading edge is checked against it, and over an empty block the scroll
 * goes back to the block's edge.
 */
#include <stdio.h>

#include "system/state.h"
#include "video/video.h"

/* the play field's scroll, whole pixels */
static uint16_t pixels(uint32_t position)
{
    return (uint16_t)(position >> 8);
}

/*
 * @routine 81:CB24 BlockAt
 * WHAT IT DOES: The screen map's entry for the 256-pixel block at (x, y) in
 * the level (negative: an empty block), the map read inside its border
 * (from its byte 66): column x / 256 (5 bits), row y / 256 (4 bits).
 */
int16_t BlockAt(uint16_t x, uint16_t y)
{
    unsigned at = 66u + (((x >> 8) << 1) & 0x3Eu) + ((y >> 2) & 0x3C0u);

    if (at / 2u >= SCREEN_MAP_PLACES) {
        static int told;

        if (!told) {
            told = 1;
            fprintf(stderr, "video: BlockAt reads the screen map at byte %u, past it\n", at);
        }
        return 0;
    }
    return (int16_t)g.screen_map.places[at / 2u];
}

/* the scroll back to a block's start: pixels' low byte and fraction 0, the
   pixels' high byte `block` (the byte past the 24 bits is read by nothing) */
static uint32_t block_start(uint8_t block)
{
    return (uint32_t)block << 16;
}

/*
 * @routine 81:CA84 CameraStopAcross
 * WHAT IT DOES: Stops the play field scrolling into an empty block across:
 * moving left, the blocks at the screen's left edge at its top and bottom
 * rows; moving right (or not), at its right edge (255 on). Over an empty
 * block the scroll X goes to the next block's start (left) or its block's
 * start (right), and g.camera_stopped_x counts it.
 * HOW: The bottom row's y moving left is the top's + $FF plus BlockAt's
 * carry (the original's ADC after it: bit 1 of the y); moving right + $FF.
 */
void CameraStopAcross(void)
{
    ScrollLayer *l = &g.layers[0];
    uint16_t x = pixels(l->scroll_x), y = g.scroll_y_before;

    g.camera_stopped_x = 0;
    if (x < g.scroll_x_before) {
        if (BlockAt(x, y) >= 0 && BlockAt(x, (uint16_t)(y + 0xFF + (y >> 1 & 1))) >= 0)
            return;
        l->scroll_x = block_start((uint8_t)((x >> 8) + 1));
    } else {
        uint16_t right = (uint16_t)(x + 0xFF);

        if (BlockAt(right, y) >= 0 && BlockAt(right, (uint16_t)(y + 0xFF)) >= 0)
            return;
        l->scroll_x = block_start((uint8_t)(x >> 8));
    }
    g.camera_stopped_x++;
}

/*
 * @routine 81:CADA CameraStopUpDown
 * WHAT IT DOES: Stops the play field scrolling into an empty block up or
 * down: moving up, the blocks at the screen's top edge at its left and right
 * columns; moving down (or not), at its bottom edge (255 down). Over an
 * empty block the scroll Y goes to the next block's start (up) or its
 * block's start (down), and g.camera_stopped_y counts it.
 */
void CameraStopUpDown(void)
{
    ScrollLayer *l = &g.layers[0];
    uint16_t x = pixels(l->scroll_x), y = pixels(l->scroll_y);

    if (y < g.scroll_y_before) {
        if (BlockAt(x, y) >= 0 && BlockAt((uint16_t)(x + 0xFF), y) >= 0)
            return;
        l->scroll_y = block_start((uint8_t)((y >> 8) + 1));
    } else {
        uint16_t bottom = (uint16_t)(y + 0xFF);

        if (BlockAt(x, bottom) >= 0 && BlockAt((uint16_t)(x + 0xFF), bottom) >= 0)
            return;
        l->scroll_y = block_start((uint8_t)(y >> 8));
    }
    g.camera_stopped_y++;
}
