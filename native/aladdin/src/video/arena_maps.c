/*
 * arena_maps.c: level $11's arena drawn from maps wider than the console's
 * (Jeff's decision, 2026-11-23: the snake's floor across a wide view, with
 * no seam).
 *
 * In the fight BG1 draws the video map $4000 and BG2 $4800 (kind $81:
 * $41 and $49), each 64 x 32 cells, and the original fills only the
 * console's one screen of them with the fight's floor: BG2's band of the
 * snake's body (the play field frozen at the arena's lock) and BG1's rows
 * that upload $0A writes (the flames, rows 20-31 of the first screen). The
 * other screen still holds the corridor, which a wide view showed beside
 * the arena. Here each of the two maps is copied into a map of ARENA_COLS
 * columns: the arena's 32 columns as the console has them, then each floor
 * row continued by its own repeat (measured in the arena's columns 8-31),
 * and every other row as the console's map repeats (every 64 columns).
 * ARENA_COLS is a multiple of 64 and of every repeat met (4 and 12), so the
 * map wraps with no seam at any view's width.
 */
#include <stdio.h>
#include <string.h>

#include "data/video_tables.h"
#include "system/runahead.h"
#include "system/state.h"
#include "video/between.h"
#include "video/level_maps.h"
#include "video/video.h"
#include "video/video_internal.h"
#include "video/view.h"

#define ARENA_COLS 192              /* 3 x 64, 16 x 12 */
#define CONSOLE_COLS 64
#define ROWS 32
#define SCREEN_COLS 32              /* the console's screen */
#define FLOOR_UPLOAD 0x0A           /* the fight's floor on BG1 (kind $81's flash) */
#define LONGEST_REPEAT 16
#define MEASURED_FROM 8             /* the arena's columns a repeat is measured on: 8-31 (BG2's band has art that does not repeat in
                                       columns 3 and 4 of its rows 24 and 25, kept where it is) */

static uint16_t console_words[2][CONSOLE_COLS * ROWS];      /* $4000 and $4800 as written, row by row (kept: between.h) */
static uint8_t changed[2];
RUNAHEAD_KEEP(changed)
static nshvc_map arena[2];

void arena_maps_init(void)
{
    between_keep(console_words, sizeof console_words);
    arena[0] = nshvc_map_create(ARENA_COLS, ROWS);
    between_map_made(arena[0], "map.arena_back", ARENA_COLS, ROWS);
    arena[1] = nshvc_map_create(ARENA_COLS, ROWS);
    between_map_made(arena[1], "map.arena_front", ARENA_COLS, ROWS);
}

/* a write of `w` x `h` cells at (x, y) to video map `map` (video.c), as its words */
void arena_maps_note(VideoMap map, uint32_t x, uint32_t y, uint32_t w, uint32_t h, const uint16_t *words)
{
    int k;

    if (map != VIDEO_MAP_BACK && map != VIDEO_MAP_FRONT)
        return;
    k = map == VIDEO_MAP_FRONT;
    for (uint32_t r = 0; r < h && y + r < ROWS; r++)
        memcpy(&console_words[k][(y + r) * CONSOLE_COLS + x], words + r * w, w * sizeof *words);
    changed[k] = 1;
}

/* the first column of the arena's screen in the console's maps: its lock's x (kind $81) */
static uint32_t arena_column(void)
{
    return (ARENA11_X >> 3) & (CONSOLE_COLS - 1);
}

static void rebuild(int k)
{
    static uint16_t out[ARENA_COLS * ROWS];
    static int told;
    NumberedUpload u = numbered_upload(FLOOR_UPLOAD);
    uint32_t first = ((uint32_t)(u.to - 0x4000) >> 5) & (ROWS - 1), rows = (uint32_t)u.bytes / 2u / SCREEN_COLS;
    uint32_t s = arena_column();

    for (uint32_t r = 0; r < ROWS; r++) {
        const uint16_t *row = &console_words[k][r * CONSOLE_COLS];
        uint16_t a[SCREEN_COLS];
        int floor_row = k == 1 || (r >= first && r < first + rows);
        uint32_t p = 0;

        for (uint32_t c = 0; c < SCREEN_COLS; c++)
            a[c] = row[(s + c) & (CONSOLE_COLS - 1)];
        for (uint32_t q = 1; floor_row && !p && q <= LONGEST_REPEAT; q++) {
            uint32_t c = MEASURED_FROM;

            while (c + q < SCREEN_COLS && a[c] == a[c + q])
                c++;
            if (c + q >= SCREEN_COLS)
                p = q;
        }
        if (p && ARENA_COLS % p) {
            if (!told) {
                told = 1;
                fprintf(stderr, "video: level $11's arena row %u repeats every %u columns, not a divisor of %u: drawn as the console's\n",
                        (unsigned)r, (unsigned)p, ARENA_COLS);
            }
            p = 0;
        }
        for (uint32_t j = 0; j < ARENA_COLS; j++) {
            /* outside the arena's screen a row with no repeat is the level's own map there (the backdrop for $4000, the play field for $4800,
               both scrolled as the camera in level $11), margins included: the console's ring holds only what the original streamed, and a
               column its window never showed holds nothing (a flat column 32 pixels left of the frame at 398 wide) */
            int32_t wc = (int32_t)(ARENA11_X >> 3) + (j < ARENA_COLS / 2 ? (int32_t)j : (int32_t)j - ARENA_COLS);
            uint16_t lw;

            out[r * ARENA_COLS + j] = j < SCREEN_COLS ? a[j]
                                    : p ? a[MEASURED_FROM + (j - MEASURED_FROM) % p]
                                    : level_maps_word(k ? 0 : 1, wc, (int32_t)r, &lw) ? lw
                                        : row[(s + j) & (CONSOLE_COLS - 1)];
        }
    }
    between_map_write_snes(arena[k], 0, 0, ARENA_COLS, ROWS, out);
}

/* the level's whole maps were made or made again (a load, a resize): the arena's columns past its screen are read from them, so both are made
   again at the update's end (on level $11) */
void arena_maps_level_maps_made(void)
{
    changed[0] = changed[1] = 1;
}

/* at the update's end (video_update_end): on level $11 the arena's maps made again from the console's maps an update changed */
void arena_maps_update_end(void)
{
    for (int k = 0; k < 2; k++)
        if (changed[k]) {
            changed[k] = 0;
            if ((uint8_t)g.level == 0x11)
                rebuild(k);
        }
}

/*
 * arena_maps_layer: on level $11 in a level's view, a layer `i` (its scroll the console's, less the view's dx) that draws the
 * video map $4000 or $4800 is given that map's arena map, its scroll moved so the arena's first column is where the console's
 * was; 0 (nothing changed) else. In the fight BG1 is on $4000 and BG2 on $4800; from the arena's lock to the fight's set-up
 * the layers are on $4000 alone, which the console's ring leaves with a column the original never wrote (a flat column 32
 * pixels left of the frame at 398 wide).
 */
int arena_maps_layer(int i, nshvc_layer *l)
{
    if (i > 1 || !video_level_view() || (uint8_t)g.level != 0x11 || (g.bg_map[i] != VIDEO_MAP_BACK && g.bg_map[i] != VIDEO_MAP_FRONT))
        return 0;
    l->map = arena[g.bg_map[i] == VIDEO_MAP_FRONT];
    l->scroll_x -= ARENA11_X;           /* the layer's x ARENA11_X shows the arena map's column 0 (the console's arena_column()) */
    return 1;
}
