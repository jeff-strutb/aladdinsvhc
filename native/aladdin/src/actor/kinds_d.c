/*
 * kinds_d.c: the kinds n5-kinds-d converts, registered for the actor loop
 * (kinds.c searches this table). One row a kind: { kind, its code, its code
 * again if it has a second entry, again if a third, its wait routine }
 * (actor_own.h's KindCode; actor-core.md section 3); the last row's code
 * and wait NULL. This file is n5-kinds-d's alone.
 */
#include "actor/actor_own.h"
#include "actor/kinds_d.h"
#include "actor/kinds_e.h"

const KindCode kinds_d[] = {
    { 0x08, Kind08Start, NULL, NULL, ActorKind08Wait },   /* $07 1-11 but the guard's: other code, not converted (reported) */
    { 0x20, Kind20Start, Kind20Start, Kind20Start, ActorWaitTurnAnim08 },
    { 0x30, Kind30Start, Kind30Start, NULL, Kind30Wait },
    { 0x34, Kind34Start, Kind34Start, NULL, NULL },             /* wait 83:BBF1 Kind34Wait: its flags never ask for it */
    { 0x51, Kind51Start, Kind51Start, NULL, ActorWaitWake },
    { 0x53, Kind53Held, Kind53Held, NULL, Kind53Wait },
    { 0x6A, Kind6AStart, Kind6AStart, NULL, Kind6AWait },
    { 0x69, Kind69Start, Kind69Start, NULL, Kind69Wait },
    { 0xA7, KindA7Start, NULL, NULL, NULL },
    { 0xA5, KindA5Start, NULL, NULL, ActorKindA5Wait },
    { 0xA0, KindA0Start, NULL, NULL, ActorKindA0Wait },
    { 0x91, Kind91Start, NULL, NULL, Kind91FreeOff },   /* second A5:F603 Kind91FreeOff as code: not registered */
    { 0x44, Kind44Start, Kind44Start, NULL, Kind44Wait },
    { 0x71, Kind71Start, NULL, NULL, Kind71Wait },   /* second 83:F40C: no routine there */
    { 0x4B, Kind4BStart, NULL, NULL, NULL },   /* second and wait are kind $4D's start in the table: its flags never run a wait */
    { 0x4E, Kind4EStart, NULL, NULL, Kind4EWait },
    { 0x5F, Kind5FStart, Kind5FSecond, NULL, NULL },
    { 0x84, Kind84Start, NULL, NULL, ActorKind84Wait },
    { 0x22, Kind22Start, Kind22Start, NULL, Kind22Wait },
    { 0x5C, Kind5CStart, Kind5CStart, NULL, Kind5CWait },
    { 0x5D, Kind5DStart, NULL, NULL, Kind5DWait },   /* second 83:E352 is its wait routine as code: never reached found */
    { 0x66, Kind66Start, Kind66Second, NULL, ActorKind66Wait },
    { 0x6C, Kind6CStart, Kind6CSecond, NULL, ActorWaitRange00 },
    { 0x87, Kind87Start, NULL, NULL, ActorKind87Wait },   /* left aside: second 83:F9BA LandingPad (kind $8A's too) */
    { 0xA4, KindA4Start, NULL, NULL, ActorKindA4Wait },
    { 0, NULL, NULL, NULL, NULL }
};
