/*
 * children_start.c: actors starting their children.
 */
#include "actor/actor_a.h"

/*
 * @routine 83:D078 ActorVariantChildStep
 * WHAT IT DOES: For an actor with a variant in bits 2-4 of its $07: its
 * $07 becomes 8 plus that variant, its $14 counts up, and a child of kind
 * $30 starts at its spawn point $3E; when the child was created, its flag
 * (its own $41's mask) is set in g.opened_flags and its $14 counts back
 * down. No variant: its flag is set in g.opened_flags and
 * g.figures_touched counts up.
 */
void ActorVariantChildStep(Actor *a)
{
    uint8_t variant = a->spawn_byte & 0x1C;

    if (!variant) {
        g.opened_flags |= a->own.bytes[OWN_41];
        g.figures_touched++;
        return;
    }
    a->spawn_byte = (uint8_t)((variant >> 2) | 0x08);
    a->facing++;
    a->spawn_at = 0x3E;
    if (ActorCreateChild8(a, 0x30)) {
        g.opened_flags |= a->own.bytes[OWN_41];
        a->facing--;
    }
}
