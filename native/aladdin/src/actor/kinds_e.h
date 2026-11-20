/*
 * kinds_e.h: the code of kinds E's files (n5-kinds-e) that the actor
 * table rows and other files name: each kind's code, its wait routines,
 * and the steps other kinds share. This file is n5-kinds-e's alone.
 */
#ifndef ALADDIN_KINDS_E_H
#define ALADDIN_KINDS_E_H

#include "actor/actor.h"

/* ---- kinds' code (each takes its record, returns frames to wait or
   ACTOR_ENDED; see actor.h) ---- */

int Kind21Start(Actor *a);      /* A5:F309 */
int Kind2EStart(Actor *a);      /* 83:AADC */
int Kind35Start(Actor *a);      /* 83:BBF2, and its second entry */
int Kind31Start(Actor *a);      /* 83:ABEA, and its second entry */
int Kind31Second(Actor *a);     /* 83:AF1C: its second entry's start */
int Kind98Start(Actor *a);      /* 84:A112, and its second entry */
int Kind3BStart(Actor *a);      /* 83:BF83, and its second entry (83:C2A5: ActorEndPlaced) */
int Kind2AStart(Actor *a);      /* 83:A1E5, and its second entry */
int Kind2ASecond(Actor *a);     /* 83:A37B: its second entry's start */
int Kind54Start(Actor *a);      /* 83:D6D7, and its second entry (83:D7F7: ActorEndPlaced) */
int Kind55Start(Actor *a);      /* 83:D7FB, and its second entry (83:D8DE: ActorEndPlaced) */
int Kind97Start(Actor *a);      /* 84:9F4F (its start stub 83:93B5 Kind97Enter) */
int Kind28Start(Actor *a);      /* 83:A03E, and its second entry */
int Kind28Second(Actor *a);     /* 83:A196: its second entry's start */
int KindA1Start(Actor *a);      /* 84:A97F, and its second entry */
int KindA1Hit(Actor *a);        /* 84:A9FC: its second entry's start */
int KindA6Stay(Actor *a);       /* 84:AC3F */
int Kind8FWalk(Actor *a);       /* 83:FA4F, and its second entry (83:FAB7: ActorEndPlaced) */
int Kind94Spark(Actor *a);      /* 83:FABB */
int Kind56Start(Actor *a);      /* 83:D8E2 */
int Kind9EStart(Actor *a);      /* 84:A820 */
int Kind61Start(Actor *a);      /* 83:E4EC */
int Kind64Start(Actor *a);      /* 83:E4F0 */
int Kind65Start(Actor *a);      /* 83:E5BB */
int Kind40Start(Actor *a);      /* 83:C507, and its second entry (83:C565: ActorEndPlaced) */
int Kind7BStart(Actor *a);      /* 83:C572, and its second entry (83:C5D1) */
int KindA8Start(Actor *a);      /* 83:FB1A */
int Kind61Second(Actor *a);     /* 83:8DFD: kinds $61 and $64's second entry: Kind65Start */
int Kind64Second(Actor *a);
int Kind94Second(Actor *a);     /* kind $94's second entry: KindA8Start */
int Kind48Start(Actor *a);      /* 83:CE14, and its second entry (83:CE22) */
int Kind5AStart(Actor *a);      /* 83:DEBE */
int Kind58Start(Actor *a);      /* 83:DBBB */
int Kind98Hit(Actor *a);        /* 84:A2A6: its second entry's start */

/* ---- wait routines (kind2e.c, kind35.c, kind_steps.c): ACTOR_GOES_ON, or
   ACTOR_ENDED when it ended its actor ---- */

int ActorWaitEndLowKinds(Actor *a);  /* 83:ABD8: kind $2E's wait */
int ActorWaitAnimEnd10(Actor *a);    /* 83:BD0B: kind $35's wait */
int Kind31Wait(Actor *a);            /* 83:AEEC: kind $31's wait */
int Kind3BWait(Actor *a);            /* 83:C21C: kind $3B's wait */
int Kind2AWait(Actor *a);            /* 83:A413: kind $2A's wait */
int ActorWaitHurtOnTouch(Actor *a);  /* 83:D8CE: kind $55's wait, and kind $54's (83:D7E7 Kind54Wait, the same code) */
int Kind28Wait(Actor *a);            /* 83:A1BD: kind $28's wait */
int KindA1Touch(Actor *a);           /* 84:A9C7: kind $A1's wait */
int Kind9EWaitWork(Actor *a);        /* 84:A8BD: kind $9E's wait */
int Kind40Wait(Actor *a);            /* 83:C569: kind $40's wait */
int Kind7BWait(Actor *a);            /* 83:C5D5: kind $7B's wait */
int Kind5AWait(Actor *a);            /* 83:DEF3: kind $5A's wait */
int Kind58Wait(Actor *a);            /* 83:DC9B: kind $58's wait */
int Kind98WaitWork(Actor *a);        /* 84:A39C: kind $98's wait */
int Kind91FreeOff(Actor *a);         /* A5:F603: kinds $91 and $92's wait */
int KindAnimateSend(Actor *a);      /* 84:903B: kinds $21, $76 and $93's wait (A5:F3E3 and A5:F434 the same) */

#endif
