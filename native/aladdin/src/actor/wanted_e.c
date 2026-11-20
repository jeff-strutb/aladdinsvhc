/*
 * wanted_e.c: stand-ins for the routines of other areas that kinds
 * E's actors call, until their owners' code lands (each asked for under the
 * owner's header's wanted lines: actor.h, video.h). Each is weak, so the
 * owner's definition replaces it at link time; each prints its name once
 * and does the least it can: no picture sent. Delete a line when its owner lands.
 */
#include <stdio.h>

#include "actor/actor_own.h"

#define TELL_ONCE(name) do { static int told; if (!told++) fprintf(stderr, "%s not converted (actor/wanted_e.c stand-in)\n", name); } while (0)









__attribute__((weak)) void ActorLiftPlayerStep(Actor *a, uint8_t offset)
{
    (void)a;
    (void)offset;
    TELL_ONCE("ActorLiftPlayerStep (83:FB57)");
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

__attribute__((weak)) void CarrierPairSend(Actor *sender)
{
    (void)sender;
    TELL_ONCE("CarrierPairSend (84:A056)");
}

__attribute__((weak)) bool ActorClampAboveFloor12(Actor *a)
{
    (void)a;
    TELL_ONCE("ActorClampAboveFloor12 (84:8F2F)");
    return false;
}

