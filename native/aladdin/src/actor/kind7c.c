/*
 * kind7c.c: actor kind $7C, the backdrop mover, and kind $A3, a shot that
 * steps out of its sender and flies off.
 */
#include "actor/actor_a.h"
#include "audio/sound.h"
#include "data/actor_a_tables.h"

int Kind7DKeepSpot(Actor *a);   /* kind $7D's start (kind7d.c): this kind's second entry */

#define MOVE_FRAMES OWN_40      /* +$40 (word): frames left of its move ($0333) */
#define HUM OWN_42              /* +$42: frames until its hum is queued again */
#define SOUNDED OWN_43          /* +$43: nonzero, its end sound was queued */

#define MOVE_LENGTH 0x0333
#define SCROLL_END 0x08E8       /* once the play field's scroll x reaches it, the mover's sounds stop */

enum { MOVE = ACTOR_RESUME_OWN, TURN };

static bool camera_past_x08e8(void)
{
    return pixels(g.layers[0].scroll_x) >= SCROLL_END;
}

/*
 * @routine 84:90C2 MoverSound
 * WHAT IT DOES: Queues sound `command` while the camera is not yet past
 * x $08E8 (CameraPastX08E8: the play field's scroll x reached it).
 */
static void MoverSound(uint8_t command)
{
    if (!camera_past_x08e8())
        SoundQueueAdd(command);
}

/*
 * @routine 84:9083 BackdropOffsetStep
 * WHAT IT DOES: Moves the backdrop layer against the play field by one
 * frame's step for the actor's facing (added to the backdrop's y offset, 32
 * bits), and counts the actor's frames: true when its +$40 word reached 0.
 */
static bool BackdropOffsetStep(Actor *a)
{
    uint32_t offset = g.backdrop_offset_y | (uint32_t)g.backdrop_offset_y_high << 16;
    uint16_t frames = (uint16_t)(a->own.bytes[MOVE_FRAMES] | a->own.bytes[MOVE_FRAMES + 1] << 8);

    offset += backdrop_step(a->facing);
    g.backdrop_offset_y = (uint16_t)offset;
    g.backdrop_offset_y_high = (uint16_t)(offset >> 16);
    frames--;
    a->own.bytes[MOVE_FRAMES] = (uint8_t)frames;
    a->own.bytes[MOVE_FRAMES + 1] = (uint8_t)(frames >> 8);
    return frames == 0;
}

/* a move's start: $0333 frames to count out */
static void move_start(Actor *a)
{
    a->own.bytes[MOVE_FRAMES] = (uint8_t)MOVE_LENGTH;
    a->own.bytes[MOVE_FRAMES + 1] = (uint8_t)(MOVE_LENGTH >> 8);
}

/*
 * @routine 84:9044 BackdropMover
 * WHAT IT DOES: Actor kind $7C: moves the backdrop layer against the play
 * field one way for $0333 frames with a hum (sound $3F) every $30, stops
 * with a clunk ($0D), waits $3F frames, and goes back the other way, for
 * good. Its second entry (83:9297) goes on as kind $7D's start
 * (Kind7DKeepSpot). The record's kind $76 also takes this code as its second
 * entry.
 */
int Kind7CStart(Actor *a)
{
    switch (a->resume) {
    case ACTOR_START:
        a->x = 0;
        a->y = with_pixels(a->y, 0);
        a->flags = 0x80;
        a->own.bytes[HUM] = 0;
        a->own.bytes[SOUNDED] = 0;
        move_start(a);
        a->resume = MOVE;
        return 1;
    case ACTOR_START_SECOND:
        return ActorRunAs(a, Kind7DKeepSpot, ACTOR_START);
    case MOVE:
        if (--a->own.bytes[HUM] & 0x80) {
            MoverSound(0x3F);
            a->own.bytes[HUM] = 0x30;
        }
        if (!BackdropOffsetStep(a))
            return 1;
        MoverSound(0x0D);
        a->resume = TURN;
        return 0x3F;
    default:                            /* TURN */
        a->facing ^= 0x01;
        move_start(a);
        a->resume = MOVE;
        return 1;
    }
}

/*
 * @routine 84:90A2 MoverEndSound
 * WHAT IT DOES: Kind $7C's wait routine (83:9292 ActorKind7CWait): once the
 * camera is past x $08E8, queues sound command $F1 one time; before that
 * keeps its flag clear so it can be queued.
 */
int Kind7CWait(Actor *a)
{
    if (!camera_past_x08e8())
        a->own.bytes[SOUNDED] = 0;
    else if (!a->own.bytes[SOUNDED]) {
        SoundQueueAdd(0xF1);
        a->own.bytes[SOUNDED]++;
    }
    return ACTOR_GOES_ON;
}

enum { A3_STEP = ACTOR_RESUME_OWN, A3_FLY0, A3_FLY };

/*
 * @routine 84:8349 KindA3Shot
 * WHAT IT DOES: Actor kind $A3: a shot facing right with its look $0400;
 * shown, it waits 8 frames, steps out across by its +$07's step, waits 8
 * more, then flies off (motion preset $6C) until it can no longer be hit,
 * and ends. Its wait routine animates it (the core's AnimateWhileWaiting,
 * which the record's KindA3Animate is).
 */
int KindA3Shot(Actor *a)
{
    switch (a->resume) {
    case ACTOR_START:
        a->anim_flags |= 0x02;
        a->flags |= 0x80;
        a->look = 0x0400;
        a->flip = 0;
        a->facing = 0;
        AnimStart(a, ka3_anim(a->spawn_byte));
        a->resume = A3_STEP;
        return 8;
    case A3_STEP:
        a->x = with_pixels(a->x, (uint16_t)(pixels(a->x) + (uint16_t)ka3_step(a->spawn_byte)));
        a->resume = A3_FLY0;
        return 8;
    case A3_FLY0:
        ActorMotionPreset(a, 0x6C);
        a->resume = A3_FLY;
        return 1;
    default:
        ActorFly(a);
        if (a->flags & 0x40)
            return 1;
        return ActorEndPlaced(a);
    }
}
