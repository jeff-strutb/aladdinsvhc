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

int Kind20Start(Actor *a);      /* 83:97AF: the guard (and 83:9C19 Kind20Hit, 83:9C49 Kind20Dazed) */
int Kind30Start(Actor *a);      /* 83:B684: the pickups */
int Kind34Start(Actor *a);      /* 83:BB97 */
int Kind51Start(Actor *a);      /* 83:D239, and 83:D2BE Kind51Second */
int Kind5CStart(Actor *a);      /* 83:E000: the Genie's head of level 8 */
int Kind5DStart(Actor *a);      /* 83:E29B: kind $5C's cloud */
int Kind66Start(Actor *a);      /* 84:839E */
int Kind6CStart(Actor *a);      /* 83:E55E: the spot that holds Aladdin */

/* ---- shared with other kinds ---- */

uint8_t Kind5CPathByLives(void);            /* 83:E22A: kind $5C's path by Aladdin's lives (0-2); kind $5D's heights by it */
uint8_t Kind5DParentGone(const Actor *a);   /* 83:E3C1 (83:E3BD Long, kind $8B's): its maker (kind $5C, Actor.next) has gone */

/* ---- wait routines (kind_waits.c; each: ACTOR_GOES_ON or ACTOR_ENDED) ---- */

int ActorWaitTurnAnim08(Actor *a);  /* 83:9C06: kind $20's (kind20.c) */
int Kind5CWait(Actor *a);       /* 83:E1EE: kind $5C's (kind5c.c) */
int Kind5DWait(Actor *a);       /* 83:E352: kind $5D's (kind5d.c) */
int ActorKind66Wait(Actor *a);  /* 83:944E: kind $66's (kind66.c) */
int Kind27Wait(Actor *a);       /* 83:A012: kind $27's (kinds B's kind27.c has its start) */
int Kind30Wait(Actor *a);       /* 83:B94A */
int ActorWaitWake(Actor *a);    /* 83:D2A2: kind $51's (kind51.c) */

#endif
