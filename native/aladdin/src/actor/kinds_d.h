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

int Kind20Start(Actor *a);      /* 83:97AF: the guard (and 83:9C19 Kind20Hit, 83:9C49 Kind20Dazed) */
int Kind30Start(Actor *a);      /* 83:B684: the pickups */
int Kind34Start(Actor *a);      /* 83:BB97 */
int Kind51Start(Actor *a);      /* 83:D239, and 83:D2BE Kind51Second */

/* ---- wait routines (kind_waits.c; each: ACTOR_GOES_ON or ACTOR_ENDED) ---- */

int ActorWaitTurnAnim08(Actor *a);  /* 83:9C06: kind $20's (kind20.c) */
int Kind27Wait(Actor *a);       /* 83:A012: kind $27's (kinds B's kind27.c has its start) */
int Kind30Wait(Actor *a);       /* 83:B94A */
int ActorWaitWake(Actor *a);    /* 83:D2A2: kind $51's (kind51.c) */

#endif
