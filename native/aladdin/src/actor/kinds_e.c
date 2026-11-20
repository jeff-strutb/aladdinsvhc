/*
 * kinds_e.c: the kinds n5-kinds-e converts, registered for the actor loop
 * (kinds.c searches this table). One row a kind: { kind, its code, its code
 * again if it has a second entry, again if a third, its wait routine }; the last row's code
 * and wait NULL. This file is n5-kinds-e's alone.
 */
#include "actor/actor_own.h"

const KindCode kinds_e[] = {
    { 0, NULL, NULL, NULL, NULL }
};
