/*
 * kinds_d.h: the code of kinds session D's files (kind20.c ... kinda5.c,
 * kind_waits.c, kind_waits_84.c), as the kinds tables and the other
 * kind files call it. A wait routine here whose kind's start is another
 * session's is named in that session's kinds table row.
 */
#ifndef ALADDIN_KINDS_D_H
#define ALADDIN_KINDS_D_H

#include "actor/actor.h"

/* ---- the kinds' code (each: frames to wait, or ACTOR_ENDED) ---- */

/* kind $6C's resume points, from here: kind $66's holder ($07 bit 4) goes
   on in kind $6C's code, so its points stay apart from kind $66's */
#define KIND6C_RESUME 0x100

int Kind08Start(Actor *a);      /* 83:9205: the second guard (guard_start.c) */
int Kind20Start(Actor *a);      /* 83:97AF: the guard (and 83:9C19 Kind20Hit, 83:9C49 Kind20Dazed) */
int Kind30Start(Actor *a);      /* 83:B684: the pickups */
int Kind34Start(Actor *a);      /* 83:BB97 */
int Kind51Start(Actor *a);      /* 83:D239, and 83:D2BE Kind51Second */
int KindA4Start(Actor *a);      /* 84:AA24: the Genie's screen's figures */
int Kind53Held(Actor *a);       /* 83:D566: a thing held then dropped; its second entry 83:D694 Kind53Thrown */
int Kind6AStart(Actor *a);      /* 83:EB5E: its spawners and children */
int Kind69Start(Actor *a);      /* 83:E9F7 (and 83:EADC Kind69EscortEnd); also kind $68's second */
int KindA7Start(Actor *a);      /* 84:AC66: a figure of the ending */
int KindA5Start(Actor *a);      /* 84:AB9C: the bombs and their column */
int KindA0Start(Actor *a);      /* 84:A8E0: a ledge that gives way */
int Kind91Start(Actor *a);      /* A5:F534: a marker steering the escort */
int Kind44Start(Actor *a);      /* 83:B45B */
int Kind71Start(Actor *a);      /* 83:F370 */
int Kind4BStart(Actor *a);      /* 83:CE2A */
int Kind4EStart(Actor *a);      /* 83:CFA9 (and 83:CFC1 Kind4EBounced) */
int Kind5FStart(Actor *a);      /* 83:E465 */
int Kind84Start(Actor *a);      /* 84:95E3 */
int Kind22Start(Actor *a);      /* 83:9CD2 (second 83:9CDE ends it) */
int Kind5CStart(Actor *a);      /* 83:E000: the Genie's head of level 8 */
int Kind87Start(Actor *a);      /* 83:F77F: the catcher */
int Kind5DStart(Actor *a);      /* 83:E29B: kind $5C's cloud */
int Kind66Start(Actor *a);      /* 84:839E */
int Kind6CStart(Actor *a);
int Kind6CSecond(Actor *a);     /* its second entry: on in Kind65Start */      /* 83:E55E: the spot that holds Aladdin */

/* ---- shared with other kinds ---- */

uint8_t Kind5CPathByLives(void);            /* 83:E22A: kind $5C's path by Aladdin's lives (0-2); kind $5D's heights by it */
uint8_t Kind5DParentGone(const Actor *a);   /* 83:E3C1 (83:E3BD Long, kind $8B's): its maker (kind $5C, Actor.next) has gone */

/* ---- wait routines (kind_waits.c; each: ACTOR_GOES_ON or ACTOR_ENDED) ---- */

int ActorKind08Wait(Actor *a);  /* 83:9209: kind $08's (guard_states.c) */
int ActorWaitTurnAnim08(Actor *a);  /* 83:9C06: kind $20's (kind20.c) */
int ActorKindA4Wait(Actor *a);  /* 83:9430: kind $A4's (kinda4.c) */
int Kind5CWait(Actor *a);       /* 83:E1EE: kind $5C's (kind5c.c) */
int ActorKind87Wait(Actor *a);  /* 83:9310 (83:F991 KindWaitF991): kind $87's (kind87.c) */
int Kind5DWait(Actor *a);       /* 83:E352: kind $5D's (kind5d.c) */
int ActorKind66Wait(Actor *a);  /* 83:944E: kind $66's (kind66.c) */
int Kind53Wait(Actor *a);       /* 83:D64A */
int Kind6AWait(Actor *a);       /* 83:EC20 */
int Kind69Wait(Actor *a);       /* 83:EB24 */
int ActorKindA5Wait(Actor *a);  /* 83:9439 (84:AC31 KindA5WaitWork) */
int ActorKindA0Wait(Actor *a);  /* 83:9404 (84:A975 KindA0AnimateFreeOff) */
int Kind44Wait(Actor *a);       /* 83:B488 */
int Kind71Wait(Actor *a);       /* 83:F3F1 */
int Kind4EWait(Actor *a);       /* 83:CFF8 */
int ActorKind84Wait(Actor *a);  /* 83:92E9 */
int Kind22Wait(Actor *a);       /* 83:9CE2 */
int Kind27Wait(Actor *a);       /* 83:A012: kind $27's (kinds B's kind27.c has its start) */
int Kind30Wait(Actor *a);       /* 83:B94A */
int ActorWaitWake(Actor *a);    /* 83:D2A2: kind $51's (kind51.c) */

#endif
