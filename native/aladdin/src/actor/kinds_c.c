/*
 * kinds_c.c: the kinds n5-kinds-c converts, registered for the actor loop
 * (kinds.c searches this table). One row a kind: { kind, its code, its code
 * again if it has a second entry, again if a third, its wait routine }
 * (actor_own.h's KindCode; actor-core.md section 3); the last row's code
 * and wait NULL. This file is n5-kinds-c's alone.
 */
#include "actor/kinds_c.h"

const KindCode kinds_c[] = {
    { 0x2B, Kind2BCode, Kind2BCode, NULL, Kind2BWait },
    { 0x2D, Kind2DCode, Kind2DCode, Kind2DCode, ActorWaitHitsRange8 },
    { 0x2F, Kind2FCode, Kind2FCode, NULL, ActorWaitHitsRange0 },
    { 0x4F, Kind4FCode, NULL, NULL, ActorWaitTest42Anim2 },    /* its second (83:D108) is an RTS no state reaches */
    { 0x90, Kind90Code, NULL, NULL, NULL },                    /* its second and its wait (83:9366) reach an RTL: nothing */
    { 0, NULL, NULL, NULL, NULL }
};
