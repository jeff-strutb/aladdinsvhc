/*
 * wait_step.c: wait routines shared by several kinds.
 */
#include "actor/actor_own.h"

/*
 * @routine 83:E5B6 ActorWaitRange00
 * WHAT IT DOES: A wait routine (kinds $61, $64, $6C): each frame it waits,
 * it ends once out of range (box 0).
 */
int ActorWaitRange00(Actor *a)
{
    return ActorEndOutOfRange(a, 0x00);
}
