/*
 * kinds_b.c: the kinds n5-kinds-b converts, registered for the actor loop
 * (kinds.c searches this table). One row a kind: { kind, its code, its code
 * again if it has a second entry, again if a third, its wait routine }
 * (actor_own.h's KindCode; actor-core.md section 3); the last row's code
 * and wait NULL. This file is n5-kinds-b's alone.
 *
 * Each row is the kind's entries of the record's tables at $83:8CAB/8DAB
 * (start), 8DFD/8EFD (second), 90A1/91A1 (third) and 8F4F/904F (wait),
 * the stubs of the record's kind_starts.c among them.
 */
#include "actor/actor_own.h"
#include "actor/kinds_d.h"             /* Kind27Wait (83:A012), n5-kinds-d's */

int ActorBriefAnim8(Actor *a);          /* kind_brief.c */
int ActorBriefAnimB(Actor *a);
int Kind27Start(Actor *a);              /* kind27.c */
int Kind4DStart(Actor *a);              /* kind4d.c */
int ActorWaitTest42Anim(Actor *a);

const KindCode kinds_b[] = {
    /* $11: 83:9216 Kind11Start; its wait 83:91F3 ActorWaitNothing does
       nothing, and its flags never get bit 7 */
    { 0x11, ActorBriefAnim8, NULL, NULL, NULL },
    { 0x13, ActorBriefAnimB, NULL, NULL, NULL },            /* 83:9222 Kind13Start; no wait entry */
    { 0x27, Kind27Start, Kind27Start, NULL, Kind27Wait },   /* its second entry: ActorEndPlaced, in Kind27Start */
    /* $4D: its second entry 83:CFA9 is kind $4E's start (Kind4EStart,
       n5-kinds-d's): not in this row yet (the attract demo never takes it there) */
    { 0x4D, Kind4DStart, NULL, NULL, ActorWaitTest42Anim },
    { 0, NULL, NULL, NULL, NULL }
};
