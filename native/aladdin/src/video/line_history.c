/*
 * line_history.c: a list of an actor's past heights, newest first, kept in
 * the text map's last 32 words (the original's $7E:DFC0) and sent with
 * them as upload 11.
 */
#include "system/state.h"
#include "video/video.h"

#define HISTORY_AT (32 * 31)        /* the text map's word $3E0: its last row */
#define HISTORY_WORDS 32
#define UPLOAD_LAST_ROWS 11         /* the text map's last two rows to the text layer */

/*
 * @routine 84:8EBD ActorYHistoryPush
 * WHAT IT DOES: Pushes actor `a`'s height into the list: the older 31
 * entries move down one, the new head is its y less 256, in 9 bits, with
 * bit 14 set; the y less 256 whole goes to the backdrop layer's scroll y
 * pixels (the original's $18CD); the upload is asked for.
 */
void ActorYHistoryPush(const Actor *a)
{
    uint16_t y = (uint16_t)((uint16_t)(a->y >> 8) - 0x0100);

    for (int i = HISTORY_WORDS - 1; i > 0; i--)
        g.text_map[HISTORY_AT + i] = g.text_map[HISTORY_AT + i - 1];
    g.layers[1].scroll_y = (g.layers[1].scroll_y & 0xFFu) | (uint32_t)y << 8;
    g.text_map[HISTORY_AT] = (uint16_t)((y & 0x01FF) | 0x4000);
    g.vram_upload_number = UPLOAD_LAST_ROWS;
}
