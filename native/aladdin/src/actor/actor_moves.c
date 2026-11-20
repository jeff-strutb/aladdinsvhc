/*
 * actor_moves.c: an actor's place kept, and moves along a direction.
 */
#include "actor/actor_own.h"
#include "data/actor_a_tables.h"

/*
 * @routine 81:9D56 ActorKeepSpot
 * WHAT IT DOES: Keeps the actor's place: its x and y pixels into its own
 * $4C-$4D and $4E-$4F.
 */
void ActorKeepSpot(Actor *a)
{
    uint16_t x = pixels(a->x), y = pixels(a->y);

    a->own.bytes[OWN_4C] = (uint8_t)x;
    a->own.bytes[OWN_4D] = (uint8_t)(x >> 8);
    a->own.bytes[OWN_4E] = (uint8_t)y;
    a->own.bytes[OWN_4F] = (uint8_t)(y >> 8);
}

/*
 * @routine 81:A73D ActorMoveAlong
 * WHAT IT DOES: Moves the actor along direction `direction` (0-$1FF) of the
 * direction table, at `speed`.
 * Also 81:A748 ActorMoveScaled, its second half: the step's low byte times
 * the speed, its middle byte times it plus the first product's high byte (8
 * bits, no carry on), and its high byte as it is, added to the position,
 * across and down.
 */
void ActorMoveAlong(Actor *a, uint16_t direction, uint8_t speed)
{
    DirectionStep s = direction_step(direction);
    uint16_t low_x = (uint16_t)(speed * s.x[0]), mid_x = (uint16_t)(speed * s.x[1]);
    uint16_t low_y = (uint16_t)(speed * s.y[0]), mid_y = (uint16_t)(speed * s.y[1]);
    uint32_t step_x = (uint32_t)s.x[2] << 16 | (uint32_t)(uint8_t)(mid_x + (low_x >> 8)) << 8 | (low_x & 0xFF);
    uint32_t step_y = (uint32_t)s.y[2] << 16 | (uint32_t)(uint8_t)(mid_y + (low_y >> 8)) << 8 | (low_y & 0xFF);

    a->x = wrap24(a->x + step_x);
    a->y = wrap24(a->y + step_y);
}
