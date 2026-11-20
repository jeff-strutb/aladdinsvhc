/*
 * kind69.c: actor kind $69 (also kind $68's second state): a figure that,
 * on a cue, opens a circle wipe round itself and hands its place to the
 * level's escort, then follows the escort's facing and motion with its
 * poses. In its second state (state $8C: Aladdin has landed on it) it sets
 * the escort going on, rides it through two poses, and ends. Its wait
 * routine is Kind69Wait. (Kind $54's wait routine, 83:D7E7, also filed
 * here, is kind $54's.)
 */
#include "actor/actor_own.h"
#include "actor/guard.h"
#include "actor/kinds_d.h"
#include "audio/sound.h"
#include "data/kinds_d_tables.h"
#include "player/player.h"
#include "video/video.h"
#include "video/video_tasks.h"

/* its own bytes */
#define STEPS OWN_40            /* steps of the wipe left; frames of a ride */
#define HANDED OWN_42           /* set once the escort has its place (the wait routine: it rides the escort) */
#define RADIUS OWN_45           /* the wipe's radius */

#define WINDOW_SLOT 6           /* the window effect's task slot */

enum {
    WAIT_CUE = ACTOR_RESUME_OWN,    /* until the scene's cue */
    WIPE_START,                 /* after 2 frames: the wipe's steps */
    WIPE,                       /* each 3 frames a step */
    FOLLOW,                     /* each frame after the escort */
    RIDE_FIRST,                 /* the second state: 32 frames of pose 2 on the escort */
    RIDE_SECOND                 /* and of pose 3 */
};

/* The escort (actors[1]). */
static Actor *escort(void) { return &g.actors[1]; }

/*
 * @routine 83:EB4B ActorOnEscort
 * WHAT IT DOES: Puts it on the escort actor: its x the escort's, its y 4
 * above the escort's.
 */
static void on_escort(Actor *a)
{
    a->x = with_pixels(a->x, pixels(escort()->x));
    a->y = with_pixels(a->y, (uint16_t)(pixels(escort()->y) - 4));
}

/*
 * @routine 83:EAAF ActorCircleWipeStep
 * WHAT IT DOES: One step of a circle wipe round it: the radius up one and
 * the circle drawn round its place on the screen (8 pixels above its y).
 */
static void wipe_step(Actor *a)
{
    a->own.bytes[RADIUS]++;
    CircleWipeStep((int16_t)(pixels(a->x) - pixels(g.layers[0].scroll_x)),
                   (int16_t)(pixels(a->y) - pixels(g.layers[0].scroll_y) - 8), a->own.bytes[RADIUS]);
}

/* A ride: animation `anim`, 32 frames on the escort. */
static int ride(Actor *a, uint8_t anim, uint16_t next)
{
    AnimStart(a, anim);
    a->own.bytes[STEPS] = 0x20;
    a->resume = next;
    return 1;
}

/*
 * @routine 83:E9F7 Kind69Start
 * WHAT IT DOES: Actor kind $69 (and $68 in its second state): faces left
 * and waits for the scene's cue ($BE); then a circle wipe opens round it
 * over 32 steps of 3 frames (window effect 2 started in task slot 6, the
 * sub screen on, colour math adding to the backdrop), and the level's
 * escort takes its place (its $07 3, its code at GuardRestart, at its x and
 * y); from then on each frame it faces as the escort does and shows pose 1
 * while the escort moves, pose 0 while it stands.
 * Also 83:EADC Kind69EscortEnd (its second entry): the escort set going on
 * (its +$08 bit 3 set, its $07 6, restarted), it rides the escort through
 * poses 2 and 3 for 32 frames each, counts itself in g.scene_cue_first
 * ($00BD) and ends.
 */
int Kind69Start(Actor *a)
{
    uint8_t pose;

    switch (a->resume) {
    case ACTOR_START:
        a->flags |= 0x30;
        a->probes = k69_probes();
        a->own.bytes[RADIUS] = 0;
        a->facing = a->flip = 1;
        a->own.bytes[HANDED] = 0;
        a->resume = WAIT_CUE;
        return 1;
    case WAIT_CUE:
        if (!g.scene_cue)
            return 1;
        a->own.bytes[STEPS] = 0x20;
        g.sub_layers = 0x03;
        g.blend_use_sub = 1;
        g.blend_prevent = 0;
        g.blend_clip = 0;
        g.blend_direct_colour = 0;
        g.blend_layers = 0x20;
        g.blend_half = 0;
        g.blend_subtract = 0;
        g.window_main = 0x03;
        g.window_use[0] = 2;
        g.window_use[1] = 2;
        g.window_use[2] = g.window_use[3] = g.window_use[4] = g.window_use[5] = 0;
        wipe_step(a);
        TaskCreate(WINDOW_SLOT, 4, TASK_ENTRY_WINDOW_EFFECT);
        a->resume = WIPE_START;
        return 2;
    case WIPE_START:
        a->flags |= 0x80;
        AnimStart(a, 0);
        a->resume = WIPE;
        return 3;
    case WIPE:
        wipe_step(a);
        if (--a->own.bytes[STEPS])
            return 3;
        a->own.bytes[HANDED]++;
        a->flags = (uint8_t)((a->flags & 0xCF) | 0x10);
        g.line_effects_on = 0;
        g.sub_layers = 0;
        g.window_main = 0;
        g.window_use[0] = g.window_use[1] = 0;
        escort()->spawn_byte = 0x03;
        escort()->resume = G_RESTART;
        escort()->x = with_pixels(escort()->x, pixels(a->x));
        escort()->y = with_pixels(escort()->y, pixels(a->y));
        a->resume = FOLLOW;
        return 1;
    case FOLLOW:
        a->facing = a->flip = escort()->flip;
        pose = (escort()->speed_x & 0xFFFF) ? 1 : 0;
        if ((uint8_t)(pose << 1) != a->anim)
            AnimStart(a, pose);
        return 1;

    case ACTOR_START_SECOND:
        escort()->anim_flags |= 0x08;
        escort()->spawn_byte = 0x06;
        escort()->resume = G_RESTART;
        return ride(a, 0x02, RIDE_FIRST);
    case RIDE_FIRST:
        AnimStep(a);
        on_escort(a);
        if (--a->own.bytes[STEPS])
            return 1;
        return ride(a, 0x03, RIDE_SECOND);
    case RIDE_SECOND:
        AnimStep(a);
        on_escort(a);
        if (--a->own.bytes[STEPS])
            return 1;
        g.scene_cue_first++;
        return ActorEndPlaced(a);
    default:
        return 1;
    }
}

/*
 * @routine 83:EB24 Kind69Wait
 * WHAT IT DOES: What kind $69 runs each frame it waits: its animation;
 * while it rides the escort it stays on it, stops the player's shots (the
 * escort's +$08 bit 3 cleared), and when Aladdin lands on it, it goes into
 * its second state (its flags bit 7 cleared, state $8C) with sound $12.
 */
int Kind69Wait(Actor *a)
{
    AnimStep(a);
    if (!a->own.bytes[HANDED])
        return ACTOR_GOES_ON;
    on_escort(a);
    escort()->anim_flags &= 0xF7;
    (void)ActorStopsShot(a);
    if (PlayerLandsOnActor(a)) {
        a->flags &= 0x7F;
        a->state = 0x8C;
        SoundQueueAdd(0x12);
    }
    return ACTOR_GOES_ON;
}
