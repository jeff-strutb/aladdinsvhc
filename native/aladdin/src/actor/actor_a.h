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

/* the shots' code (thrown_shot.c): kinds $10, $14-$17 and $B9. Its resume
   points other actors' code sends a shot to (the original wrote the
   addresses $81:EB76 and $81:EB7C into the shot's record) */
enum {
    SHOT_AT_FLY = ACTOR_RESUME_OWN,     /* 81:EAEB: a frame of its flight */
    SHOT_AT_HIT,                        /* 81:EB46: ShotHit after its first frame */
    SHOT_AT_ENDS,                       /* 81:EB76: it ends */
    SHOT_AT_TURNED_BACK,                /* 81:EB7C: turned back, it flies off harmless */
    SHOT_AT_FLY_OFF                     /* 81:EB82: a frame of that flight */
};
int ShotCode(Actor *a);

void flag_drop_case(Actor *a);      /* flag_drops.c: ActorFlagOnce's cases */

#endif
