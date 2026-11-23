/*
 * mode7_source.c: what Mode7ScreenSetup reads. The original reads the 256
 * tiles and the 32 x 32 map back from video memory (the first 8,192 bytes
 * and the words from $4000); video/ takes them from the caller (video.h), so
 * the screens that put up a Mode 7 picture copy their graphics through
 * VramCopyKept, which keeps what lands in those places.
 */
#include <string.h>

#include "data/video_tables.h"
#include "game/game.h"
#include "system/state.h"
#include "video/video.h"
#include "system/runahead.h"

#define TILE_WORDS 0x1000           /* video words $0000-$0FFF: the 256 tiles */
#define MAP_AT 0x4000               /* the picture's map, 32 x 32 words */
#define MAP_WORDS 0x400

static uint8_t kept_tiles[2 * TILE_WORDS];
RUNAHEAD_KEEP(kept_tiles)
static uint8_t kept_map[2 * MAP_WORDS];
RUNAHEAD_KEEP(kept_map)

/* what part of entry e lies inside [lo, lo + n) words, into `into` */
static void keep(const VramCopyEntry *e, unsigned lo, unsigned n, uint8_t *into)
{
    unsigned from, words;

    if (e->to >= lo + n || e->to + e->words <= lo)
        return;
    from = e->to > lo ? 0 : lo - e->to;
    words = e->words - from;
    if (e->to + from + words > lo + n)
        words = lo + n - (e->to + from);
    memcpy(&into[2 * (e->to + from - lo)], &g.unpacked[e->from + 2 * from], 2 * words);
}

/*
 * VramCopyKept: VramCopy (table 0) or VramCopy2 (table 1) of an entry that
 * also keeps what it puts at video words $0000-$0FFF or $4000-$43FF.
 */
void VramCopyKept(unsigned table, int entry)
{
    VramCopyEntry e = vram_copy_entry(table, (unsigned)entry);

    keep(&e, 0, TILE_WORDS, kept_tiles);
    keep(&e, MAP_AT, MAP_WORDS, kept_map);
    if (table == 0)
        VramCopy(entry);
    else
        VramCopy2(entry);
}

/* Mode7ScreenSetup (81:AF3B, video_mode7_screen_setup) from what was kept. */
void Mode7ScreenSetupKept(uint8_t row_flag)
{
    video_mode7_screen_setup(row_flag, kept_tiles, kept_map);
}
