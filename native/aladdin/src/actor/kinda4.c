/*
 * kinda4.c: actor kind $A4, figures of the Genie's screen with its cue
 * (g.screen_cue, as kind $A8's) and its step (g.genie_screen_step):
 * by its $07 one blinks while the cue is set, one drops and shakes the
 * screen's y, one rises, flies and leaves a child, and two only pose once
 * and end. Its wait routine is ActorKindA4Wait.
 */
#include <stdio.h>

#include "actor/actor_own.h"
#include "actor/kinds_d.h"
#include "audio/sound.h"
#include "data/kinds_d_tables.h"

/* its own bytes */
#define COUNT OWN_40            /* way 4: frames of its flight */
#define LEFT OWN_41             /* frames left of a step; way 2's shake: its entry */

enum {
    PULSE_OFF = ACTOR_RESUME_OWN,   /* way 0: hidden until the cue */
    PULSE_STEADY,               /* shown 127 frames */
    PULSE_BLINK,                /* then blinking every 32 */
    DROP_STEP,                  /* way 2: until the screen's step lets it go */
    DROP_GO,                    /* after $43 frames */
    DROP_FALL,                  /* its 32 frames of falling against its pull */
    DROP_SETTLE,                /* after 10 frames */
    DROP_SINK,                  /* 5 frames of the screen's y down 16 */
    DROP_SHAKE,                 /* the shake, entries 8 down to 0 */
    RISE_STEP,                  /* way 4: until the screen's step lets it go */
    RISE_POSE,                  /* after $98 frames */
    RISE_GO,                    /* after 20 frames */
    RISE_FLY,                   /* its 17 frames of flight */
    END_AFTER,                  /* ways 6 and 8: its pose's wait, then it ends */
    STAY                        /* a way past its table (reported) */
};

/* its pixel bytes, as the original writes them one at a time */
static void x_low_set(Actor *a, uint8_t v) { a->x = with_pixels(a->x, (uint16_t)((pixels(a->x) & 0xFF00) | v)); }
static void y_low_set(Actor *a, uint8_t v) { a->y = with_pixels(a->y, (uint16_t)((pixels(a->y) & 0xFF00) | v)); }
static void y_high_set(Actor *a, uint8_t v) { a->y = with_pixels(a->y, (uint16_t)((pixels(a->y) & 0x00FF) | v << 8)); }

/* The screen's step lets ways 2 and 4 go on: negative, or 2. */
static bool step_lets(void)
{
    return (g.genie_screen_step & 0x80) || g.genie_screen_step == 2;
}

/* Way 0: hidden (+$08 bit 3 cleared) until the cue. */
static int pulse_off(Actor *a)
{
    a->anim_flags &= 0xF7;
    a->resume = PULSE_OFF;
    return 1;
}

/*
 * @routine 84:AA24 KindA4Start
 * WHAT IT DOES: Actor kind $A4, a figure of the Genie's screen, by its $07
 * (0, 2, 4, 6; $10 or more for the last, its low 4 bits kept): 0 shows
 * while the cue is set, steady for 127 frames, then blinking every 32, for
 * good; 2 waits for the screen's step, then after $43 frames (sound $41,
 * pose 1) falls 32 frames against its pull (gravity $FF, speed entry $8A),
 * and after 10 sinks the screen's y 16 pixels a frame for 5 and shakes it
 * by its table (sound $3A at entry 5 and at the end), then ends; 4 is put
 * at y $018A (x low byte $80) with motion preset $74, and once the step
 * comes waits $98 frames, poses (sound $43), after 20 flies for 17 frames
 * (pose 3, 4 pixels up, sound $42) and leaves a child of its kind ($07
 * $10), then ends; 6 (its look's low byte 0, pose 5, 14 frames) and $10
 * up (sound $2D, pose 4, x low byte $34, y low byte $16, 48 frames) pose
 * once and end.
 */
int KindA4Start(Actor *a)
{
    uint8_t way;

    switch (a->resume) {
    case ACTOR_START:
        a->flags |= 0x90;
        a->look = (uint16_t)((a->look & 0x00FF) | 0x0600);
        way = a->spawn_byte;
        if (way >= 0x10) {
            a->spawn_byte &= 0x0F;
            way = 0x08;
        }
        switch (way) {
        case 0x00:
            AnimStart(a, 0);
            return pulse_off(a);
        case 0x02:
            a->resume = DROP_STEP;
            return 1;
        case 0x04:
            y_high_set(a, 0x01);
            x_low_set(a, 0x80);
            y_low_set(a, 0x8A);
            a->anim_flags = 0x01;
            ActorMotionPreset(a, 0x74);
            a->spawn_at = 0;
            a->resume = RISE_STEP;
            return 1;
        case 0x06:
            a->look &= 0xFF00;
            AnimStart(a, 5);
            a->resume = END_AFTER;
            return 0x0E;
        case 0x08:
            SoundQueueAdd(0x2D);
            AnimStart(a, 4);
            x_low_set(a, 0x34);
            y_low_set(a, 0x16);
            a->resume = END_AFTER;
            return 0x30;
        default: {
            static int told;

            /* past its table of five ways: the original jumps where the
               next words point */
            if (!told++)
                fprintf(stderr, "KindA4Start: way %u (past its table) not converted\n", way);
            a->resume = STAY;
            return 1;
        }
        }

    case PULSE_OFF:
        if (!g.screen_cue)
            return 1;
        a->own.bytes[LEFT] = 0x7F;
        a->resume = PULSE_STEADY;
        return 1;
    case PULSE_STEADY:
        if (!g.screen_cue)
            return pulse_off(a);
        if (--a->own.bytes[LEFT])
            return 1;
        a->own.bytes[LEFT] = 0x20;
        a->resume = PULSE_BLINK;
        return 1;
    case PULSE_BLINK:
        if (!g.screen_cue)
            return pulse_off(a);
        if (--a->own.bytes[LEFT])
            return 1;
        a->anim_flags ^= 0x08;
        a->own.bytes[LEFT] = 0x20;
        return 1;

    case DROP_STEP:
        if (!step_lets())
            return 1;
        a->resume = DROP_GO;
        return 0x43;
    case DROP_GO:
        SoundQueueAdd(0x41);
        AnimStart(a, 1);
        ActorSpeedYSetGravity(a, 0xFF, 0x8A);
        a->own.bytes[LEFT] = 0x20;
        a->resume = DROP_FALL;
        return 1;
    case DROP_FALL:
        ActorFallAgainst(a);
        if (--a->own.bytes[LEFT])
            return 1;
        a->resume = DROP_SETTLE;
        return 0x0A;
    case DROP_SETTLE:
        a->own.bytes[LEFT] = 0x05;
        a->resume = DROP_SINK;
        return 1;
    case DROP_SINK:
        g.genie_screen_y = (uint16_t)(g.genie_screen_y + 0x10);
        if (--a->own.bytes[LEFT])
            return 1;
        a->own.bytes[LEFT] = 0x08;
        a->resume = DROP_SHAKE;
        return 1;
    case DROP_SHAKE:
        g.genie_screen_y = (uint16_t)(g.genie_screen_y + ka4_shake(a->own.bytes[LEFT]));
        if (a->own.bytes[LEFT] == 0x05)
            SoundQueueAdd(0x3A);
        if (!(--a->own.bytes[LEFT] & 0x80))
            return 1;
        SoundQueueAdd(0x3A);
        return ActorEndPlaced(a);

    case RISE_STEP:
        if (!step_lets())
            return 1;
        a->resume = RISE_POSE;
        return 0x98;
    case RISE_POSE:
        y_high_set(a, 0x00);
        AnimStart(a, 2);
        SoundQueueAdd(0x43);
        a->resume = RISE_GO;
        return 0x14;
    case RISE_GO:
        AnimStart(a, 3);
        y_low_set(a, (uint8_t)(pixels(a->y) - 4));
        SoundQueueAdd(0x42);
        a->own.bytes[COUNT] = 0x11;
        ActorFly(a);
        a->resume = RISE_FLY;
        return 1;
    case RISE_FLY:
        if (--a->own.bytes[COUNT]) {
            ActorFly(a);
            return 1;
        }
        a->spawn_byte = 0x10;
        (void)ActorSpawnSameLook(a, a->kind);
        return ActorEndPlaced(a);

    case END_AFTER:
        return ActorEndPlaced(a);
    default:                            /* STAY */
        return 1;
    }
}

/*
 * @routine 83:9430 ActorKindA4Wait
 * WHAT IT DOES: Kind $A4's wait routine, through 84:AB97 KindA4WaitWork:
 * its animation steps.
 */
int ActorKindA4Wait(Actor *a)
{
    AnimStep(a);
    return ACTOR_GOES_ON;
}
