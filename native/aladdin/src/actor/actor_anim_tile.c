/*
 * actor_anim_tile.c: an actor's animation by the tile under it and its
 * speed.
 */
#include "actor/actor_own.h"

/*
 * @routine A5:E9D2 ActorAnimByTile
 * WHAT IT DOES: Picks the actor's animation: $16 on a tile whose behaviour
 * (`behaviour`, the last ground test's, the original's $0012) has bit 5;
 * else by its speed across (its low word): 0 standing; moving, 1 or 2 by
 * the speed's side of $1C0 once it is 16 or more from it; near $1C0,
 * nothing when it plays animation 2 or 1, else 1.
 */
void ActorAnimByTile(Actor *a, uint8_t behaviour)
{
    uint16_t speed = (uint16_t)(a->speed_x & 0xFFFF), off;

    if (behaviour & 0x20) {
        ActorAnimChange(a, 0x16);
        return;
    }
    if (!speed) {
        ActorAnimChange(a, 0x00);
        return;
    }
    off = (uint16_t)(speed - 0x01C0);
    if ((uint16_t)(off + 0x10) >= 0x20) {
        ActorAnimChange(a, (off & 0x8000) ? 0x01 : 0x02);
        return;
    }
    if (a->anim == 0x04 || a->anim == 0x02)
        return;
    ActorAnimChange(a, 0x01);
}
