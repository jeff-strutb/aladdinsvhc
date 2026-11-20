/*
 * kind46.c: actor kinds $46 and $62, a thing that catches Aladdin, holds
 * and shakes him, lets him fall, breaks into pieces and falls itself, then
 * bounces him (kind $46); kind $62 stays hittable, makes no landing sound
 * and falls on after landing.
 */
#include "actor/actor_a.h"
#include "audio/sound.h"
#include "data/actor_a_tables.h"
#include "map/map.h"
#include "player/player.h"

#define FRAMES OWN_40           /* +$40: frames, or shake steps, left */
#define FLY_LEFT OWN_41         /* +$41: frames left of the fly-off */
#define KIND62 OWN_42           /* +$42: 0 kind $46, 1 kind $62 */
#define DROPPED OWN_43          /* +$43: nonzero, straight to the shake, the player not moved */
#define HELD OWN_44             /* +$44: nonzero, it holds the player */

#define ZONE 0x6C               /* ActorZoneHasTestPoint's zone */
#define LAND_Y 0xA8             /* kind $62 falls on from this y pixels low byte */
#define PIECE_FIRST 0x81        /* the two pieces' +$07: $81, $80 */
#define PIECE_LAST 0x84         /* the three pieces' (landed): $84, $83, $82 */

enum {
    CATCH = ACTOR_RESUME_OWN,   /* each frame until he is caught */
    HOLD_PLACE,                 /* after the frame it holds him: he goes to it */
    HOLD_FRAMES,                /* each frame of the 48 */
    SHAKE_STEP,                 /* after each shake step's 2 frames */
    FALL,                       /* each frame of its fall */
    FALL_ON,                    /* kind $62: falls on for good */
    BOUNCE,                     /* kind $46: bounces him */
    FLY_OFF                     /* a piece flying off */
};

/* ActorSpawnSameLook with its own kind, each child counted live */
static bool spawn_counted(Actor *a)
{
    if (!ActorSpawnSameLook(a, a->kind))
        return false;
    g.kind_count[a->kind]++;
    return true;
}

/*
 * @routine 83:C903 ActorFallTileTest
 * WHAT IT DOES: For a falling actor: whether the map tile 10 pixels below
 * its position has behaviour 2; an actor above the level's top (y
 * negative) is ended instead (`ended` set).
 */
static bool fall_tile_test(Actor *a, bool *ended)
{
    MapPoint p = { 0 };

    *ended = false;
    if (pixels(a->y) & 0x8000) {
        *ended = true;
        return false;
    }
    p.x = pixels(a->x);
    p.y = (uint16_t)(pixels(a->y) + 10);
    (void)TileAt(a, &p);
    return p.behaviour == 0x02;
}

/* the start of the shake step `frames` left: its x moved by the step's word; held, the player's x too */
static void shake_body(Actor *a)
{
    uint16_t x = (uint16_t)(pixels(a->x) + (uint16_t)grabber_shake(a->own.bytes[FRAMES]));

    a->x = with_pixels(a->x, x);
    if (a->own.bytes[DROPPED])
        return;
    if (!(uint8_t)g.player_no_actor_landing)
        a->own.bytes[HELD] = 0;
    else if (a->own.bytes[HELD])
        g.actors[0].x = with_pixels(g.actors[0].x, x);
}

/* the shake's start, and its end: he is let fall, it breaks in two and falls */
static int shake_start(Actor *a)
{
    a->own.bytes[FRAMES] = 8;
    shake_body(a);
    a->resume = SHAKE_STEP;
    return 2;
}

static int fall_start(Actor *a)
{
    if ((uint8_t)g.player_no_actor_landing && a->own.bytes[HELD])
        g.actors[0].resume = PLAYER_AT_FALL;
    a->flip = a->facing;
    if (!a->own.bytes[KIND62])
        a->flags &= 0x7F;
    AnimStart(a, 2);
    ActorMotionReset(a, 0x38);
    a->spawn_byte = PIECE_FIRST;
    a->spawn_at = 0x40;
    while (spawn_counted(a)) {
        a->spawn_byte--;
        if (!(a->spawn_byte & 0x80))
            break;
    }
    a->resume = FALL;
    return 1;
}

/* landed: three more pieces, and it settles on the tile */
static int landed(Actor *a)
{
    uint16_t y;

    a->spawn_byte = PIECE_LAST;
    a->spawn_at = 0x42;
    while (spawn_counted(a)) {
        if (a->spawn_byte >= PIECE_LAST)
            a->flip ^= 0x01;
        a->spawn_byte--;
        if (a->spawn_byte == PIECE_FIRST)
            break;
    }
    y = (uint16_t)(((pixels(a->y) + 0x0A) & 0xFFF0) - 9);
    a->y = with_pixels(a->y, y);
    if (a->own.bytes[KIND62]) {
        a->resume = FALL_ON;
        return 1;
    }
    AnimStart(a, 1);
    a->anim_flags = (uint8_t)((a->anim_flags & 0xFC) | 0x02);
    a->resume = BOUNCE;
    return 1;
}

/*
 * @routine 83:C763 GrabberStart
 * WHAT IT DOES: The code of actor kinds $46 and $62 (+$42: 0 for $46, 1
 * for $62), a thing that catches Aladdin: it waits until he comes into its
 * zone, holds him (his code at PlayerHeld, his place kept at it) for 48
 * frames, shakes for 8 steps of 2 frames, lets him fall (his code at
 * PlayerFall) if he is still held, breaks into two pieces and falls; when
 * it lands it breaks into three more, settles on the tile below, and from
 * then on bounces him when he comes down on it, with a sound. Kind $62's
 * stays hittable, makes no landing sound, and after landing keeps falling
 * and animating. A piece (+$07 bit 7 set) flies off at once
 * (83:C925 ActorFlyOffEnd: it can no longer be hit, motion preset $2B,
 * animation 3, $18 frames, then it ends).
 * Also 83:C761 Kind46Start and 83:C95F Kind62Start, its starts with +$42
 * 0 and 1 (the second entry of kind $46 is Kind62Start).
 */
int Kind46Start(Actor *a)
{
    bool ended;

    switch (a->resume) {
    case ACTOR_START:
    case ACTOR_START_SECOND:
        a->own.bytes[KIND62] = a->kind == 0x62 || a->resume == ACTOR_START_SECOND;
        a->own.bytes[DROPPED] = 0;
        a->own.bytes[HELD] = 0;
        a->anim_flags |= 0x02;
        a->flags |= 0x90;
        if (a->spawn_byte & 0x80) {
            a->flags &= 0x7F;
            ActorMotionPreset(a, 0x2B);
            AnimStart(a, 3);
            a->own.bytes[FLY_LEFT] = 0x18;
            a->resume = FLY_OFF;
            return 1;
        }
        a->flip = a->spawn_byte & 0x01;
        AnimStart(a, 0);
        a->resume = CATCH;
        return 1;
    case FLY_OFF:
        ActorFly(a);
        if (--a->own.bytes[FLY_LEFT])
            return 1;
        return ActorEndPlaced(a);
    case CATCH:
        if (a->own.bytes[DROPPED])
            return shake_start(a);
        if (g.player_hurt_count || (g.player_no_landing & 0x80) || !ActorZoneHasTestPoint(a, ZONE))
            return 1;
        a->facing = !g.actors[0].flip;
        g.actors[0].resume = PLAYER_AT_HELD;
        a->own.bytes[HELD]++;
        g.player_saved_x = pixels(a->x);                /* Point145DToActor: his saved place at it, 12 below its y */
        g.player_saved_y = (uint16_t)(pixels(a->y) + 12);
        a->resume = HOLD_PLACE;
        return 1;
    case HOLD_PLACE:
        g.actors[0].x = with_pixels(g.actors[0].x, pixels(a->x));       /* PlayerToActor: at it, 12 below its y */
        g.actors[0].y = with_pixels(g.actors[0].y, (uint16_t)(pixels(a->y) + 12));
        a->own.bytes[FRAMES] = 0x30;
        if (!(uint8_t)g.player_no_actor_landing)
            a->own.bytes[HELD] = 0;
        a->resume = HOLD_FRAMES;
        return 1;
    case HOLD_FRAMES:
        if (--a->own.bytes[FRAMES]) {
            if (!(uint8_t)g.player_no_actor_landing)
                a->own.bytes[HELD] = 0;
            return 1;
        }
        return shake_start(a);
    case SHAKE_STEP:
        if (--a->own.bytes[FRAMES]) {
            shake_body(a);
            return 2;
        }
        return fall_start(a);
    case FALL:
        AnimStep(a);
        ActorFall(a);
        if (a->own.bytes[KIND62] && (uint8_t)pixels(a->y) >= LAND_Y)
            return landed(a);
        if (!fall_tile_test(a, &ended)) {
            if (ended)
                return ActorEndPlaced(a);
            return 1;
        }
        if ((a->flags & 0x40) && !a->own.bytes[KIND62])
            SoundQueueAdd(0x18);
        return landed(a);
    case FALL_ON:
        AnimStep(a);
        ActorFall(a);
        return 1;
    default:                            /* BOUNCE */
        if (ActorBouncesPlayer(a))
            SoundQueueAdd(0x12);
        return 1;
    }
}

/*
 * @routine 83:C94A Kind46Wait
 * WHAT IT DOES: Kind $46's wait routine: a shot it stops sets its +$43;
 * with +$07 bit 1 it is ended when out of range (box 0).
 */
int Kind46Wait(Actor *a)
{
    if (ActorStopsShot(a))
        a->own.bytes[DROPPED] = 0x01;
    if (!(a->spawn_byte & 0x02))
        return ACTOR_GOES_ON;
    return ActorEndOutOfRange(a, 0);
}

/*
 * @routine 83:C966 Kind62Wait
 * WHAT IT DOES: What an actor of kind $62 runs each frame it waits: the
 * test whether it stops Aladdin's shot, then its end when it is out of range
 * by box 0.
 */
int Kind62Wait(Actor *a)
{
    (void)ActorStopsShot(a);
    return ActorEndOutOfRange(a, 0);
}
