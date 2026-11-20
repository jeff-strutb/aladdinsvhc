/*
 * kind43.c: actor kind $43, a platform the player can ride that breaks
 * into pieces, and the pieces, which rise when he is in reach.
 */
#include <stdio.h>

#include "actor/actor_own.h"
#include "audio/sound.h"
#include "data/kinds_b_tables.h"

/* its own bytes */
typedef struct {
    uint8_t before[OWN_40];
    uint8_t frames;             /* +$40: frames left; then the pieces left to spawn */
    uint8_t unused_41;          /* +$41 */
    uint8_t carrying;           /* +$42: the carry count (its wait's ride test runs while it is nonzero) */
    uint8_t carries;            /* +$43: nonzero, Aladdin rides it this frame (ActorTestCarries) */
    uint8_t between[OWN_4C - OWN_44];
    uint8_t test_box;           /* +$4C: the ride test's box */
    uint8_t piece_kind;         /* +$4D: the pieces' kind (ActorSpawnLookWait) */
} K43Own;

static inline K43Own *k43(Actor *a) { return (K43Own *)a->own.bytes; }

enum { WAIT_RIDE = ACTOR_RESUME_OWN, RIDDEN, BREAK, SPAWN, RISE, STAY };

#define PIECE_WAY 3             /* the pieces' +$07 */

/*
 * @routine 83:B393 Kind43Start
 * WHAT IT DOES: Actor kind $43: anim_flags bit 1, its wait routine on
 * (flags $90), facing right, not ridden, its probe list. By +$07 (the
 * original jumps through the table at $83:B3B5):
 * ways 0-2, a platform: carry count $18, the ride test's box and pose by
 * way; once Aladdin rides it, $18 frames on, sound $1A and its next pose;
 * $15 frames later it breaks into pieces (3 for way 0, 2 for 1 and 2: its
 * own kind, way 3, with its look, at spawn points two apart from the
 * way's first, each when a record is free: ActorSpawnLookWait), its
 * placement is marked gone for good and it ends.
 * way 3, a piece: no carrying, pose 6; each frame, once Aladdin is in
 * reach by entry 0 it rises out of the ground (ActorRiseWhenInReach), then
 * ends.
 * The placements give it ways 0 and 1 only and its pieces way 3; a way
 * past the table's four is not converted (it reports itself and stays).
 * Its second entry (83:B457, a JML) is ActorEndPlaced.
 */
int Kind43Start(Actor *a)
{
    K43Own *k = k43(a);
    uint8_t way = a->spawn_byte;
    int r;

    switch (a->resume) {
    case ACTOR_START:
        a->anim_flags |= 0x02;
        a->flags |= 0x90;
        a->facing = 0;
        a->flip = 0;
        k->carries = 0;
        a->probes = k43_probes();
        switch ((uint8_t)(way << 1)) {
        case 0: case 2: case 4:
            k->carrying = 0x18;
            k->test_box = k43_carry_box(way);
            AnimStart(a, k43_anim(way));
            a->resume = WAIT_RIDE;
            return 1;
        case 6:
            k->carrying = 0;
            AnimStart(a, 6);
            a->resume = RISE;
            return 1;
        default: {
            static int told;

            if (!told++)
                fprintf(stderr, "Kind43Start: way %u (past its table) not converted\n", way);
            a->resume = STAY;
            return 1;
        }
        }
    case ACTOR_START_SECOND:
        return ActorEndPlaced(a);
    case WAIT_RIDE:
        if (k->carries)
            a->resume = RIDDEN;
        return 1;
    case RIDDEN:
        if (--k->carrying)
            return 1;
        SoundQueueAdd(0x1A);
        AnimStart(a, (uint8_t)(k43_anim(way) + 1));
        k->frames = 0x15;
        k->carries = 0;
        a->resume = BREAK;
        return 1;
    case BREAK:
        if (--k->frames)
            return 1;
        k->frames = k43_pieces(way);
        a->spawn_at = k43_first_spawn_at(way);
        a->spawn_byte = PIECE_WAY;
        k->piece_kind = a->kind;
        a->resume = SPAWN;
        __attribute__((fallthrough));   /* the first piece in the same frame */
    case SPAWN:
        do {
            r = ActorSpawnLookWait(a);
            if (r != ACTOR_RETURNS)
                return r;           /* no record free: a frame, and again */
            a->spawn_at += 2;
        } while (--k->frames);
        g.placed[a->placement] = 0x80;
        return ActorEndPlaced(a);
    case RISE:
        r = ActorRiseWhenInReach(a, 0);
        if (r == ACTOR_RETURNS_C)
            return 1;               /* not in reach: a frame, and again */
        if (r == ACTOR_RETURNS)
            return ActorEndPlaced(a);
        return r;
    default:                        /* STAY */
        return 1;
    }
}

/*
 * @routine 83:B43B Kind43Wait
 * WHAT IT DOES: Kind $43's wait: not carrying, its animation; carrying,
 * a shot that reaches it ends (ActorEndsShot) and the ride test (test 0:
 * ActorTestSolid); then its end when out of range by box 0.
 */
int Kind43Wait(Actor *a)
{
    if (!k43(a)->carrying) {
        AnimStep(a);
    } else {
        (void)ActorEndsShot(a);
        ActorTestCarries(a, 0);
    }
    return ActorEndOutOfRange(a, 0);
}
