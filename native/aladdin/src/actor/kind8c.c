/*
 * kind8c.c: actor kind $8C, a cloud that rises as kind $5D's does (Kind5DRise)
 * and that Aladdin bounces off, for as long as its maker lasts and $57 frames
 * after. Its wait routine animates it (AnimateWhileWaiting).
 */
#include "actor/actor_own.h"
#include "actor/kinds_d.h"
#include "audio/sound.h"

#define BOUNCED OWN_40          /* set once Aladdin has bounced off it */
#define TIMER OWN_41            /* frames left of its part */

enum {
    RISE = ACTOR_RESUME_OWN,    /* each frame of its rise */
    UNTIL_BOUNCED,              /* each frame, until he bounces off it or its maker has gone */
    UNTIL_GONE,                 /* $B0 frames more, or until its maker has gone */
    LAST                        /* $57 frames */
};

/*
 * @routine 84:99DA Kind8CBounced
 * WHAT IT DOES: What kind $8C does as Aladdin bounces off it: its animation
 * $12 and sound $3B.
 */
static void bounced(Actor *a)
{
    AnimStart(a, 0x12);
    SoundQueueAdd(0x3B);
}

/*
 * @routine 84:9979 Kind8CStart
 * WHAT IT DOES: Actor kind $8C: rises as kind $5D's does (Kind5DStart's
 * rise); then flags $80, anim_flags 2, palette byte 6, animation $11. Each
 * frame, its maker gone (Kind5DParentGone): the last part; else Aladdin
 * bouncing off it (ActorBouncesPlayer): BOUNCED 1, Kind8CBounced, TIMER $B0,
 * and each frame the same tests (Kind8CBounced on a bounce) and TIMER down
 * to 0. The last part: anim_flags bit 4 set; TIMER $57; each frame a
 * bounce (Kind8CBounced) and TIMER down to 0; then it ends.
 */
int Kind8CStart(Actor *a)
{
    switch (a->resume) {
    case ACTOR_START:
        Kind5DRiseBegin(a);
        a->resume = RISE;
        return 1;
    case RISE:
        if (!Kind5DRiseFrame(a))
            return 1;
        a->anim_flags = 0x02;
        a->flags = 0x80;
        a->look = (uint16_t)((a->look & 0x00FF) | 0x0600);
        AnimStart(a, 0x11);
        a->resume = UNTIL_BOUNCED;
        return 1;
    case UNTIL_BOUNCED:
        if (!Kind5DParentGone(a)) {
            if (!ActorBouncesPlayer(a))
                return 1;
            a->own.bytes[BOUNCED] = 1;
            bounced(a);
            a->own.bytes[TIMER] = 0xB0;
            a->resume = UNTIL_GONE;
            return 1;
        }
        break;
    case UNTIL_GONE:
        if (!Kind5DParentGone(a)) {
            if (ActorBouncesPlayer(a))
                bounced(a);
            if (--a->own.bytes[TIMER])
                return 1;
        }
        break;
    case LAST:
        if (ActorBouncesPlayer(a))
            bounced(a);
        if (--a->own.bytes[TIMER])
            return 1;
        return ActorEndPlaced(a);
    default:
        return 1;
    }
    /* its maker has gone, or the time is up */
    a->anim_flags |= 0x10;
    a->own.bytes[TIMER] = 0x57;
    a->resume = LAST;
    return 1;
}
