/*
 * kinds_a.c: the kinds n5-actors-a converts, registered for the actor loop
 * (kinds.c searches this table). One row a kind: { kind, its code, its code
 * again if it has a second entry, again if a third, its wait routine }
 * (actor_own.h's KindCode; actor-core.md section 3); the last row's code
 * and wait NULL. This file is n5-actors-a's alone.
 */
#include "actor/actor_own.h"

const KindCode kinds_a[] = {
    { 0, NULL, NULL, NULL, NULL }
};
