/*
 * continue_choice.c: actor kind $26 on the continue screen: the two places
 * Aladdin can step down into (yes: continue; no) and the one that watches
 * him; and its wait routine, which holds him while he sinks with a place.
 */
#include "actor/actor_own.h"
#include "player/player.h"

#define ZONE 0x84                   /* ActorZoneHasTestPoint's zone */
#define SINK_FRAMES 0x10
#define BELOW 5                     /* PlayerSpotBelowActor's offset */
#define LEFT_OF 0x50                /* the watcher: Aladdin's x below this, he looks left; from RIGHT_OF on, right */
#define RIGHT_OF 0xB0
#define LOOK_LEFT 2
#define LOOK_MIDDLE 1
#define LOOK_RIGHT 4
#define LOOK_YES 3
#define LOOK_NO 5
#define HELD_FLOOR_Y 0xB0           /* the lowest the player is held at */

/* its own bytes */
typedef struct {
    uint8_t before[OWN_40];
    uint8_t sink;               /* +$40: frames left of its sinking */
    uint8_t unused41;
    uint8_t watcher;            /* +$42: set: it is the watcher */
    uint8_t unused43;
    uint8_t taken;              /* +$44: set once Aladdin has stepped into it */
    uint8_t anim;               /* +$45: the animation it shows */
    uint8_t pad;                /* +$46: the pad's directions, copied each frame */
} ContinueChoiceOwn;

_Static_assert(sizeof(ContinueChoiceOwn) <= OWN_BYTES, "a view fits the own bytes");

static ContinueChoiceOwn *own(Actor *a) { return (ContinueChoiceOwn *)a->own.bytes; }

enum {
    WAIT_FOR_HIM = ACTOR_RESUME_OWN,    /* a place: each frame until he is in its zone */
    SINK,                               /* a place: each frame of its sinking */
    STAY,                               /* a place: for good, its choice made */
    WATCH                               /* the watcher: each frame */
};

/* the watcher's frame: animation `anim` started when it is a new one, the pad kept */
static int watch(Actor *a, uint8_t anim)
{
    if (anim != own(a)->anim) {
        own(a)->anim = anim;
        AnimStart(a, anim);
    }
    own(a)->pad = (uint8_t)(g.pad1_held >> 8);
    a->resume = WATCH;
    return 1;
}

/*
 * @routine 83:9D62 ContinueChoiceActor
 * WHAT IT DOES: An actor of the continue screen; none when no continues
 * are left. $07 low four bits zero: a place Aladdin can step down into
 * ($07 zero: the yes, facing right; else the no, facing left): when he is
 * in its zone it takes him (his code goes on at PlayerSwitchHang, set below
 * it), sinks 16 pixels with him, and marks the choice. Otherwise the
 * watcher: its animation by the choice made, or by where Aladdin stands (1
 * between x 80 and 175, 2 left of it, 4 right; 3 once yes, 5 once no).
 */
static int ContinueChoiceActor(Actor *a)
{
    ContinueChoiceOwn *o = own(a);
    uint8_t variant;

    switch (a->resume) {
    case ACTOR_START:
        a->anim_flags |= 0x02;
        a->flags |= 0x90;
        a->facing = 0;
        a->flip = 0;
        o->watcher = 0;
        if (!g.continues)
            return ActorEndPlaced(a);
        variant = a->spawn_byte & 0x0F;
        if (variant) {
            o->anim = 0;
            o->watcher++;
            return watch(a, variant);
        }
        AnimStart(a, 0);
        o->taken = 0;
        if (a->spawn_byte) {
            a->facing = 1;
            a->flip = 1;
        }
        a->resume = WAIT_FOR_HIM;
        return 1;
    case WAIT_FOR_HIM:
        if (!ActorZoneHasTestPoint(a, ZONE))
            return 1;
        g.actors[0].resume = PLAYER_AT_SWITCH_HANG;
        o->taken++;
        PlayerSpotBelowActor(a, BELOW);
        o->sink = SINK_FRAMES;
        a->resume = SINK;
        return 1;
    case SINK:
        a->y = with_pixels(a->y, (uint16_t)((pixels(a->y) & 0xFF00) | (uint8_t)(pixels(a->y) + 1)));
        if (--o->sink)
            return 1;
        if (a->spawn_byte)
            g.continue_no++;
        else
            g.continue_yes++;
        a->resume = STAY;
        return 1;
    case STAY:
        return 1;
    default:                            /* WATCH */
        if (g.continue_yes)
            return watch(a, LOOK_YES);
        if (g.continue_no)
            return watch(a, LOOK_NO);
        {
            uint8_t x = (uint8_t)pixels(g.actors[0].x);

            return watch(a, x < LEFT_OF ? LOOK_LEFT : x >= RIGHT_OF ? LOOK_RIGHT : LOOK_MIDDLE);
        }
    }
}

int ContinueChoiceStart(Actor *a)
{
    return ContinueChoiceActor(a);
}

/*
 * @routine 83:9E0A ActorWaitHoldPlayer
 * WHAT IT DOES: Each frame it waits: its animation on; then, as the
 * watcher, Aladdin falling to or below y $B0 is held there (on an actor,
 * not in the air); else, once he has stepped into a place, he is put at
 * the offset below it (PlayerToActorBelow 5).
 */
int ActorWaitHoldPlayer(Actor *a)
{
    ContinueChoiceOwn *o = own(a);
    Actor *p = &g.actors[0];

    AnimStep(a);
    if (!o->watcher) {
        if (o->taken)
            PlayerToActorBelow(a, BELOW);
        return ACTOR_GOES_ON;
    }
    if ((uint8_t)pixels(p->y) < HELD_FLOOR_Y)
        return ACTOR_GOES_ON;
    if (p->speed_y < 0)
        return ACTOR_GOES_ON;
    p->y = with_pixels(p->y, HELD_FLOOR_Y);
    g.player_on_actor = 1;
    g.player_in_air = 0;
    return ACTOR_GOES_ON;
}
