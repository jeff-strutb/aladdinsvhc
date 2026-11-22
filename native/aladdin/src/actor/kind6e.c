/*
 * kind6e.c: actor kind $6E, drawn as the picture of the backdrop layer,
 * not as sprites: by its +$07, 0 a spawner of the others, 1 and 2 one that
 * rises and glides right and then speeds off, 3 one that slows and speeds
 * by a list of pairs. Its picture's frames are uploads cycled from a table,
 * and the backdrop is shown, scrolled to put the picture where the actor
 * is, only while it is near.
 */
#include <stdio.h>
#include "video/view.h"

#include "actor/actor_a.h"
#include "audio/sound.h"
#include "data/actor_a_tables.h"
#include "player/player.h"

int Kind70Start(Actor *a);      /* kind $70's start (kind70_start.c): this kind's second entry */

#define STEP OWN_40             /* +$40: the spawner's offset in its list; else frames left of a stretch */
#define PICTURE_AT OWN_41       /* +$41: the offset of its picture's frame */
#define PICTURE_TIME OWN_42     /* +$42: frames left of the frame */
#define RAMP OWN_43             /* +$43 (word): its gravity ramp, the high byte the gravity */
#define PAIRS_AT OWN_47         /* +$47: its offset in the pairs */
#define SOUND_TIME OWN_48       /* +$48: frames to Kind6EWait's next sound */
#define QUIET OWN_49            /* +$49: nonzero, Kind6EWait sounds no more */
#define KEPT_X OWN_4C           /* +$4C (word): its x kept while the touch is tested */

#define LAYERS_WITH_BACKDROP 0x17   /* BG1, BG2, BG3 and sprites */
#define LAYERS_NO_BACKDROP 0x15     /* BG1, BG3 and sprites */

enum {
    SPAWN = ACTOR_RESUME_OWN,   /* the spawner: each frame */
    RISE,                       /* a flyer: rising */
    GLIDE,                      /* gliding right, $30 frames */
    SPEED,                      /* speeding off until the layers are $15 */
    DECELERATE,                 /* the pairs: slowing to a stop */
    ACCELERATE                  /* the pairs: speeding up */
};

/*
 * @routine 83:F211 ActorStepRight3
 * WHAT IT DOES: Moves the actor 3 pixels to the right.
 */
static void ActorStepRight3(Actor *a)
{
    a->x = with_pixels(a->x, (uint16_t)(pixels(a->x) + 3));
}

static uint16_t own_word(const Actor *a, int at)
{
    return (uint16_t)(a->own.bytes[at] | a->own.bytes[at + 1] << 8);
}

static void own_word_set(Actor *a, int at, uint16_t v)
{
    a->own.bytes[at] = (uint8_t)v;
    a->own.bytes[at + 1] = (uint8_t)(v >> 8);
}

/* the sound, the picture and the flags a mover starts with */
static void mover_start(Actor *a)
{
    a->own.bytes[PICTURE_AT] = 0;
    a->own.bytes[PICTURE_TIME] = 0;
    SoundQueueAdd(0x2A);
    a->own.bytes[SOUND_TIME] = 0x9D;
    a->own.bytes[QUIET] = 0;
    a->flags = 0x80;
}

/* the speeding off's frame: speed, ramp, gravity, flight, the step right */
static int speed_off_frame(Actor *a)
{
    uint16_t ramp;

    ActorAccelerateX(a);
    if ((uint16_t)a->speed_x > 0x0200)
        a->speed_x = (a->speed_x & ~0xFFFF) | 0x0200;
    ramp = (uint16_t)(own_word(a, RAMP) + 0x0080);
    if (ramp > 0x3000)
        ramp = 0x3000;
    own_word_set(a, RAMP, ramp);
    a->accel_y = (uint8_t)(ramp >> 8);
    ActorFly(a);
    ActorStepRight3(a);
    if (g.main_layers == LAYERS_NO_BACKDROP)
        return ActorEndPlaced(a);
    return 1;
}

/*
 * @routine 83:F049 Kind6EStart
 * WHAT IT DOES: Actor kind $6E, the backdrop picture: by its +$07, 0 a
 * spawner that makes a kind $6E child each time the view's scroll passes the
 * next point of its list (at the range point, with the list's +$07 and
 * slot); 1 and 2 one that rises on a motion preset by +$07 and glides right
 * 3 pixels a frame, then speeds off right (its acceleration and gravity
 * ramping up) until the main screen's layers are $15 and ends; 3 one that
 * slows and speeds by a list of pairs, turning each time, then a sound and
 * ends. (The original's copier tests, reading and writing cartridge RAM
 * that this cartridge lacks, find none here.) Its second entry (83:F296)
 * goes on as kind $70's start.
 */
int Kind6EStart(Actor *a)
{
    switch (a->resume) {
    case ACTOR_START:
        switch (a->spawn_byte) {
        case 0:
            a->flip = 0;
            a->own.bytes[STEP] = 0;
            g.layers[1].scroll_x = with_pixels(g.layers[1].scroll_x,
                                               (uint16_t)((pixels(g.layers[1].scroll_x) & 0x00FF) | 0x0100));
            g.vram_upload_number = 1;
            a->resume = SPAWN;
            return 1;
        case 1:
        case 2:
            mover_start(a);
            ActorMotionPreset(a, k6e_motion(a->spawn_byte));
            a->resume = RISE;
            return 1;
        case 3:
            a->own.bytes[PICTURE_AT] = 0;
            a->own.bytes[PICTURE_TIME] = 0;
            a->own.bytes[PAIRS_AT] = 0;
            SoundQueueAdd(0x2A);
            a->own.bytes[SOUND_TIME] = 0x9D;
            a->own.bytes[QUIET] = 0;
            a->flags = 0x80;
            a->facing = 0;
            ActorSpeedXSet(a, 0x45);
            a->accel_x = k6e_pair(a->own.bytes[PAIRS_AT]++);
            a->resume = DECELERATE;
            return 1;
        default: {
            static int told;

            /* past the table's four ways: the original jumps where the next words point */
            if (!told++)
                fprintf(stderr, "Kind6EStart: way %u (past its table) not converted\n", a->spawn_byte);
            a->resume = ACTOR_RESUME_OWN + 100;
            return 1;
        }
        }
    case ACTOR_START_SECOND:
        return ActorRunAs(a, Kind70Start, ACTOR_START);
    case SPAWN: {
        K6ESpawn s = k6e_spawn(a->own.bytes[STEP]);

        if (pixels(g.layers[0].scroll_x) < s.scroll_x)
            return 1;
        a->x = with_pixels(a->x, g.range_x);
        a->y = with_pixels(a->y, g.range_y);
        a->spawn_byte = s.byte07;
        a->spawn_at = s.slot;
        (void)ActorSpawnShared(a, 0x6E);
        a->own.bytes[STEP] += 4;
        return 1;
    }
    case RISE:
        ActorFly(a);
        ActorStepRight3(a);
        if (a->speed_y < 0)
            return 1;
        a->own.bytes[STEP] = 0x30;
        a->resume = GLIDE;
        return 1;
    case GLIDE:
        ActorStepRight3(a);
        if (--a->own.bytes[STEP])
            return 1;
        a->own.bytes[QUIET]++;
        SoundQueueAdd(0x2B);
        a->speed_x = 0;
        own_word_set(a, RAMP, 0);
        a->accel_x = 0x10;
        a->resume = SPEED;
        return 1;
    case SPEED:
        return speed_off_frame(a);
    case DECELERATE:
        ActorDecelerateX(a);
        ActorMoveX(a);
        ActorStepRight3(a);
        if (wrap24((uint32_t)a->speed_x))
            return 1;
        a->facing ^= 0x01;
        a->own.bytes[STEP] = k6e_pair(a->own.bytes[PAIRS_AT]++);
        a->resume = ACCELERATE;
        return 1;
    case ACCELERATE:
        ActorAccelerateX(a);
        ActorMoveX(a);
        ActorStepRight3(a);
        if (--a->own.bytes[STEP])
            return 1;
        if (!k6e_pair((uint8_t)(a->own.bytes[PAIRS_AT] + 1))) {
            SoundQueueAdd(0x23);
            return ActorEndPlaced(a);
        }
        a->accel_x = k6e_pair(a->own.bytes[PAIRS_AT]++);
        a->resume = DECELERATE;
        return 1;
    default:
        return 1;
    }
}

/*
 * @routine 83:F225 BackdropPictureStep
 * WHAT IT DOES: Plays the actor's picture on by a frame of time: when the
 * picture's frame has run its time, the next one (after the last, the
 * first) is asked for as a fixed VRAM upload.
 */
static void BackdropPictureStep(Actor *a)
{
    uint8_t at;

    if (--a->own.bytes[PICTURE_TIME] < 0x80)
        return;
    at = (uint8_t)(a->own.bytes[PICTURE_AT] + 2);
    if (k6e_frame((uint8_t)(at - 1)) & 0x80)
        at = 0;
    a->own.bytes[PICTURE_TIME] = k6e_frame((uint8_t)(at + 1)) & 0x7F;
    g.vram_upload_number = k6e_frame(at);
    a->own.bytes[PICTURE_AT] = at;
}

/* whether `mine` is within 256 + `extra` (the view's dx or dy) of `from` (either side) */
static bool near_point(uint16_t mine, uint16_t from, int32_t extra)
{
    return (uint16_t)(mine - from + 0x0100 + extra) < (uint16_t)(0x0200 + 2 * extra);
}

/*
 * @routine 83:F244 BackdropActorShow
 * WHAT IT DOES: Shows the backdrop, with the actor's picture on it, while
 * the actor is within 256 pixels of the range point on both axes: the
 * backdrop is scrolled to put its point (128, 128) where the actor is on
 * the screen (the play field's scroll less the actor's pixels plus 128,
 * turned round). Farther away the backdrop is not shown.
 */
static void BackdropActorShow(const Actor *a)
{
    uint16_t x = pixels(a->x), y = pixels(a->y);

    if (near_point(x, g.range_x, view.dx) && near_point(y, g.range_y, view.dy)) {
        g.layers[1].scroll_x = with_pixels(g.layers[1].scroll_x,
                                           (uint16_t)(0 - (uint16_t)(x - pixels(g.layers[0].scroll_x) - 0x0080)));
        g.layers[1].scroll_y = with_pixels(g.layers[1].scroll_y,
                                           (uint16_t)(0 - (uint16_t)(y - pixels(g.layers[0].scroll_y) - 0x0080)));
        g.main_layers = LAYERS_WITH_BACKDROP;
        return;
    }
    g.main_layers = LAYERS_NO_BACKDROP;
}

/*
 * @routine 83:F1D5 Kind6EWait
 * WHAT IT DOES: Kind $6E's wait: a sound ($33) every 63 frames while its
 * +$49 is clear; its backdrop picture stepped and shown; touching the test
 * box (moved by its variant's offset for the test) while the player can be
 * hit, the player enters state $8C. (Its flags bit 6 is clear in play, so
 * the touch never meets.)
 */
int Kind6EWait(Actor *a)
{
    uint16_t x;

    if (!a->own.bytes[QUIET] && !--a->own.bytes[SOUND_TIME]) {
        a->own.bytes[SOUND_TIME] = 0x3F;
        SoundQueueAdd(0x33);
    }
    BackdropPictureStep(a);
    BackdropActorShow(a);
    x = pixels(a->x);
    own_word_set(a, KEPT_X, x);
    a->x = with_pixels(a->x, (uint16_t)(x + k6e_touch_dx(a->spawn_byte)));
    if (ActorHitsTestBoxIfPlayerFree(a))
        (void)PlayerEnter8CFacing(a);
    a->x = with_pixels(a->x, own_word(a, KEPT_X));
    return ACTOR_GOES_ON;
}
