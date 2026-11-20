/*
 * kind3c.c: actor kind $3C, a thing that drifts across while it bobs up
 * and down, and bounces Aladdin when he comes down on it.
 */
#include "actor/actor_own.h"
#include "audio/sound.h"
#include "data/actor_a_tables.h"

#define LEG OWN_40              /* +$40: frames left before its vertical speed turns */
#define LEG_FRAMES 0x55

enum { DRIFT = ACTOR_RESUME_OWN };

/*
 * @routine 83:B109 ActorSpeedYTurn
 * WHAT IT DOES: Turns the actor's vertical speed round: its top byte
 * inverted, its low word negated (the carry of the negation is lost, as the
 * original's).
 */
static void ActorSpeedYTurn(Actor *a)
{
    uint32_t v = wrap24((uint32_t)a->speed_y);
    uint32_t top = (~v >> 16) & 0xFF;
    uint32_t low = (uint16_t)(~v + 1);

    a->speed_y = signed24(top << 16 | low);
}

/*
 * @routine 83:B0CB Kind3CDrift
 * WHAT IT DOES: Actor kind $3C (through its second entry 83:B150: a JMP to
 * ActorEndPlaced, the same end): anim_flags bit 1, flags $90, its probe
 * list, facing right, animation 0, the speeds of entries $15 across and $42
 * vertical; it drifts up and down: its vertical speed turns every $55
 * frames.
 */
int Kind3CDrift(Actor *a)
{
    switch (a->resume) {
    case ACTOR_START:
        a->anim_flags |= 0x02;
        a->flags |= 0x90;
        a->probes = k3c_probes();
        a->facing = 0;
        a->flip = 0;
        AnimStart(a, 0);
        ActorSpeedXSet(a, 0x15);
        ActorSpeedYSet(a, 0x42);
        a->own.bytes[LEG] = LEG_FRAMES;
        ActorSpeedYTurn(a);
        a->resume = DRIFT;
        return 1;
    case ACTOR_START_SECOND:
        return ActorEndPlaced(a);
    default:
        ActorMoveY(a);
        if (--a->own.bytes[LEG])
            return 1;
        a->own.bytes[LEG] = LEG_FRAMES;
        ActorSpeedYTurn(a);
        return 1;
    }
}

/*
 * @routine 83:B11C Kind3CWait
 * WHAT IT DOES: Kind $3C's wait routine: drifts across, turning round on
 * the even counts of the frame count's high byte; while high on the screen
 * (its y pixel's low byte below $D4) it bounces Aladdin, with sound $12, and
 * then turns its rise into a fall (while rising: the frames it has gone
 * turn round); ended once left behind.
 */
int Kind3CWait(Actor *a)
{
    if (!(g.sprite_flicker & 0x01))
        a->facing ^= 0x01;
    ActorMoveX(a);
    if ((uint8_t)pixels(a->y) < 0xD4 && ActorBouncesPlayer(a)) {
        SoundQueueAdd(0x12);
        if (a->speed_y < 0) {
            a->own.bytes[LEG] = (uint8_t)(LEG_FRAMES - a->own.bytes[LEG]);
            ActorSpeedYTurn(a);
        }
    }
    return ActorEndIfLeftBehind(a);
}
