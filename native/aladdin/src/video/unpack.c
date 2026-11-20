/*
 * unpack.c: unpacking packed graphics into the unpack buffer (g.unpacked,
 * the original's bank $7F).
 *
 * Packed data is groups: a flag byte and a fill byte, then a byte for each
 * set flag bit; the group's 8 bytes take, from bit 7 down, the next of
 * those bytes for a set bit and the fill byte for a clear one. A length
 * unpacks in whole groups, rounded up.
 */
#include <stdio.h>

#include "data/data.h"
#include "system/state.h"
#include "video/video.h"

/* Unpacks `p` to g.unpacked at `to`; returns where the next unpack would
   go. One that would run past the buffer is reported and left out. */
static uint16_t unpack(PackedGraphics p, uint16_t to)
{
    if ((uint32_t)to + 8u * p.groups > sizeof g.unpacked) {
        fprintf(stderr, "video: an unpack of %u groups at %04X runs past the unpack buffer; left out\n", p.groups, to);
        return to;
    }
    return (uint16_t)(to + unpack_graphics(p, &g.unpacked[to]));
}

/*
 * @routine 81:BAF1 UnpackSet
 * WHAT IT DOES: Unpacks packed set `set` (graphics) into the unpack buffer
 * at the place its entry gives.
 * HOW: UnpackSetBegin (81:BB04), UnpackBegin (81:BB3D) and UnpackGroup
 * (81:BB49) are data/'s unpack_graphics and the set's entry.
 */
void UnpackSet(int set)
{
    UnpackSetEntry e = unpack_set((uint8_t)set);

    (void)unpack(e.packed, e.to);
}

/*
 * @routine 81:B92D LevelGraphicsLoad
 * WHAT IT DOES: Loads the level's graphics: each part for video memory
 * unpacked to the buffer's start and its bytes copied to its video
 * address; then the parts kept unpacked, one after another from $5800 of
 * the buffer, where the actors' graphics go on (g.unpack_next). With no
 * parts for video memory it does nothing more.
 * HOW: LevelGraphicsLoadEnd (81:B9F1), its shared way out, folded in.
 */
void LevelGraphicsLoad(void)
{
    unsigned n = level_vram_parts(g.level);

    if (n == 0)
        return;
    for (unsigned i = 0; i < n; i++) {
        LevelVramPart p = level_vram_part(g.level, i);

        (void)unpack(p.packed, 0x0000);
        vram_write(p.to, &g.unpacked[0], p.bytes / 2u, VRAM_WORDS);
    }
    g.unpack_next = 0x5800;
    n = level_kept_parts(g.level);
    for (unsigned i = 0; i < n; i++)
        g.unpack_next = unpack(level_kept_part(g.level, i), g.unpack_next);
}

/*
 * @routine 81:BA9E AnimSetGraphicsUnpack
 * WHAT IT DOES: Unpacks animation set `set`'s graphics into the unpack
 * buffer where the last unpack stopped (g.unpack_next), noting where they
 * went (g.kind_gfx_at); a set whose entry's word is 0 has none.
 */
void AnimSetGraphicsUnpack(uint8_t set)
{
    AnimSetGraphics a;

    g.kind_gfx_at[set] = g.unpack_next;
    a = anim_set_graphics(set);
    if (a.length)
        g.unpack_next = unpack(a.packed, g.unpack_next);
}

/*
 * @routine 81:BC16 UnpackTask
 * Dead: no task is created at it (the record: "Nothing found creates it");
 * the bytes $16 $BC $81 (its address, as a task's entry or a long pointer
 * names it) are nowhere in the cartridge.
 */
