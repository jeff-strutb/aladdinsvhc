/*
 * kind8b.c: actor kind $8B, a cloud that rises as kind $5D's does (Kind5DRise),
 * then bobs, takes hold of Aladdin when he is within reach, lifts him and
 * lets him go; ended once its maker has gone. Its wait routine (Kind8BWait)
 * does the bobbing and the holding.
 */
#include "actor/actor_own.h"
#include "actor/actor_a.h"
#include "actor/kinds_d.h"
#include "audio/sound.h"
#include "player/player.h"

#define PHASE OWN_13            /* Kind8BGrab's phase: 0 it waits for Aladdin, 1 it keeps him, 2 it waits to again */
#define GRABBED OWN_40          /* set once it has taken hold of Aladdin */
#define TIMER OWN_41            /* frames before it ends after that */
#define COUNT OWN_46            /* phase 2's frames */
#define ENDS OWN_47             /* set when it is to end: its wait routine ends the actor */
#define STEP OWN_48             /* Kind8BStep's step, 0-8 */
#define Y_LIMIT_LO OWN_49       /* the y (a word) its steps move to */
#define Y_LIMIT_HI OWN_4A

enum {
    RISE = ACTOR_RESUME_OWN,    /* each frame of its rise */
    UNTIL_TAKEN,                /* each frame, until it has taken hold of him or its maker has gone */
    UNTIL_GONE,                 /* $FF frames more, or until its maker has gone */
    THE_END,                    /* after the $57 frames' wait */
    STAYS                       /* each frame, until its wait routine ends it */
};

static uint16_t y_limit(const Actor *a)
{
    return (uint16_t)(a->own.bytes[Y_LIMIT_LO] | a->own.bytes[Y_LIMIT_HI] << 8);
}

static void set_y_limit(Actor *a, uint16_t v)
{
    a->own.bytes[Y_LIMIT_LO] = (uint8_t)v;
    a->own.bytes[Y_LIMIT_HI] = (uint8_t)(v >> 8);
}

/*
 * @routine 84:987A Kind8BStep
 * WHAT IT DOES: One step of its motion, by STEP: 0 nothing; 1 its speed set
 * (preset $5C) and on into 2, each frame it flies against the speed until
 * its y is below Y_LIMIT (STEP up to 3); 3 preset $5D and on into 4, flying
 * with it until its y is Y_LIMIT or more (STEP 1); 5 preset $5E and on into 6,
 * flying against it until the speed's top byte is negative (STEP up); 7
 * Y_LIMIT = its y less 8, preset $5F and on into 8, flying with it until the
 * top byte is not negative (STEP 4, its acceleration $0A).
 */
static void step(Actor *a)
{
    uint8_t *s = &a->own.bytes[STEP];

    switch (*s) {
    case 1:
        ActorMotionPreset(a, 0x5C);
        ++*s;
        /* fall through */
    case 2:
        ActorFlyAgainst(a);
        if (pixels(a->y) < y_limit(a))
            ++*s;
        return;
    case 3:
        ActorMotionPreset(a, 0x5D);
        ++*s;
        /* fall through */
    case 4:
        ActorFly(a);
        if (pixels(a->y) >= y_limit(a))
            *s = 1;
        return;
    case 5:
        ActorMotionPreset(a, 0x5E);
        ++*s;
        /* fall through */
    case 6:
        ActorFlyAgainst(a);
        if ((a->speed_y >> 16) & 0x80)
            ++*s;
        return;
    case 7:
        set_y_limit(a, (uint16_t)(pixels(a->y) - 8));
        ActorMotionPreset(a, 0x5F);
        ++*s;
        /* fall through */
    case 8:
        ActorFly(a);
        if ((a->speed_y >> 16) & 0x80)
            return;
        *s = 4;
        a->accel_y = 0x0A;
        return;
    default:                            /* 0, and past the table's nine words: nothing sets one */
        return;
    }
}

/*
 * @routine 84:98FF Kind8BGrab
 * WHAT IT DOES: The other step of its wait, by PHASE: 0, it takes hold of
 * Aladdin when he is within reach and nothing keeps him (his code to
 * PlayerHeld, his place kept at its place, GRABBED 1, animation $15, STEP 5;
 * with ENDS set the let-go count up); 1, it keeps him at its x and y + 7
 * while he is kept, then (or when he is not) animation $16 and a 16-frame
 * wait; 2, that wait.
 */
static void grab(Actor *a)
{
    Actor *p = &g.actors[0];

    switch (a->own.bytes[PHASE]) {
    case 0:
        if ((uint8_t)g.player_no_actor_landing)
            return;
        if (!ActorGrabbableByPlayer(a))
            return;
        p->resume = PLAYER_AT_HELD;
        g.player_saved_x = pixels(a->x);
        g.player_saved_y = pixels(a->y);
        g.player_no_actor_landing = (uint16_t)((g.player_no_actor_landing & 0xFF00) | (uint8_t)((uint8_t)g.player_no_actor_landing + 1));
        a->own.bytes[GRABBED] = 1;
        AnimStart(a, 0x15);
        a->own.bytes[STEP] = 5;
        a->own.bytes[PHASE]++;
        if (a->own.bytes[ENDS])
            g.player_let_go++;
        return;
    case 1:
        if ((uint8_t)g.player_no_actor_landing) {
            p->x = with_pixels(p->x, pixels(a->x));
            p->y = with_pixels(p->y, (uint16_t)(pixels(a->y) + 7));
            if (!a->own.bytes[ENDS])
                return;
            g.player_let_go++;
        }
        a->own.bytes[PHASE]++;
        a->own.bytes[COUNT] = 0x10;
        AnimStart(a, 0x16);
        return;
    case 2:
        if (!--a->own.bytes[COUNT])
            a->own.bytes[PHASE] = 0;
        return;
    default:                            /* past the table's three words: nothing sets one */
        return;
    }
}

/*
 * @routine 84:9861 Kind8BWaitWork
 * WHAT IT DOES: What actors of kind $8B do each frame they wait: Kind8BStep
 * and Kind8BGrab, their animation steps, and once ENDS is set the actor
 * ends. (Also 83:932E ActorKind8BWait, its stub.)
 */
int Kind8BWait(Actor *a)
{
    step(a);
    grab(a);
    AnimStep(a);
    if (a->own.bytes[ENDS])
        return ActorEndPlaced(a);
    return ACTOR_GOES_ON;
}

/*
 * @routine 84:9813 Kind8BStart
 * WHAT IT DOES: Actor kind $8B: rises as kind $5D's does (Kind5DStart's
 * rise); then flags $80, anim_flags 2, palette byte 6, animation $10, STEP 1,
 * Y_LIMIT its y. Each frame, its maker gone (Kind5DParentGone): the end;
 * GRABBED set: TIMER $FF and each frame the same test and TIMER down to 0.
 * The end: anim_flags bit 4 set, a wait of $57 frames, ENDS up (its wait
 * routine then ends it).
 */
int Kind8BStart(Actor *a)
{
    switch (a->resume) {
    case ACTOR_START:
        Kind5DRiseBegin(a);
        a->resume = RISE;
        return 1;
    case RISE:
        if (!Kind5DRiseFrame(a))
            return 1;
        a->anim_flags = 0x02;
        a->flags = 0x80;
        a->look = (uint16_t)((a->look & 0x00FF) | 0x0600);
        AnimStart(a, 0x10);
        a->own.bytes[STEP] = 1;
        set_y_limit(a, pixels(a->y));
        a->resume = UNTIL_TAKEN;
        return 1;
    case UNTIL_TAKEN:
        if (!Kind5DParentGone(a)) {
            if (!a->own.bytes[GRABBED])
                return 1;
            a->own.bytes[TIMER] = 0xFF;
            a->resume = UNTIL_GONE;
            return 1;
        }
        break;
    case UNTIL_GONE:
        if (!Kind5DParentGone(a) && --a->own.bytes[TIMER])
            return 1;
        break;
    case THE_END:
        a->own.bytes[ENDS]++;
        a->resume = STAYS;
        return 1;
    default:                            /* STAYS: its wait routine ends it */
        return 1;
    }
    a->anim_flags |= 0x10;
    a->resume = THE_END;
    return 0x57;
}
