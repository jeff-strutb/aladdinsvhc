/*
 * actor_84_misc.c: bank $84 actor helpers: a child spawned with a hit box
 * of $A2, an actor freed far out of range telling its parent, a chain of
 * followers, a clamp above the floor, the player's fall test, and a grab
 * test.
 */
#include "machine.h"
#include "ops65.h"
void abi_ActorAnimFreeFar(void);
void abi_ActorClampAboveFloor(void);
void abi_ActorFreeOutOfRangeLong(void);
void abi_ActorGrabTest(void);
void abi_ActorHitsTestBoxIfActiveLong(void);
void abi_ActorMoveAlong(void);
void abi_ActorSpawn(void);
void abi_ActorSpawnA2(void);
void abi_ActorStompOrHurtLong(void);
void abi_ActorsFollowSteps(void);
void abi_AnimStep(void);
void abi_PlayerEnter8CFacingLong(void);
void abi_PlayerFallCheck(void);
#include "actor.h"
#include "hw.h"
#include "wram.h"
#include "wram_player.h"
#include "wram_state.h"
#include "wram_video.h"






