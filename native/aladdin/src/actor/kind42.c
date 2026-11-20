/*
 * kind42.c: actor kind $42, the pot a kind $20 guard with +$07 bit 4
 * carries over its head: it rides there until the guard's throw, then
 * flies at Aladdin, rolls or bounces along the ground, and breaks; it
 * drops when the guard is struck. No level places such a guard.
 */
#include "actor/actor_own.h"
#include "audio/sound.h"
#include "data/kinds_b_tables.h"
#include "map/map.h"

/* its own bytes */
typedef struct {
    uint8_t before[OWN_40];
    uint8_t carrier;            /* +$40: its guard, as its index in g.actors (written by the guard) */
    uint8_t left;               /* +$41: frames left of its breaking */
} K42Own;

static inline K42Own *k42(Actor *a) { return (K42Own *)a->own.bytes; }

enum {
    CARRIED = ACTOR_RESUME_OWN, /* each frame on its guard */
    FLIGHT,                     /* thrown, $07 0: flying until it lands */
    ROLL,                       /* rolling on the ground */
    BOUNCE,                     /* thrown, $07 else: flying until it lands */
    BOUNCE_SPRING,              /* landed: 2 frames, then up again */
    DROP,                       /* its guard struck: falling */
    BREAK                       /* its breaking animation */
};

/* the guard's picture ($27) at its throw, and when it is struck */
#define GUARD_THROWS 0x15
#define GUARD_STRUCK_A 0x0E
#define GUARD_STRUCK_B 0x11
#define ROLL_SPEED 0x0180u      /* the roll's speed across (16.8, its low two bytes) */
#define FALL_TOO_FAST 0x0800u   /* landing at this vertical speed (low two bytes) or more breaks it */
#define PASSED 0x10             /* pixels past Aladdin that count as past him */

static Actor *guard_of(Actor *a) { return &g.actors[k42(a)->carrier]; }

/*
 * @routine 83:C69A Kind42Ahead
 * WHAT IT DOES: The kind of the map's tile ahead of the pot (by the table
 * of x offsets by facing) at its own height; 0 empty.
 * (The original loads its y + $10 and then loads its y over it.)
 */
static uint8_t Kind42Ahead(Actor *a)
{
    MapPoint p;

    p.x = (uint16_t)(k42_ahead_x(a->facing) + pixels(a->x));
    p.y = pixels(a->y);
    return TileAt(a, &p);
}

/*
 * @routine 83:C6B6 Kind42PastPlayer
 * WHAT IT DOES: Once the flying pot is past Aladdin (more than 16 pixels
 * beyond him the way it faces), tells its guard (the guard's +$4F 2)
 * unless the guard has been told already.
 */
static void Kind42PastPlayer(Actor *a)
{
    uint16_t px = pixels(g.actors[0].x);
    uint16_t x = pixels(a->x);
    Actor *guard;

    if (a->facing == 0 ? (uint16_t)(px + PASSED) >= x : (uint16_t)(px - PASSED) < x)
        return;
    guard = guard_of(a);
    if (!guard->own.bytes[OWN_4F])
        guard->own.bytes[OWN_4F] = 2;
}

/*
 * @routine 83:C738 Kind42Break
 * WHAT IT DOES: The pot breaks (also its second entry, state $80-$BF,
 * through 83:8DFD): it can no longer hurt, sound $15 if it could be hit,
 * its breaking animation (1) for 16 frames, and its end.
 */
static int Kind42Break(Actor *a)
{
    a->flags &= (uint8_t)~0x80;
    if (a->flags & 0x40)
        SoundQueueAdd(0x15);
    AnimStart(a, 1);
    k42(a)->left = 0x10;
    a->resume = BREAK;
    return 1;
}

/* Thrown with +$07 0: in flight, from its next frame. */
static int flight(Actor *a)
{
    a->resume = FLIGHT;
    return 1;
}

/*
 * @routine 83:C5DE Kind42Start
 * WHAT IT DOES: Actor kind $42, the pot: pose 0, anim_flags bit 1, flags
 * bit 4, look's high byte 6, its probe list. Each frame on its guard a
 * harmless shot can down it; its guard gone (state 0), it breaks; its
 * guard's picture $0E or $11 (struck), it drops (no longer hurting,
 * preset $24, animating and falling until out of the ground, then it
 * breaks); $15 (the throw), it flies at Aladdin (hurting, preset $23). By
 * +$07: 0, it flies until it lands, then rolls at speed $0180 (sound $14
 * if it can be hit) until a tile ahead breaks it, or flies again off an
 * edge; landing at a vertical speed of $0800 or more breaks it. Else
 * (83:C714 Kind42Bounces) it flies until a tile ahead breaks it or it
 * lands, rests 2 frames and springs up again (preset $25). In flight and
 * rolling it tells its guard once it is past Aladdin (Kind42PastPlayer).
 */
int Kind42Start(Actor *a)
{
    Actor *guard;

    switch (a->resume) {
    case ACTOR_START:
        AnimStart(a, 0);
        a->anim_flags |= 0x02;
        a->flags |= 0x10;
        a->look = (uint16_t)((a->look & 0x00FF) | 0x0600);
        a->probes = k42_probes();
        a->resume = CARRIED;
        return 1;
    case ACTOR_START_SECOND:
        return Kind42Break(a);
    case CARRIED:
        (void)ActorDownedByHarmlessShot(a);
        guard = guard_of(a);
        if (!guard->state)
            return Kind42Break(a);
        if (guard->picture == GUARD_STRUCK_A || guard->picture == GUARD_STRUCK_B) {
            a->flags &= (uint8_t)~0x80;
            ActorMotionPreset(a, 0x24);
            a->resume = DROP;
            return 1;
        }
        if (guard->picture != GUARD_THROWS)
            return 1;
        a->flags |= 0x80;
        ActorMotionPreset(a, 0x23);
        if (a->spawn_byte) {
            a->resume = BOUNCE;
            return 1;
        }
        return flight(a);
    case FLIGHT:
        ActorFly(a);
        Kind42PastPlayer(a);
        if (!ActorOutOfGround(a))
            return 1;
        a->speed_x = signed24((uint32_t)(a->speed_x >> 16) << 16 | ROLL_SPEED);
        if ((uint16_t)a->speed_y >= FALL_TOO_FAST)
            return Kind42Break(a);
        if (a->flags & 0x40)
            SoundQueueAdd(0x14);
        (void)ActorSnapToGround(a);
        a->resume = ROLL;
        return 1;
    case ROLL:
        ActorMoveX(a);
        Kind42PastPlayer(a);
        if (Kind42Ahead(a))
            return Kind42Break(a);
        if (ActorSnapToGround(a))
            return 1;
        a->speed_y = 0;                 /* off an edge: in flight again */
        return flight(a);
    case BOUNCE:
        ActorFly(a);
        if (Kind42Ahead(a))
            return Kind42Break(a);
        Kind42PastPlayer(a);
        if (!ActorOutOfGround(a))
            return 1;
        (void)ActorSnapToGround(a);
        a->resume = BOUNCE_SPRING;
        return 2;
    case BOUNCE_SPRING:
        ActorMotionPreset(a, 0x25);
        a->resume = BOUNCE;
        return 1;
    case DROP:
        AnimStep(a);
        ActorFly(a);
        if (!ActorOutOfGround(a))
            return 1;
        return Kind42Break(a);
    default:                            /* BREAK */
        AnimStep(a);
        if (--k42(a)->left)
            return 1;
        return ActorEndPlaced(a);
    }
}

/*
 * @routine 83:C6E3 Kind42Wait
 * WHAT IT DOES: Kind $42's wait routine (each frame it waits, flags bit
 * 7): a harmless shot downs it, a hitting one ends; Aladdin standing on it
 * plays sound $12; when it hurts him and he was not already hurting
 * (g.player_hurt_time 0), its guard's +$4F is set to 1 if it is 0; its
 * animation steps; it ends when out of range by box 0.
 */
int Kind42Wait(Actor *a)
{
    Actor *guard;

    (void)ActorDownedByHarmlessShot(a);
    switch (ActorBouncesOrHurtsPlayer(a)) {
    case 1:
        SoundQueueAdd(0x12);
        break;
    case 2:
        if (!g.player_hurt_time) {
            guard = guard_of(a);
            if (!guard->own.bytes[OWN_4F])
                guard->own.bytes[OWN_4F] = 1;
        }
        break;
    default:
        break;
    }
    AnimStep(a);
    return ActorEndOutOfRange(a, 0);
}
