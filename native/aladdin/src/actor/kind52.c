/*
 * kind52.c: actor kind $52's start, the third level end's guard of act 1
 * stage 0. Its wait routine is Kind52Wait (kind_waits.c, kinds D's).
 */
#include "actor/actor_own.h"
#include "audio/sound.h"
#include "data/kinds_b_tables.h"
#include "player/player.h"
#include "system/system.h"
#include "video/video.h"

/* its own bytes */
typedef struct {
    uint8_t before[OWN_40];
    uint8_t frames;             /* +$40: frames left of what it is doing */
    uint8_t rest;               /* +$41: frames left before it may strike again */
    uint8_t spare;              /* +$42: counted down each frame; set only by Kind52SpareWay (dead) */
    uint8_t turn_in;            /* +$43: the variant: frames left to its next turn */
    uint8_t life;               /* +$44: the variant: frames left of turning toward the player */
    uint8_t unused_45;          /* +$45 */
    uint8_t done_turning;       /* +$46: the variant: nonzero once its turning time is over */
    uint8_t hit_frames;         /* +$47: the second entry's frames left */
    uint8_t shootable;          /* +$48: nonzero, Kind52Wait tests shots on it */
} K52Own;

_Static_assert(sizeof(K52Own) <= sizeof(((Actor *)0)->own.bytes), "a view fits the own bytes");

static inline K52Own *k52(Actor *a) { return (K52Own *)a->own.bytes; }

enum {
    WAIT_NEAR = ACTOR_RESUME_OWN,   /* until the player comes near */
    RISE_1,                         /* pose 4, $17 frames */
    RISE_2,                         /* shootable, 9 frames */
    RISE_3,                         /* pose 1, $18 frames */
    LOOP,                           /* each frame: faces him, picks a way by his distance */
    STRIKE,                         /* pose 2, $6C frames */
    STAND_0,                        /* far: pose 0 */
    STAND_1,                        /* far: pose 1 */
    FLY,                            /* the variant, each frame */
    HIT                             /* the second entry: pose 3, $A0 frames */
};

/* Faces the player. */
static void face_player(Actor *a)
{
    a->facing = ActorSideOfPlayer(a);
    a->flip = (uint8_t)(a->facing ^ 0x01);
}

/* The far ways: pose 0 for $10 frames (RandomNext's high byte negative)
   or $20. */
static int stand_0(Actor *a)
{
    k52(a)->frames = (RandomNext() & 0x80) ? 0x10 : 0x20;
    AnimStart(a, 0);
    a->resume = STAND_0;
    return 1;
}

/*
 * @routine 83:D359 Kind52Start
 * WHAT IT DOES: Actor kind $52 started: not shootable, anim_flags bit 1.
 * +$07 zero: its wait routine on (flags $90), spawn place and +$07 $4C,
 * pose 5, until the player is inside box $34 (ActorNearPlayer); it faces
 * him and rises (pose 4 for $17 frames, then shootable, 9 frames, pose 1
 * for $18 frames). Then each frame, facing him, by his distance across:
 * within $38 pixels it strikes (pose 2 for $6C frames, sound $20 with $42
 * frames left, then pose 0 and a rest of RandomFromList $0A frames) unless
 * resting; further it stands, pose 0 for $10 or $20 frames then pose 1 for
 * $20 or $2C (by RandomNext), and again, until he is inside box $24. Its
 * rest and +$42 count down each of those frames.
 * +$07 nonzero, the variant: pose 7, anim_flags bits 0-1 cleared, its
 * facing times 8 (a direction of 16), turning one step toward the player
 * (ActorDirectionToPlayer) every $14 frames for RandomFromList $10 frames;
 * each frame it moves by direction set $12, its animation steps, it ends
 * out of range (box 0), and the player is hurt (PlayerEnter8CFacing) when
 * its zone $34 meets his test box.
 * 83:D42E Kind52SpareWay, a way of the loop with pose 6 and a copy of
 * itself, is dead: the way table at $83:D3F7 holds $D3FD, $D46C, $D46C (its
 * X is 0, 2 or 4) and no branch, jump, pointer or resume address in the
 * ROM names $D42E-$D46B (the record's proof); not converted.
 * Its second entry (state $80 up: a shot ended in it by Kind52Wait), at
 * 83:D545 with no tag of its own in the record: not shootable, sound $24,
 * pose 3, +$07 0, its animation on for $A0 frames (+$47), then it starts
 * again from its start entry, as the guard.
 */
int Kind52Start(Actor *a)
{
    K52Own *k = k52(a);
    uint16_t across;

    switch (a->resume) {
    case ACTOR_START:
        k->shootable = 0;
        a->anim_flags |= 0x02;
        if (a->spawn_byte) {
            AnimStart(a, 7);
            a->anim_flags &= (uint8_t)~0x03;
            a->facing = (uint8_t)(a->facing << 3);
            k->life = (uint8_t)RandomFromList(0x10 / 2);
            k->turn_in = 0x14;
            k->done_turning = 0;
            a->resume = FLY;
            return 1;
        }
        a->flags |= 0x90;
        a->spawn_at = 0x4C;
        a->spawn_byte = 0x4C;
        AnimStart(a, 5);
        a->resume = WAIT_NEAR;
        return 1;
    case WAIT_NEAR:
        if (!ActorNearPlayer(a, 0x34))
            return 1;
        face_player(a);
        AnimStart(a, 4);
        k->frames = 0x17;
        a->resume = RISE_1;
        return 1;
    case RISE_1:
        if (--k->frames)
            return 1;
        k->shootable = 1;
        k->frames = 0x09;
        a->resume = RISE_2;
        return 1;
    case RISE_2:
        if (--k->frames)
            return 1;
        AnimStart(a, 1);
        k->frames = 0x18;
        a->resume = RISE_3;
        return 1;
    case RISE_3:
        if (--k->frames)
            return 1;
        a->resume = LOOP;
        return 1;
    case LOOP:
        face_player(a);
        across = (uint16_t)(pixels(a->x) - pixels(g.actors[0].x));
        if (across & 0x8000)
            across = (uint16_t)-across;
        if (k->rest)
            k->rest--;
        if (k->spare)
            k->spare--;
        if (across >= 0x38)
            return stand_0(a);
        if (k->rest)
            return 1;
        AnimStart(a, 2);
        k->frames = 0x6C;
        a->resume = STRIKE;
        return 1;
    case STRIKE:
        if (k->frames == 0x42)
            SoundQueueAdd(0x20);
        if (--k->frames)
            return 1;
        AnimStart(a, 0);
        k->rest = (uint8_t)RandomFromList(0x0A / 2);
        a->resume = LOOP;
        return 1;
    case STAND_0:
        if (!--k->frames) {
            k->frames = (RandomNext() & 0x80) ? 0x20 : 0x2C;
            AnimStart(a, 1);
            a->resume = STAND_1;
            return 1;
        }
        if (ActorNearPlayer(a, 0x24))
            a->resume = LOOP;
        return 1;
    case STAND_1:
        if (!--k->frames)
            return stand_0(a);
        if (ActorNearPlayer(a, 0x24))
            a->resume = LOOP;
        return 1;
    case FLY:
        if (!k->done_turning) {
            if (!--k->life)
                k->done_turning++;
            if (!--k->turn_in) {
                uint8_t off;

                k->turn_in = 0x14;
                off = (uint8_t)((ActorDirectionToPlayer(a) - a->facing) & 0x0F);
                if (off)
                    a->facing = (uint8_t)((off >= 0x08 ? a->facing - 1 : a->facing + 1) & 0x0F);
            }
        }
        ActorMoveByVector(a, 0x12);
        AnimStep(a);
        if (ActorEndOutOfRange(a, 0x00) == ACTOR_ENDED)
            return ACTOR_ENDED;
        if (ActorZoneHasTestBox(a, 0x34))
            (void)PlayerEnter8CFacing(a);
        return 1;
    case ACTOR_START_SECOND:
        k->shootable = 0;
        SoundQueueAdd(0x24);
        AnimStart(a, 3);
        a->spawn_byte = 0;
        k->hit_frames = 0xA0;
        a->resume = HIT;
        return 1;
    case HIT:
        AnimStep(a);
        if (--k->hit_frames)
            return 1;
        a->resume = ACTOR_START;
        return Kind52Start(a);
    default:
        return 1;
    }
}


/*
 * @routine 83:D518 Kind52Wait
 * WHAT IT DOES: Kind $52's wait routine: its animation on; when it is
 * shootable a shot ends in it (ActorEndsShot); stomped or hurting as
 * touched (ActorStompOrHurt); on its animation's pictures 0-3 and $0A
 * (kinds_b_tables) the player is hurt when that picture's zone meets his
 * test box; it ends when out of range (box 0).
 */
int Kind52Wait(Actor *a)
{
    int i;

    AnimStep(a);
    if (k52(a)->shootable)
        (void)ActorEndsShot(a);
    (void)ActorStompOrHurt(a);
    for (i = K52_HURT_PICTURES - 1; i >= 0; i--)
        if (a->picture == k52_hurt_picture((uint8_t)i)) {
            if (ActorZoneHasTestBox(a, k52_hurt_zone((uint8_t)i)))
                (void)PlayerEnter8CFacing(a);
            break;
        }
    return ActorEndOutOfRange(a, 0x00);
}
