/*
 * kind3f.c: actor kind $3F, a platform that crumbles a while after
 * Aladdin first rides it.
 */
#include "actor/actor_own.h"
#include "audio/sound.h"
#include "data/kinds_b_tables.h"

/* its own bytes */
typedef struct {
    uint8_t before[OWN_40];
    uint8_t frames;             /* +$40: frames left of its crumbling */
    uint8_t unused_41;          /* +$41 */
    uint8_t carrying;           /* +$42: nonzero, its wait tests the ride; frames left once ridden */
    uint8_t carries;            /* +$43: nonzero, Aladdin rides it this frame (ActorTestCarries) */
    uint8_t between[OWN_4C - OWN_44];
    uint8_t test_box;           /* +$4C: the ride test's box */
} K3FOwn;

static inline K3FOwn *k3f(Actor *a) { return (K3FOwn *)a->own.bytes; }

enum { WAIT_RIDE = ACTOR_RESUME_OWN, RIDDEN, CRUMBLE };

/*
 * @routine 83:B19A Kind3FStart
 * WHAT IT DOES: Actor kind $3F, a platform: anim_flags bit 1, its wait
 * routine on (flags $90), facing right; its ride test's box by +$07, pose
 * +$07 * 2. It waits until Aladdin rides it (its wait sets +$43), then $30
 * frames on it crumbles (pose +$07 * 2 + 1, sound $2C) and ends $38 frames
 * later. Its second entry (83:B1FD, a JML) is ActorEndPlaced.
 */
int Kind3FStart(Actor *a)
{
    K3FOwn *k = k3f(a);

    switch (a->resume) {
    case ACTOR_START:
        a->anim_flags |= 0x02;
        a->flags |= 0x90;
        a->facing = 0;
        a->flip = 0;
        k->carries = 0;
        k->carrying = 0x30;
        k->test_box = k3f_test_box(a->spawn_byte);
        AnimStart(a, (uint8_t)(a->spawn_byte << 1));
        a->resume = WAIT_RIDE;
        return 1;
    case ACTOR_START_SECOND:
        return ActorEndPlaced(a);
    case WAIT_RIDE:
        if (k->carries)
            a->resume = RIDDEN;
        return 1;
    case RIDDEN:
        if (--k->carrying)
            return 1;
        AnimStart(a, (uint8_t)((a->spawn_byte << 1) + 1));
        SoundQueueAdd(0x2C);
        k->frames = 0x38;
        k->carries = 0;
        a->resume = CRUMBLE;
        return 1;
    default:                            /* CRUMBLE */
        if (--k->frames)
            return 1;
        return ActorEndPlaced(a);
    }
}

/*
 * @routine 83:B1E8 Kind3FWait
 * WHAT IT DOES: Kind $3F's wait: not carrying, its animation; carrying,
 * the ride test (ActorTestCarries, test 2: ActorTestLand); then its end
 * when out of range by box 0.
 */
int Kind3FWait(Actor *a)
{
    if (!k3f(a)->carrying)
        AnimStep(a);
    else
        ActorTestCarries(a, 2);
    return ActorEndOutOfRange(a, 0);
}
