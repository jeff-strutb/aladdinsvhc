/*
 * flash.c: an actor and its palette flashing while the chain's head is struck.
 */
#include "actor/actor_own.h"
#include "video/video.h"

/* the flash's phase: its low bit set, the flash shows (g.head_hit_frames, $1B29, counts down) */
#define PHASE (g.head_hit_frames & 1)

/*
 * @routine 84:8E3C ActorLookFlash
 * WHAT IT DOES: On odd counts of the flash countdown shows the actor's
 * sprites in palette 3; on even counts gives them back their kind's.
 */
void ActorLookFlash(Actor *a)
{
    uint8_t high;

    if (PHASE)
        high = (uint8_t)(((a->look >> 8) & 0xF1) | 0x06);
    else
        high = (uint8_t)(g.kind_look[a->kind] >> 8);
    a->look = (uint16_t)((a->look & 0x00FF) | high << 8);
}

/*
 * @routine 84:8E56 PaletteFlash
 * WHAT IT DOES: On odd counts of the flash countdown turns palette 2
 * white; on even counts gives it back its kept colours.
 */
void PaletteFlash(void)
{
    if (PHASE)
        PaletteWhite(2);
    else
        PaletteRestoreReversed(2);
}
