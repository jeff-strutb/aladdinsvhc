/*
 * kind5d.c: actor kind $5D of level 8, a cloud the Genie's head (kind
 * $5C) makes: it rises to its height (by Aladdin's lives and its number),
 * leaving sparkles; when Aladdin lands on it, it tells its maker, sinks
 * under him and rises again, and again when he leaves it. Once its maker
 * is gone, or he has landed, its wait routine runs its timed end.
 *
 * Its second entry in the record's table is 83:E352 Kind5DWait, its wait
 * routine, entered as code: it would return through the loop's caller.
 * Nothing sets a kind $5D's state bit 7 that this session has found; the
 * row names no second entry, so the loop reports it if it is ever reached.
 */
#include "actor/actor_own.h"
#include "actor/kinds_d.h"
#include "audio/sound.h"
#include "data/kinds_d_tables.h"
#include "player/player.h"

/* its own bytes */
#define STATE OWN_13            /* its wait routine's step: 2 starts its end at once */
#define LANDED OWN_40           /* counted up when Aladdin first lands on it (its maker waits for it) */
#define COUNT OWN_41            /* frames to its next sparkle as it rises; its wait's frames */
#define HEIGHT_AT OWN_42        /* its height's offset in its heights */
#define CLEARED_47 OWN_47       /* cleared as it rises (read by nothing found) */
#define ON OWN_48               /* nonzero when Aladdin landed on it this frame */
#define WAS_ON OWN_49           /* ON as the frame before (its wait keeps it) */
#define GO OWN_4A               /* nonzero once he has landed: its wait starts its end */
#define PARENT_GONE OWN_4E      /* its maker's (kind $5C's): nonzero once it has gone */

enum {
    RISE = ACTOR_RESUME_OWN,    /* each frame of its rise */
    WAIT_FIRST,                 /* waiting for him to land */
    SINK_FIRST,                 /* sinking under him */
    BACK_FIRST,                 /* rising back */
    WAIT_SECOND,                /* waiting for him to leave */
    SINK_SECOND,
    BACK_SECOND,
    IDLE                        /* 83:E30E Kind5DIdle: its timed end (its wait sends it here) */
};

/*
 * @routine 83:E3C1 Kind5DParentGone
 * WHAT IT DOES: Whether its maker (Actor.next, set by kind $5C) has gone:
 * its maker's +$4E. Also 83:E3BD Kind5DParentGoneLong, for kind $8B.
 */
uint8_t Kind5DParentGone(const Actor *a)
{
    return a->next->own.bytes[PARENT_GONE];
}

/*
 * @routine 83:E3AD Kind5DParentCheck
 * WHAT IT DOES: Once its maker has gone, starts its timed end (its wait's
 * step 2), if it has not started.
 */
static void parent_check(Actor *a)
{
    if (Kind5DParentGone(a) && a->own.bytes[STATE] < 2)
        a->own.bytes[STATE] = 2;
}

/*
 * @routine 83:E312 Kind5DLandTest
 * WHAT IT DOES: Whether Aladdin lands on it this frame (PlayerLandOnActor,
 * box $50): then sound $29 and ON 1; else ON 0.
 */
static bool land_test(Actor *a)
{
    a->own.bytes[ON] = 0;
    if (!PlayerLandOnActor(a, 0x50))
        return false;
    SoundQueueAdd(0x29);
    a->own.bytes[ON]++;
    return true;
}

/*
 * @routine 83:E326 Kind5DSinkStep
 * WHAT IT DOES: A frame of the cloud sinking (ActorFlyAgainst) or, as
 * 83:E33C Kind5DRiseStep, rising back (ActorFly): its place kept first
 * (ActorKeepSpot), and Aladdin landed on it carried with it. Returns true
 * while its vertical speed's top byte is negative.
 */
static bool step(Actor *a, bool rising)
{
    bool landed = land_test(a);

    ActorKeepSpot(a);
    if (rising)
        ActorFly(a);
    else
        ActorFlyAgainst(a);
    if (landed)
        PlayerMoveWithActor(a);
    return (a->speed_y >> 16) & 0x80;
}

/* Its sink: preset $49, each frame a sinking step while the speed's top
   byte is not negative. */
static int sink(Actor *a, uint16_t at)
{
    ActorMotionPreset(a, 0x49);
    a->resume = at;
    return 1;
}

/* A wait for a change in his landing: true when it changed. */
static bool changed(Actor *a)
{
    parent_check(a);
    (void)land_test(a);
    return a->own.bytes[ON] != a->own.bytes[WAS_ON];
}

/*
 * @routine 83:E29B Kind5DStart
 * WHAT IT DOES: Actor kind $5D, a cloud: rises (Kind5DRise), then waits
 * for Aladdin to land on it; then marks it for its maker, sinks under him
 * (preset $49) and rises back (preset $4A), and waits for him to leave
 * it; then sinks and rises once more, and waits for him again. Each frame
 * of the waits, Kind5DParentCheck and Kind5DLandTest.
 * Also 83:E3CB Kind5DRise: from where its maker made it, up (speed entry
 * $45) to its height (by Aladdin's lives, Kind5CPathByLives, and twice its
 * $07), with sound $28 and a sparkle (kind $5F, its $07 bit 0 flipped each
 * time) every 4 frames; and 83:E30E Kind5DIdle, where its wait routine
 * sends it for its end.
 */
int Kind5DStart(Actor *a)
{
    switch (a->resume) {
    case ACTOR_START:
        SoundQueueAdd(0x28);
        a->own.bytes[HEIGHT_AT] = (uint8_t)(k5d_heights_at(Kind5CPathByLives()) + (a->spawn_byte << 1));
        a->spawn_at = 0;
        a->spawn_byte = 0;
        a->own.bytes[LANDED] = 0;
        a->own.bytes[COUNT] = 0;
        a->own.bytes[CLEARED_47] = 0;
        ActorSpeedYSet(a, 0x45);
        a->resume = RISE;
        return 1;
    case RISE:
        ActorMoveY(a);
        if (pixels(a->y) < k5d_height(a->own.bytes[HEIGHT_AT])) {
            if (--a->own.bytes[COUNT] & 0x80) {
                (void)ActorCreateChild8(a, 0x5F);
                a->own.bytes[COUNT] = 0x03;
                a->spawn_byte ^= 0x01;
            }
            return 1;
        }
        a->anim_flags = 0x01;
        a->flags = 0x90;
        a->own.bytes[ON] = 0;
        a->own.bytes[WAS_ON] = 0;
        a->own.bytes[GO] = 0;
        a->own.bytes[STATE] = 0;
        AnimStart(a, 0);
        a->resume = WAIT_FIRST;
        return 1;
    case WAIT_FIRST:
        if (!changed(a))
            return 1;
        a->own.bytes[LANDED]++;
        a->own.bytes[GO] = 1;
        return sink(a, SINK_FIRST);
    case WAIT_SECOND:
        if (!changed(a))
            return 1;
        return sink(a, SINK_SECOND);
    case SINK_FIRST:
    case SINK_SECOND:
        if (!step(a, false))
            return 1;
        ActorMotionPreset(a, 0x4A);
        a->resume = a->resume == SINK_FIRST ? BACK_FIRST : BACK_SECOND;
        return 1;
    case BACK_FIRST:
    case BACK_SECOND:
        if (step(a, true))
            return 1;
        a->resume = a->resume == BACK_FIRST ? WAIT_SECOND : WAIT_FIRST;
        return 1;
    default:                            /* IDLE */
        return 1;
    }
}

/*
 * @routine 83:E352 Kind5DWait
 * WHAT IT DOES: Kind $5D's wait routine, a timed sequence: WAS_ON = ON and
 * its animation each frame; once GO is set, $B0 frames; its +$08 bit 4
 * set, $1F frames, then animation 1 and its code to go on at Kind5DIdle;
 * $38 frames, then it ends for good.
 */
int Kind5DWait(Actor *a)
{
    uint8_t *count = &a->own.bytes[COUNT];

    a->own.bytes[WAS_ON] = a->own.bytes[ON];
    AnimStep(a);
    switch (a->own.bytes[STATE]) {
    case 0:
        if (a->own.bytes[GO]) {
            *count = 0xB0;
            a->own.bytes[STATE]++;
        }
        return ACTOR_GOES_ON;
    case 1:
        if (!--*count)
            a->own.bytes[STATE]++;
        return ACTOR_GOES_ON;
    case 2:
        a->anim_flags |= 0x10;
        *count = 0x1F;
        a->own.bytes[STATE]++;
        return ACTOR_GOES_ON;
    case 3:
        if (--*count)
            return ACTOR_GOES_ON;
        AnimStart(a, 1);
        *count = 0x38;
        a->resume = IDLE;
        a->own.bytes[STATE]++;
        return ACTOR_GOES_ON;
    default:                            /* 4 (none past it is set) */
        if (--*count)
            return ACTOR_GOES_ON;
        return ActorEndPlaced(a);
    }
}
