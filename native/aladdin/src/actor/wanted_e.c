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




__attribute__((weak)) void ActorPictureSend(Actor *a)
{
    (void)a;
    TELL_ONCE("ActorPictureSend (81:949E)");
}


