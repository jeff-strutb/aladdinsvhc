/*
 * kind40.c: actor kind $40, a spring Aladdin bounces off; and its wait.
 */
#include "actor/actor_own.h"
#include "actor/kinds_e.h"
#include "player/player.h"

enum {
    WATCH = ACTOR_RESUME_OWN,   /* each frame until he falls onto it */
    REST,                       /* after its 32 frames at the push */
};

/* pose 0, then each frame from the next */
static int rest_pose(Actor *a)
{
    AnimStart(a, 0x00);
    a->resume = WATCH;
    return 1;
}

/*
 * @routine 83:C507 Kind40Start
 * WHAT IT DOES: Actor kind $40, a spring: while Aladdin falls onto it from
 * above (he is in the air, his speed down not negative, his test box on
 * it) it plays its push (pose 1), sends his code on to PLAYER_AT_SPRUNG
 * and rests 32 frames; a touch while he has no health turns it to state
 * $8C (no script has that). Its start sets +$08 bit 1 and its wait routine
 * on; its probe list ($D29B) is read by nothing for this kind. Its second
 * entry (83:C565) ends it.
 */
int Kind40Start(Actor *a)
{
    switch (a->resume) {
    case ACTOR_START:
        a->anim_flags |= 0x02;
        a->flags |= 0x90;
        return rest_pose(a);
    case ACTOR_START_SECOND:
        return ActorEndPlaced(a);
    case WATCH:
        if (!g.player_in_air || g.actors[0].speed_y < 0 || !ActorHitsTestBox(a))
            return 1;
        if (!g.health) {
            a->state = 0x8C;
            return rest_pose(a);
        }
        if ((uint16_t)(pixels(g.actors[0].y) + 8) >= pixels(a->y))
            return 1;
        AnimStart(a, 0x01);
        g.actors[0].resume = PLAYER_AT_SPRUNG;
        a->resume = REST;
        return 0x20;
    case REST:
        return rest_pose(a);
    default:
        return 1;
    }
}

/*
 * @routine 83:C569 Kind40Wait
 * WHAT IT DOES: Kind $40's wait: its animation on, then its end when it is
 * out of range by box $18.
 */
int Kind40Wait(Actor *a)
{
    AnimStep(a);
    return ActorEndOutOfRange(a, 0x18);
}
