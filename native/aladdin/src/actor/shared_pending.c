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

PENDING bool ActorInRange(const Actor *a, uint8_t at) { (void)a; (void)at; unconverted("ActorInRange"); return true; }
PENDING int ActorEndOutOfRange(Actor *a, uint8_t at) { (void)a; (void)at; unconverted("ActorEndOutOfRange"); return ACTOR_GOES_ON; }
PENDING bool ActorFreeOutOfRange(Actor *a, uint8_t at) { (void)a; (void)at; unconverted("ActorFreeOutOfRange"); return false; }
PENDING int ActorEndIfLeftBehind(Actor *a) { (void)a; unconverted("ActorEndIfLeftBehind"); return ACTOR_GOES_ON; }
PENDING int ActorEndFarWithChildren(Actor *a) { (void)a; unconverted("ActorEndFarWithChildren"); return ACTOR_GOES_ON; }

PENDING uint8_t ActorSideOfPlayer(const Actor *a) { (void)a; unconverted("ActorSideOfPlayer"); return 0; }
PENDING uint8_t ActorPlayerBelow(const Actor *a) { (void)a; unconverted("ActorPlayerBelow"); return 0; }
PENDING bool ActorNearPlayer(const Actor *a, uint8_t at) { (void)a; (void)at; unconverted("ActorNearPlayer"); return false; }
PENDING bool ActorNearPlayerX(const Actor *a, uint8_t at) { (void)a; (void)at; unconverted("ActorNearPlayerX"); return false; }
PENDING bool ActorNearPlayerY(const Actor *a, uint8_t at) { (void)a; (void)at; unconverted("ActorNearPlayerY"); return false; }
PENDING bool ActorNearPoint(const Actor *a, uint8_t at, uint16_t x, uint16_t y) { (void)a; (void)at; (void)x; (void)y; unconverted("ActorNearPoint"); return false; }
PENDING uint8_t ActorDirectionToPlayer(const Actor *a) { (void)a; unconverted("ActorDirectionToPlayer"); return 0; }
PENDING uint8_t ActorDirectionToPoint(const Actor *a, uint16_t x, uint16_t y) { (void)a; (void)x; (void)y; unconverted("ActorDirectionToPoint"); return 0; }
PENDING uint8_t ActorAimAt(const Actor *a, uint16_t x, uint16_t y) { (void)a; (void)x; (void)y; unconverted("ActorAimAt"); return 0; }
PENDING void ActorFacePlayer(Actor *a) { (void)a; unconverted("ActorFacePlayer"); }

PENDING bool ActorHitsTestBox(const Actor *a) { (void)a; unconverted("ActorHitsTestBox"); return false; }
PENDING bool ActorHitsTestBoxIfActive(const Actor *a) { (void)a; unconverted("ActorHitsTestBoxIfActive"); return false; }
PENDING bool ActorHitsTestBoxIfPlayerFree(const Actor *a) { (void)a; unconverted("ActorHitsTestBoxIfPlayerFree"); return false; }
PENDING bool ActorHurtsPlayerOnTouch(Actor *a) { (void)a; unconverted("ActorHurtsPlayerOnTouch"); return false; }
PENDING bool ActorStompOrHurt(Actor *a) { (void)a; unconverted("ActorStompOrHurt"); return false; }
PENDING bool ActorBouncesPlayer(Actor *a) { (void)a; unconverted("ActorBouncesPlayer"); return false; }
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
