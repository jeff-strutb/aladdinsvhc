/*
 * tile_blocks.c: queueing 16 x 16 blocks of map words for the frame's end
 * (g.tile_blocks: a video address and four map words, two at it and two a
 * map row, 32 words, on).
 */
#include <stdio.h>

#include "system/state.h"
#include "video/video.h"

/* the next record, or NULL (reported) when the queue's 16 are used */
static TileBlock *block_next(void)
{
    if (g.tile_blocks_count >= TILE_BLOCK_QUEUE_SIZE) {
        static int told;

        if (!told) {
            told = 1;
            fprintf(stderr, "video: the tile-block queue is full; a block left out\n");
        }
        return NULL;
    }
    return &g.tile_blocks[g.tile_blocks_count++];
}

static void block(uint16_t to, uint16_t a, uint16_t b, uint16_t c, uint16_t d)
{
    TileBlock *t = block_next();

    if (!t)
        return;
    t->to = to;
    t->words[0] = a;
    t->words[1] = b;
    t->words[2] = c;
    t->words[3] = d;
}

/*
 * @routine 81:A23C TileBlockQueueRun
 * WHAT IT DOES: Queues one 16 x 16 block at video address `to`: map words
 * t, t + 1 on top and t + 2, t + 3 under; the queue marked ready.
 */
void TileBlockQueueRun(uint16_t to, uint16_t t)
{
    block(to, t, (uint16_t)(t + 1), (uint16_t)(t + 2), (uint16_t)(t + 3));
    g.tile_blocks_pending++;
}

/*
 * @routine 81:A260 TileBlockQueueOne
 * WHAT IT DOES: Queues one 16 x 16 block at `to` from a sheet 16 tiles
 * wide: t, t + 1 on top, t + 16, t + 17 under; the queue marked ready.
 */
void TileBlockQueueOne(uint16_t to, uint16_t t)
{
    block(to, t, (uint16_t)(t + 1), (uint16_t)(t + 16), (uint16_t)(t + 17));
    g.tile_blocks_pending++;
}

/*
 * @routine 81:A287 TileBlockQueue
 * WHAT IT DOES: Queues four 16 x 16 blocks, two by two, at `to`, `to` + 2,
 * `to` + $40 and `to` + $42, from a sheet 16 tiles wide: each block t,
 * t + 1, t + 16, t + 17, t stepping by 2 from block to block; the queue
 * marked ready.
 */
void TileBlockQueue(uint16_t to, uint16_t t)
{
    static const uint16_t at[4] = { 0x00, 0x02, 0x40, 0x42 };

    for (int i = 0; i < 4; i++, t = (uint16_t)(t + 2))
        block((uint16_t)(to + at[i]), t, (uint16_t)(t + 1), (uint16_t)(t + 16), (uint16_t)(t + 17));
    g.tile_blocks_pending++;
}
