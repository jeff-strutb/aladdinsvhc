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
int Kind98Hit(Actor *a);        /* 84:A2A6: its second entry's start */

/* ---- wait routines (kind2e.c, kind35.c, kind_steps.c): ACTOR_GOES_ON, or
   ACTOR_ENDED when it ended its actor ---- */

int ActorWaitEndLowKinds(Actor *a);  /* 83:ABD8: kind $2E's wait */
int ActorWaitAnimEnd10(Actor *a);    /* 83:BD0B: kind $35's wait */
int Kind31Wait(Actor *a);            /* 83:AEEC: kind $31's wait */
int Kind3BWait(Actor *a);            /* 83:C21C: kind $3B's wait */
int Kind98WaitWork(Actor *a);        /* 84:A39C: kind $98's wait */
int KindAnimateSend(Actor *a);      /* 84:903B: kinds $21, $76 and $93's wait (A5:F3E3 and A5:F434 the same) */

#endif
