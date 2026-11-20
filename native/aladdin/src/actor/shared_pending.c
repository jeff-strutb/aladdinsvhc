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

PENDING void ActorFacePlayer(Actor *a) { (void)a; unconverted("ActorFacePlayer"); }

PENDING bool ActorZoneHasTestBox(const Actor *a, uint8_t zone_at) { (void)a; (void)zone_at; unconverted("ActorZoneHasTestBox"); return false; }
PENDING bool ActorZoneHasTestPoint(const Actor *a, uint8_t zone_at) { (void)a; (void)zone_at; unconverted("ActorZoneHasTestPoint"); return false; }
PENDING bool ActorZoneHurtsPlayer(Actor *a, uint8_t zone_at) { (void)a; (void)zone_at; unconverted("ActorZoneHurtsPlayer"); return false; }
PENDING Actor *ActorTouchesSecondPool(const Actor *a) { (void)a; unconverted("ActorTouchesSecondPool"); return NULL; }
PENDING ActorSolid ActorSolidForPlayer(Actor *a, uint8_t at) { (void)a; (void)at; unconverted("ActorSolidForPlayer"); return ACTOR_SOLID_BESIDE; }
PENDING void ActorCarryPlayer(Actor *a) { (void)a; unconverted("ActorCarryPlayer"); }

PENDING Actor *ActorDownedByShot(Actor *a) { (void)a; unconverted("ActorDownedByShot"); return NULL; }
PENDING Actor *ActorHitByShotCC(Actor *a) { (void)a; unconverted("ActorHitByShotCC"); return NULL; }
PENDING Actor *ActorTurnsShotBack(Actor *a) { (void)a; unconverted("ActorTurnsShotBack"); return NULL; }
PENDING Actor *ActorEndsShot(Actor *a) { (void)a; unconverted("ActorEndsShot"); return NULL; }
PENDING Actor *ActorDownedByHarmlessShot(Actor *a) { (void)a; unconverted("ActorDownedByHarmlessShot"); return NULL; }

PENDING void ActorPriorityClear(Actor *a) { (void)a; unconverted("ActorPriorityClear"); }
PENDING void Spawn11AtY(const Actor *at) { (void)at; unconverted("Spawn11AtY"); }
PENDING void Spawn13AtY(const Actor *at) { (void)at; unconverted("Spawn13AtY"); }

PENDING int ActorHopFromTable(Actor *a, uint8_t at) { (void)a; (void)at; unconverted("ActorHopFromTable"); return ACTOR_RETURNS; }
PENDING int ActorSpawnLookWait(Actor *a) { (void)a; unconverted("ActorSpawnLookWait"); return ACTOR_RETURNS; }
PENDING int ActorRiseWhenInReach(Actor *a, uint8_t at) { (void)a; (void)at; unconverted("ActorRiseWhenInReach"); return ACTOR_RETURNS_C; }
PENDING int ActorKnockedAway(Actor *a, uint8_t anim, uint8_t sound, uint8_t preset)
{
    (void)anim; (void)sound; (void)preset;
    unconverted("ActorKnockedAway");
    return ActorEndPlaced(a);
}
