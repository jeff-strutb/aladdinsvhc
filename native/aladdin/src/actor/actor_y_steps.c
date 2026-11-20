/*
 * actor_y_steps.c: an actor's height following its animation.
 */
#include "actor/actor_a.h"
#include "data/actor_a_tables.h"

/*
 * @routine 83:BDE9 ActorYByFrame47
 * WHAT IT DOES: When the actor's animation frame has changed since the
 * one kept in its own $47, moves it up or down by that frame's step (the
 * table at $80:D1CE), so its height follows the animation; then keeps the
 * frame.
 */
void ActorYByFrame47(Actor *a)
{
    if (a->frame != a->own.bytes[OWN_47])
        a->y = with_pixels(a->y, (uint16_t)(pixels(a->y) + frame_y_step(a->frame)));
    a->own.bytes[OWN_47] = a->frame;
}
