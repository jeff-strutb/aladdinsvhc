/*
 * kind2c.c: actor kind $2C, a ride: when Aladdin comes into its zone it
 * takes him (his code PlayerCarried) and flies off right and down, carrying
 * him, until it reaches the x its +$07 gives; there it lets him go (his
 * code PlayerFall) and stays. Its wait routine is Kind2CWait
 * (kind_waits_rest.c), which plays its sound while it carries him. Its
 * second entry in the original's table (83:A82F) is kind $2D's start: it
 * hands its record to kind $2D's code (ActorRunAs).
 */
#include "actor/kinds_c.h"
#include "player/player.h"

#define K2C_END_FAR 0x0F93      /* the x pixels where the ride ends, with +$07 nonzero */
#define K2C_END_NEAR 0x0CF3     /* and with +$07 0 */
#define K2C_ZONE 0x74           /* ActorZoneHasTestPoint's zone */
#define K2C_PRESET 0x2D         /* ActorMotionPreset's preset for its flight */
#define K2C_ACCEL_X 0x0C        /* its horizontal acceleration */
#define K2C_TOP_X 0x04          /* the horizontal speed's middle byte it stops speeding up at */
#define K2C_RIDER_Y 0x1A        /* Aladdin's y below it while it carries him */

/* its own bytes */
typedef struct {
    uint8_t before[OWN_40];
    uint8_t end_x[2];           /* +$40-$41: the x pixels where the ride ends */
    uint8_t unused42;
    uint8_t frames;             /* +$43: frames to its wait's next sound, from 6 */
    uint8_t sounds;             /* +$44: nonzero while it carries him: its wait's sound */
} Kind2COwn;

_Static_assert(sizeof(Kind2COwn) <= OWN_BYTES, "a view fits the own bytes");

static Kind2COwn *own(Actor *a) { return (Kind2COwn *)a->own.bytes; }

static uint16_t end_x(Actor *a) { return (uint16_t)(own(a)->end_x[0] | own(a)->end_x[1] << 8); }

enum {
    WAIT_FOR_HIM = ACTOR_RESUME_OWN,    /* each frame until he is in its zone */
    CARRY,                              /* each frame of the ride */
    STAY,                               /* for good, where it let him go */
};

/*
 * @routine 83:A806 Kind2CCarry
 * WHAT IT DOES: Keeps Aladdin with the ride: his x its x, his y $1A below
 * its y.
 */
static void carry(Actor *a)
{
    Actor *player = &g.actors[0];

    player->x = with_pixels(player->x, pixels(a->x));
    player->y = with_pixels(player->y, (uint16_t)(pixels(a->y) + K2C_RIDER_Y));
}

/*
 * @routine 83:A758 Kind2CStart
 * WHAT IT DOES: Actor kind $2C: a ride. It waits until Aladdin comes into
 * its zone (he can land on an actor), then takes him (his code
 * PlayerCarried, its wait's sound on) and flies off: speeding up right and
 * down to a speed of 4 across, carrying him $1A below it, until it
 * reaches the end x ($0CF3, $0F93 with +$07 set); there it puts him and
 * itself at that x and lets him go (his code PlayerFall), and stays for
 * good.
 */
static int start(Actor *a)
{
    uint16_t x = a->spawn_byte ? K2C_END_FAR : K2C_END_NEAR;

    own(a)->end_x[0] = (uint8_t)x;
    own(a)->end_x[1] = (uint8_t)(x >> 8);
    a->anim_flags |= 0x02;
    a->flags |= 0x90;
    AnimStart(a, 0);
    own(a)->sounds = 0;
    own(a)->frames = 6;
    a->resume = WAIT_FOR_HIM;
    return 1;
}

static int wait_for_him(Actor *a)
{
    Actor *player = &g.actors[0];

    if ((g.player_no_landing & 0x80) || !ActorZoneHasTestPoint(a, K2C_ZONE))
        return 1;
    g.rider_kind = a->kind;
    own(a)->sounds++;
    g.player_flags |= 0xFE;
    a->anim_flags &= (uint8_t)~0x08;
    player->facing = 0;
    player->flip = 0;
    player->resume = PLAYER_AT_CARRIED;
    ActorMotionPreset(a, K2C_PRESET);
    a->accel_x = K2C_ACCEL_X;
    a->resume = CARRY;
    return 1;
}

static int ride(Actor *a)
{
    Actor *player = &g.actors[0];

    if ((uint8_t)(a->speed_x >> 8) != K2C_TOP_X) {
        ActorAccelerateX(a);
        ActorAccelerateY(a);
    }
    ActorMoveRight(a);
    ActorStepY(a);
    carry(a);
    if (pixels(a->x) < end_x(a))
        return 1;
    g.player_flags &= (uint8_t)~0xFE;
    g.player_hurts = 0;
    a->anim_flags |= 0x08;
    a->x = with_pixels(a->x, end_x(a));
    player->x = with_pixels(player->x, end_x(a));
    player->speed_x = (int32_t)(((uint32_t)player->speed_x & 0xFFu) | ((uint32_t)a->speed_x & ~0xFFu));
    player->resume = PLAYER_AT_FALL;
    own(a)->sounds = 0;
    a->resume = STAY;
    return 1;
}

/* Its second entry: kind $2D's start (83:A82F). */
static int second(Actor *a)
{
    return ActorRunAs(a, Kind2DCode, ACTOR_START);
}

int Kind2CCode(Actor *a)
{
    switch (a->resume) {
    case ACTOR_START:
        return start(a);
    case ACTOR_START_SECOND:
        return second(a);
    case WAIT_FOR_HIM:
        return wait_for_him(a);
    case CARRY:
        return ride(a);
    case STAY:
        return 1;
    default:
        return kind_lost(a, "Kind2CCode");
    }
}
