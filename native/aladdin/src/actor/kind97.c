/*
 * kind97.c: actor kind $97, a dropper kept in place: by its +$07, it waits
 * for the point actors are kept in range of to come into a window, then
 * drops a child (kind $37, from above that point) and waits for it to end,
 * or takes two free records and sends a carrier pair, and waits for that.
 * No script meets any of it: written from its bytes.
 *
 * @routine 83:93B5 Kind97Enter: its start stub, the JML to Kind97Start; its
 * row in kinds_e.c is that jump.
 * Dead, not converted: @routine 84:9FFB Kind97WaitWork, its wait work
 * (only RTSs), and its stub 83:93B9: nothing sets bit 7 of a kind $97's
 * +$09 (the record's @unreachable proof at 83:93B9), so no wait runs.
 */
#include "actor/actor_own.h"
#include "actor/kinds_e.h"
#include "data/kinds_e_tables.h"

#define CHILD_KIND 0x37
#define OWN_KIND 0x97

/* its own bytes */
typedef struct {
    uint8_t before[OWN_40];
    uint8_t timer;              /* +$40: frames left to wait */
    uint8_t busy;               /* +$41: nonzero while its child is out */
} Kind97Own;

/* past +$49, by OwnPlace (+$4A and +$4B are own's last bytes) */
#define TAKEN(a) ((a)->own.bytes[OWN_4A])      /* +$4A: the first record ActorFreeListTake gave (its place in g.free_pool) */
#define COUNT(a) ((a)->own.bytes[OWN_4B])      /* +$4B: at 4 or more it waits (its pairs count it) */
/* +$4E-$4F: its child's record, as its index in g.actors */
static uint16_t child_of(Actor *a) { return (uint16_t)(a->own.bytes[OWN_4E] | a->own.bytes[OWN_4F] << 8); }

_Static_assert(sizeof(Kind97Own) <= OWN_BYTES, "a view fits the own bytes");

static inline Kind97Own *own(Actor *a) { return (Kind97Own *)a->own.bytes; }

enum {
    DROP_TIMER = ACTOR_RESUME_OWN,  /* the dropper: each frame of its timer */
    DROP_LOOK,                      /* each frame until idle and the point in its window */
    DROP_AWAIT,                     /* each frame until its child's record is free */
    SEND_FRAME,                     /* the sender: each frame */
    SEND_AWAIT,                     /* each frame until its child's record is free */
};

/* g.range_x within the window whose words start at `at` (low, high) */
static int in_window(uint8_t at)
{
    return g.range_x >= k97_window(at) && k97_window((uint8_t)(at + 2)) >= g.range_x;
}

static int child_out(Actor *a)
{
    return g.actors[child_of(a)].state != ACTOR_STATE_FREE;
}

/*
 * @routine 84:A003 Kind97DropChild
 * WHAT IT DOES: Kind $97's drop: a child of kind $37 made as if from 80
 * pixels above the point actors are kept in range of (its own place and
 * kind set to those while it is made and placed at spawn point $44, then
 * put back), with +$07 one more than its own, new; while it is out, +$41
 * counts and its timer is $78.
 */
static void drop_child(Actor *a)
{
    uint32_t x = a->x, y = a->y;
    Actor *child;

    a->x = with_pixels(a->x, g.range_x);
    a->y = with_pixels(a->y, (uint16_t)(g.range_y - 0x50));
    a->kind = CHILD_KIND;
    child = ActorCreate(CHILD_KIND);
    if (child) {
        uint16_t i = (uint16_t)(child - g.actors);

        a->own.bytes[OWN_4E] = (uint8_t)i;
        a->own.bytes[OWN_4F] = (uint8_t)(i >> 8);
        child->spawn_byte = (uint8_t)(a->spawn_byte + 1);
        a->spawn_at = 0x44;
        ActorSpawnPlace(a, child);
        child->state = ACTOR_STATE_NEW;
        own(a)->busy++;
        own(a)->timer = 0x78;
    }
    a->y = with_pixels(a->y, pixels(y));
    a->x = with_pixels(a->x, pixels(x));
    a->kind = OWN_KIND;
}

/*
 * @routine 84:9F4F Kind97Start
 * WHAT IT DOES: Actor kind $97: with +$07 bit 0 clear, a dropper: after 30
 * frames, each time it is idle and the point actors are kept in range of
 * lies within its window, it drops a child and waits until the child's
 * record is free, then its timer again. With bit 0 set, a sender: each
 * frame its count is below 4 and it is idle, once its timer runs out and
 * the point lies within the window its +$07 picks (bits $11), it takes two
 * free records, sends a carrier pair (CarrierPairSend) and waits until its
 * child's record is free. The windows are in bank $80, where the actor
 * loop runs its kinds (no trace reads them).
 */
int Kind97Start(Actor *a)
{
    Kind97Own *o = own(a);

    switch (a->resume) {
    case ACTOR_START:
        o->busy = 0;
        if (!(a->spawn_byte & 0x01)) {
            o->timer = 0x1E;
            a->resume = DROP_TIMER;
            return 1;
        }
        o->timer = 0x01;
        COUNT(a) = 0;
        a->resume = SEND_FRAME;
        return 1;
    case DROP_TIMER:
        if (--o->timer)
            return 1;
        a->resume = DROP_LOOK;
        return 1;
    case DROP_LOOK:
        if (o->busy || !in_window(K97_DROP_WINDOW))
            return 1;
        drop_child(a);
        a->resume = DROP_AWAIT;
        return 1;
    case DROP_AWAIT:
        if (child_out(a))
            return 1;
        o->busy = 0;
        a->resume = DROP_TIMER;
        return 1;
    case SEND_FRAME: {
        int taken;

        if (COUNT(a) >= 4 || o->busy || --o->timer)
            return 1;
        o->timer++;
        if (!in_window((a->spawn_byte & 0x11) ? K97_PAIR_WINDOW_A : K97_PAIR_WINDOW_B))
            return 1;
        taken = ActorFreeListTake(2);
        if (taken < 0)
            return 1;
        TAKEN(a) = (uint8_t)taken;
        CarrierPairSend(a);
        a->resume = SEND_AWAIT;
        return 1;
    }
    case SEND_AWAIT:
        if (child_out(a))
            return 1;
        o->busy = 0;
        a->resume = SEND_FRAME;
        return 1;
    default:
        return 1;
    }
}
