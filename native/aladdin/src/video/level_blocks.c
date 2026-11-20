/*
 * level_blocks.c: the level's blocks as it loads.
 */
#include <stdio.h>

#include "system/state.h"
#include "video/video.h"

/*
 * @routine 81:C025 BlockSourceAdd
 * WHAT IT DOES: While a level loads, lists one more block's source (a full
 * list is reported; the original checks no bound).
 */
void BlockSourceAdd(uint16_t source)
{
    if (g.block_sources_end >= BLOCK_SOURCES_SIZE) {
        fprintf(stderr, "video: the block sources' list is full\n");
        return;
    }
    g.block_sources[g.block_sources_end++] = source;
}
