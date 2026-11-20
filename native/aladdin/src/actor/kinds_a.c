/*
 * kinds_a.c: the kinds n5-actors-a converts, registered for the actor loop
 * (kinds.c searches this table). One row a kind: { kind, its code, its code
 * again if it has a second entry, again if a third, its wait routine }
 * (actor_own.h's KindCode; actor-core.md section 3); the last row's code
 * and wait NULL. This file is n5-actors-a's alone.
 */
#include "actor/actor_a.h"
#include "actor/kinds_d.h"

int Kind3CDrift(Actor *a);              /* kind3c.c */
int Kind3CWait(Actor *a);
int Kind46Start(Actor *a);              /* kind46.c */
int Kind46Wait(Actor *a);
int Kind62Wait(Actor *a);
int Kind6EStart(Actor *a);              /* kind6e.c */
int Kind6EWait(Actor *a);
int Kind7CStart(Actor *a);              /* kind7c.c */
int Kind7CWait(Actor *a);
int KindA3Shot(Actor *a);
int Kind67Start(Actor *a);              /* kind67.c */
int Kind67Wait(Actor *a);
int Kind7AFly(Actor *a);                /* kind7a.c */
int Kind7DKeepSpot(Actor *a);           /* kind7d.c */
int Kind7DWait(Actor *a);
int Kind88Start(Actor *a);              /* kind88.c */
int Kind89Start(Actor *a);

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
    { 0x3C, Kind3CDrift, Kind3CDrift, NULL, Kind3CWait },
    /* $46: its second entry is Kind62Start, which Kind46Start runs as kind $62 (+$42 = 1);
       $62: its second entry PlatformCourseStart (course_platform.c) is left aside */
    { 0x46, Kind46Start, Kind46Start, NULL, Kind46Wait },
    { 0x62, Kind46Start, Kind62Second, NULL, Kind62Wait },
    /* $67: its second entry, kind $68's start, is left aside */
    { 0x67, Kind67Start, NULL, NULL, Kind67Wait },
    { 0x6E, Kind6EStart, Kind6EStart, NULL, Kind6EWait },
    { 0x7C, Kind7CStart, Kind7CStart, NULL, Kind7CWait },
    { 0xA3, KindA3Shot, NULL, NULL, AnimateWhileWaiting },
    { 0x7A, Kind7AFly, NULL, NULL, NULL },
    { 0x7D, Kind7DKeepSpot, Kind7DKeepSpot, NULL, Kind7DWait },
    { 0x88, Kind88Start, Kind88Start, NULL, Kind88Work },
    { 0x89, Kind89Start, NULL, NULL, NULL },
    { 0, NULL, NULL, NULL, NULL }
};
