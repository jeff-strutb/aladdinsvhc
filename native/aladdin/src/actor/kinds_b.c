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
#include "actor/kinds_b.h"
#include "actor/kinds_d.h"             /* Kind27Wait (83:A012), n5-kinds-d's */
#include "actor/kinds_e.h"             /* KindAnimateSend (84:903B), n5-kinds-e's: kind $76's wait (through 83:9285) */

int ActorBriefAnim8(Actor *a);          /* kind_brief.c */
int ActorBriefAnimB(Actor *a);
int Kind27Start(Actor *a);              /* kind27.c */
int Kind4DStart(Actor *a);              /* kind4d.c */
int ActorWaitTest42Anim(Actor *a);
int Kind38Start(Actor *a);              /* kind38.c */
int Kind3FStart(Actor *a);              /* kind3f.c */
int Kind3FWait(Actor *a);
int Kind42Wait(Actor *a);               /* kind42.c (Kind42Start in kinds_b.h) */
int Kind43Start(Actor *a);              /* kind43.c */
int Kind43Wait(Actor *a);
int Kind49Carry(Actor *a);              /* kind49.c */
int Kind49Wait(Actor *a);
int Kind52Start(Actor *a);              /* kind52.c */
int Kind52Wait(Actor *a);
int Kind5BStart(Actor *a);              /* kind5b.c */
int ActorWaitStepSounds(Actor *a);
int Kind5EBrief(Actor *a);              /* kind_5e.c */
int ActorWaitAnimRange10(Actor *a);
int Kind70Start(Actor *a);              /* kind70_start.c */
int Kind70Wait(Actor *a);
int Kind76Start(Actor *a);              /* kind76.c */
int Kind77Start(Actor *a);              /* kind77.c */
int Kind78Start(Actor *a);
int Kind77Wait(Actor *a);
int Kind79Rise(Actor *a);               /* kind79.c */
int Kind79Wait(Actor *a);
int Kind86Start(Actor *a);              /* kind86.c */
int Kind86Wait(Actor *a);
int Kind99Start(Actor *a);              /* kind9a.c */
int Kind99Wait(Actor *a);
int Kind9AStart(Actor *a);
int Kind9AWait(Actor *a);
int KindA2Sent(Actor *a);               /* kind_a2.c */

const KindCode kinds_b[] = {
    /* $11: 83:9216 Kind11Start; its wait 83:91F3 ActorWaitNothing does
       nothing, and its flags never get bit 7 */
    { 0x11, ActorBriefAnim8, NULL, NULL, NULL },
    { 0x13, ActorBriefAnimB, NULL, NULL, NULL },            /* 83:9222 Kind13Start; no wait entry */
    { 0x27, Kind27Start, Kind27Start, NULL, Kind27Wait },   /* its second entry: ActorEndPlaced, in Kind27Start */
    /* $4D: its second entry 83:CFA9 is kind $4E's start (Kind4EStart,
       n5-kinds-d's): not in this row yet (the attract demo never takes it there) */
    { 0x4D, Kind4DStart, NULL, NULL, ActorWaitTest42Anim },
    /* $38: its wait 83:BD22 Kind37Wait is kind $37's start (n5-actors-a's):
       left aside until a record can run another kind's code */
    { 0x38, Kind38Start, Kind38Start, NULL, NULL },
    { 0x3F, Kind3FStart, Kind3FStart, NULL, Kind3FWait },
    { 0x42, Kind42Start, Kind42Start, NULL, Kind42Wait },
    { 0x43, Kind43Start, Kind43Start, NULL, Kind43Wait },
    { 0x49, Kind49Carry, NULL, NULL, Kind49Wait },
    { 0x52, Kind52Start, Kind52Start, NULL, Kind52Wait },
    { 0x5B, Kind5BStart, Kind5BStart, NULL, ActorWaitStepSounds },
    { 0x5E, Kind5EBrief, Kind5EBrief, NULL, ActorWaitAnimRange10 },
    /* $70: its second 83:F370 is kind $71's start (Kind71Start, n5-kinds-d's): left aside */
    { 0x70, Kind70Start, NULL, NULL, Kind70Wait },
    /* $76: its second 83:928A is BackdropMover (84:9044, n5-actors-a's): left aside */
    { 0x76, Kind76Start, NULL, NULL, KindAnimateSend },
    /* $77, $78: their second 83:F541 is kind $79's start (Kind79Rise): left aside */
    { 0x77, Kind77Start, NULL, NULL, Kind77Wait },
    { 0x78, Kind78Start, NULL, NULL, Kind77Wait },
    { 0x79, Kind79Rise, Kind79Rise, NULL, Kind79Wait },
    { 0x86, Kind86Start, Kind86Start, NULL, Kind86Wait },
    { 0x99, Kind99Start, Kind99Start, NULL, Kind99Wait },
    { 0x9A, Kind9AStart, Kind9AStart, NULL, Kind9AWait },
    { 0xA2, KindA2Sent, KindA2Sent, NULL, ActorAnimFreeFar },    /* its wait 83:941A: ActorAnimFreeFar */
    { 0, NULL, NULL, NULL, NULL }
};
