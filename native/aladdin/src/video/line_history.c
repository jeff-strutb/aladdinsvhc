/*
 * line_history.c: a list of an actor's past heights, newest first, kept in
 * the text map's last 32 words (the original's $7E:DFC0) and sent with
 * them as upload 11.
 *
 * Level $11 draws its lines below the status band in mode 2, whose BG2
 * columns take their vertical scroll from that row (offset-per-tile; the
 * line tables $80:B9C3 and $80:B9CA put BG3's y at $F0 there, so row 30
 * holds the horizontal entries and row 31 the vertical): the floor shows
 * the riser's past heights as a wave across the screen. The native game's
 * BG3 in a level is the status line's own map (hud_layer.c), and a wider
 * view has more columns than the row, so the entries are given to the
 * engine as a table of BG2's own (nshvc_layer.offset_per_tile_own, API
 * 6.14), in the terms of the map BG2 draws (in the arena the video map
 * $4800, 512 x 256; a whole-level map with its margin where one is drawn).
 * The heights are also kept whole in floor_heights, longer than the row, so
 * a view wider than the original's continues the wave to its right edge
 * (Jeff's decision, 2026-11-23). Left of the original's screen the floor
 * shows the heights still to come, as a copy of the riser flies on
 * (RiserFuture, actor/kind88.c): column -k the k-th next, so the wave comes
 * in from the left as it moves (Jeff, 2026-11-24: "an organic slither, not a
 * duplication"; the newest height there was flat, a mirror looked copied).
 * Past the heights that can be known (the riser at rest unsent, or a course a
 * later sweep picks) the last known one.
 */
#include <stdio.h>
#include <string.h>

#include "system/state.h"
#include "video/between.h"
#include "video/video.h"
#include "video/video_internal.h"
#include "video/view.h"

#define HISTORY_AT (32 * 31)        /* the text map's word $3E0: its last row */
#define HISTORY_WORDS 32
#define UPLOAD_LAST_ROWS 11         /* the text map's last two rows to the text layer */
#define ACROSS_AT (32 * 30)         /* the text map's word $3C0: the horizontal entries' row */
#define BG2_OFFSETS 0x4000          /* an entry's bit for BG2 */

/* the heights whole (y less 256, 16 bits), newest first: the row's 32 and as many more as the widest view's columns
   right of the original's screen need (native only) */
#define FLOOR_HEIGHTS (HISTORY_WORDS + (VIEW_WIDEST_W - VIEW_ORIGINAL_W) / 2 / 8 + 2)
static int16_t floor_heights[FLOOR_HEIGHTS];

/* BG2's offset table: a column for each offset column of the widest view, row 0 across, row 1 down */
#define TABLE_W (VIEW_WIDEST_W / 8 + 2)
static nshvc_map table;
static uint16_t table_words[2 * TABLE_W];

void line_history_init(void)
{
    between_keep(floor_heights, sizeof floor_heights);      /* the picture's, kept as the update before's (between.h) */
    table = nshvc_map_create(TABLE_W, 2);
}

/* Level11ScreenSetup: the row is made all heights 0 ($4000), and so is the list */
void LineHistoryClear(void)
{
    memset(floor_heights, 0, sizeof floor_heights);
}

/*
 * @routine 84:8EBD ActorYHistoryPush
 * WHAT IT DOES: Pushes actor `a`'s height into the list: the older 31
 * entries move down one, the new head is its y less 256, in 9 bits, with
 * bit 14 set; the y less 256 whole goes to the backdrop layer's scroll y
 * pixels (the original's $18CD); the upload is asked for. (Native: the
 * height also goes whole to the head of floor_heights.)
 */
void ActorYHistoryPush(const Actor *a)
{
    uint16_t y = (uint16_t)((uint16_t)(a->y >> 8) - 0x0100);

    for (int i = HISTORY_WORDS - 1; i > 0; i--)
        g.text_map[HISTORY_AT + i] = g.text_map[HISTORY_AT + i - 1];
    g.layers[1].scroll_y = (g.layers[1].scroll_y & 0xFFu) | (uint32_t)y << 8;
    g.text_map[HISTORY_AT] = (uint16_t)((y & 0x01FF) | 0x4000);
    g.vram_upload_number = UPLOAD_LAST_ROWS;
    memmove(&floor_heights[1], &floor_heights[0], (FLOOR_HEIGHTS - 1) * sizeof floor_heights[0]);
    floor_heights[0] = (int16_t)y;
}

static int32_t floordiv8(int32_t v) { return v >= 0 ? v / 8 : -((-v + 7) / 8); }

/*
 * line_history_offsets: on level $11, BG2's layer `l` (its scroll already in the map's terms, video.c draw_layers) is
 * given the table of its offsets; `orig_x` is BG2's x as the original sends it, `bg3_x` BG3's; the map BG2 draws has
 * the level's top at `margin_y` and is `tall` pixels high. Offset column c of the
 * engine (view pixels 8c - f to 8c - f + 7, f the scroll's fine part; the view's origin is 0 in a level) reads table
 * column c - 1, which holds the original's column o for the same pixels (o = (x - dx + its fine part) / 8): o 1-32 the
 * row's entries, whole; o past 32 the older heights (while the row's last entry is on); o 0 none (the layer's own scroll, the newest height); o -1 and
 * less, left of the original's screen, the riser's next heights (RiserFuture), -o - 1 of them on.
 */
void line_history_offsets(nshvc_layer *l, int32_t orig_x, int32_t bg3_x, int32_t margin_y, int32_t tall)
{
    static int told_across, told_list, told_tall;
    int32_t f = l->scroll_x & 7, h7 = orig_x & 7, shift = bg3_x >> 3;

    if (tall <= 0 || tall > 0x400) {        /* an entry holds 10 bits: a taller map could not be given its rows */
        if (!told_tall) {
            told_tall = 1;
            fprintf(stderr, "video: level $11's BG2 map is %d pixels tall: its offsets are not drawn\n", (int)tall);
        }
        return;
    }
    for (int i = 0; i < HISTORY_WORDS && !told_list; i++)
        if ((g.text_map[HISTORY_AT + i] & BG2_OFFSETS) && (uint16_t)(floor_heights[i] & 0x1FF) != (g.text_map[HISTORY_AT + i] & 0x1FF)) {
            told_list = 1;
            fprintf(stderr, "video: level $11's heights list and the text map's row differ at word %d\n", i);
        }
    {
        static int16_t future[TABLE_W];
        int nf = view.dx > 0 ? RiserFuture(future, TABLE_W) : 0;
        int on = (g.text_map[HISTORY_AT] & BG2_OFFSETS) != 0;      /* the newest entry on */

        for (int32_t j = 0; j < TABLE_W; j++) {
            int32_t o = floordiv8(8 * (j + 1) - f - view.dx + h7);
            int32_t k = -o - 1 - shift;                     /* left of the original's screen: the k-th next height */
            int16_t h;

            if (o > -1)
                continue;
            h = k < 0 ? floor_heights[-k - 1 < FLOOR_HEIGHTS ? -k - 1 : 0] : nf ? future[k < nf ? k : nf - 1] : floor_heights[0];
            table_words[j] = 0;
            table_words[TABLE_W + j] = on ? (uint16_t)((((h + margin_y) % tall + tall) % tall) | BG2_OFFSETS) : 0;
        }
    }
    for (int32_t j = 0; j < TABLE_W; j++) {
        int32_t o = floordiv8(8 * (j + 1) - f - view.dx + h7);      /* the original's offset column for table column j */
        uint16_t across = 0, down = 0;

        if (o <= -1)
            continue;                   /* left of the original's screen: made above */

        if (o >= 1) {
            int32_t age = o - 1 + shift;
            /* past the row the older heights, while the row's last entry is on (a restart clears the text map, $2020: the
               row's columns are then the newest height, and so are these) */
            int on = (g.text_map[HISTORY_AT + HISTORY_WORDS - 1] & BG2_OFFSETS) != 0;

            if (o <= HISTORY_WORDS) {
                uint16_t h = g.text_map[ACROSS_AT + (age & 31)];

                age &= 31;
                on = (g.text_map[HISTORY_AT + age] & BG2_OFFSETS) != 0;
                /* the horizontal entries: the same picture as none when they give BG2's x in whole characters, in its
                   512-pixel map (the original's are all $4000 with BG2 at 512); any other is not drawn, and told */
                if ((h & BG2_OFFSETS) && (((h & 0x3F8) - (orig_x & ~7)) & 0x1FF) && !told_across) {
                    told_across = 1;
                    fprintf(stderr, "video: level $11's horizontal offsets move BG2 (%04X at x %d): not drawn\n", h, (int)orig_x);
                }
            }
            if (on && age < FLOOR_HEIGHTS) {
                int32_t v = ((floor_heights[age] + margin_y) % tall + tall) % tall;

                down = (uint16_t)(v | BG2_OFFSETS);
            }
        }
        table_words[j] = across;
        table_words[TABLE_W + j] = down;
    }
    nshvc_map_write_snes(table, 0, 0, TABLE_W, 2, table_words);
    l->offset_per_tile = table;
    l->offset_per_tile_own = 1;
}
