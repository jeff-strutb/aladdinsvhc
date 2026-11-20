/*
 * actor_a.h: what actors A's files share and the rest of the game does
 * not.
 */
#ifndef ALADDIN_ACTOR_A_H
#define ALADDIN_ACTOR_A_H

#include "actor/actor_own.h"

/* The sums a test against the test box leaves for the tests after it (the
   original's scratch words $0010, $0012 and the last sum in A). */
typedef struct TestBoxTouch {
    uint16_t half_h;            /* its half-height and the test box's, added */
    uint16_t height;            /* twice that */
    uint16_t y_sum;             /* its y less the test box's, plus half_h: below height on a touch */
} TestBoxTouch;

/* ActorHitsTestBoxIfActive, with its sums in *t on a touch */
bool actor_touches_test_box(const Actor *a, TestBoxTouch *t);

/* 83:84C7 ScratchBelowBy10, and ActorStompOrHurt16's margin of 16: the
   test box overlaps the actor's top part, less than `margin` pixels of
   height (Aladdin is coming down on it from above) */
static inline bool touch_from_above(const TestBoxTouch *t, uint16_t margin)
{
    return (uint16_t)(t->height - margin) < t->y_sum;
}

void PlayerStandOnActor(const Actor *a, const TestBoxTouch *t);    /* 83:84D5 */
void ActorSpawnKind11AtPlayer(void);                               /* 83:84F4 */

#endif
