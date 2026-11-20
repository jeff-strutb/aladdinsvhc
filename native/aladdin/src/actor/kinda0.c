/*
 * kinda0.c: actor kind $A0, a ledge that gives way: once Aladdin stands on
 * it or hangs from it, it shakes for 32 frames, then falls, carrying him
 * down while he stands on it. Its wait routine is ActorKindA0Wait.
 */
#include "actor/actor_own.h"
#include "actor/kinds_d.h"
#include "player/player.h"

#define ON OWN_40               /* the fall: nonzero while Aladdin stands on it */
#define FRAMES OWN_41           /* the shake's frames left */
#define SOLID_BOX 0x30          /* ActorSolidForPlayer's box */
#define DOWN_HELD 0x0400        /* his pad's Down: he cannot hang */

enum {
    WAIT_STAND = ACTOR_RESUME_OWN,  /* each frame: is he on it or hanging from it? */
    SHAKE,                      /* its 32 frames of shaking */
    FALL                        /* each frame of its fall */
};

/* Stood on it, or held: ActorSolidForPlayer says he stands on it, or (unless
   his pad's Down is held) ActorHoldPlayer has him (his code at PlayerHeld).
   True when one of them says so. */
static bool stood_or_held(Actor *a)
{
    if (ActorSolidForPlayer(a, SOLID_BOX) == ACTOR_SOLID_ON_TOP)
        return true;
    if (!(g.pad1_held & DOWN_HELD)) {
        ActorHoldPlayer(a);
        return g.actors[0].resume == PLAYER_AT_HELD;
    }
    return false;
}

/*
 * @routine 84:A8E0 KindA0Start
 * WHAT IT DOES: Actor kind $A0, a ledge that gives way: solid for Aladdin
 * (and to hang from) in pose 0; once he stands on it or hangs from it, 32
 * frames more as it is, then pose 1 and it falls (gravity $0A), moving him
 * down with it while he stands on it; it stays solid and holdable as it
 * falls.
 */
int KindA0Start(Actor *a)
{
    switch (a->resume) {
    case ACTOR_START:
        a->anim_flags |= 0x02;
        a->flags |= 0x90;
        AnimStart(a, 0);
        a->own.bytes[OWN_13] = 0x07;
        a->resume = WAIT_STAND;
        return 1;
    case WAIT_STAND:
        if (!stood_or_held(a))
            return 1;
        a->own.bytes[FRAMES] = 0x20;
        a->resume = SHAKE;
        return 1;
    case SHAKE:
        (void)stood_or_held(a);
        if (--a->own.bytes[FRAMES])
            return 1;
        AnimStart(a, 1);
        ActorSpeedYSetGravity(a, 0x0A, 0x00);
        a->resume = FALL;
        return 1;
    case FALL: {
        uint16_t kept = 0;

        a->own.bytes[ON] = 0;
        if (ActorSolidForPlayer(a, SOLID_BOX) == ACTOR_SOLID_ON_TOP) {
            a->own.bytes[ON]++;
            kept = pixels(a->y);
        }
        ActorFall(a);
        if (a->own.bytes[ON])
            g.actors[0].y = with_pixels(g.actors[0].y, (uint16_t)(pixels(g.actors[0].y) + pixels(a->y) - kept));
        if (!(g.pad1_held & DOWN_HELD))
            ActorHoldPlayer(a);
        return 1;
    }
    default:
        return 1;
    }
}

/*
 * @routine 83:9404 ActorKindA0Wait
 * WHAT IT DOES: Kind $A0's wait routine, through 84:A975
 * KindA0AnimateFreeOff: its animation steps, and it is freed once out of
 * range (box 0).
 */
int ActorKindA0Wait(Actor *a)
{
    AnimStep(a);
    return ActorFreeOutOfRange(a, 0x00) ? ACTOR_ENDED : ACTOR_GOES_ON;
}
