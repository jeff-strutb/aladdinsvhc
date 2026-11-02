/*
 * spawn_at.c: a new actor of a kind at a place the caller puts in two
 * work RAM words, started at its next turn.
 */
#include "machine.h"
#include "ops65.h"
#include "actor.h"

#define SPAWN_X 0x002Cu    /* where the actor goes: x, then y (pixels, words) */
#define SPAWN_Y 0x002Eu
#define SPAWN_07 0x0030u   /* the byte it gets at $07, kept during the call */
#define ACTOR_NEW 0x0Cu    /* the state of a record that starts at its next turn */

/* Not yet filed: a free record of the first pool for kind A (its $06): X
   the record (16 bits), C clear; none: C set. */
#define ACTOR_ALLOC 0x82AA2Bu


