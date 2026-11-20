/*
 * kinds_a.c: the kinds n5-actors-a converts, registered for the actor loop
 * (kinds.c searches this table). One row a kind: { kind, its code, its code
 * again if it has a second entry, again if a third, its wait routine }
 * (actor_own.h's KindCode; actor-core.md section 3); the last row's code
 * and wait NULL. This file is n5-actors-a's alone.
 */
#include "actor/actor_a.h"

const KindCode kinds_a[] = {
    /* the shots (thrown_shot.c): the record's tables give kinds $10 and
       $14-$17 the start 83:920E (ShotFly), kind $10 the second 83:9212
       (ShotHit) and all the wait 83:91F3 (ActorWaitNothing: their flags
       never ask for it); kind $B9's start reads past the starts' table into
       the seconds' ($83:9212, ShotHit) and its second past that into the
       waits' (83:91F3, an RTS run as code, which nothing can reach: no
       record of kind $B9 is ever put in a state from $80) */
    { 0x10, ShotCode, ShotCode, NULL, NULL },
    { 0x14, ShotCode, NULL, NULL, NULL },
    { 0x15, ShotCode, NULL, NULL, NULL },
    { 0x16, ShotCode, NULL, NULL, NULL },
    { 0x17, ShotCode, NULL, NULL, NULL },
    { 0xB9, ShotCode, NULL, NULL, NULL },
    { 0, NULL, NULL, NULL, NULL }
};
