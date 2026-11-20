/*
 * kinds_c.c: the kinds n5-kinds-c converts, registered for the actor loop
 * (kinds.c searches this table). One row a kind: { kind, its code, its code
 * again if it has a second entry, again if a third, its wait routine }; the last row's code
 * and wait NULL. This file is n5-kinds-c's alone.
 */
#include "actor/actor_own.h"

const KindCode kinds_c[] = {
    { 0, NULL, NULL, NULL, NULL }
};
