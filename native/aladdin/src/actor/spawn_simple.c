/*
 * spawn_simple.c: simple spawns.
 */
#include "actor/actor_own.h"

/*
 * @routine 81:A9E9 ActorSpawnShared
 * WHAT IT DOES: A child of `kind` at the spawner's spawn point, started at
 * its next turn and sharing the spawner's animated-tile slot, if a record
 * is free (ActorAlloc, then ActorSpawnTyped and ActorTileSlotShare: the
 * steps ActorSpawn takes); NULL when none is.
 * Also 81:A9EF ActorSpawnSharedAt, the part once the record is taken.
 */
Actor *ActorSpawnShared(Actor *spawner, uint8_t kind)
{
    return ActorSpawn(spawner, kind);
}
