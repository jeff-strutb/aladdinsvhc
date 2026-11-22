/*
 * wait_step.c: wait routines shared by several kinds.
 */
#include "actor/actor_own.h"
#include "video/view.h"

/*
 * @routine 83:E5B6 ActorWaitRange00
 * WHAT IT DOES: A wait routine (kinds $61, $64, $6C): each frame it waits,
 * it ends once out of range (box 0).
 */
int ActorWaitRange00(Actor *a)
{
    return ActorEndOutOfRange(a, 0x00);
}

/*
 * @routine 83:9749 ActorEndIfAhead
 * WHAT IT DOES: The tail some wait routines run: the actor ends once it is
 * more than $120 + dx pixels right of the play field's left edge (the camera is
 * the original's, the view's right edge is dx past its screen's; 32 past it); else it goes on.
 */
int ActorEndIfAhead(Actor *a)
{
    if ((uint16_t)(pixels(g.layers[0].scroll_x) + 0x0120 + view.dx) < pixels(a->x))
        return ActorEndPlaced(a);
    return ACTOR_GOES_ON;
}
