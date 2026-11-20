/*
 * escort.c: the level's escort actor, started beside the player.
 */
#include "actor/actor_own.h"
#include "data/actor_tables.h"

#define ESCORT_KIND 0x08            /* its kind: its hit box and its code */
#define ESCORT_LOOK 0x0220          /* its sprite attributes */

/*
 * @routine 81:85A1 EscortStart
 * WHAT IT DOES: Starts the level's escort actor (actors[1]) beside the
 * player, when the level has one: its +$07 the level's entry less one, new,
 * kind 8, look $0220, the player's x and y pixels.
 */
void EscortStart(void)
{
    uint8_t e = level_escort(g.level);
    Actor *a = &g.actors[1];

    if (!e)
        return;
    a->spawn_byte = (uint8_t)(e - 1);
    a->state = ACTOR_STATE_NEW;
    a->kind = ESCORT_KIND;
    a->look = ESCORT_LOOK;
    a->x = with_pixels(a->x, pixels(g.actors[0].x));
    a->y = with_pixels(a->y, pixels(g.actors[0].y));
}
