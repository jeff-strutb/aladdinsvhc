/*
 * kind87.c: actor kind $87, a catcher: when Aladdin's test box comes into
 * its zone it takes hold of him, shakes him about, lets him go, drops, and
 * breaks into five of its children before it ends; a child ($07 nonzero)
 * drops to the ground, waits its turn, and floats away. Its wait routine
 * is ActorKind87Wait. Its second entry is kind $8A's start (83:F9BA
 * LandingPad): Kind87Second hands the record over.
 */
#include "actor/actor_own.h"
#include "actor/kinds_d.h"
#include "audio/sound.h"
#include "data/actor_a_tables.h"
#include "data/kinds_d_tables.h"
#include "map/map.h"
#include "player/player.h"

/* its own bytes */
#define STEP OWN_40             /* the shake's steps left */
#define FRAMES OWN_41           /* frames left of a hold, a blink or a rise; children left to make */
#define DONE OWN_42             /* nonzero: no catch (its wait sets it when it stops a shot) */
#define CAUGHT OWN_43           /* counted up as it catches him */
#define CHILD OWN_44            /* a child's: nonzero once it rises (then its wait hurts on touch) */
#define HELD OWN_45             /* nonzero while it holds him */
#define KIND2D_STOPPED OWN_4F   /* kind $2D's: nonzero once an actor has stopped it */

enum {
    WATCH = ACTOR_RESUME_OWN,   /* the catcher: each frame, for him in its zone */
    PUT_HIM,                    /* caught: the next frame he is put at it */
    HOLD,                       /* its 48 frames of holding him */
    SHAKE,                      /* after each step's 2 frames */
    FALL,                       /* each frame of its drop */
    BLINK,                      /* its 11 frames of blinking */
    CHILD_TURN,                 /* a child: after its turn's wait */
    CHILD_RISE_START,           /* after 24 frames */
    CHILD_RISE                  /* its 12 frames of rising */
};

/*
 * @routine 83:F8E4 Kind87PlayerToIt
 * WHAT IT DOES: Puts Aladdin at its place: its x, its y less 8.
 */
static void player_to_it(const Actor *a)
{
    Actor *p = &g.actors[0];

    p->x = with_pixels(p->x, pixels(a->x));
    p->y = with_pixels(p->y, (uint16_t)(pixels(a->y) - 8));
}

/*
 * @routine 83:F8F6 Kind87HoldAtIt
 * WHAT IT DOES: Where Aladdin is held (g.player_saved_x, _y): its x, its
 * y less 8.
 */
static void hold_at_it(const Actor *a)
{
    g.player_saved_x = pixels(a->x);
    g.player_saved_y = (uint16_t)(pixels(a->y) - 8);
}

/* A step of the shake: its x moved by the step's pick; while it holds
   him (and the catch was made), his x with it; COP 2. */
static int shake_step(Actor *a)
{
    uint16_t x = (uint16_t)(pixels(a->x) + k87_shake_x(a->own.bytes[STEP]));

    a->x = with_pixels(a->x, x);
    if (!a->own.bytes[DONE]) {
        if (!(uint8_t)g.player_no_actor_landing)
            a->own.bytes[HELD] = 0;
        else if (a->own.bytes[HELD])
            g.actors[0].x = with_pixels(g.actors[0].x, x);
    }
    a->resume = SHAKE;
    return 2;
}

/* The shake of 8 steps. */
static int shake(Actor *a)
{
    a->own.bytes[STEP] = 0x08;
    return shake_step(a);
}

/* A held frame: once he is let go, it no longer holds him. */
static int hold(Actor *a)
{
    if (!(uint8_t)g.player_no_actor_landing)
        a->own.bytes[HELD] = 0;
    a->resume = HOLD;
    return 1;
}

/*
 * @routine 83:F77F Kind87Start
 * WHAT IT DOES: Actor kind $87. $07 0, the catcher (probes at its own
 * list): shown and hittable, it waits until Aladdin's test box is in its
 * zone $7C (not while he may not land on actors or is hurt); then it holds
 * him (PlayerHeld, 48 frames) facing him, shakes him through 8 steps of
 * its table, lets him go (PlayerFall if it still held him), drops to the
 * ground (gravity $38) with sound $3D, makes five children of its kind
 * (ActorSpawnSameLook, $07 1-5, spawn points $74 on by 2, counted), blinks
 * 11 frames and ends. With DONE set it skips the catch. $07 nonzero, a
 * child (its probe list its own): it drops to the ground, waits by its
 * $07, rises 12 frames (2 pixels a frame) and ends.
 */
int Kind87Start(Actor *a)
{
    switch (a->resume) {
    case ACTOR_START:
        a->anim_flags |= 0x02;
        a->flags |= 0x90;
        a->own.bytes[CAUGHT] = 0;
        a->own.bytes[HELD] = 0;
        if (a->spawn_byte) {
            uint8_t turn;

            a->own.bytes[CHILD] = 0;
            a->probes = k87_child_probes();
            while (!ActorOutOfGround(a))
                a->y = with_pixels(a->y, (uint16_t)(pixels(a->y) + 8));
            (void)ActorSnapToGround(a, NULL);
            turn = k87_child_wait(a->spawn_byte);
            a->resume = CHILD_TURN;
            if (turn)
                return turn;
        } else {
            a->own.bytes[DONE] = 0;
            a->probes = k87_probes();
            AnimStart(a, 3);
            a->resume = WATCH;
            return 1;
        }
        /* fall through - a child with no turn to wait */
    case CHILD_TURN:
        a->own.bytes[CHILD]++;
        AnimStart(a, 2);
        a->resume = CHILD_RISE_START;
        return 0x18;
    case CHILD_RISE_START:
        a->speed_y = 0x200;
        a->own.bytes[FRAMES] = 0x0C;
        a->resume = CHILD_RISE;
        return 1;
    case CHILD_RISE:
        ActorStepYBack(a);
        return --a->own.bytes[FRAMES] ? 1 : ActorEndPlaced(a);

    case WATCH:
        if (a->own.bytes[DONE])
            return shake(a);
        if ((g.player_no_landing & 0x80) || g.player_hurt_count || !ActorZoneHasTestPoint(a, 0x7C))
            return 1;
        a->own.bytes[CAUGHT]++;
        a->facing = g.actors[0].flip ? 0 : 1;
        g.actors[0].resume = PLAYER_AT_HELD;
        a->own.bytes[HELD]++;
        hold_at_it(a);
        a->resume = PUT_HIM;
        return 1;
    case PUT_HIM:
        player_to_it(a);
        a->own.bytes[FRAMES] = 0x30;
        return hold(a);
    case HOLD:
        if (--a->own.bytes[FRAMES])
            return hold(a);
        return shake(a);
    case SHAKE:
        if (--a->own.bytes[STEP])
            return shake_step(a);
        if ((uint8_t)g.player_no_actor_landing && a->own.bytes[HELD])
            g.actors[0].resume = PLAYER_AT_FALL;
        a->flip = a->facing;
        AnimStart(a, 1);
        ActorMotionReset(a, 0x38);
        a->resume = FALL;
        return 1;
    case FALL: {
        int i;

        ActorFall(a);
        if (!ActorOutOfGround(a))
            return 1;
        SoundQueueAdd(0x3D);
        (void)ActorSnapToGround(a, NULL);
        a->spawn_at = 0x72;
        for (i = 0; i < 5; i++) {
            a->spawn_byte++;
            a->spawn_at = (uint8_t)(a->spawn_at + 2);
            if (ActorSpawnSameLook(a, a->kind))
                g.kind_count[a->kind]++;
        }
        a->flags &= 0x7F;
        a->own.bytes[FRAMES] = 0x0B;
        a->frame_time = 0x0B;
        a->resume = BLINK;
        return 1;
    }
    case BLINK:
        a->anim_flags ^= 0x08;
        return --a->own.bytes[FRAMES] ? 1 : ActorEndPlaced(a);
    default:
        return 1;
    }
}

/*
 * @routine 83:F908 ActorStopsTouchingKind2D
 * WHAT IT DOES: Stops every actor of kind $2D in the pool that touches it
 * and is not stopped yet: by the $2D's hit box, each half grown by 4,
 * around the $2D's place, its place within: the $2D marked (+$4F 1), its
 * wait routine off, +$08 bit 7 set, state $8C. Only KindWaitF991 calls it.
 * (The original's walk also takes the 80 bytes at $141B, past the pool's
 * last record, as a record, kind byte $1421; no routine of the record
 * names $1421. The native walk stops at the pool's last record.)
 */
static void stops_touching_kind2d(const Actor *a)
{
    int i;

    for (i = ACTOR_RECORDS - ACTOR_POOL_SIZE; i < ACTOR_RECORDS; i++) {
        Actor *t = &g.actors[i];
        HitBox box;
        uint16_t half;

        if (!t->state || t->kind != 0x2D || t->own.bytes[KIND2D_STOPPED])
            continue;
        box = hit_box(t->kind);
        half = (uint16_t)(box.half_w + 4);
        if ((uint16_t)(pixels(t->x) + half - pixels(a->x)) >= (uint16_t)(half << 1))
            continue;
        half = (uint16_t)(box.half_h + 4);
        if ((uint16_t)(pixels(t->y) + half - pixels(a->y)) >= (uint16_t)(half << 1))
            continue;
        t->own.bytes[KIND2D_STOPPED] = 1;
        t->flags &= 0x7F;
        t->anim_flags |= 0x80;
        t->state = 0x8C;
    }
}

/* Kind $87's second entry (state $80-$BF): the record goes on in kind $8A's
   code (83:F9BA LandingPad) from its start. */
int Kind87Second(Actor *a)
{
    return ActorRunAs(a, LandingPad, ACTOR_START);
}

/*
 * @routine 83:9310 ActorKind87Wait
 * WHAT IT DOES: Kind $87's wait routine, through 83:F991 KindWaitF991:
 * animates; the catcher (+$07 0), until it has caught him, sets DONE when
 * it stops a shot (ActorStopsShot); a child, once it rises, hurts Aladdin
 * on touch and stops the kind $2D actors that touch it; freed once out of
 * range.
 */
int ActorKind87Wait(Actor *a)
{
    AnimStep(a);
    if (!a->spawn_byte) {
        if (!a->own.bytes[CAUGHT] && ActorStopsShot(a))
            a->own.bytes[DONE] = 1;
    } else if (a->own.bytes[CHILD]) {
        (void)ActorHurtsPlayerOnTouch(a);
        stops_touching_kind2d(a);
    }
    return ActorFreeOutOfRange(a, 0x00) ? ACTOR_ENDED : ACTOR_GOES_ON;
}
