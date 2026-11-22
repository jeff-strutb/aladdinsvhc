/*
 * kind_waits_rest.c: the wait routines of kinds C's kinds that the record
 * keeps together (the actor loop's wait table, 83:8F4F, reaches them each
 * frame an actor of the kind waits): they animate, test touches and shots,
 * and end the actor when it is out of range by its box. Each returns
 * ACTOR_GOES_ON, or ACTOR_ENDED when it ended its actor. The record's file
 * also holds the waits of other sessions' kinds ($22, $34, $3F, $42-$44,
 * $48, $4E, $56-$58, $5A, $5C, $5D, $65, $6A, $71, $77): each is written by
 * its kind's owner (one kind, one owner).
 */
#include "actor/kinds_c.h"
#include "video/view.h"

/* ---- the views of the kinds' own bytes these routines read ---- */

typedef struct {                /* kind $2B */
    uint8_t before[OWN_4E];
    uint8_t stays;              /* +$4E: nonzero: never ended out of range */
    uint8_t unused4f;
    uint8_t unused4a;
    uint8_t cleared;            /* +$4B: cleared while bit 4 of its +$07 is set */
} Kind2BWaitOwn;

typedef struct {                /* kind $2C */
    uint8_t before[OWN_43];
    uint8_t frames;             /* +$43: frames to its next sound */
    uint8_t sounds;             /* +$44: nonzero: sound $40 every 6 frames */
} Kind2CWaitOwn;

typedef struct {                /* kind $41 */
    uint8_t before[OWN_42];
    uint8_t active;             /* +$42: nonzero: it turns shots back and Aladdin can stand on it */
    uint8_t frames;             /* +$43: frames before he can stand on it again */
    uint8_t unused44[OWN_48 - OWN_44];
    uint8_t x_limit[2];         /* +$48-$49: the x its side is judged by */
    uint8_t unused4c[OWN_4A - OWN_4C];
    uint8_t y_limit[2];         /* +$4A-$4B: below this y it may end */
} Kind41WaitOwn;

typedef struct {                /* kind $50 */
    uint8_t before[OWN_45];
    uint8_t downed;             /* +$45: counted up each time a shot downs it facing right */
} Kind50WaitOwn;

#define VIEW(type, a) ((type *)(a)->own.bytes)
#define WORD(b) ((uint16_t)((b)[0] | (b)[1] << 8))

_Static_assert(sizeof(Kind41WaitOwn) <= OWN_BYTES && sizeof(Kind2BWaitOwn) <= OWN_BYTES, "a view fits the own bytes");

/* the y pixels of a 16.8 place, moved by `by` */
static uint32_t y_moved(uint32_t y, int by) { return with_pixels(y, (uint16_t)(pixels(y) + by)); }

/*
 * @routine 83:91F3 ActorWaitNothing
 * WHAT IT DOES: The wait of the kinds that do nothing while they wait
 * (kinds 4-6, 9-$0B, $25 and others); declared in kinds_c.h for every
 * owner's row.
 */
int ActorWaitNothing(Actor *a)
{
    (void)a;
    return ACTOR_GOES_ON;
}

/*
 * @routine 83:9D46 Kind23Wait
 * @routine 83:DEB9 Kind59Wait
 * WHAT IT DOES: The wait of kinds $23 and $59: its animation. Two copies of
 * the same bytes (AnimStep, RTS) in the original, one function here.
 */
int Kind23Wait(Actor *a)
{
    AnimStep(a);
    return ACTOR_GOES_ON;
}

/*
 * @routine 83:B190 Kind3EWait
 * WHAT IT DOES: Kind $3E's wait: its end when out of range by box $38.
 */
int Kind3EWait(Actor *a)
{
    return ActorEndOutOfRange(a, 0x38);
}

/*
 * @routine 83:BB91 Kind33Wait
 * @routine 83:BF7E Kind3AWait
 * WHAT IT DOES: The wait of kinds $33 and $3A: its end when out of range
 * by box 8. Two copies of the same bytes in the original.
 */
int Kind33Wait(Actor *a)
{
    return ActorEndOutOfRange(a, 0x08);
}

/*
 * @routine 83:A709 Kind2BWait
 * WHAT IT DOES: Kind $2B's wait: its animation; Aladdin touching it is
 * hurt; a shot can down it; with bit 4 of its +$07 its +$4B is cleared;
 * unless +$4E is set, its end when out of range by box 8.
 */
int Kind2BWait(Actor *a)
{
    AnimStep(a);
    (void)ActorHurtsPlayerOnTouch(a);
    (void)ActorDownedByShot(a);
    if (a->spawn_byte & 0x10)
        VIEW(Kind2BWaitOwn, a)->cleared = 0;
    if (VIEW(Kind2BWaitOwn, a)->stays)
        return ACTOR_GOES_ON;
    return ActorEndOutOfRange(a, 0x08);
}

/*
 * @routine 83:A818 Kind2CWait
 * WHAT IT DOES: Kind $2C's wait: its animation; while +$44 is set, sound
 * $40 every 6 frames.
 */
int Kind2CWait(Actor *a)
{
    Kind2CWaitOwn *o = VIEW(Kind2CWaitOwn, a);

    AnimStep(a);
    if (o->sounds && !--o->frames) {
        SoundQueueAdd(0x40);
        o->frames = 6;
    }
    return ACTOR_GOES_ON;
}

/*
 * @routine 83:B303 Kind41Wait
 * WHAT IT DOES: Kind $41's wait. Active: it turns shots back; every $20
 * frames at most, while he has health, Aladdin coming down on it (its box
 * 8 pixels higher) stands on it, with sound $12; it animates. Without bit
 * 3 or 4 of its +$07 it ends when out of range by box $28. With either it
 * ends for good at or below y $2F0; above its +$4A and on its side of its
 * +$48 (its facing: 0 left of it, 1 at or right) it stays; else it ends
 * when out of range by box 0.
 * Inactive: with bit 3 or 4 it ends for good once g.range_x is $100 or
 * more short of it, else when out of range by box 0.
 */
int Kind41Wait(Actor *a)
{
    Kind41WaitOwn *o = VIEW(Kind41WaitOwn, a);
    uint16_t y;

    if (!o->active) {
        if (!(a->spawn_byte & 0x18))
            return ActorEndOutOfRange(a, 0x00);
        if ((uint16_t)(pixels(a->x) - 0x100 - view.dx) < g.range_x)
            return ACTOR_GOES_ON;
        return ActorEndPlaced(a);
    }
    (void)ActorTurnsShotBack(a);
    if (o->frames) {
        o->frames--;
    } else {
        uint32_t kept = a->y;

        a->y = y_moved(a->y, -8);
        if (g.health && ActorBouncesOrHurtsPlayer(a) == 1) {
            SoundQueueAdd(0x12);
            o->frames = 0x20;
        }
        a->y = with_pixels(a->y, pixels(kept));
    }
    AnimStep(a);
    if (!(a->spawn_byte & 0x18))
        return ActorEndOutOfRange(a, 0x28);
    y = pixels(a->y);
    if (y >= 0x02F0)
        return ActorEndPlaced(a);
    if (y < WORD(o->y_limit) && (pixels(a->x) >= WORD(o->x_limit)) == a->facing)
        return ACTOR_GOES_ON;
    return ActorEndOutOfRange(a, 0x00);
}

/*
 * @routine 83:D1B0 Kind50Wait
 * WHAT IT DOES: Kind $50's wait: its animation; Aladdin touching it is
 * hurt or stomps it (a margin of $10); a shot downing it while it faces
 * right counts in its +$45; its end when out of range by box 0.
 */
int Kind50Wait(Actor *a)
{
    AnimStep(a);
    (void)ActorStompOrHurt16(a);
    if (ActorDownedByShot(a) != NULL && !(a->facing & 0x80))
        VIEW(Kind50WaitOwn, a)->downed++;
    return ActorEndOutOfRange(a, 0x00);
}

