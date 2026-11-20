/*
 * shared_pending.c: stand-ins for the shared routines actor.h declares
 * that actors A has not converted yet, so the kinds' code that calls them
 * links. Each is weak: the converted routine replaces it at link time, and
 * its line here goes when it lands. Each prints its name once to stderr
 * and answers "none": in range, not near, no touch, no shot, back to its
 * caller; one that never returns ends its actor.
 */
#include "actor/actor_own.h"

#define PENDING __attribute__((weak))

PENDING int ActorEndIfLeftBehind(Actor *a) { (void)a; unconverted("ActorEndIfLeftBehind"); return ACTOR_GOES_ON; }
PENDING int ActorEndFarWithChildren(Actor *a) { (void)a; unconverted("ActorEndFarWithChildren"); return ACTOR_GOES_ON; }





PENDING int ActorHopFromTable(Actor *a, uint8_t at) { (void)a; (void)at; unconverted("ActorHopFromTable"); return ACTOR_RETURNS; }
PENDING int ActorSpawnLookWait(Actor *a) { (void)a; unconverted("ActorSpawnLookWait"); return ACTOR_RETURNS; }
PENDING int ActorRiseWhenInReach(Actor *a, uint8_t at) { (void)a; (void)at; unconverted("ActorRiseWhenInReach"); return ACTOR_RETURNS_C; }
