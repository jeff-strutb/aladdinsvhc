/*
 * ground_probes.c: an actor's tests of the ground in front of it.
 */
#include "actor/actor_own.h"
#include "map/map.h"

/*
 * @routine A5:EB17 ActorOutOfWall8
 * WHAT IT DOES: Moves the actor out of a wall at its probe 8 in front:
 * true when it was moved; what the probe found in `p` (its tile's
 * behaviour, the original's $0012).
 */
bool ActorOutOfWall8(Actor *a, MapPoint *p)
{
    return ActorOutOfWallAt(a, 0x08, p) != 0;
}
