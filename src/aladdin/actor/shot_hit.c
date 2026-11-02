/*
 * shot_hit.c: an actor hit by a shot: an actor of the second pool (the
 * five records from $09CB) that can hit, found by the test at $83:8578.
 * The shot is stopped, or turned back, and a small actor marks the hit.
 */
#include "machine.h"
#include "ops65.h"
#include "actor.h"

void abi_ActorStateSet8C(void);

#define ACTOR_11 0x11u              /* a shot's: negative, it does not hit */
#define TURNED_BACK_AT 0xEB7Cu      /* where a turned-back shot's code resumes (its own bank) */

/* Not yet filed */
#define SHOT_TEST 0x838578u         /* C clear: a shot overlaps the actor (D); Y = its record, X 16-bit */
#define SPAWN_11_AT_Y 0x83827Du     /* an actor of kind $11 started at record Y's place */
#define SPAWN_13_AT_Y 0x8382B5u     /* an actor of kind $13 started at record Y's place */
#define SHOT_HIT_80FC 0x8380FCu     /* the shot test, then (code no script has run) */






