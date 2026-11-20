/*
 * kind71.c: actor kind $71, by its $07: one that paces left and right, one
 * that steps back and then moves right for good, or one that stays,
 * showing animations 1 and 0 in turn. Its wait routine is Kind71Wait.
 */
#include "actor/actor_own.h"
#include "actor/kinds_d.h"

#define LEG OWN_42              /* frames left of a pace leg */
#define PACE 0x08               /* $07 bit: it paces */
#define RUNS 0x10               /* $07 bit: it steps back and moves right */

enum {
    PACE_LEG = ACTOR_RESUME_OWN,    /* each frame of a leg of its pace */
    RUN,                        /* each frame moving right for good */
    STAY_FIRST,                 /* after $39 frames of animation 1 */
    STAY_SECOND                 /* after $38 frames of animation 0 */
};

/*
 * @routine 83:F370 Kind71Start
 * WHAT IT DOES: Actor kind $71, by its $07: bit 3, it paces, $90 frames
 * one way and $90 the other at speed $80, first to the left; else bit 4,
 * it steps back $171 pixels and moves right at speed $0100 for good; else
 * it stays, facing by bit 0, in animation 1 for $39 frames and 0 for $38,
 * again and again.
 */
int Kind71Start(Actor *a)
{
    switch (a->resume) {
    case ACTOR_START:
        a->anim_flags |= 0x02;
        a->flags |= 0x90;
        if (a->spawn_byte & PACE) {
            a->facing = a->flip = 1;
            a->speed_x = 0x80;
            AnimStart(a, 0);
            a->own.bytes[LEG] = 0x90;
            a->resume = PACE_LEG;
            return 1;
        }
        if (a->spawn_byte & RUNS) {
            a->facing = a->flip = 0;
            a->x = with_pixels(a->x, (uint16_t)(pixels(a->x) - 0x0171));
            AnimStart(a, 0);
            a->speed_x = 0x0100;
            a->resume = RUN;
            return 1;
        }
        a->facing = a->flip = (uint8_t)(a->spawn_byte & 0x01);
        AnimStart(a, 1);
        a->resume = STAY_FIRST;
        return 0x39;
    case PACE_LEG:
        ActorMoveX(a);
        if (--a->own.bytes[LEG])
            return 1;
        a->facing ^= 0x01;
        a->flip = a->facing;
        a->own.bytes[LEG] = 0x90;
        return 1;
    case RUN:
        ActorMoveX(a);
        return 1;
    case STAY_FIRST:
        AnimStart(a, 0);
        a->resume = STAY_SECOND;
        return 0x38;
    case STAY_SECOND:
        AnimStart(a, 1);
        a->resume = STAY_FIRST;
        return 0x39;
    default:
        return 1;
    }
}

/*
 * @routine 83:F3F1 Kind71Wait
 * WHAT IT DOES: Kind $71's wait routine: Aladdin in its hurt zone (zone
 * (its picture number + $0F) * 4: the original reads its +$27, which is
 * its picture) is hurt; its animation steps; it ends once ahead of the
 * view ($07 bit 4 set) or left behind (clear).
 */
int Kind71Wait(Actor *a)
{
    (void)ActorZoneHurtsPlayer(a, (uint8_t)((a->picture + 0x0F) << 2));
    AnimStep(a);
    return (a->spawn_byte & 0x10) ? ActorEndIfAhead(a) : ActorEndIfLeftBehind(a);
}
