/*
 * backdrop_offset.c: the backdrop layer's scroll taken from the play
 * field's, with an offset down.
 */
#include "system/state.h"
#include "video/parallax.h"

/*
 * @routine 81:CA6A BackdropFollowOffsetY
 * WHAT IT DOES: Moves the backdrop with the play field: the same scroll x
 * (its pixels), and its scroll y the play field's plus the backdrop's
 * offset.
 * HOW: The original adds 4 bytes from the fraction byte (two words with
 * the carry between): the offset's are g.backdrop_offset_y and
 * g.backdrop_offset_y_high; the scroll's fourth byte is read by nothing,
 * so the sum is kept to the scroll's 24 bits.
 */
void BackdropFollowOffsetY(void)
{
    ScrollLayer *play = &g.layers[0], *back = &g.layers[1];
    uint32_t offset = (uint32_t)g.backdrop_offset_y | (uint32_t)g.backdrop_offset_y_high << 16;

    back->scroll_x = (back->scroll_x & 0xFF0000FFu) | (play->scroll_x & 0x00FFFF00u);
    back->scroll_y = (play->scroll_y + offset) & 0x00FFFFFFu;
}
