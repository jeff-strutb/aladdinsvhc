/*
 * wanted_e.c: stand-ins for the routines of other areas that kinds
 * E's actors call, until their owners' code lands (each asked for under the
 * owner's header's wanted lines: actor.h, video.h). Each is weak, so the
 * owner's definition replaces it at link time; each prints its name once
 * and does the least it can: no picture sent. Delete a line when its owner lands.
 */
#include <stdio.h>

#include "actor/actor_own.h"
#include "level/level.h"
#include "player/player.h"

#define TELL_ONCE(name) do { static int told; if (!told++) fprintf(stderr, "%s not converted (actor/wanted_e.c stand-in)\n", name); } while (0)







__attribute__((weak)) void ActorStepCarry(Actor *a)
{
    (void)a;
    TELL_ONCE("ActorStepCarry (83:C1EC)");
}

__attribute__((weak)) void ActorStepToTargetCarry(Actor *a)
{
    (void)a;
    TELL_ONCE("ActorStepToTargetCarry (83:C1F6)");
}

__attribute__((weak)) void ActorLiftPlayerStep(Actor *a, uint8_t offset)
{
    (void)a;
    (void)offset;
    TELL_ONCE("ActorLiftPlayerStep (83:FB57)");
}

__attribute__((weak)) uint8_t LevelFlagFromMask(uint8_t flag)
{
    TELL_ONCE("LevelFlagFromMask (83:C2F0)");
    return g.level_switch_flags[flag & 7];
}

__attribute__((weak)) int LevelFlagSpotFind(uint16_t x, uint16_t y)
{
    (void)x;
    (void)y;
    TELL_ONCE("LevelFlagSpotFind (83:C2A9)");
    return -1;
}

__attribute__((weak)) int ActorGoToPoint(Actor *a)
{
    (void)a;
    TELL_ONCE("ActorGoToPoint (83:FC5D)");
    return ACTOR_RETURNS;
}

__attribute__((weak)) bool ActorProbeAheadInGround(const Actor *a, uint16_t ahead, uint16_t up)
{
    (void)a;
    (void)ahead;
    (void)up;
    TELL_ONCE("ActorProbeAheadInGround (83:FD6D)");
    return false;
}

__attribute__((weak)) void PlayerSpotBelowActor(const Actor *a, uint8_t offset)
{
    (void)a;
    (void)offset;
    TELL_ONCE("PlayerSpotBelowActor (82:BB9C)");
}

__attribute__((weak)) void PlayerToActorBelow(const Actor *a, uint8_t offset)
{
    (void)a;
    (void)offset;
    TELL_ONCE("PlayerToActorBelow (82:BB26)");
}

__attribute__((weak)) void ActorSpawn5E(Actor *a)
{
    (void)a;
    TELL_ONCE("ActorSpawn5E (83:D7C9)");
}

__attribute__((weak)) void ActorSpawnCopy(Actor *a)
{
    (void)a;
    TELL_ONCE("ActorSpawnCopy (83:D7D4)");
}
