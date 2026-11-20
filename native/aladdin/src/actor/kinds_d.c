/*
 * kinds_d.c: the kinds n5-kinds-d converts, registered for the actor loop
 * (kinds.c searches this table). One row a kind: { kind, its code, its code
 * again if it has a second entry, again if a third, its wait routine }
 * (actor_own.h's KindCode; actor-core.md section 3); the last row's code
 * and wait NULL. This file is n5-kinds-d's alone.
 */
#include "actor/actor_own.h"
#include "actor/kinds_d.h"

const KindCode kinds_d[] = {
    { 0x20, Kind20Start, Kind20Start, Kind20Start, ActorWaitTurnAnim08 },
    { 0x30, Kind30Start, Kind30Start, NULL, Kind30Wait },
    { 0x34, Kind34Start, Kind34Start, NULL, NULL },             /* wait 83:BBF1 Kind34Wait: its flags never ask for it */
    { 0x51, Kind51Start, Kind51Start, NULL, ActorWaitWake },
    { 0x5C, Kind5CStart, Kind5CStart, NULL, Kind5CWait },
    { 0x5D, Kind5DStart, NULL, NULL, Kind5DWait },   /* second 83:E352 is its wait routine as code: never reached found */
    { 0x66, Kind66Start, NULL, NULL, ActorKind66Wait },   /* left aside: second 84:8629 Kind73Start (kind $73's start) */
    { 0x6C, Kind6CStart, NULL, NULL, NULL },   /* left aside: second 83:E5BB Kind65Start, wait 83:E5B6 ActorWaitRange00 (shared) */
    { 0x87, Kind87Start, NULL, NULL, ActorKind87Wait },   /* left aside: second 83:F9BA LandingPad (kind $8A's too) */
    { 0, NULL, NULL, NULL, NULL }
};
