/*
 * kind5b.c: actor kind $5B, a thing that is thrown up and falls away:
 * with +$07 bit 7 clear it first shows, waits, leaves a copy of itself
 * (counted in its kind's count), then is thrown; with bit 7 set (the copy)
 * it is thrown at once and lets its wait routine know it is falling.
 */
#include "actor/actor_own.h"
#include "audio/sound.h"
#include "data/kinds_b_tables.h"

/* its own bytes */
typedef struct {
    uint8_t before[OWN_40];
    uint8_t frames;             /* +$40: frames left of the fall */
    uint8_t kept_07;            /* +$41: its +$07, kept over the copy's spawn */
    uint8_t falling;            /* +$42: set while the bit 7 kind falls (its wait routine reads it) */
    uint8_t cleared_43;         /* +$43: cleared as it starts */
    uint8_t between[OWN_4C - OWN_44];
    uint8_t carry_box;          /* +$4C: the box ActorTestCarries tests the player with */
} K5BOwn;

static inline K5BOwn *k5b(Actor *a) { return (K5BOwn *)a->own.bytes; }

enum { POSE_1 = ACTOR_RESUME_OWN, COPY, RISE, PAUSE, FALL };

#define COPY_SPAWN_AT 0x4A      /* the copy's spawn place */

/* Thrown: no speed across, sent up by speed entry $3C, gravity 4; it rises
   from the next frame on. */
static int thrown(Actor *a)
{
    ActorStopX(a);
    ActorSpeedYSet(a, 0x3C);
    a->accel_y = 0x04;
    a->resume = RISE;
    return 1;
}

/*
 * @routine 83:DF14 Kind5BStart
 * WHAT IT DOES: Actor kind $5B: its wait routine on (flags $90), +$42 and
 * +$43 0, its +$07 kept at +$41. +$07 bit 7 clear: anim_flags bits 0-1,
 * pose 2 for its delay (by +$07, kinds_b_tables), pose 1 for 32 frames, a
 * copy of itself at spawn place $4A with +$07 $FF (ActorSpawnSameLook;
 * spawned: its kind's count up), +$07 back; then thrown up: it flies while
 * it rises, waits 40 frames, flies for 128 and ends. Bit 7 set:
 * anim_flags bit 1, pose 0, thrown at once with +$42 set; it rises, waits
 * 40 frames, flies for 96, +$42 cleared, and it ends. Its second entry
 * (state $80 up), 83:DFC9, ends it (ActorEndPlaced).
 */
int Kind5BStart(Actor *a)
{
    K5BOwn *k = k5b(a);
    uint8_t wait;

    switch (a->resume) {
    case ACTOR_START_SECOND:            /* 83:DFC9: a JML to ActorEndPlaced */
        return ActorEndPlaced(a);
    case ACTOR_START:
        k->cleared_43 = 0;
        k->falling = 0;
        a->flags |= 0x90;
        k->kept_07 = a->spawn_byte;
        if (a->spawn_byte & 0x80) {
            a->anim_flags |= 0x02;
            AnimStart(a, 0);
            k->falling++;
            return thrown(a);
        }
        a->anim_flags |= 0x03;
        AnimStart(a, 2);
        wait = k5b_delay(a->spawn_byte);
        a->resume = POSE_1;
        return wait ? wait : 256;
    case POSE_1:
        AnimStart(a, 1);
        a->resume = COPY;
        return 0x20;
    case COPY:
        a->spawn_at = COPY_SPAWN_AT;
        a->spawn_byte = 0xFF;
        if (ActorSpawnSameLook(a, a->kind))
            g.kind_count[a->kind]++;
        a->spawn_byte = k->kept_07;
        return thrown(a);
    case RISE:
        ActorFly(a);
        if (a->speed_y < 0)
            return 1;
        a->resume = PAUSE;
        return 0x28;
    case PAUSE:
        k->frames = (k->kept_07 & 0x80) ? 0x60 : 0x80;
        a->resume = FALL;
        return 1;
    case FALL:
        ActorFly(a);
        if (--k->frames)
            return 1;
        if (k->kept_07 & 0x80)
            k->falling = 0;
        return ActorEndPlaced(a);
    default:
        return 1;
    }
}


/*
 * @routine 83:DFCD ActorWaitStepSounds
 * WHAT IT DOES: Kind $5B's wait routine: its animation on. +$07 bit 7 set
 * (the falling copy): while +$42 is set, every 8th count of the frame
 * count ($032D) a sound by its x's low 4 bits (kinds_b_tables), and it
 * hurts the player on touch. Bit 7 clear: it carries the player
 * (ActorCarryPlayer) and tests whether he rides it (ActorTestCarries, test
 * 2, box $3C at +$4C). It ends once the camera has left it behind.
 */
int ActorWaitStepSounds(Actor *a)
{
    AnimStep(a);
    if (a->spawn_byte & 0x80) {
        if (k5b(a)->falling && !(g.sprite_flicker & 0x07))
            SoundQueueAdd(k5b_step_sound((uint8_t)(pixels(a->x) & 0x0F)));
        (void)ActorHurtsPlayerOnTouch(a);
    } else {
        ActorCarryPlayer(a);
        k5b(a)->carry_box = 0x3C;
        ActorTestCarries(a, 0x02);
    }
    return ActorEndIfLeftBehind(a);
}
