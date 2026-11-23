/*
 * slot_gfx.c: the sprite palette and graphics slots past the original's four (slot_gfx.h).
 */
#include "data/actor_tables.h"
#include "video/slot_gfx.h"
#include "video/view.h"

/*
 * The slots a view has: the original's 4 at 256 x 224, and for a larger view
 * 4 scaled by its area, rounded up (as the sprite budget and the draw lists
 * are), at most SLOTS_MAX.
 */
unsigned slots_count(void)
{
    uint64_t area = (uint64_t)view.w * (uint64_t)view.h;
    uint64_t whole = (uint64_t)VIEW_ORIGINAL_W * VIEW_ORIGINAL_H;
    uint64_t n = (SLOTS_STD * area + whole - 1) / whole;

    return n > SLOTS_MAX ? SLOTS_MAX : (unsigned)n;
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
