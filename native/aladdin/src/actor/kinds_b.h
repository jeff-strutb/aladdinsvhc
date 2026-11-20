/*
 * kinds_b.h: the code of kinds session B's files, as the kinds tables and
 * other sessions' kind files call it.
 */
#ifndef ALADDIN_KINDS_B_H
#define ALADDIN_KINDS_B_H

#include "actor/actor.h"

/* kind $42 (kind42.c): the pot. Its start is also where kind $20's
   carrier guard goes on once its record has become the pot's (n5-kinds-d's
   Kind20Carrier); the pot's +$40 (OWN_40) holds the guard's record as its
   index in g.actors. */
int Kind42Start(Actor *a);      /* 83:C5DE; second 83:C738 Kind42Break in it */

#endif
