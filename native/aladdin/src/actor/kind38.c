/*
 * kind38.c: actor kind $38, which rolls along until Aladdin is close, then
 * turns to him and flies up at him; once downed (its second entry,
 * Kind37Second) it drops or flies away. Its wait routine is Kind37Wait
 * (83:BD22, kind $37's start, the actor core's).
 *
 * Not converted, dead (the record's own proofs):
 *   83:BEA0 Kind38StepEndOutOfRange  "Nothing in the game calls it (no JSR,
 *                                     JSL, JMP, pointer or word in the ROM
 *                                     names it)"
 *   83:BEAF Kind38End                "No branch, jump, word or resume address
 *                                     in the ROM leads to it"
 */
#include "actor/actor_own.h"
#include "audio/sound.h"
#include "data/kinds_b_tables.h"

/* its own bytes */
typedef struct {
    uint8_t before[OWN_40];
    uint8_t frames;             /* +$40: frames left of a pause */
    uint8_t unused_41_44[4];    /* +$41-$44 */
    uint8_t up;                 /* +$45: set once it flies (Kind37Wait then ends it as other actors) */
    uint8_t hit;                /* +$46: Kind37Wait's: 1 bounced on, 2 downed by a shot */
    uint8_t frame_kept;         /* +$47: ActorYByFrame47's frame */
    uint8_t side;               /* +$48: the side Aladdin is on (ActorSideOfPlayer) */
    uint8_t checks;             /* +$49: near checks left before it may turn */
} K38Own;

static inline K38Own *k38(Actor *a) { return (K38Own *)a->own.bytes; }

enum {
    ROLL = ACTOR_RESUME_OWN,    /* each frame it rolls */
    TURN_PAUSE,                 /* turned to him: 16 frames */
    SHOW,                       /* animation 1: 32 frames */
    FLY,                        /* flies up at him, for good */
    DOWN_SHOW,                  /* bounced on: 10 frames of its animation */
    DOWN_RISE,                  /* 16 frames rising against the pull */
    DOWN_PAUSE,                 /* 16 frames */
    DOWN_DROP,                  /* drops away until out of range */
    SHOT_WAITED,                /* downed by a shot: after its 10 frames, the loop's first BRK */
    SHOT_AWAY                   /* flies away until out of range */
};

/* The turn to him: 16 frames facing his side. */
static int turn(Actor *a)
{
    K38Own *k = k38(a);

    k->frames = 0x10;
    a->facing = (uint8_t)(k->side & 0x01);
    a->flip = a->facing;
    a->resume = TURN_PAUSE;
    return 1;
}

/* A frame that ends it out of range by box 0, else waits a frame. */
static int on_or_out(Actor *a)
{
    return ActorEndOutOfRange(a, 0) == ACTOR_ENDED ? ACTOR_ENDED : 1;
}

/*
 * @routine 83:BE01 Kind38Start
 * WHAT IT DOES: Actor kind $38: anim_flags 2, flags $90, facing and flip
 * 1, animation 0, its probe list ($80:D1D6). It rolls along (speed entry
 * $18, its height following its frames, ActorYByFrame47) until it meets a
 * wall (ActorOutOfWallProbe4) or Aladdin is close across (span 2): at the
 * first near check, and after that every 16th, a random pick from list
 * side + 3 by the side he is on; 0 rolls on for 16 more checks. Then it
 * turns to his side, waits 16 frames, flies (+$45 set), shows animation 1
 * for 32 frames and flies up at him (preset $30, against the pull) for
 * good.
 * Its +$4A is set to 0 and +$4B to 8 as the original's (no code of kind
 * $38 reads them).
 *
 * @routine 83:BD72 Kind37Second
 * WHAT IT DOES: Kind $38's second entry (state $8C, once Kind37Wait has
 * downed it): animation 3 and sound $0F. Bounced on (+$46 1): 10 frames of
 * its animation, preset $26's vertical half and 16 frames rising against
 * the pull, 16 frames still, then down (speed entry $27) until out of
 * range (box 0). Downed by a shot: preset $1F, 10 frames, then it flies
 * until out of range. (It is kind $37's second entry too: kind $37's code
 * is its own session's.)
 */
int Kind38Start(Actor *a)
{
    K38Own *k = k38(a);

    switch (a->resume) {
    case ACTOR_START:
        a->anim_flags = 0x02;
        a->flags = 0x90;
        a->facing = 1;
        a->flip = 1;
        AnimStart(a, 0);
        a->probes = k38_probes();
        k->frame_kept = 0;
        a->own.bytes[OWN_4B] = 0x08;
        a->own.bytes[OWN_4A] = 0;
        k->up = 0;
        k->hit = 0;
        k->checks = 1;
        ActorSpeedXSet(a, 0x18);
        a->resume = ROLL;
        return 1;
    case ROLL:
        ActorMoveX(a);
        ActorYByFrame47(a);
        if (ActorOutOfWallProbe4(a)) {
            k->side = ActorSideOfPlayer(a);
            return turn(a);
        }
        if (!ActorNearPlayerX(a, 0x02))
            return 1;
        if (--k->checks)
            return 1;
        k->side = ActorSideOfPlayer(a);
        if (RandomFromList((uint8_t)(k->side + 3)) & 0xFF)
            return turn(a);
        k->checks = 0x10;
        return 1;
    case TURN_PAUSE:
        if (--k->frames)
            return 1;
        k->up++;
        AnimStart(a, 1);
        k->frames = 0x20;
        a->resume = SHOW;
        return 1;
    case SHOW:
        if (--k->frames)
            return 1;
        ActorMotionPreset(a, 0x30);
        a->resume = FLY;
        return 1;
    case FLY:
        ActorFlyAgainst(a);
        return 1;

    case ACTOR_START_SECOND:
        AnimStart(a, 3);
        SoundQueueAdd(0x0F);
        if (k->hit != 1) {
            ActorMotionPreset(a, 0x1F);
            a->resume = SHOT_WAITED;
            return 0x0A;
        }
        k->frames = 0x0A;
        a->resume = DOWN_SHOW;
        return 1;
    case DOWN_SHOW:
        AnimStep(a);
        if (--k->frames)
            return 1;
        k->frames = 0x10;
        ActorMotionPresetY(a, 0x26);
        a->resume = DOWN_RISE;
        return 1;
    case DOWN_RISE:
        AnimStep(a);
        ActorFallAgainst(a);
        if (--k->frames)
            return 1;
        k->frames = 0x10;
        a->resume = DOWN_PAUSE;
        return 1;
    case DOWN_PAUSE:
        AnimStep(a);
        if (--k->frames)
            return 1;
        ActorSpeedYSet(a, 0x27);
        a->resume = DOWN_DROP;
        return 1;
    case DOWN_DROP:
        AnimStep(a);
        ActorMoveY(a);
        return on_or_out(a);
    case SHOT_WAITED:
        a->resume = SHOT_AWAY;
        return 1;
    case SHOT_AWAY:
        AnimStep(a);
        ActorFly(a);
        return on_or_out(a);
    default:
        return 1;
    }
}
