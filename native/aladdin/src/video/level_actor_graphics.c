/*
 * level_actor_graphics.c: the graphics of the actor kinds a level places.
 */
#include <string.h>

#include "actor/actor.h"
#include "data/actor_tables.h"
#include "data/data.h"
#include "system/state.h"
#include "video/video.h"

/*
 * @routine 81:B9F4 LevelActorGraphicsLoad
 * WHAT IT DOES: Loads the graphics of every actor kind the level places:
 * every kind's set cleared (AnimSetsFill 0) and its graphics tables
 * ($7F:E500-$7F:EAFF) cleared; each placement's kind marked, its extra
 * palette slots noted, and (the first time) its palette byte by the
 * placement's variant (bits 5-6); then each marked kind's graphics unpacked
 * one after another from $7000 of the unpack buffer.
 */
void LevelActorGraphicsLoad(void)
{
    uint16_t level = (uint8_t)g.level;
    unsigned n = placement_count(level);

    AnimSetsFill(0);
    memset(g.kind_gfx_at, 0, sizeof g.kind_gfx_at);
    memset(g.kind_look, 0, sizeof g.kind_look);
    memset(g.kind_extra_slots, 0, sizeof g.kind_extra_slots);
    memset(g.kind_palette, 0, sizeof g.kind_palette);
    for (unsigned i = 0; i < n; i++) {
        Placement p = placement_at(level, i);
        uint8_t extra = 0;

        g.anim_set_of_kind[p.kind] = 1;
        (void)actor_graphics(p.kind, &extra);
        g.kind_extra_slots[p.kind] = extra;
        if (!g.kind_palette[p.kind])
            g.kind_palette[p.kind] = actor_graphics_variant_byte(p.kind, (uint8_t)(p.byte >> 5 & 3));
    }
    g.unpack_next = 0x7000;
    for (unsigned kind = 0; kind < 256; kind++)
        if (g.anim_set_of_kind[kind])
            AnimSetGraphicsUnpack((uint8_t)kind);
}
