/*
 * kind77.c: actor kinds $77 and $78, a platform that waits until Aladdin
 * is on it, then speeds up to the right until it reaches its end, carrying
 * him while he stays on. The two differ only in their course.
 */
#include "actor/actor_own.h"

/* its own bytes */
typedef struct {
    uint8_t before[OWN_40];
    uint8_t on;                 /* +$40: nonzero while Aladdin stands on it this frame */
    uint8_t speed_cap[2];       /* +$41-$42: it speeds up while its speed's low word is below this */
    uint8_t end_x[2];           /* +$43-$44: it moves while its x pixels are below this */
} K77Own;

static inline K77Own *k77(Actor *a) { return (K77Own *)a->own.bytes; }

static uint16_t word_of(const uint8_t w[2]) { return (uint16_t)(w[0] | w[1] << 8); }

static void word_set(uint8_t w[2], uint16_t v)
{
    w[0] = (uint8_t)v;
    w[1] = (uint8_t)(v >> 8);
}

#define K77_BOX 0x28            /* its box in ActorSolidForPlayer's boxes */
#define OWN_13_START 0x04       /* its +$13 at its start */

enum { WAIT_ON = ACTOR_RESUME_OWN, RIDE };

static int Kind77Ride(Actor *a);

/* Its start, with its course: the speed cap, the end x, its acceleration
   and the speed table's entry it starts at. */
static int k77_start(Actor *a, uint16_t cap, uint16_t end_x, uint8_t accel, uint8_t entry)
{
    K77Own *k = k77(a);

    word_set(k->speed_cap, cap);
    word_set(k->end_x, end_x);
    a->accel_x = accel;
    ActorSpeedXSet(a, entry);
    a->anim_flags |= 0x02;
    a->flags |= 0x90;
    AnimStart(a, 0);
    a->own.bytes[OWN_13] = OWN_13_START;
    a->resume = WAIT_ON;
    return 1;
}

/* One frame of the ride, after its test of whether he is on it. */
static int ride(Actor *a, bool on)
{
    K77Own *k = k77(a);
    Actor *player = &g.actors[0];
    uint16_t was_x = 0, was_y = 0;

    if (on) {                           /* he is on: where it is now, kept */
        k->on++;
        was_x = pixels(a->x);
        was_y = pixels(a->y);
    }
    if (pixels(a->x) < word_of(k->end_x)) {     /* to the right, faster up to its cap, until its end */
        if ((uint16_t)a->speed_x < word_of(k->speed_cap))
            ActorAccelerateX(a);
        ActorMoveRight(a);
    }
    if (k->on) {                        /* he moves with it */
        player->x = with_pixels(player->x, (uint16_t)(pixels(a->x) - was_x + pixels(player->x)));
        player->y = with_pixels(player->y, (uint16_t)(pixels(a->y) - was_y + pixels(player->y)));
    }
    ActorHoldPlayer(a);
    a->resume = RIDE;
    return 1;
}

/*
 * @routine 83:F4AA Kind77Start
 * WHAT IT DOES: Actor kind $77: a solid platform (box $28) that Aladdin
 * stands on and that holds him (ActorHoldPlayer); anim_flags bit 1, flags
 * $90, animation 0, its +$13 4. Once he is on it, each frame it speeds up
 * to the right (acceleration $10, from speed entry $48, while its speed is
 * below $00C0) until its x reaches $0630, and while he is on it he moves
 * with it.
 * The original keeps the platform's place as he got on in scratch work RAM
 * ($000C, $000E), read in the same frame: here, locals. On a frame he is
 * not on it, +$40 is 0 and nothing is carried.
 */
int Kind77Start(Actor *a)
{
    switch (a->resume) {
    case ACTOR_START:
        return k77_start(a, 0x00C0, 0x0630, 0x10, 0x48);
    default:
        return Kind77Ride(a);
    }
}

/*
 * @routine 83:F494 Kind78Start
 * WHAT IT DOES: Actor kind $78: the platform of kind $77 with its own
 * course: from speed entry $4B with acceleration 1, while its speed is
 * below $0180, until its x reaches $0B90.
 */
int Kind78Start(Actor *a)
{
    switch (a->resume) {
    case ACTOR_START:
        return k77_start(a, 0x0180, 0x0B90, 0x01, 0x4B);
    default:
        return Kind77Ride(a);
    }
}

/* Kinds $77 and $78 after their start (the original's code from $83:F4D2
   on): until he is on it, each frame it is solid for him and holds him;
   then each frame the ride. */
static int Kind77Ride(Actor *a)
{
    switch (a->resume) {
    case WAIT_ON:
        if (ActorSolidForPlayer(a, K77_BOX) != ACTOR_SOLID_ON_TOP) {
            ActorHoldPlayer(a);
            return 1;
        }
        return ride(a, true);
    case RIDE:
        k77(a)->on = 0;
        return ride(a, ActorSolidForPlayer(a, K77_BOX) == ACTOR_SOLID_ON_TOP);
    default:
        return 1;
    }
}

/*
 * @routine 83:F53C Kind77Wait
 * WHAT IT DOES: The wait routine of kinds $77 and $78: their animation
 * plays on (AnimStep).
 */
int Kind77Wait(Actor *a)
{
    AnimStep(a);
    return ACTOR_GOES_ON;
}
