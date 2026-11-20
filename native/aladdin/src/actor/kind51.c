/*
 * kind51.c: actor kind $51, a thing that rises and settles (Aladdin can
 * land on it once it has): its start and cycle, what it does in its second
 * state, and its variant with $07 bit 1 set, which plays once and is gone.
 * Its wait routine, ActorWaitWake, wakes it when Aladdin lands on it.
 */
#include "actor/actor_own.h"
#include "actor/kinds_d.h"
#include "audio/sound.h"
#include "video/video.h"

enum {
    DOWN = ACTOR_RESUME_OWN,    /* the cycle: down (animation 0), not to be landed on */
    UP,                         /* up (animation 1), to be landed on */
    VARIANT_PAUSE,              /* the variant: its pause with $07 bit 2 */
    VARIANT_PLAY,               /* its animation 5 */
    WOKEN_FIRST,                /* the second state: its first frame of animation 2 */
    WOKEN,                      /* then 32 frames of it */
    WOKEN_MORE,                 /* and 32 more */
    TURNED_AWAY                 /* 96 frames turned away from Aladdin */
};

/* The cycle's top: down for 64 frames, or 144 with $07 bit 0 set. */
static int cycle(Actor *a)
{
    a->own.bytes[OWN_41] = 0;
    AnimStart(a, 0);
    a->own.bytes[OWN_40] = (a->spawn_byte & 0x01) ? 0x90 : 0x40;
    a->resume = DOWN;
    return 1;
}

/* The variant's play: animation 5 for $58 frames. */
static int variant_play(Actor *a)
{
    AnimStart(a, 5);
    a->own.bytes[OWN_40] = 0x58;
    a->resume = VARIANT_PLAY;
    return 1;
}

/* The second state's turned-away frames: facing away from Aladdin, then a
   frame of waiting. */
static int turn_away(Actor *a)
{
    a->facing = a->flip = (uint8_t)(ActorSideOfPlayer(a) ^ 1);
    a->resume = TURNED_AWAY;
    return 1;
}

/*
 * @routine 83:D239 Kind51Start
 * WHAT IT DOES: Actor kind $51's start: drawn with its frame parity bits
 * and its wait routine on; with $07 bit 1 set it is the variant that plays
 * once, else it cycles for good.
 * Also 83:D24A Kind51Cycle: down (animation 0, not to be landed on) for 64
 * frames, or 144 with $07 bit 0 set; then up (animation 1, Aladdin can land
 * on it) for 128; again. 83:D279 Kind51Variant: after a 10-frame pause
 * when $07 bit 2 is set, its animation 5 for $58 frames, then gone for good
 * (83:D32F Kind51End). And 83:D2BE Kind51Second, its code in its second
 * state: animation 2 with sound $0F for 65 frames, then by a random pick
 * (list 6) animation 3, or animation 4 with a pair of actors like it
 * spawned (ActorSpawnPair); 96 frames turned away from Aladdin; then its
 * cycle. (Kind51Second sets its state's bits $0C before the cycle's first
 * yield, which marks it waiting over them: no effect. It reads the word at
 * $08F9 and does nothing with it.)
 */
int Kind51Start(Actor *a)
{
    uint8_t *frames = &a->own.bytes[OWN_40];

    switch (a->resume) {
    case ACTOR_START:
        a->anim_flags |= 0x03;
        a->flags |= 0x90;
        if (!(a->spawn_byte & 0x02))
            return cycle(a);
        a->own.bytes[OWN_41] = 0;
        if (!(a->spawn_byte & 0x04))
            return variant_play(a);
        *frames = 0x0A;
        a->resume = VARIANT_PAUSE;
        return 1;
    case VARIANT_PAUSE:
        return --*frames ? 1 : variant_play(a);
    case VARIANT_PLAY:
        AnimStep(a);
        return --*frames ? 1 : ActorEndPlaced(a);
    case DOWN:
        if (--*frames)
            return 1;
        AnimStart(a, 1);
        *frames = 0x80;
        a->own.bytes[OWN_41]++;
        a->resume = UP;
        return 1;
    case UP:
        return --*frames ? 1 : cycle(a);
    case ACTOR_START_SECOND:
        AnimStart(a, 2);
        SoundQueueAdd(0x0F);
        *frames = 0x20;
        a->resume = WOKEN_FIRST;
        return 1;
    case WOKEN_FIRST:
        AnimStep(a);
        a->resume = WOKEN;
        return 1;
    case WOKEN:
        AnimStep(a);
        if (--*frames)
            return 1;
        *frames = 0x20;
        a->resume = WOKEN_MORE;
        return 1;
    case WOKEN_MORE:
        AnimStep(a);
        if (--*frames)
            return 1;
        if ((RandomFromList(6) & 0xFF) == 0) {
            AnimStart(a, 3);
        } else {
            ActorSpawnPair(a);
            AnimStart(a, 4);
        }
        *frames = 0x60;
        return turn_away(a);
    case TURNED_AWAY:
        AnimStep(a);
        if (--*frames)
            return turn_away(a);
        a->flags |= 0x80;
        a->facing = 0;
        a->flip = 0;
        return cycle(a);
    default:
        return 1;
    }
}

/*
 * @routine 83:D2A2 ActorWaitWake
 * WHAT IT DOES: Kind $51's wait routine: each frame it waits, when bit 1
 * of its $07 is clear, its +$41 is set and Aladdin comes down on top of it
 * (PlayerLandsOnActor), it wakes: its state's bits $8C set (its second
 * entry at its next turn), its wait routine off. It ends once out of range
 * (box $10).
 */
int ActorWaitWake(Actor *a)
{
    if (!(a->spawn_byte & 0x02) && a->own.bytes[OWN_41] && PlayerLandsOnActor(a)) {
        a->state |= 0x8C;
        a->flags &= (uint8_t)~0x80;
    }
    return ActorEndOutOfRange(a, 0x10);
}
