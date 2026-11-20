/*
 * knocked_away.c: an actor knocked away by Aladdin: it shows a pose,
 * sounds, takes a motion preset and flies off until it is out of range,
 * where it ends. A kind takes it over for good (actor.h: it calls one of
 * these at the same resume point each turn, with nothing before the call
 * there; it returns waits until it ends the actor). The three entries
 * share one course, kept in a->call_at.
 */
#include "actor/actor_own.h"
#include "audio/sound.h"
#include "video/video.h"

enum {
    KNOCKED_POSE = 1,           /* 83:9553: its 16 frames' pose */
    KNOCKED_WAITED,             /* 83:959B: the Resumed entries' five frames are over: the flight's first frame waits */
    KNOCKED_FLY                 /* 83:9561: the flight */
};

#define FLY_LEVEL_A 0x0D        /* on these levels a falling actor drops behind the layers */
#define FLY_LEVEL_B 0x02

/* a->call_at's course after the first frame; the wait, or ACTOR_ENDED */
static int knocked_on(Actor *a)
{
    if (a->call_at == KNOCKED_POSE) {
        AnimStep(a);
        ActorPictureSend(a);
        if (--a->own.bytes[OWN_40] != 0)
            return 1;
        a->call_at = KNOCKED_FLY;
        return 1;
    }
    if (a->call_at == KNOCKED_WAITED) {
        a->call_at = KNOCKED_FLY;
        return 1;
    }
    /* each frame of the flight: its animation on, it flies; falling on
       levels FLY_LEVEL_A and FLY_LEVEL_B, its priority bits cleared; out
       of range by box 0, it ends */
    AnimStep(a);
    ActorPictureSend(a);
    ActorFly(a);
    if ((g.level & 0xFF) == FLY_LEVEL_A || (g.level & 0xFF) == FLY_LEVEL_B)
        if (a->speed_y >= 0)
            a->flags &= (uint8_t)~0x30;
    if (ActorEndOutOfRange(a, 0) == ACTOR_ENDED)
        return ACTOR_ENDED;
    return 1;
}

/*
 * @routine 83:9532 ActorKnockedAway
 * WHAT IT DOES: Knocks the actor away: animation `anim`, sound `sound`,
 * motion preset `preset`; it faces away from Aladdin, poses 16 frames
 * (its animation on), then flies off each frame until it is out of range
 * (box 0) and ends. On levels $0D and $02 a falling actor's priority bits
 * are cleared.
 */
int ActorKnockedAway(Actor *a, uint8_t anim, uint8_t sound, uint8_t preset)
{
    uint8_t away;

    if (a->call_at)
        return knocked_on(a);
    AnimStart(a, anim);
    ActorPictureSend(a);
    ActorMotionPreset(a, preset);
    SoundQueueAdd(sound);
    away = g.actors[0].flip ^ 0x01;
    a->facing = away;
    a->flip = away;
    a->own.bytes[OWN_40] = 0x10;
    a->call_at = KNOCKED_POSE;
    return 1;
}

/* ActorKnockedAwayResumed from its pose on: animation `anim` taken up at
   frame `frame`, motion preset `preset`; five frames, then the flight */
static int resumed(Actor *a, uint8_t anim, uint8_t frame, uint8_t preset)
{
    a->frame = frame;
    AnimResume(a, anim);
    ActorPictureSend(a);
    ActorMotionPreset(a, preset);
    a->call_at = KNOCKED_WAITED;
    return 5;
}

/*
 * @routine 83:9589 ActorKnockedAwayResumed
 * WHAT IT DOES: Knocks the actor away with its animation taken up where it
 * is: animation `anim` at frame `frame`, motion preset `preset`, five
 * frames' wait, then ActorKnockedAway's flight until it is out of range
 * and ends.
 */
int ActorKnockedAwayResumed(Actor *a, uint8_t anim, uint8_t frame, uint8_t preset)
{
    if (a->call_at)
        return knocked_on(a);
    return resumed(a, anim, frame, preset);
}

/*
 * @routine 83:959F ActorKnockedAwayResumedB
 * WHAT IT DOES: ActorKnockedAwayResumed's second copy, for kind $2D's
 * thrown copy: the same pose, preset, five frames' wait and flight.
 */
int ActorKnockedAwayResumedB(Actor *a, uint8_t anim, uint8_t frame, uint8_t preset)
{
    if (a->call_at)
        return knocked_on(a);
    return resumed(a, anim, frame, preset);
}
