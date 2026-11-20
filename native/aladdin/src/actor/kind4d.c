/*
 * kind4d.c: actor kind $4D, a thing that gives way when it is hit, leaps
 * and falls, then drops its children once.
 */
#include "actor/actor_own.h"
#include "audio/sound.h"

/* its own bytes */
typedef struct {
    uint8_t before[OWN_40];
    uint8_t frames;             /* +$40: frames left */
    uint8_t unused_41;          /* +$41 */
    uint8_t hits;               /* +$42: set when it is hit (by its hit code); 1 the first time */
} K4DOwn;

static inline K4DOwn *k4d(Actor *a) { return (K4DOwn *)a->own.bytes; }

enum { WAIT_HIT = ACTOR_RESUME_OWN, PAUSE, FALL, STAY };

/*
 * @routine 83:CE7B Kind4DStart
 * WHAT IT DOES: Actor kind $4D, a thing that gives way when it is hit: its
 * wait routine on, anim_flags bit 1, pose 0, until its +$42 is set. Then
 * sound $12, a pause of 8 frames on the first hit (1 after), pose 1, sent
 * up (speed entry $7E, gravity $38) and falling for 53 frames, on the
 * first hit facing away from the player. Then pose 2, sent up again (entry
 * $2A), sound $3C, anim_flags bits 0-1, its children dropped once
 * (ActorFlagOnce), and it stays.
 * The original first counts a byte of copier RAM ($70:1000 + X) up and,
 * when the count stays written, fills work RAM $0000-$001F with $AA; on the
 * cartridge that byte is open bus and never reads back what was written,
 * so the fill never runs and is not converted.
 */
int Kind4DStart(Actor *a)
{
    K4DOwn *k = k4d(a);

    switch (a->resume) {
    case ACTOR_START:
        a->anim_flags |= 0x02;
        a->flags |= 0x90;
        AnimStart(a, 0);
        k->hits = 0;
        a->resume = WAIT_HIT;
        return 1;
    case WAIT_HIT:
        if (!k->hits)
            return 1;
        SoundQueueAdd(0x12);
        k->frames = k->hits == 1 ? 0x08 : 0x01;
        a->resume = PAUSE;
        return 1;
    case PAUSE:
        if (--k->frames)
            return 1;
        AnimStart(a, 1);
        ActorSpeedYSet(a, 0x7E);
        a->accel_y = 0x38;
        k->frames = 0x35;
        if (k->hits == 1) {
            a->facing = (uint8_t)(g.actors[0].flip ^ 0x01);
            a->flip = a->facing;
        }
        a->resume = FALL;
        return 1;
    case FALL:
        ActorFall(a);
        if (--k->frames)
            return 1;
        AnimStart(a, 2);
        ActorSpeedYSet(a, 0x2A);
        SoundQueueAdd(0x3C);
        a->anim_flags |= 0x03;
        ActorFlagOnce(a);
        a->resume = STAY;
        return 1;
    default:                            /* STAY */
        return 1;
    }
}


/*
 * @routine 83:CF8C ActorWaitTest42Anim
 * WHAT IT DOES: Kind $4D's wait routine. Each frame it waits, while its
 * +$42 is 0: Aladdin bouncing off it counts 1, a shot reaching it 2 more;
 * then its animation on, and it ends once out of range (box $10).
 */
int ActorWaitTest42Anim(Actor *a)
{
    K4DOwn *k = k4d(a);

    if (!k->hits) {
        if (ActorBouncesPlayer(a))
            k->hits++;
        if (ActorTakesShot(a))
            k->hits += 2;
    }
    AnimStep(a);
    return ActorEndOutOfRange(a, 0x10);
}
