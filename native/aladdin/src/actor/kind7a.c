/*
 * kind7a.c: actor kind $7A, a thing that flies its course for good.
 */
#include "actor/actor_own.h"

enum { FLY = ACTOR_RESUME_OWN };

/*
 * @routine 83:F757 Kind7AFly
 * WHAT IT DOES: Actor kind $7A: facing right, anim_flags bit 1, flags bit
 * 4, animation 0, it flies its course (motion preset $53) for good.
 */
int Kind7AFly(Actor *a)
{
    if (a->resume == ACTOR_START) {
        a->facing = 0;
        a->flip = 0;
        a->anim_flags |= 0x02;
        a->flags |= 0x10;
        AnimStart(a, 0);
        ActorMotionPreset(a, 0x53);
        a->resume = FLY;
        return 1;
    }
    ActorFly(a);
    return 1;
}
