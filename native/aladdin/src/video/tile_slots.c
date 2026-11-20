/*
 * tile_slots.c: the four sprite palette slots actor kinds claim, and the
 * kind's palette put in its slot's place in the palette. (The record names
 * them tile slots and the palette its "tile copy"; their copies are 16
 * colours into $7E:FD00 + $100 on, the sprite palettes.)
 */
#include "actor/actor.h"
#include "data/actor_tables.h"
#include "data/video_tables.h"
#include "system/state.h"
#include "video/video.h"

/*
 * @routine 82:AB77 TileSlotFill
 * WHAT IT DOES: Puts the kind's palette (its kind_palette'th of the actors'
 * palettes) into the sprite palette its look names (bits 9-11), and has the
 * palette sent.
 * HOW: TileSlotFillLong (82:AB6F), its call for other banks, is this.
 */
void TileSlotFill(uint8_t kind)
{
    unsigned to = 128u + 16u * (g.kind_look[kind] >> 9 & 7u);

    for (unsigned i = 0; i < 16; i++)
        g.palette[to + i] = actor_palette_colour(g.kind_palette[kind], i);
    g.palette_upload++;
}

/*
 * @routine 82:ABCE TileSlotClaim
 * WHAT IT DOES: Claims a free palette slot for `kind` and actor `a`, tried
 * from 4 down to 1: the slot marked used by the kind, its number to the
 * actor and the kind, its look to the actor's and the kind's. 0 claimed,
 * -1 when all four are taken.
 */
int TileSlotClaim(uint8_t kind, Actor *a)
{
    for (int x = 3; x >= 0; x--) {
        uint16_t look;

        if (g.palette_slot_users[x])
            continue;
        g.palette_slot_users[x]++;
        g.palette_slot_kind[x] = kind;
        a->tile_slot = (uint8_t)(x + 1);
        g.kind_slot[kind] = (uint8_t)(x + 1);
        look = palette_slot_look((uint8_t)(x + 1));
        a->look = look;
        g.kind_look[kind] = look;
        return 0;
    }
    return -1;
}
