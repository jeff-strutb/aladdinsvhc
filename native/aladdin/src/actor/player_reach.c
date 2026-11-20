/*
 * player_reach.c: whether the player is within an actor's reach, by an
 * entry of the reach table.
 */
#include "actor/actor_own.h"
#include "data/actor_a_tables.h"

/*
 * @routine 83:FCF5 PlayerInReach
 * WHAT IT DOES: Tells whether the player is within entry `at`'s reach of
 * the actor: his x within the across reach either side of its x, and his y
 * within the down reach below its y (a reach of 0 is not tested).
 */
bool PlayerInReach(const Actor *a, uint8_t at)
{
    ReachEntry r = reach_entry(at);
    const Actor *player = &g.actors[0];

    if (r.across && (uint16_t)(pixels(a->x) + r.across - pixels(player->x)) >= (uint16_t)(2 * r.across))
        return false;
    if (r.down && (uint16_t)(r.down + pixels(a->y) - pixels(player->y)) >= r.down)
        return false;
    return true;
}
