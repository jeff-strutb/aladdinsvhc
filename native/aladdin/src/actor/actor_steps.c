/*
 * actor_steps.c: small steps of an actor's code.
 */
#include "actor/actor_a.h"
#include "data/actor_a_tables.h"

/*
 * @routine 83:CF10 ActorFlagOnce
 * WHAT IT DOES: Each of eight such actors (by the low 3 bits of its $07)
 * has a flag in g.level_flags: when its flag is set already, it is set
 * again; else the case by bits 3-4 of $07 runs (flag_drops.c), which
 * drops its children and sets the flag. Its flag's mask (bit n, the table
 * at $80:D316) is kept in its own $41.
 */
void ActorFlagOnce(Actor *a)
{
    uint8_t mask = (uint8_t)(1u << (a->spawn_byte & 0x07));

    a->own.bytes[OWN_41] = mask;
    if (g.level_flags & mask)
        g.level_flags |= mask;
    else
        flag_drop_case(a);
}

/*
 * @routine 83:F031 ActorYByFrame
 * WHAT IT DOES: When the actor's animation frame has changed, moves it up
 * or down by that frame's step (the table at $80:D673), so its height
 * follows the animation; the frame kept in its own $49.
 */
void ActorYByFrame(Actor *a)
{
    if (a->frame != a->own.bytes[OWN_49])
        a->y = with_pixels(a->y, (uint16_t)(pixels(a->y) + frame_y_step_83(a->frame)));
    a->own.bytes[OWN_49] = a->frame;
}
