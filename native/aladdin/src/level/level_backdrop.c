/*
 * level_backdrop.c: what a level does to its backdrop layer as it starts
 * (the table by level at $81:8E0E): most levels nothing; level 6 has the
 * backdrop draw no edges; levels 7 and $17 also clear video memory's
 * $3800-$47FF.
 */
#include "level/level.h"
#include "system/state.h"
#include "video/level_maps.h"
#include "video/video.h"

#define BACKDROP_CLEAR_AT 0x3800    /* the video word address cleared */
#define BACKDROP_CLEAR_WORDS 0x1000

/*
 * @routine 81:8E06 LevelBackdropStart
 * WHAT IT DOES: The level's own start for its backdrop layer: none for
 * most levels (each of levels 0-5 has its own empty routine, the rest
 * share one); for level 6 the backdrop draws no new edges as it scrolls;
 * for levels 7 and $17, $1000 zero words to video memory from $3800 first.
 */
void LevelBackdropStart(void)
{
    switch ((uint8_t)g.level) {
    case 0x07:
    case 0x17:
        vram_fill(BACKDROP_CLEAR_AT, 0, BACKDROP_CLEAR_WORDS);
        LevelMapsBackdropOff();         /* the backdrop is the picture uploaded into BACK_TALL, not the level's map */
        /* fall through */
    case 0x06:
        g.layers[1].edges = 0;
        break;
    default:
        break;
    }
}
