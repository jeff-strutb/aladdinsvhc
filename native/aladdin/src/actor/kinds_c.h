/*
 * kinds_c.h: the code of the actor kinds n5-kinds-c converts (the stage 5
 * split's "kinds C" record files, and the pieces of those kinds that other
 * record files hold), as kinds_c.c's table and other kinds' code call it.
 * Each kind's code is one ActorCode for all its entries (actor.h): it
 * switches on a->resume.
 *
 * A kind whose code another of these kinds' entries runs (kind $2C's second
 * entry is kind $2D's start) numbers its own resume points from a base of
 * its own, so that kind's code can hand them on.
 */
#ifndef ALADDIN_KINDS_C_H
#define ALADDIN_KINDS_C_H

#include <stdio.h>

#include "actor/actor_own.h"

/* the resume points' bases of the kinds whose code another kind runs */
#define KIND2D_RESUME (ACTOR_RESUME_OWN + 0x100)

/* A resume point the kind's code has no case for: it says so once and the
   actor stays where it is, a frame at a time. */
static inline int kind_lost(Actor *a, const char *code)
{
    static int told;

    if (!told++)
        fprintf(stderr, "%s: resume point %u of kind $%02X has no case\n", code, a->resume, a->kind);
    return 1;
}

/* ---- the kinds' code (each: frames to wait, or ACTOR_ENDED) ---- */

int Kind2BCode(Actor *a);       /* kind $2B: 83:A467, 83:A725 */
int Kind33Code(Actor *a);       /* kind $33: 83:B966 */
int Kind2DCode(Actor *a);       /* kind $2D: 83:A82F, 83:AA5A, 83:AA95 */
int Kind2DStart(Actor *a);      /* 83:A82F, also kind $2C's second entry */
int Kind2FCode(Actor *a);       /* kind $2F: 83:B4BE, 83:B643 */
int Kind4FCode(Actor *a);       /* kind $4F: 83:D01A */
int Kind90Code(Actor *a);       /* kind $90: A5:F4D2 */

/* ---- wait routines (each: ACTOR_GOES_ON or ACTOR_ENDED) ---- */

int ActorWaitNothing(Actor *a);     /* 83:91F3: the wait of many kinds (4-6, 9-$1F, $25, $29, $32 ...): nothing */
int Kind23Wait(Actor *a);           /* 83:9D46, and 83:DEB9 Kind59Wait */
int Kind3EWait(Actor *a);           /* 83:B190 */
int Kind33Wait(Actor *a);           /* 83:BB91, and 83:BF7E Kind3AWait */
int Kind2BWait(Actor *a);           /* 83:A709 */
int Kind2CWait(Actor *a);           /* 83:A818 */
int Kind41Wait(Actor *a);           /* 83:B303 */
int Kind50Wait(Actor *a);           /* 83:D1B0 */
int ActorWaitHitsRange8(Actor *a);  /* 83:AA47: kind $2D's */
int ActorWaitHitsRange0(Actor *a);  /* 83:B630: kind $2F's */
int ActorWaitTest42Anim2(Actor *a); /* 83:D0EF: kind $4F's */

#endif
