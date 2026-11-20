/*
 * kind56.c: actor kind $56 of level 3, the monkey at the market's end: it
 * waits until Aladdin is right of x $0528, runs a while, and waits for his
 * walk-out scene (g.scene_cue); then it runs at him, gives its sign
 * (g.scene_cue = $80) and runs off. Its wait, 83:D9B5 Kind56Wait (only
 * AnimStep), is the core's AnimateWhileWaiting, which its row names.
 */
#include "actor/actor_own.h"
#include "actor/kinds_e.h"
#include "data/kinds_e_tables.h"
#include "map/map.h"

#define RUN_SPEED 0x30          /* ActorSpeedXSet's entry for its runs */
#define NEAR 0xEC               /* added to its x's low byte: it runs until that is below his */

/* its own bytes: +$40, the frames left of its first run */
typedef struct {
    uint8_t before[OWN_40];
    uint8_t steps;              /* +$40 */
} Kind56Own;

static inline Kind56Own *own(Actor *a) { return (Kind56Own *)a->own.bytes; }

enum {
    WAIT_PLAYER = ACTOR_RESUME_OWN, /* each frame until he is right of x $0528 */
    RUN_SET,                        /* after its 76 frames */
    RUN_1,                          /* each of its first run's $F0 frames */
    WAIT_CUE,                       /* each frame until the scene's cue */
    RUN_2_SET,                      /* after its $40 frames */
    RUN_2,                          /* each frame of its run at him */
    SIGN,                           /* after its $80 frames posing */
    OFF_SET,                        /* after its $10 frames */
    OFF,                            /* each frame of its run off, for good */
};

static void set_facing(Actor *a, uint8_t f)
{
    a->facing = f;
    a->flip = f;
}

/* a run: pose 1, speed $30, from the next frame */
static int run(Actor *a, int at)
{
    AnimStart(a, 0x01);
    ActorSpeedXSet(a, RUN_SPEED);
    a->resume = (uint16_t)at;
    return 1;
}

/* each frame of a run on the ground */
static void run_step(Actor *a)
{
    MapPoint p;

    ActorMoveX(a);
    (void)ActorSnapToGround(a, &p);
}

/*
 * @routine 83:D8E2 Kind56Start
 * WHAT IT DOES: Actor kind $56: put at x $05B0, y $01B0 facing left (+$08
 * bit 0 and its wait routine on, its probe list), pose 0, it waits until
 * Aladdin's x is $0528 to $05FF, then after 76 frames runs right $F0
 * frames on the ground and turns (pose 2, +$08 bits 0-1 = 2); once his
 * walk-out scene sets g.scene_cue it waits $40 frames, runs at him until
 * it is near (its x's low byte less $14 below his), poses (3) $80 frames,
 * sets g.scene_cue to $80, and after $10 frames runs off right for good, 4
 * pixels higher. Its second entry is its wait routine entered as code,
 * which no script reaches: not converted.
 */
int Kind56Start(Actor *a)
{
    switch (a->resume) {
    case ACTOR_START:
        a->anim_flags |= 0x01;
        a->flags |= 0x90;
        a->probes = k56_probes();
        a->x = with_pixels(a->x, 0x05B0);
        a->y = with_pixels(a->y, 0x01B0);
        set_facing(a, 1);
        AnimStart(a, 0x00);
        a->resume = WAIT_PLAYER;
        return 1;
    case WAIT_PLAYER: {
        uint16_t px = pixels(g.actors[0].x);

        if ((px >> 8) != 0x05 || (uint8_t)px < 0x28)
            return 1;
        a->resume = RUN_SET;
        return 0x4C;
    }
    case RUN_SET:
        set_facing(a, 0);
        own(a)->steps = 0xF0;
        return run(a, RUN_1);
    case RUN_1:
        run_step(a);
        if (--own(a)->steps)
            return 1;
        a->anim_flags = (uint8_t)((a->anim_flags & 0xFC) | 0x02);
        set_facing(a, 1);
        AnimStart(a, 0x02);
        a->resume = WAIT_CUE;
        return 1;
    case WAIT_CUE:
        if (!g.scene_cue)
            return 1;
        a->resume = RUN_2_SET;
        return 0x40;
    case RUN_2_SET:
        return run(a, RUN_2);
    case RUN_2:
        run_step(a);
        if ((uint8_t)((uint8_t)pixels(a->x) + NEAR) >= (uint8_t)pixels(g.actors[0].x))
            return 1;
        AnimStart(a, 0x03);
        a->resume = SIGN;
        return 0x80;
    case SIGN:
        g.scene_cue = 0x80;
        a->resume = OFF_SET;
        return 0x10;
    case OFF_SET:
        set_facing(a, 0);
        a->y = with_pixels(a->y, (uint16_t)(pixels(a->y) - 4));
        return run(a, OFF);
    case OFF:
        ActorMoveX(a);
        return 1;
    default:
        return 1;
    }
}
