/*
 * spawn_group.c: actors that spawn children around themselves: a ring of
 * five, a pair, a copy of themselves, a child of a set kind.
 */
#include "machine.h"
#include "ops65.h"
#include "actor.h"

void abi_ActorSpawnSameLook(void);

/* Spawns a child of kind A at the actor's spawn point when a record is
   free: A = 0 (Z set) when it did, C set and A nonzero when none was. */
#define SPAWN_KIND 0x81A9B1u
/* In bank $80 (DB): five x and y offsets (words, 4 bytes an entry) the
   ring's children are spawned at, each from the one before. */
#define RING_OFFSETS 0xD36Eu
#define RING_KIND 0x34
#define ACTOR_43 0x43u            /* here: the ring entry's offset */




