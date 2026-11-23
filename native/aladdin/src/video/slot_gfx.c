/*
 * slot_gfx.c: the sprite palette and graphics slots past the original's four (slot_gfx.h).
 */
#include "data/actor_tables.h"
#include "video/slot_gfx.h"
#include "video/view.h"

/*
 * The slots a view starts with: the original's 4 at 256 x 224, and for a
 * larger view 4 scaled by its area, rounded up (as the sprite budget and the
 * draw lists are), at most SLOTS_MAX.
 */
static unsigned slots_at_start(void)
{
    uint64_t area = (uint64_t)view.w * (uint64_t)view.h;
    uint64_t whole = (uint64_t)VIEW_ORIGINAL_W * VIEW_ORIGINAL_H;
    uint64_t n = (SLOTS_STD * area + whole - 1) / whole;

    return n > SLOTS_MAX ? SLOTS_MAX : (unsigned)n;
}

static unsigned slots_open;     /* the slots open in a larger view: its start, and those opened since */

unsigned slots_count(void)
{
    unsigned start = slots_at_start();

    if (start <= SLOTS_STD)
        return start;               /* the original's four (the view can change as a level loads) */
    if (slots_open < start) {
        slots_open = start;
        video_slot_sets_make(start);
    }
    return slots_open;
}

unsigned slots_open_now(void)
{
    return slots_open;
}

/* a snapshot's slots: as many open as it had (the view it brings has been put in) */
void slots_adopt(unsigned open)
{
    slots_open = open;
    if (open > SLOTS_STD)
        video_slot_sets_make(open);
}

/* More slots open in a larger view, so a kind never fails for want of one; the original's 4 stay at 256 x 224. A slot
   once open stays open: its palette and tiles are only read by the actors that claim it. */
int slots_grow(unsigned count)
{
    if (slots_at_start() <= SLOTS_STD || count > SLOTS_MAX)
        return 0;
    if (count > slots_count()) {
        slots_open = count;
        video_slot_sets_make(count);
    }
    return 1;
}

uint32_t slot_look(unsigned slot)
{
    if (slot <= SLOTS_STD)
        return palette_slot_look((uint8_t)slot);
    return 0x8000u | SLOT_PALETTE_BITS | (uint32_t)slot << 16;
}

uint32_t look_vram(uint32_t look, unsigned tile)
{
    unsigned slot = look_slot(look);

    if (slot)
        return VRAM_SLOT_TILES + 16u * (SLOT_TILES * (slot - 5) + tile);
    return (uint16_t)((((look & 0x01FF) + tile) << 4) + 0x6000);
}

uint32_t vram_add(uint32_t address, uint16_t step)
{
    return address >= VRAM_SLOT_TILES ? address + step : (uint16_t)(address + step);
}

unsigned look_palette_number(uint32_t look)
{
    unsigned bits = look >> 9 & 7;

    if (look_slot(look) && bits == 7)
        return 16 + look_slot(look) - 5;
    return 8 + bits;
}
