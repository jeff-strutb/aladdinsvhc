/*
 * kind61.c: actor kinds $61 and $64, pads that bounce Aladdin: when his
 * test box meets one he is sent on in his bounce code (PLAYER_AT_BOUNCE)
 * from its top, kind $61 also setting his x to its own; while he is hurt it
 * pushes him aside instead.
 */
#include "actor/actor_own.h"
#include "actor/kinds_e.h"
#include "player/player.h"

/* its own bytes */
typedef struct {
    uint8_t before[OWN_40];
    uint8_t wait;               /* +$40: frames before it can bounce him again */
    uint8_t keep_x;             /* +$41: 0 (kind $61): his x is set to its own */
} PadOwn;

static inline PadOwn *own(Actor *a) { return (PadOwn *)a->own.bytes; }

#define LEVEL_TAKEN_TEST 0x0A   /* the level whose pads test that he is free and they can be hit */

enum {
    PAD = ACTOR_RESUME_OWN,     /* each frame */
    PLACE,                      /* the frame after a bounce: he is put on it */
};

/* the pad's start, both kinds (83:E4F4): its wait routine on, no wait */
static int pad_start(Actor *a)
{
    a->flags |= 0x80;
    own(a)->wait = 0;
    a->resume = PAD;
    return 1;
}

/* each frame: unless landing on actors is off (bit 7), or in level $0A
   while he is held or it cannot be hit, or it still waits: his test box
   on it: g.player_in_air set; hurt, he is pushed aside; else $14 frames of
   wait, sound $12, his code to his bounce, and next frame his place */
static int pad_frame(Actor *a)
{
    if (g.player_no_landing & 0x80)
        return 1;
    if ((uint8_t)g.level == LEVEL_TAKEN_TEST
        && ((uint8_t)g.player_no_actor_landing || !(a->flags & 0x40)))
        return 1;
    if (own(a)->wait) {
        own(a)->wait--;
        return 1;
    }
    if (!ActorHitsTestBoxIfActive(a))
        return 1;
    g.player_in_air = 1;
    if (g.player_hurt_count) {
        PlayerPushAside(a);
        return 1;
    }
    own(a)->wait = 0x14;
    SoundQueueAdd(0x12);
    g.actors[0].resume = PLAYER_AT_BOUNCE;
    a->resume = PLACE;
    return 1;
}

/*
 * @routine 83:E4F0 Kind64Start
 * WHAT IT DOES: Actor kind $64 (and $61 from 83:E4F4): a pad (pad_frame).
 * After a bounce, the next frame his y is its y less 8 (the original's SBC
 * of 7 runs with the carry its REP #$21 cleared before the frame's wait),
 * his x its x for kind $61 (+$41 0), and g.player_landed the byte at $0001
 * (1 in every frame of the recorded attract play's work RAM, and the
 * value the core's other landings store). Its second entry is Kind65Start
 * (Kind64Second, Kind61Second).
 */
int Kind64Start(Actor *a)
{
    switch (a->resume) {
    case ACTOR_START:
        own(a)->keep_x = 1;
        return pad_start(a);
    case PAD:
        return pad_frame(a);
    case PLACE: {
        Actor *p = &g.actors[0];

        p->y = with_pixels(p->y, (uint16_t)(pixels(a->y) - 8));
        if (!own(a)->keep_x)
            p->x = with_pixels(p->x, pixels(a->x));
        g.player_landed = 1;
        a->resume = PAD;
        return 1;
    }
    default:
        return 1;
    }
}

/* kinds $61 and $64's second entry (state $80 up, 83:8DFD): Kind65Start, the
   continues counter's start, which then owns the record (ActorRunAs) */
int Kind64Second(Actor *a) { return ActorRunAs(a, Kind65Start, ACTOR_START); }
int Kind61Second(Actor *a) { return ActorRunAs(a, Kind65Start, ACTOR_START); }

/*
 * @routine 83:E4EC Kind61Start
 * WHAT IT DOES: Actor kind $61: the pad that bounces Aladdin and puts him
 * at its x (+$41 0), then on as Kind64Start.
 */
int Kind61Start(Actor *a)
{
    if (a->resume == ACTOR_START) {
        own(a)->keep_x = 0;
        return pad_start(a);
    }
    return Kind64Start(a);
}
